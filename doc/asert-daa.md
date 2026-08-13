# WojakCoin ASERT Difficulty Adjustment (aserti3-2d)

## Summary

At mainnet height **190000**, WojakCoin switches from the legacy V2 24-block
averaged DAA to **ASERT** (absolutely scheduled exponentially rising targets),
using the Bitcoin Cash / eCash **aserti3-2d** integer algorithm.

This activates at the **same height** as the 15-minute max future block time
rule (`nMaxFutureBlockTimeActivationHeight`), so DAA recovery and time-warp
hardening ship together.

## Why

V2 (24-block average, 2× harden / 4× ease clamps) caused multi-minute to
hour-long stalls after multipool hashrate spikes (e.g. ~29 minutes after
block 176499). Difficulty only eased *after* a hard block was found.

ASERT sets the next target from an absolute schedule anchored at the last
pre-activation block:

```
next_target = anchor_target * 2^((t - t_ref - (h - h_ref + 1) * T) / τ)
```

- `T` = 120 s (block spacing)
- `τ` = **7200 s** (2-hour half-life)
- Every half-life of schedule skew doubles or halves the target

A 30-minute tip stall eases the *next* target immediately (~19% easier at
τ = 2 h), without waiting for a slow find at the old difficulty.

## Parameters (mainnet)

| Param | Value |
|-------|--------|
| `nAsertActivationHeight` | 190000 |
| `nAsertHalfLife` | 7200 (2 hours) |
| `nMaxFutureBlockTimeActivationHeight` | 190000 (paired) |
| Anchor | block **189999** (bits + parent timestamp) |

## Networks

| Network | ASERT |
|---------|--------|
| mainnet | height ≥ 190000 |
| testnet | always (height ≥ 0) |
| regtest | disabled (`fPowNoRetargeting`) |

## References

- BCH aserti3-2d specification / Bitcoin Cash Node `CalculateASERT`
- Mark Lundeberg / Jonathan Toomim ASERT design
