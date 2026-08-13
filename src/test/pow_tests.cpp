// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffffU);
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffffU);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c168fd0U);
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fdU);
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, std::string chainName)
{
    const auto chainParams = CreateChainParams(args, chainName);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 4*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
        targ_max /= consensus.nPowTargetTimespan*4;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::SIGNET);
}

/* ---- DAA V3: calm ASERT + delayed RTT unit tests ---- */

static Consensus::Params MakeV3Params(const Consensus::Params& base)
{
    Consensus::Params p = base;
    p.nAsertActivationHeight = 5;
    p.nAsertHalfLife = 3 * 60 * 60;
    p.nAsertRttStartDelay = 15 * 60 + 2 * 60; // FTL + T
    p.nAsertRttHalfLife = 15 * 60;
    p.nDifficultyV2ForkHeight = 0;
    p.fPowAllowMinDifficultyBlocks = false;
    p.fPowNoRetargeting = false;
    return p;
}

BOOST_AUTO_TEST_CASE(asert_on_schedule_unchanged)
{
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1d00ffff);
    const int64_t T = 120;
    const int64_t halfLife = 3 * 60 * 60;

    arith_uint256 next = CalculateASERT(ref, T, /*nTimeDiff=*/T, /*nHeightDiff=*/0, powLimit, halfLife);
    BOOST_CHECK_EQUAL(next.GetCompact(), ref.GetCompact());
}

BOOST_AUTO_TEST_CASE(asert_behind_schedule_eases)
{
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1c0fffff);
    const int64_t T = 120;
    const int64_t halfLife = 3 * 60 * 60;

    const int64_t nTimeDiff = T + halfLife;
    arith_uint256 next = CalculateASERT(ref, T, nTimeDiff, 0, powLimit, halfLife);

    arith_uint256 twice = ref << 1;
    BOOST_CHECK(next > ref);
    arith_uint256 lo = (twice * 999) / 1000;
    arith_uint256 hi = (twice * 1001) / 1000;
    BOOST_CHECK(next >= lo && next <= hi);
}

BOOST_AUTO_TEST_CASE(asert_ahead_schedule_hardens)
{
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1c0fffff);
    const int64_t T = 120;
    const int64_t halfLife = 3 * 60 * 60;

    const int64_t nHeightDiff = halfLife / T;
    const int64_t nTimeDiff = T;
    arith_uint256 next = CalculateASERT(ref, T, nTimeDiff, nHeightDiff, powLimit, halfLife);

    arith_uint256 half = ref >> 1;
    BOOST_CHECK(next < ref);
    arith_uint256 lo = (half * 999) / 1000;
    arith_uint256 hi = (half * 1001) / 1000;
    BOOST_CHECK(next >= lo && next <= hi);
}

BOOST_AUTO_TEST_CASE(asert_clamps_to_pow_limit)
{
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref = powLimit >> 1;
    const int64_t T = 120;
    const int64_t halfLife = 3 * 60 * 60;
    const int64_t nTimeDiff = T + halfLife * 40;
    arith_uint256 next = CalculateASERT(ref, T, nTimeDiff, 0, powLimit, halfLife);
    BOOST_CHECK(next <= powLimit);
    BOOST_CHECK_EQUAL(next.GetCompact(), powLimit.GetCompact());
}

BOOST_AUTO_TEST_CASE(asert_activation_height_matches_timewarp)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& c = chainParams->GetConsensus();
    BOOST_CHECK_EQUAL(c.nAsertActivationHeight, c.nMaxFutureBlockTimeActivationHeight);
    BOOST_CHECK_EQUAL(c.nAsertActivationHeight, 190000);
    BOOST_CHECK_EQUAL(c.nAsertHalfLife, 3 * 60 * 60);
    BOOST_CHECK_EQUAL(c.nAsertRttStartDelay, 15 * 60 + 2 * 60);
    BOOST_CHECK_EQUAL(c.nAsertRttHalfLife, 15 * 60);
}

