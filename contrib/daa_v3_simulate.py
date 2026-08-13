#!/usr/bin/env python3
"""
DAA V3 simulation — calm absolute ASERT + delayed RTT (Wojak mainnet params).

Models hashrate scenarios and reports block times / relative difficulty.
Uses float exponential form of ASERT (same formula as aserti3-2d; not consensus
bit-identical). Good for policy comparison, not for validating exact nBits.

Usage:
  python3 contrib/daa_v3_simulate.py
  python3 contrib/daa_v3_simulate.py --seed 7 --blocks 500
"""

from __future__ import annotations

import argparse
import math
import random
import statistics
from dataclasses import dataclass, field
from typing import Callable, List, Optional, Tuple


# --- Mainnet V3 params (chainparams) ---
T = 120  # target spacing seconds
TAU = 3 * 60 * 60  # baseline ASERT half-life
RTT_START = 15 * 60 + 2 * 60  # FTL + T = 17 min
RTT_TAU = 15 * 60  # RTT half-life on excess only
POW_LIMIT_DIFF = 1e-12  # floor relative difficulty (arbitrary, far below 1)


def asert_next_diff(
    anchor_d: float,
    t_tip: int,
    t_ref: int,
    h_tip: int,
    h_ref: int,
    tau: float = TAU,
    t_block: Optional[int] = None,
    rtt_start: int = RTT_START,
    rtt_tau: float = RTT_TAU,
    use_rtt: bool = True,
) -> float:
    """
    Relative difficulty (higher = harder).
    Absolute ASERT on schedule from tip; optional delayed RTT using t_block.
    """
    # base: D' = D_anchor * 2^(-(t_tip - t_ref - (h_tip-h_ref+1)*T) / tau)
    # (target grows with lag → difficulty falls)
    exponent = (t_tip - t_ref - (h_tip - h_ref + 1) * T) / tau
    base = anchor_d * (2.0 ** (-exponent))
    if base < POW_LIMIT_DIFF:
        base = POW_LIMIT_DIFF

    if not use_rtt or t_block is None or rtt_tau <= 0 or rtt_start <= 0:
        return base

    st = t_block - t_tip
    if st <= rtt_start:
        return base
    excess = st - rtt_start
    # next = base * 2^(excess / rtt_tau) in target space → D /= 2^(excess/rtt_tau)
    eased = base / (2.0 ** (excess / rtt_tau))
    return max(eased, POW_LIMIT_DIFF)


def v2_next_diff(window: List[Tuple[int, float]], n: int = 24) -> float:
    """
    Simplified V2: avg of last n difficulties, scale by actual/target timespan,
    clamp actual span to [half, 4x] of n*T. (Approximation of target-average V2.)
    """
    if len(window) < n + 1:
        return window[-1][1] if window else 1.0
    recent = window[-(n + 1) :]
    times = [r[0] for r in recent]
    diffs = [r[1] for r in recent[1:]]  # n difficulties of last n blocks
    avg_d = sum(diffs) / len(diffs)
    actual = times[-1] - times[0]
    target = n * T
    if actual < target / 2:
        actual = target / 2
    if actual > target * 4:
        actual = target * 4
    # longer actual → easier (lower D)
    return max(avg_d * (target / actual), POW_LIMIT_DIFF)


@dataclass
class Block:
    height: int
    time: int
    diff: float  # relative difficulty used to mine this block
    hashrate: float
    solvetime: int
    rtt_applied: bool = False


@dataclass
class SimResult:
    name: str
    blocks: List[Block] = field(default_factory=list)

    def solvetimes(self, start: int = 0) -> List[int]:
        return [b.solvetime for b in self.blocks[start:] if b.height > 0]

    def summary(self, label: str = "") -> str:
        sts = self.solvetimes(1)
        if not sts:
            return f"{self.name}: no blocks"
        diffs = [b.diff for b in self.blocks if b.height > 0]
        p50 = statistics.median(sts)
        p90 = sorted(sts)[int(0.9 * (len(sts) - 1))]
        p99 = sorted(sts)[int(0.99 * (len(sts) - 1))]
        mx = max(sts)
        mean = statistics.mean(sts)
        dmin, dmax = min(diffs), max(diffs)
        over_10m = sum(1 for s in sts if s > 600)
        over_17m = sum(1 for s in sts if s > RTT_START)
        over_30m = sum(1 for s in sts if s > 1800)
        return (
            f"{self.name}{label}\n"
            f"  blocks={len(sts)}  mean_st={mean:.1f}s ({mean/T:.2f}×T)  "
            f"median={p50}s  p90={p90}s  p99={p99}s  max={mx}s ({mx/60:.1f}m)\n"
            f"  st>10m: {over_10m}  st>17m(RTT): {over_17m}  st>30m: {over_30m}\n"
            f"  rel_diff min={dmin:.4g}  max={dmax:.4g}  final={diffs[-1]:.4g}  "
            f"max/min={dmax/dmin:.2f}×"
        )


