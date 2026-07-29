WojakCore version 1.12.2.0
==========================

Notable changes
===============

Raise per-transaction policy sigops limit
-----------------------------------------

`MAX_STANDARD_TX_SIGOPS` is raised from 4,000 to 16,000 (policy/standardness
only). This unblocks Bitcoin Computer and similar metaprotocols that were
rejected with `bad-txns-too-many-sigops`.

`MAX_BLOCK_SIGOPS` remains 20,000 (no consensus change). Older nodes stay more
restrictive on relay and do not fork.

See: https://github.com/bitcoin-computer/monorepo/pull/456#issuecomment-5037946183

1.12.2.0 change log
===================

Policy
------

- Raise `MAX_STANDARD_TX_SIGOPS` from 4,000 to 16,000
