# WojakCoin DAA V3 — Calm ASERT + Delayed RTT

## Intent

Wojak is **not** BCH/BTC: multipool hashrate can swing hard. But fixing that with
**ultra-short half-lives and continuous RTT** overshoots and destroys stability.

V3 goals:

1. **Normal mining** — stable block cadence via a **few-hour** absolute ASERT baseline  
2. **Near stall / difflock only** — **delayed RTT** eases *this* block after FTL+blocktime  
3. **Stable hashrate** — RTT stays completely idle  

Activates at height **190000** with the 15-minute max future block time rule.

## Why not the previous short-τ + continuous RTT draft

| Issue | Effect |
|-------|--------|
| τ ≈ 30 min baseline | Over-reacts to Poisson noise and multipool blips → overshoot |
| RTT from the first second past T | Constant messy swings; every slightly late block eases |
| BCH τ ≈ 2 days copied naively | Wrong scale discussion: BCH is *stable* hashrate; we want calm hours, not days of paralysis |

BCH’s multi-day half-life is correct for **stable** hash. Wojak uses a **shorter but still multi-hour** baseline so variance settles without panic, plus a **gated** RTT for difflock escape only.

## Design

### Layer 1 — Baseline absolute ASERT (`τ = 3 hours`)

```
base = anchor_target * 2^((t_tip − t_ref − (h_tip − h_ref + 1)·T) / τ)
```

- `T = 120 s`
- Anchor = block **189999** (fixed; no nBits error accumulation)
- Candidate `nTime` **ignored** for the schedule
- Integer **aserti3-2d** (BCH/eCash)

Under normal variance this is the only layer that matters. A 200× hash burst is braked by ASERT rising on the schedule; after they leave, baseline eases over hours—not locked forever like V2 clamps.

### Layer 2 — Delayed RTT (stall / difflock escape)

```
rtt_start = FTL + T = 15 min + 2 min = 17 min
if st > rtt_start:
    next = base * 2^((st − rtt_start) / τ_rtt)   # τ_rtt = 15 min
else:
    next = base   # RTT completely silent
```

- One block may sit through a full FTL window **without** RTT thrash  
- Only after **~17 minutes** of solvetime does difficulty start dropping on *this* block  
- Excess-only easing → no free swing for ordinary late blocks  
- Miners refresh GBT after the delay if stuck (difflock break)  
- Absolute schedule still uses the anchor only  

### What we do not claim

- **KMD assetchains** (e.g. POW-heavy modes with POS): stuck behaviour is largely
  **POS-ratio gating** (wallet rejects excess POW until a POS block), not a pure
  PoW RTT DAA. That mechanism does **not** exist on non-assetchain KMD/ARRR/HUSH/VRSC.
- **XEC-style dual wall** (baseline + explosive reject of too-close blocks) is an
  interesting *secondary* anti-burst idea; not implemented here. Baseline ASERT
  already brakes sustained 200×; delayed RTT only helps clear the resulting
  difflock, it does not try to be the attack wall.

## Parameters (mainnet)

| Param | Value | Role |
|-------|--------|------|
| `nAsertActivationHeight` | **190000** | With time-warp FTL |
| `nAsertHalfLife` | **10800** (3 h) | Calm baseline |
| `nAsertRttStartDelay` | **1020** (15m+2m) | RTT silent until stall window |
| `nAsertRttHalfLife` | **900** (15 m) | Ease only *excess* after delay |
| Anchor | height **189999** | Absolute ASERT reference |

## Networks

| Network | Behaviour |
|---------|-----------|
| mainnet | V3 from 190000; V2 for 1000 ≤ h < 190000 |
| testnet | V3 always on |
| regtest | unchanged (`fPowNoRetargeting`) |

## References

- BCH **aserti3-2d** (absolute ASERT integer math)
- zawy12 difficulty-algorithms discussions (ASERT preferred over clamped SMA; continuous limits are harmful) — used selectively; **not** as a mandate for ultra-short τ on small coins
- Wojak V2 stall history (~29 min after 176499) — motivates delayed RTT, not continuous RTT
