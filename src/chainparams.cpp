// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2024 WojakCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <key_io.h>
#include <script/standard.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the WojakCoin genesis block.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "382017 Price Phillip Retires";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x000000004536a4f8fa9d88f0001ca9f9825f8d9fd3ba6383a2f030c0427bf085");
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000000004536a4f8fa9d88f0001ca9f9825f8d9fd3ba6383a2f030c0427bf085");
        consensus.BIP65Height = std::numeric_limits<int>::max();
        consensus.BIP66Height = std::numeric_limits<int>::max();
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60;
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.nDifficultyV2ForkHeight = 1000;
        consensus.nMaxReorgDepth = 20;
        consensus.nReorgLimitActivationHeight = 151600;
        consensus.nMaxFutureBlockTimeActivationHeight = 190000;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916;
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = std::numeric_limits<int>::max();
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000010000");
        consensus.defaultAssumeValid = uint256S("0x00");

        // WojakCoin message start (magic bytes)
        pchMessageStart[0] = 0x6f;
        pchMessageStart[1] = 0x8d;
        pchMessageStart[2] = 0xa5;
        pchMessageStart[3] = 0x79;
        nDefaultPort = 20759;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 0;

        // WojakCoin genesis block
        genesis = CreateGenesisBlock(1501724714, 1252099851, 0x1d00ffff, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000004536a4f8fa9d88f0001ca9f9825f8d9fd3ba6383a2f030c0427bf085"));
        assert(genesis.hashMerkleRoot == uint256S("0x2d94b8253252a0bf3c5202b26388dd3c468ab0bec4aad107b84d46ef6e8b791a"));

        vSeeds.clear();
        vSeeds.emplace_back("wojak-seed.s3na.xyz");

        // WojakCoin addresses start with 'W'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,73);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,201);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "wj";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        // Development fund: LP incentive pool (8%) + Dev vault (2%) = 10%
        // Miners retain 90% + all transaction fees
        vLPIncentiveAddress = {
            "3BNf3MaohVD2xskcdTJjpJN327ncweZzjk",
        };
        vDevVaultAddress = {
            "38gywG5YhmpDXNHEtT4KpFBovvoRsZNoVa",
        };
        vDevelopmentFundStartHeight = 180000; // WojakCoin: dev fund not yet activated, set to real height when forking
        vDevelopmentFundLastHeight = 1500000;

        checkpointData = {
            {
                { 0, uint256S("0x000000004536a4f8fa9d88f0001ca9f9825f8d9fd3ba6383a2f030c0427bf085") },
                { 500, uint256S("0x000000000000000e9e48539d842e3bc080a8e9821335665883957cb47903a87f") },
                { 600, uint256S("0x0000000000000010394cccce3cfd3379c10a2b27a360c52890cbbd02f1a27110") },
            }
        };

        chainTxData = ChainTxData{
            1501724714,
            0,
            100.0
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = std::numeric_limits<int>::max();
        consensus.BIP66Height = std::numeric_limits<int>::max();
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60;
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.nDifficultyV2ForkHeight = 1000;
        consensus.nMaxReorgDepth = 200;
        consensus.nReorgLimitActivationHeight = 0;
        consensus.nMaxFutureBlockTimeActivationHeight = 0;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512;
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = std::numeric_limits<int>::max();
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000001f4f4f4f4");
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0x4d;
        pchMessageStart[1] = 0xaa;
        pchMessageStart[2] = 0x61;
        pchMessageStart[3] = 0xf9;
        nDefaultPort = 30759;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1501724714, 1252099851, 0x1d00ffff, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "twj";

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        vLPIncentiveAddress = {
            "2N2vs76WqJwiPAfPAJavcSFMJETznm7w9Hy",
        };
        vDevVaultAddress = {
            "2MzFC111aKEKZj9unZagCSCB59H1bdm3Vwp",
        };
        vDevelopmentFundStartHeight = 2000; // WojakCoin: dev fund not yet activated, set to real height when forking
        vDevelopmentFundLastHeight = 500000;

        checkpointData = {
            {
                { 0, uint256S("0x000000004536a4f8fa9d88f0001ca9f9825f8d9fd3ba6383a2f030c0427bf085") }
            }
        };

        chainTxData = ChainTxData{
            1501724714,
            0,
            100.0
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = -1;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 0;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60;
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.nDifficultyV2ForkHeight = 0;
        consensus.nMaxReorgDepth = 0;
        consensus.nReorgLimitActivationHeight = 0;
        consensus.nMaxFutureBlockTimeActivationHeight = 0;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0;
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000001");
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xf3;
        pchMessageStart[1] = 0x3b;
        pchMessageStart[2] = 0xaf;
        pchMessageStart[3] = 0x6b;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x15891fb03fdcde8900d286178d5c4cda8e28d590ba0ea233b76eba8cf0ac4763"));

        vFixedSeeds.clear();
        vSeeds.clear();

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        vLPIncentiveAddress = {
            "2N2vs76WqJwiPAfPAJavcSFMJETznm7w9Hy",
        };
        vDevVaultAddress = {
            "2MzFC111aKEKZj9unZagCSCB59H1bdm3Vwp",
        };
        vDevelopmentFundStartHeight = 5; // WojakCoin: dev fund not yet activated, set to real height when forking
        vDevelopmentFundLastHeight = 5000;

        checkpointData = {
            {
                {0, uint256S("0x15891fb03fdcde8900d286178d5c4cda8e28d590ba0ea233b76eba8cf0ac4763")},
            }
        };

        chainTxData = ChainTxData{
            1296688602,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "wjrt";
    }
};

void SetupChainParamsBaseOptions(ArgsManager& argsman);

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<const CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<const CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::SIGNET)
        return std::unique_ptr<const CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<const CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}

