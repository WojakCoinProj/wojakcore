// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2024-2026 WojakCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Legacy V2 DAA (24-block target average). Used between nDifficultyV2ForkHeight
// and nAsertActivationHeight. Kept for historical validation only.
// ---------------------------------------------------------------------------
unsigned int GetNextWorkRequiredV2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int64_t nBlocksToAverage = 24;
    const int64_t nAveragingTargetTimespan = nBlocksToAverage * params.nPowTargetSpacing;

    arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    unsigned int nProofOfWorkLimit = bnPowLimit.GetCompact();

    if (pindexLast == nullptr)
        return nProofOfWorkLimit;

    if (pindexLast->nHeight < nBlocksToAverage)
        return pindexLast->nBits;

    const CBlockIndex* pindex = pindexLast;
    arith_uint256 bnPastTargetAvg;

    for (unsigned int nCountBlocks = 1; nCountBlocks <= nBlocksToAverage; nCountBlocks++)
    {
        arith_uint256 bnTarget;
        bnTarget.SetCompact(pindex->nBits);

        if (nCountBlocks == 1)
        {
            bnPastTargetAvg = bnTarget;
        }
        else
        {
            bnPastTargetAvg = (bnPastTargetAvg * nCountBlocks + bnTarget) / (nCountBlocks + 1);
        }

        if (nCountBlocks != nBlocksToAverage)
        {
            if (pindex->pprev == nullptr)
                return nProofOfWorkLimit;
            pindex = pindex->pprev;
        }
    }

    arith_uint256 bnNew(bnPastTargetAvg);

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindex->GetBlockTime();
    int64_t nTargetTimespan = nAveragingTargetTimespan;

    if (nActualTimespan < nTargetTimespan/2)
        nActualTimespan = nTargetTimespan/2;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// ---------------------------------------------------------------------------
// DAA V3 — absolute ASERT (aserti3-2d) + real-time target (RTT) easing.
//
// Research (zawy12 difficulty-algorithms #50; BCH aserti3-2d; Komodo TSA-RTT):
//  - BTC/LTC 2016-block SMA: small coins die under multipool (on-off mining).
//  - V2 24-block average + 2×/4× clamps: recovery freezes at the clamp;
//    difficulty can "stop moving" after a hashrate spike.
//  - ASERT is the only (near) perfectly correct every-block DAA for a target
//    schedule. BCH uses τ≈2 days — fine when hashrate is stable.
//  - Small multipool chains need SHORT half-life (minutes, not days) or the
//    chain stalls for hours after rented hashrate leaves.
//  - Pure ASERT still freezes *this* block's target until it is found. RTT
//    (zawy TSA family) eases the current block as solvetime grows so
//    difficulty keeps moving during a stall. Absolute ASERT for the schedule
//    is independent of intermediate nBits, so RTT does not poison the anchor.
//
// Schedule (absolute ASERT, ignore candidate nTime):
//   base = anchor_target * 2^((t_tip - t_ref - (h_tip - h_ref + 1)*T) / τ)
// RTT (when mining/validating a header with nTime):
//   next = base * 2^((nTime - t_tip - T) / τ_rtt)
// τ = nAsertHalfLife (30m), τ_rtt = nAsertRttHalfLife (15m), T = 120s.
// ---------------------------------------------------------------------------

arith_uint256 CalculateASERT(const arith_uint256& refTarget,
                             const int64_t nPowTargetSpacing,
                             const int64_t nTimeDiff,
                             const int64_t nHeightDiff,
                             const arith_uint256& powLimit,
                             const int64_t nHalfLife)
{
    // Input target must never be zero nor exceed powLimit.
    assert(refTarget > 0 && refTarget <= powLimit);
    assert(nHalfLife > 0);
    assert(nHeightDiff >= 0);

    // ASERT formula (ideal real numbers):
    //   new_target = old_target * 2^((nTimeDiff - nPowTargetSpacing * (nHeightDiff + 1)) / nHalfLife)
    //
    // Integer aserti3-2d: 16.16 fixed-point exponent + cubic approx of 2^frac.

    // Keep the product inside int64 before the / nHalfLife (same bound as BCH).
    assert(llabs(nTimeDiff - nPowTargetSpacing * nHeightDiff) < (1ll << (63 - 16)));
    const int64_t exponent = ((nTimeDiff - nPowTargetSpacing * (nHeightDiff + 1)) * 65536) / nHalfLife;

    // Arithmetic right-shift required for negative exponents (floor division).
    static_assert(int64_t(-1) >> 1 == int64_t(-1),
                  "ASERT algorithm needs arithmetic shift support");

    int64_t shifts = exponent >> 16;
    const auto frac = uint16_t(exponent);
    assert(exponent == (shifts * 65536) + frac);

    // 2^x ≈ 1 + 0.695502049 x + 0.2262698 x^2 + 0.0782318 x^3  for 0 <= x < 1
    // (error vs true 2^x < 0.013%). Factor is scaled by 65536.
    const uint32_t factor = 65536 + ((
        + 195766423245049ull * frac
        + 971821376ull * frac * frac
        + 5127ull * frac * frac * frac
        + (1ull << 47)
        ) >> 48);

    // this is always < 2^241 when refTarget has enough leading zeros (mainnet powLimit)
    arith_uint256 nextTarget = refTarget * factor;

    // Apply 2^(integer part) and undo the extra *65536 from `factor`.
    shifts -= 16;
    if (shifts <= 0) {
        nextTarget >>= -shifts;
    } else {
        const auto nextTargetShifted = nextTarget << shifts;
        if ((nextTargetShifted >> shifts) != nextTarget) {
            // Would have overflowed 256 bits → treat as powLimit.
            nextTarget = powLimit;
        } else {
            nextTarget = nextTargetShifted;
        }
    }

    if (nextTarget == 0) {
        nextTarget = arith_uint256(1);
    } else if (nextTarget > powLimit) {
        nextTarget = powLimit;
    }

    return nextTarget;
}