def mine_solvetime(diff: float, hashrate: float, rng: random.Random) -> int:
    """Exponential solvetime: E[st] = T * diff / hashrate."""
    mean = T * diff / max(hashrate, 1e-12)
    # floor at 1s
    st = max(1, int(rng.expovariate(1.0 / mean)))
    return st


def simulate_asert(
    name: str,
    hashrate_fn: Callable[[int, int], float],
    n_blocks: int,
    rng: random.Random,
    use_rtt: bool = True,
    h0: int = 190000,
    t0: int = 1_700_000_000,
    d0: float = 1.0,
) -> SimResult:
    """
    hashrate_fn(height, wall_time) -> relative hashrate.
    Anchor = pre-activation block at h0-1.
    """
    res = SimResult(name=name)
    # Anchor block (last pre-V3)
    h_ref = h0 - 1
    t_ref_parent = t0 - T  # parent of anchor
    t_anchor = t0
    d_anchor = d0

    # Tip starts at anchor
    h = h_ref
    t = t_anchor
    # Store chain tip state; difficulty of next block computed before mining
    res.blocks.append(Block(height=h, time=t, diff=d_anchor, hashrate=1.0, solvetime=T))

    for i in range(n_blocks):
        # Schedule ASERT uses tip only (not candidate time)
        # We need candidate time for RTT: iterate — first guess without RTT, then
        # if RTT would apply we re-sample with eased diff (GBT refresh model).
        base_d = asert_next_diff(
            d_anchor, t, t_ref_parent, h, h_ref, use_rtt=False
        )
        hr = hashrate_fn(h + 1, t)
        # Provisional solvetime at base difficulty
        st = mine_solvetime(base_d, hr, rng)
        t_cand = t + st
        d_use = base_d
        rtt_on = False

        if use_rtt and st > RTT_START:
            # Miner refreshes template as wall clock advances past delay.
            # Approximate: final block uses RTT at true find time (honest nTime ≈ real).
            d_rtt = asert_next_diff(
                d_anchor, t, t_ref_parent, h, h_ref, t_block=t_cand, use_rtt=True
            )
            if d_rtt < base_d * 0.999:
                # Re-mine with eased difficulty from the point RTT engaged.
                # Split: mine hard until RTT_START, then continue with easing.
                # Simpler model: expected remaining work after delay uses d_rtt path.
                # Use iterative: find st such that exponential with time-varying D.
                st = mine_with_delayed_rtt(base_d, d_anchor, t, t_ref_parent, h, h_ref, hr, rng)
                t_cand = t + st
                d_use = asert_next_diff(
                    d_anchor, t, t_ref_parent, h, h_ref, t_block=t_cand, use_rtt=True
                )
                rtt_on = st > RTT_START

        h += 1
        t = t_cand
        res.blocks.append(
            Block(height=h, time=t, diff=d_use, hashrate=hr, solvetime=st, rtt_applied=rtt_on)
        )
    return res


def mine_with_delayed_rtt(
    base_d: float,
    d_anchor: float,
    t_tip: int,
    t_ref: int,
    h_tip: int,
    h_ref: int,
    hr: float,
    rng: random.Random,
) -> int:
    """
    Piecewise mining: difficulty = base until RTT_START wall seconds, then
    eases with claimed solvetime (continuous GBT refresh). Sample as discrete
    10s ticks after delay for speed.
    """
    # Phase 1: constant base_d for first RTT_START seconds of work
    # Survival: P(not found by S) = exp(-S / mean)
    mean1 = T * base_d / max(hr, 1e-12)
    # Sample whether found before delay
    st1 = max(1, int(rng.expovariate(1.0 / mean1)))
    if st1 <= RTT_START:
        return st1

    # Not found by delay: continue from RTT_START with easing difficulty
    elapsed = RTT_START
    tick = 10
    while elapsed < RTT_START + 48 * 3600:  # safety 48h
        t_cand = t_tip + elapsed
        d = asert_next_diff(
            d_anchor, t_tip, t_ref, h_tip, h_ref, t_block=t_cand, use_rtt=True
        )
        mean = T * d / max(hr, 1e-12)
        # Probability of find in this tick
        p = 1.0 - math.exp(-tick / mean)
        if rng.random() < p:
            # uniform within tick
            return elapsed + rng.randint(1, tick)
        elapsed += tick
    return elapsed


