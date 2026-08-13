# WojakCoin DAA V3 — Absolute ASERT + Real-Time Target (RTT)

## Problem (small multipool chains)

| Algo | Why it fails on Wojak-class chains |
|------|-------------------------------------|
| **BTC 2016-block SMA** | Hashrate can leave for weeks of wall time; difficulty barely moves. Small coins die. |
| **V2 24-block average + 2×/4× clamps** | Multipool burst spikes difficulty; clamps freeze recovery so difficulty “stops moving”; 30–60+ min stalls (seen ~29 min after height 176499). |
| **Long-half-life ASERT (BCH-style, τ≈days)** | Correct for *stable* hashrate. On multipool chains, recovery is too slow after rented hash leaves. |
| **Pure ASERT alone** | Next block eases only *after* a hard block is found. While the tip is stuck, **this** block’s target is frozen. |

zawy12’s ranking (issue #50): ASERT/EMA are the preferred every-block DAAs; timespan **limits enable attacks**; small coins should prefer **faster** response than BCH’s multi-day smoothing. Extreme on-off (Komodo subchains) used **TSA/RTT** so difficulty keeps moving during a stall.

## Solution (activates at height **190000** with 15-min future-time)

### Layer 1 — Absolute ASERT (`τ = 30 minutes`)

```
base = anchor_target * 2^((t_tip − t_ref − (h_tip − h_ref + 1)·T) / τ)
```

- `T = 120 s`, `τ = nAsertHalfLife = 1800 s`
- Anchor = block **189999** (fixed forever → no nBits round-off accumulation)
- Candidate header time **not** used here (schedule integrity)
- Integer **aserti3-2d** (BCH/eCash): 16.16 fixed-point + cubic `2^x`

**30 min late on schedule → ~2× easier next base target** (vs 2 hours previously).

### Layer 2 — Real-time target easing (`τ_rtt = 15 minutes`)

While mining/validating the *current* block with header time `nTime`:

```
next = base * 2^((nTime − t_tip − T) / τ_rtt)
```

- Miners refresh GBT as wall-clock advances → **nBits eases continuously during a stall**
- Does not wait for a lucky find at the old high difficulty
- Bounded by the **15-minute max future block time** (same activation height) so free future-stamping is limited
- Absolute ASERT still uses only the anchor → RTT nBits do **not** poison the long-term schedule

**~15 min stuck → ~2× easier on this block; ~30 min → ~4×**, etc., until `powLimit`.

### What we deliberately do **not** use

| Rejected | Reason |
|----------|--------|
| Timespan 2×/4× clamps | zawy: corrupt hashrate estimate / enable attacks; cause “stuck difficulty” |
| Cryptonote SMA + cut/lag | Amplifying oscillations on small coins |
| Long LWMA-only without RTT | Difficulty still frozen on the block being mined |
| BCH τ=2 days | Wrong scale for multipool volatility |

## Parameters (mainnet)

| Param | Value |
|-------|--------|
| `nAsertActivationHeight` | **190000** |
| `nMaxFutureBlockTimeActivationHeight` | **190000** (paired) |
| `nAsertHalfLife` | **1800** (30 min) |
| `nAsertRttHalfLife` | **900** (15 min) |
| `nPowTargetSpacing` | 120 s |
| Anchor | height **189999** |

## Networks

| Network | Behaviour |
|---------|-----------|
| mainnet | V3 from 190000; V2 for 1000 ≤ h < 190000 |
| testnet | V3 always on |
| regtest | unchanged (`fPowNoRetargeting`) |

## References

- zawy12, [Summary of Difficulty Algorithms #50](https://github.com/zawy12/difficulty-algorithms/issues/50) — ASERT preferred; RTT/TSA for stuck-chain / extreme on-off
- zawy12, [TSA RTT #36](https://github.com/zawy12/difficulty-algorithms/issues/36)
- Bitcoin Cash **aserti3-2d** (Mark Lundeberg / Jonathan Toomim / BCHN)
- Historical Wojak V2 stall: ~29 minutes after block 176499 at ~130–140M difficulty
