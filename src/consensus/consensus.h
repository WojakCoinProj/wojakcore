// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <cstdint>

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 1000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 20;  // WojakCoin: 20 blocks maturity

/** Maximum allowed block timestamp in the future (seconds). Tighter than BTC to reduce time-warp attack on difficulty retarget. */
static const int64_t MAX_FUTURE_BLOCK_TIME = 15 * 60;  // 15 minutes (after activation)
/** Legacy limit before activation (2 hours, same as Bitcoin). */
static const int64_t MAX_FUTURE_BLOCK_TIME_LEGACY = 2 * 60 * 60;
/** Activation time (Unix timestamp): use 15-min limit for blocks with nTime >= this (chain time, not system time). e.g. 2026-03-19 00:00:00 UTC. */
static const int64_t ACTIVATE_MAX_FUTURE_BLOCK_TIME_15MIN = 1773878400;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