def simulate_v2(
    name: str,
    hashrate_fn: Callable[[int, int], float],
    n_blocks: int,
    rng: random.Random,
    h0: int = 1000,
    t0: int = 1_700_000_000,
    d0: float = 1.0,
) -> SimResult:
    res = SimResult(name=name)
    h, t, d = h0, t0, d0
    window: List[Tuple[int, float]] = [(t, d)]
    res.blocks.append(Block(height=h, time=t, diff=d, hashrate=1.0, solvetime=T))
    for _ in range(n_blocks):
        hr = hashrate_fn(h + 1, t)
        st = mine_solvetime(d, hr, rng)
        h += 1
        t += st
        res.blocks.append(Block(height=h, time=t, diff=d, hashrate=hr, solvetime=st))
        window.append((t, d))
        d = v2_next_diff(window)
    return res


# --- Hashrate scenarios ---

def hr_stable(_h: int, _t: int) -> float:
    return 1.0


def make_burst_then_leave(burst_start: int, burst_blocks: int, mult: float) -> Callable[[int, int], float]:
    def hr(h: int, _t: int) -> float:
        if burst_start <= h < burst_start + burst_blocks:
            return mult
        return 1.0
    return hr


def make_leave_after(start_h: int, remain: float = 0.05) -> Callable[[int, int], float]:
    """Dedicated miners only after start_h (simulates multipool exit)."""
    def hr(h: int, _t: int) -> float:
        return remain if h >= start_h else 1.0
    return hr


def make_burst_then_cripple(
    burst_h: int, burst_n: int, mult: float, remain: float
) -> Callable[[int, int], float]:
    def hr(h: int, _t: int) -> float:
        if burst_h <= h < burst_h + burst_n:
            return mult
        if h >= burst_h + burst_n:
            return remain
        return 1.0
    return hr


def print_window(res: SimResult, h_lo: int, h_hi: int) -> None:
    print(f"  --- heights {h_lo}..{h_hi} ---")
    print(f"  {'h':>7} {'st':>6} {'st_m':>6} {'diff':>10} {'HR':>6} {'RTT':>4}")
    for b in res.blocks:
        if h_lo <= b.height <= h_hi and b.height > 0:
            print(
                f"  {b.height:7d} {b.solvetime:6d} {b.solvetime/60:6.1f} "
                f"{b.diff:10.4g} {b.hashrate:6.2f} {'Y' if b.rtt_applied else '':>4}"
            )


