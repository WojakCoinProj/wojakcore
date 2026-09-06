# Recommended Fix: Stabilize 2-Minute Block Times

**Date:** 2026-09-06  
**Status:** Implemented (Difficulty V3 / LWMA)  
**Scope:** `src/pow.cpp` — new `GetNextWorkRequiredV3`  
**Mainnet activation height:** **205000**

## Problem

`nPowTargetSpacing` is already correctly set to **2 minutes**. Blocks still *felt* too fast because V2’s short window, off-by-one average, and asymmetric 2×-up / 4×-down clamp produced bursty sub-minute intervals under hash spikes (median ~65s while mean stayed near 120s).

## Implemented fix (Difficulty V3)

Replace V2 at a height fork with **LWMA-1** (N=60, T=`nPowTargetSpacing`):

| Network | `nDifficultyV3ForkHeight` |
|---------|---------------------------|
| main    | **205000** |
| test / signet | 1000 |
| regtest | 0 (disabled; unit tests call V3 directly) |

### Behavior

- Weighted solvetimes over the last **60** blocks (≈2 hours)
- Per-interval solvetime clamped to `[1, 6T]`
- Symmetric per-step target clamp: **max 2× easier or 2× harder**
- Cap at `powLimit`
- Pre-fork history still validated with V2 / Bitcoin-style retarget

### Files

- `src/consensus/params.h` — `nDifficultyV3ForkHeight`
- `src/chainparams.cpp` — activation heights
- `src/pow.h` / `src/pow.cpp` — `GetNextWorkRequiredV3` + branch before V2
- `src/test/pow_tests.cpp` — on-target / fast / slow / activation tests

## Rollout

1. Merge and release node binaries before height 205000 (~1.5–2 weeks from tip ~193k at 2-min blocks).
2. Miners/pools must upgrade; old nodes will reject post-fork `nBits`.
3. Soak on testnet (V3 from height 1000).

## Explicit non-goals

- Changing `nPowTargetSpacing` (remains 2 minutes)
- Rewriting history or changing PoW hash
- Soft-config-only slowdown (consensus fork required)
