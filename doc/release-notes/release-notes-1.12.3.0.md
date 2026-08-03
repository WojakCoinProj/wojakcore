WojakCore version 1.12.3.0
==========================

Notable changes
===============

Fix daemon crash on startup on aarch64 (arm64)
----------------------------------------------

The Linux aarch64 binaries crashed with `SIGSEGV` immediately after logging
`scheduler thread start`, so arm64 Docker hosts (Apple Silicon, Graviton) never
reached a usable node and containers restarted in a loop.

The cause was the depends build of Boost 1.59 (2015), whose `boost::bind` /
`boost::function` headers are miscompiled by modern GCC on aarch64: the
scheduler thread dispatched its first `boost::function` call with a corrupt
`this` pointer. Depends now builds Boost 1.70, and libevent is updated from
2.0.22 to 2.1.12-stable alongside it.

Operators running arm64 no longer need the `platform: linux/amd64` emulation
workaround in `docker-compose.yml`.

`bytespersigop` no longer rejects sigop-dense transactions
----------------------------------------------------------

Transactions below the `MAX_STANDARD_TX_SIGOPS` limit (16,000) are no longer
rejected for sigop *density*. Following Bitcoin Core 0.13, `-bytespersigop`
is now a fee-based policy: a transaction's virtual size for relay and mining
is `max(actual size, sigops * bytespersigop)`, so dense scripts pay more fee
instead of being dropped.

Bitcoin Computer transactions now relay with default settings. Node operators
and mining pools no longer need `bytespersigop=0` in `wojakcoin.conf`.

This is a policy (standardness) change only. Block validity, including
`MAX_BLOCK_SIGOPS`, is unchanged, so upgraded and non-upgraded nodes stay in
consensus.

New RPC: `generatetoaddress`
---------------------------

`generatetoaddress numblocks address` mines blocks directly to a given address
on regtest, without requiring wallet keypool state. Backported from Bitcoin
Core 0.13; `generate` is unchanged.

1.12.3.0 change log
===================

Policy
------

- Apply `-bytespersigop` as a fee-based policy instead of rejecting
  sigop-dense transactions

RPC
---

- Add `generatetoaddress` (regtest)

Build system
------------

- depends: update Boost 1.59 -> 1.70, fixing the aarch64 startup crash
- depends: update libevent 2.0.22 -> 2.1.12-stable
- ci: smoke-boot the built daemon on regtest in both Linux release jobs, so
  aarch64 startup failures fail the release instead of shipping

Credits
=======

Thanks to the Bitcoin Computer team for the integration testing and the
detailed report in bitcoin-computer/monorepo#456.