std::string CChainParams::GetLPIncentiveAddressAtHeight(int nHeight) const
{
    if (vLPIncentiveAddress.empty())
        return "";
    size_t addressChangeInterval = (vDevelopmentFundLastHeight - vDevelopmentFundStartHeight + 1) / vLPIncentiveAddress.size();
    if (addressChangeInterval == 0) addressChangeInterval = 1;
    size_t i = (nHeight - vDevelopmentFundStartHeight) / addressChangeInterval;
    if (i >= vLPIncentiveAddress.size())
        i = vLPIncentiveAddress.size() - 1;
    return vLPIncentiveAddress[i];
}

CScript CChainParams::GetLPIncentiveScriptAtHeight(int nHeight) const
{
    std::string addr = GetLPIncentiveAddressAtHeight(nHeight);
    if (addr.empty())
        return CScript();
    CTxDestination dest = DecodeDestination(addr);
    if (!IsValidDestination(dest))
        return CScript();
    const ScriptHash* scriptID = boost::get<ScriptHash>(&dest);
    if (!scriptID)
        return CScript();
    return CScript() << OP_HASH160 << ToByteVector(*scriptID) << OP_EQUAL;
}

std::string CChainParams::GetDevVaultAddressAtHeight(int nHeight) const
{
    if (vDevVaultAddress.empty())
        return "";
    size_t addressChangeInterval = (vDevelopmentFundLastHeight - vDevelopmentFundStartHeight + 1) / vDevVaultAddress.size();
    if (addressChangeInterval == 0) addressChangeInterval = 1;
    size_t i = (nHeight - vDevelopmentFundStartHeight) / addressChangeInterval;
    if (i >= vDevVaultAddress.size())
        i = vDevVaultAddress.size() - 1;
    return vDevVaultAddress[i];
}

CScript CChainParams::GetDevVaultScriptAtHeight(int nHeight) const
{
    std::string addr = GetDevVaultAddressAtHeight(nHeight);
    if (addr.empty())
        return CScript();
    CTxDestination dest = DecodeDestination(addr);
    if (!IsValidDestination(dest))
        return CScript();
    const ScriptHash* scriptID = boost::get<ScriptHash>(&dest);
    if (!scriptID)
        return CScript();
    return CScript() << OP_HASH160 << ToByteVector(*scriptID) << OP_EQUAL;
}