unsigned int GetNextWorkRequiredASERT(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    const arith_uint256 powLimit = UintToArith256(params.powLimit);

    // Testnet / regtest: allow min-difficulty if the tip is stale (legacy rule).
    if (params.fPowAllowMinDifficultyBlocks && pblock != nullptr &&
        pblock->GetBlockTime() > pindexLast->GetBlockTime() + 2 * params.nPowTargetSpacing) {
        return powLimit.GetCompact();
    }

    // Anchor = last pre-ASERT block (activation height - 1). Absolute schedule
    // forever references this block — intermediate RTT nBits do not accumulate error.
    const int nAnchorHeight = std::max(0, params.nAsertActivationHeight - 1);
    assert(pindexLast->nHeight >= nAnchorHeight);

    const CBlockIndex* pindexAnchor = pindexLast->GetAncestor(nAnchorHeight);
    assert(pindexAnchor != nullptr);

    const int64_t nAnchorPrevTime = pindexAnchor->pprev
        ? pindexAnchor->pprev->GetBlockTime()
        : pindexAnchor->GetBlockTime();

    bool fNegative = false, fOverflow = false;
    arith_uint256 refTarget;
    refTarget.SetCompact(pindexAnchor->nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || refTarget == 0 || refTarget > powLimit) {
        return powLimit.GetCompact();
    }

    // --- Layer 1: absolute ASERT from tip schedule (candidate nTime ignored) ---
    const int64_t nTimeDiff = pindexLast->GetBlockTime() - nAnchorPrevTime;
    const int64_t nHeightDiff = static_cast<int64_t>(pindexLast->nHeight) - pindexAnchor->nHeight;

    arith_uint256 nextTarget = CalculateASERT(
        refTarget,
        params.nPowTargetSpacing,
        nTimeDiff,
        nHeightDiff,
        powLimit,
        params.nAsertHalfLife);

    // --- Layer 2: real-time easing for the block being mined/validated ---
    // As solvetime grows past T, THIS block's target eases so the chain does
    // not sit at a frozen high difficulty until someone lucks a find.
    // Miners refresh GBT with current time; nBits drops continuously while stalled.
    // Bounded by FTL (15 min after activation) so free future-stamping is limited.
    if (params.nAsertRttHalfLife > 0 && pblock != nullptr) {
        int64_t st = pblock->GetBlockTime() - pindexLast->GetBlockTime();
        // Reject non-positive solvetimes for RTT (monotonicity); keep base ASERT.
        if (st < 1) st = 1;
        // Cap extreme claims (paranoia); FTL already limits honest-ish future.
        const int64_t stCap = params.nAsertRttHalfLife * 32; // ≤ 2^32 ease theoretically
        if (st > stCap) st = stCap;

        // next = base * 2^((st - T) / τ_rtt)  via CalculateASERT(heightDiff=0)
        nextTarget = CalculateASERT(
            nextTarget,
            params.nPowTargetSpacing,
            st,
            /*nHeightDiff=*/0,
            powLimit,
            params.nAsertRttHalfLife);
    }

    return nextTarget.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    const int nNextHeight = pindexLast->nHeight + 1;

    // WojakCoin: ASERT DAA (activates with time-warp fix at nAsertActivationHeight).
    // nAsertActivationHeight == 0 → always active from height 1 (testnet).
    // nAsertActivationHeight  < 0 → disabled.
    if (params.nAsertActivationHeight >= 0 && nNextHeight >= params.nAsertActivationHeight) {
        const int nAnchorHeight = std::max(0, params.nAsertActivationHeight - 1);
        if (pindexLast->nHeight >= nAnchorHeight) {
            return GetNextWorkRequiredASERT(pindexLast, pblock, params);
        }
    }

    // WojakCoin: V2 difficulty algorithm (pre-ASERT)
    if (params.nDifficultyV2ForkHeight > 0 && nNextHeight >= params.nDifficultyV2ForkHeight)
        return GetNextWorkRequiredV2(pindexLast, pblock, params);

    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    if (nNextHeight % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