def main() -> None:
    ap = argparse.ArgumentParser(description="Simulate Wojak DAA V3 vs V2")
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--blocks", type=int, default=400)
    args = ap.parse_args()
    rng = random.Random(args.seed)
    n = args.blocks

    print("=" * 72)
    print("Wojak DAA V3 simulation")
    print(f"  T={T}s  τ_ASERT={TAU}s ({TAU/3600:.1f}h)  "
          f"RTT_start={RTT_START}s ({RTT_START/60:.0f}m)  τ_RTT={RTT_TAU}s")
    print(f"  seed={args.seed}  blocks/scenario={n}")
    print("=" * 72)

    scenarios = []

    # 1) Stable
    r1 = simulate_asert("V3 stable HR=1", hr_stable, n, random.Random(args.seed), use_rtt=True)
    scenarios.append(r1)
    print("\n" + r1.summary())

    # 2) Multipool 20× for 30 blocks then full leave to 5% dedicated
    h_burst = 190050
    r2 = simulate_asert(
        "V3 multipool 20××30 then HR=0.05",
        make_burst_then_cripple(h_burst, 30, 20.0, 0.05),
        n,
        random.Random(args.seed + 1),
        use_rtt=True,
    )
    scenarios.append(r2)
    print("\n" + r2.summary())
    print_window(r2, h_burst - 2, h_burst + 45)

    # 3) Same without RTT
    r3 = simulate_asert(
        "V3 same multipool, RTT OFF",
        make_burst_then_cripple(h_burst, 30, 20.0, 0.05),
        n,
        random.Random(args.seed + 1),  # same path until RTT matters
        use_rtt=False,
    )
    scenarios.append(r3)
    print("\n" + r3.summary())

    # Compare max solvetime after burst between RTT on/off
    def max_st_after(res: SimResult, h0: int) -> int:
        return max((b.solvetime for b in res.blocks if b.height >= h0 + 30), default=0)

    print("\n  [compare post-burst max solvetime]")
    print(f"    RTT ON : max_st after leave = {max_st_after(r2, h_burst)}s "
          f"({max_st_after(r2, h_burst)/60:.1f}m)")
    print(f"    RTT OFF: max_st after leave = {max_st_after(r3, h_burst)}s "
          f"({max_st_after(r3, h_burst)/60:.1f}m)")
    rtt_blocks = sum(1 for b in r2.blocks if b.rtt_applied)
    print(f"    RTT engaged on {rtt_blocks} blocks (should be rare)")

    # 4) Mild multipool 5××20 then back to 1.0 (no cripple)
    r4 = simulate_asert(
        "V3 mild multipool 5××20 then HR=1",
        make_burst_then_leave(190040, 20, 5.0),
        n,
        random.Random(args.seed + 2),
        use_rtt=True,
    )
    scenarios.append(r4)
    print("\n" + r4.summary())
    rtt4 = sum(1 for b in r4.blocks if b.rtt_applied)
    print(f"  RTT engaged: {rtt4} blocks (expect ~0 if dedicated hash remains)")

    # 5) V2 same harsh multipool for comparison
    # Map heights into V2 sim starting 1000
    def hr_v2(h: int, t: int) -> float:
        # align: sim height 1000+i ~ scenario height 190000+i
        abs_h = 190000 + (h - 1000)
        return make_burst_then_cripple(h_burst, 30, 20.0, 0.05)(abs_h, t)

    r5 = simulate_v2(
        "V2 multipool 20××30 then HR=0.05",
        hr_v2,
        n,
        random.Random(args.seed + 1),
    )
    scenarios.append(r5)
    print("\n" + r5.summary())
    # window around burst in V2 coords
    v2_burst = 1000 + (h_burst - 190000)
    print_window(r5, v2_burst - 2, v2_burst + 45)

    # 6) RTT silence check: stable chain should not use RTT
    print("\n" + "=" * 72)
    print("CHECKS")
    print("=" * 72)
    stable_rtt = sum(1 for b in r1.blocks if b.rtt_applied)
    print(f"  [ ] stable: RTT count = {stable_rtt} (want 0)  "
          f"{'PASS' if stable_rtt == 0 else 'FAIL'}")
    print(f"  [ ] mild multipool return: RTT count = {rtt4} (want 0 or very low)  "
          f"{'PASS' if rtt4 <= 2 else 'WARN'}")
    mean_stable = statistics.mean(r1.solvetimes(1))
    print(f"  [ ] stable mean st ≈ T: {mean_stable:.1f}s vs {T}s  "
          f"{'PASS' if abs(mean_stable - T) / T < 0.15 else 'WARN'}")
    # Harsh: RTT should reduce max stall vs no RTT
    mx_on = max_st_after(r2, h_burst)
    mx_off = max_st_after(r3, h_burst)
    print(f"  [ ] harsh leave max_st RTT on ≤ off: {mx_on}s ≤ {mx_off}s  "
          f"{'PASS' if mx_on <= mx_off else 'FAIL'}")
    # V2 often worse max after leave
    mx_v2 = max((b.solvetime for b in r5.blocks if b.height >= v2_burst + 30), default=0)
    print(f"  [ ] V3+RTT max post-leave vs V2: V3={mx_on}s V2={mx_v2}s  "
          f"{'PASS' if mx_on <= mx_v2 * 1.05 else 'INFO'}")

    # Diff swing on stable (should be modest)
    diffs = [b.diff for b in r1.blocks if b.height > 10]
    swing = max(diffs) / min(diffs) if diffs else 1
    print(f"  [ ] stable diff max/min = {swing:.3f}× (want modest, <~3)  "
          f"{'PASS' if swing < 3 else 'WARN'}")

    print("\nDone.")


if __name__ == "__main__":
    main()
