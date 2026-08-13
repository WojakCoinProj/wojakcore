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

/* ---- ASERT (aserti3-2d) unit tests ---- */

BOOST_AUTO_TEST_CASE(asert_on_schedule_unchanged)
{
    // If the chain is exactly on schedule, target stays (nearly) equal to anchor.
    // nHeightDiff = 0, nTimeDiff = T  →  exponent uses (T - T*(0+1)) = 0
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1d00ffff);
    const int64_t T = 120;
    const int64_t halfLife = 7200;

    arith_uint256 next = CalculateASERT(ref, T, /*nTimeDiff=*/T, /*nHeightDiff=*/0, powLimit, halfLife);
    // Exactly on schedule → identical compact target
    BOOST_CHECK_EQUAL(next.GetCompact(), ref.GetCompact());
}

BOOST_AUTO_TEST_CASE(asert_behind_schedule_eases)
{
    // One full half-life behind schedule → target roughly doubles (easier).
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1c0fffff); // mid-range target
    const int64_t T = 120;
    const int64_t halfLife = 7200;

    // nHeightDiff=0, time = T + halfLife  →  one half-life late for the next block
    const int64_t nTimeDiff = T + halfLife;
    arith_uint256 next = CalculateASERT(ref, T, nTimeDiff, 0, powLimit, halfLife);

    // next ≈ 2 * ref  (within polynomial approximation tolerance)
    arith_uint256 twice = ref << 1;
    // Allow small relative error from cubic 2^x approx
    BOOST_CHECK(next > ref);
    // next should be within ~0.1% of 2*ref
    arith_uint256 lo = (twice * 999) / 1000;
    arith_uint256 hi = (twice * 1001) / 1000;
    BOOST_CHECK(next >= lo && next <= hi);
}

BOOST_AUTO_TEST_CASE(asert_ahead_schedule_hardens)
{
    // One full half-life ahead of schedule → target roughly halves (harder).
    const arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 ref;
    ref.SetCompact(0x1c0fffff);
    const int64_t T = 120;
    const int64_t halfLife = 7200;

    // nHeightDiff=0, time = T - halfLife (clamped by reality but math works)
    // Better: heightDiff large with small time → ahead of schedule
    // nHeightDiff = halfLife/T = 60 blocks, nTimeDiff = T  (only 1 interval of wall time for 60 blocks)
    // exponent = (T - T*(60+1)) / halfLife = T*(-60)/halfLife = 120*-60/7200 = -1.0 → half
    const int64_t nHeightDiff = halfLife / T; // 60
    const int64_t nTimeDiff = T; // only one spacing of wall time
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
    // Start near powLimit; large positive lag should clamp
    arith_uint256 ref = powLimit >> 1;
    const int64_t T = 120;
    const int64_t halfLife = 7200;
    // Many half-lives late
    const int64_t nTimeDiff = T + halfLife * 40;
    arith_uint256 next = CalculateASERT(ref, T, nTimeDiff, 0, powLimit, halfLife);
    BOOST_CHECK(next <= powLimit);
    BOOST_CHECK_EQUAL(next.GetCompact(), powLimit.GetCompact());
}

BOOST_AUTO_TEST_CASE(asert_activation_height_matches_timewarp)
{
    // Mainnet consensus: ASERT and 15-min future-time activate together.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& c = chainParams->GetConsensus();
    BOOST_CHECK_EQUAL(c.nAsertActivationHeight, c.nMaxFutureBlockTimeActivationHeight);
    BOOST_CHECK_EQUAL(c.nAsertActivationHeight, 190000);
    BOOST_CHECK_EQUAL(c.nAsertHalfLife, 2 * 60 * 60);
}

BOOST_AUTO_TEST_CASE(asert_chain_stall_recovers)
{
    // Simulate multipool leave: 24 on-schedule blocks, then a 30-minute stall
    // at the tip. Next target must be strictly easier than the tip's target.
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    Consensus::Params params = chainParams->GetConsensus();
    // Force ASERT on for synthetic chain at low height
    params.nAsertActivationHeight = 10;
    params.nAsertHalfLife = 2 * 60 * 60;
    params.nDifficultyV2ForkHeight = 0; // skip V2
    params.fPowAllowMinDifficultyBlocks = false;
    params.fPowNoRetargeting = false;

    const int N = 40;
    std::vector<CBlockIndex> blocks(N);
    const int64_t t0 = 1700000000;
    const unsigned int nBits = 0x1c0fffff;

    for (int i = 0; i < N; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        // On schedule until last block; last gap is 30 minutes
        if (i == 0) {
            blocks[i].nTime = t0;
        } else if (i < N - 1) {
            blocks[i].nTime = blocks[i - 1].nTime + params.nPowTargetSpacing;
        } else {
            blocks[i].nTime = blocks[i - 1].nTime + 30 * 60; // 30 min stall
        }
        blocks[i].nBits = nBits;
        blocks[i].nStatus = BLOCK_VALID_TREE;
        blocks[i].phashBlock = nullptr;
    }

    // Build fake header for GetNextWorkRequired (timestamp not used by ASERT)
    CBlockHeader hdr;
    hdr.nTime = blocks[N - 1].nTime + params.nPowTargetSpacing;

    unsigned int nextBits = GetNextWorkRequired(&blocks[N - 1], &hdr, params);

    arith_uint256 tipTarget, nextTarget;
    tipTarget.SetCompact(nBits);
    nextTarget.SetCompact(nextBits);
    // After a 30-minute stall, ASERT must ease (larger target)
    BOOST_CHECK(nextTarget > tipTarget);
}

BOOST_AUTO_TEST_SUITE_END()
