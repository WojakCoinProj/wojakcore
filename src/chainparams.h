// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2024 WojakCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMS_H
#define BITCOIN_CHAINPARAMS_H

#include <chainparamsbase.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <protocol.h>

#include <memory>
#include <vector>

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;

    int GetHeight() const {
        const auto& final_checkpoint = mapCheckpoints.rbegin();
        return final_checkpoint->first;
    }
};

struct ChainTxData {
    int64_t nTime;
    int64_t nTxCount;
    double dTxRate;
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * WojakCoin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    int GetDefaultPort() const { return nDefaultPort; }

    const CBlock& GenesisBlock() const { return genesis; }
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    bool RequireStandard() const { return fRequireStandard; }
    bool IsTestChain() const { return m_is_test_chain; }
    bool IsMockableChain() const { return m_is_mockable_chain; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    uint64_t AssumedBlockchainSize() const { return m_assumed_blockchain_size; }
    uint64_t AssumedChainStateSize() const { return m_assumed_chain_state_size; }
    bool MineBlocksOnDemand() const { return consensus.fPowNoRetargeting; }
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<std::string>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP() const { return bech32_hrp; }
    const std::vector<SeedSpec6>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const ChainTxData& TxData() const { return chainTxData; }

    int GetDevelopmentFundStartHeight() const { return vDevelopmentFundStartHeight; }
    int GetLastDevelopmentFundBlockHeight() const { return vDevelopmentFundLastHeight; }
    double GetLPIncentivePercent() const { return 0.08; }
    double GetDevVaultPercent() const { return 0.02; }

    std::string GetLPIncentiveAddressAtHeight(int height) const;
    CScript GetLPIncentiveScriptAtHeight(int height) const;
    std::string GetDevVaultAddressAtHeight(int height) const;
    CScript GetDevVaultScriptAtHeight(int height) const;
    std::vector<unsigned char> GetLPIncentiveOPReturn() const { return vLPIncentiveOPReturn; }
    std::vector<unsigned char> GetDevVaultOPReturn() const { return vDevVaultOPReturn; }
protected:
    CChainParams() {}

    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    int nDefaultPort;
    uint64_t nPruneAfterHeight;
    uint64_t m_assumed_blockchain_size;
    uint64_t m_assumed_chain_state_size;
    std::vector<std::string> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string bech32_hrp;
    std::string strNetworkID;
    CBlock genesis;
    std::vector<SeedSpec6> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool m_is_test_chain;
    bool m_is_mockable_chain;
    CCheckpointData checkpointData;
    ChainTxData chainTxData;

    std::vector<std::string> vLPIncentiveAddress;
    std::vector<std::string> vDevVaultAddress;
    int vDevelopmentFundStartHeight;
    int vDevelopmentFundLastHeight;
    std::vector<unsigned char> vLPIncentiveOPReturn;
    std::vector<unsigned char> vDevVaultOPReturn;
};

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain);

const CChainParams &Params();

void SelectParams(const std::string& chain);

#endif // BITCOIN_CHAINPARAMS_H