BOOST_AUTO_TEST_CASE(asert_chain_stall_recovers)
{
    // Prior tip gap of 3h: baseline ASERT alone must ease next target.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    Consensus::Params params = MakeV3Params(chainParams->GetConsensus());

    const int N = 40;
    std::vector<CBlockIndex> blocks(N);
    const int64_t t0 = 1700000000;
    const unsigned int nBits = 0x1c0fffff;

    for (int i = 0; i < N; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        if (i == 0) {
            blocks[i].nTime = t0;
        } else if (i < N - 1) {
            blocks[i].nTime = blocks[i - 1].nTime + params.nPowTargetSpacing;
        } else {
            blocks[i].nTime = blocks[i - 1].nTime + 3 * 60 * 60; // 3h gap on tip
        }
        blocks[i].nBits = nBits;
        blocks[i].nStatus = BLOCK_VALID_TREE;
        blocks[i].phashBlock = nullptr;
    }

    // On-time candidate: RTT silent (st = T << rtt_start); only baseline acts
    CBlockHeader hdr;
    hdr.nTime = blocks[N - 1].nTime + params.nPowTargetSpacing;

    unsigned int nextBits = GetNextWorkRequired(&blocks[N - 1], &hdr, params);

    arith_uint256 tipTarget, nextTarget;
    tipTarget.SetCompact(nBits);
    nextTarget.SetCompact(nextBits);
    BOOST_CHECK(nextTarget > tipTarget);
}

BOOST_AUTO_TEST_CASE(asert_rtt_silent_before_delay)
{
    // Normal / FTL-window solvetimes: RTT must not change nBits vs pure baseline.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    Consensus::Params params = MakeV3Params(chainParams->GetConsensus());
    Consensus::Params noRtt = params;
    noRtt.nAsertRttHalfLife = 0;

    const int N = 20;
    std::vector<CBlockIndex> blocks(N);
    const int64_t t0 = 1700000000;
    const unsigned int nBits = 0x1c0fffff;
    for (int i = 0; i < N; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = t0 + i * params.nPowTargetSpacing;
        blocks[i].nBits = nBits;
        blocks[i].nStatus = BLOCK_VALID_TREE;
        blocks[i].phashBlock = nullptr;
    }

    // st = rtt_start exactly → still silent (only st > delay engages)
    CBlockHeader atDelay;
    atDelay.nTime = blocks[N - 1].nTime + params.nAsertRttStartDelay;
    // st just under delay
    CBlockHeader under;
    under.nTime = blocks[N - 1].nTime + params.nAsertRttStartDelay - 1;

    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[N - 1], &atDelay, params),
        GetNextWorkRequired(&blocks[N - 1], &atDelay, noRtt));
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[N - 1], &under, params),
        GetNextWorkRequired(&blocks[N - 1], &under, noRtt));
}

BOOST_AUTO_TEST_CASE(asert_rtt_eases_only_after_delay)
{
    // Past FTL+T: RTT engages; further excess → easier than at the threshold.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    Consensus::Params params = MakeV3Params(chainParams->GetConsensus());

    const int N = 20;
    std::vector<CBlockIndex> blocks(N);
    const int64_t t0 = 1700000000;
    const unsigned int nBits = 0x1c0fffff;
    for (int i = 0; i < N; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = t0 + i * params.nPowTargetSpacing;
        blocks[i].nBits = nBits;
        blocks[i].nStatus = BLOCK_VALID_TREE;
        blocks[i].phashBlock = nullptr;
    }

    CBlockHeader atStart;
    atStart.nTime = blocks[N - 1].nTime + params.nAsertRttStartDelay; // no RTT yet
    CBlockHeader deepStall;
    deepStall.nTime = blocks[N - 1].nTime + params.nAsertRttStartDelay + 15 * 60; // +1 RTT half-life excess

    unsigned int bitsStart = GetNextWorkRequired(&blocks[N - 1], &atStart, params);
    unsigned int bitsDeep = GetNextWorkRequired(&blocks[N - 1], &deepStall, params);

    arith_uint256 tStart, tDeep;
    tStart.SetCompact(bitsStart);
    tDeep.SetCompact(bitsDeep);
    BOOST_CHECK(tDeep > tStart);
}

BOOST_AUTO_TEST_SUITE_END()
