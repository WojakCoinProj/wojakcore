// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2024 WojakCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/system.h>

#include <algorithm>
#include <cassert>

// LWMA window for Difficulty V3 (N=60 ≈ 2 hours at 2-minute blocks).
static constexpr int64_t DIFFICULTY_V3_LWMA_WINDOW = 60;

unsigned int GetNextWorkRequiredV3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    (void)pblock;
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = DIFFICULTY_V3_LWMA_WINDOW;
    // k = N*(N+1)*T/2  (sum of weights 1..N times target spacing)
    const int64_t k = N * (N + 1) * T / 2;

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    const unsigned int nProofOfWorkLimit = bnPowLimit.GetCompact();

    if (pindexLast == nullptr)
        return nProofOfWorkLimit;

    // Need N prior intervals (N+1 block headers). Fall back until enough history.
    if (pindexLast->nHeight < N)
        return pindexLast->nBits;

    arith_uint256 sumTarget;
    int64_t t = 0;
    int64_t j = 0;

    // Walk oldest -> newest over the last N blocks; weight j increases toward tip.
    for (int64_t i = pindexLast->nHeight - N + 1; i <= pindexLast->nHeight; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        assert(block);
        assert(block->pprev);

        int64_t solvetime = block->GetBlockTime() - block->pprev->GetBlockTime();
        // Clamp to [1, 6T] to limit single-block timestamp manipulation.
        solvetime = std::max<int64_t>(1, std::min<int64_t>(solvetime, 6 * T));

        j++;
        t += solvetime * j;

        arith_uint256 target;
        target.SetCompact(block->nBits);
        sumTarget += target;
    }

    // next = (avg target) * (weighted solvetimes) / k
    arith_uint256 bnNew = sumTarget / N;
    bnNew *= t;
    bnNew /= k;

    // Symmetric per-step clamp vs previous target: max 2x easier or 2x harder.
    arith_uint256 bnPrev;
    bnPrev.SetCompact(pindexLast->nBits);
    arith_uint256 bnMax = bnPrev * 2;
    if (bnMax < bnPrev || bnMax > bnPowLimit)
        bnMax = bnPowLimit;
    const arith_uint256 bnMin = bnPrev / 2;
    if (bnNew > bnMax)
        bnNew = bnMax;
    if (bnNew < bnMin)
        bnNew = bnMin;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;
    if (bnNew == 0)
        bnNew = bnMin == 0 ? arith_uint256(1) : bnMin;

    return bnNew.GetCompact();
}

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

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // WojakCoin: V3 difficulty algorithm fork (LWMA)
    if (params.nDifficultyV3ForkHeight > 0 && pindexLast->nHeight + 1 >= params.nDifficultyV3ForkHeight)
        return GetNextWorkRequiredV3(pindexLast, pblock, params);

    // WojakCoin: V2 difficulty algorithm fork
    if (params.nDifficultyV2ForkHeight > 0 && pindexLast->nHeight + 1 >= params.nDifficultyV2ForkHeight)
        return GetNextWorkRequiredV2(pindexLast, pblock, params);

    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
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
