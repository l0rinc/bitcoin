Bitcoin Core version 29.3rc2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.3/test.rc2/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoinknots/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoinknots.org/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from very old versions of Bitcoin Core or Knots is possible,
but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Knots are generally supported.

Compatibility
==============

Bitcoin Knots is supported on operating systems using the Linux kernel, macOS
13+, and Windows 10+. It is not recommended to use Bitcoin Knots on
unsupported systems.

Known Bugs
==========

In various locations, including the GUI's transaction details dialog and the
`"vsize"` result in many RPC results, transaction virtual sizes may not account
for an unusually high number of sigops (ie, as determined by the
`-bytespersigop` policy) or datacarrier penalties (ie, `-datacarriercost`).
This could result in reporting a lower virtual size than is actually used for
mempool or mining purposes.

Due to disruption of the shared Bitcoin Transifex repository, this release
still does not include updated translations, and Bitcoin Knots may be unable
to do so until/unless that is resolved.

Notable changes
===============

### P2P

- #33050 net, validation: don't punish peers for consensus-invalid txs
- #33723 chainparams: remove dnsseed.bitcoin.dashjr-list-of-p2p-nodes.us

### Validation

- #32473 Introduce per-txin sighash midstate cache for legacy/p2sh/segwitv0 scripts
- #33105 validation: detect witness stripping without re-running Script checks

### Wallet

- #33268 wallet: Identify transactions spending 0-value outputs, and add tests for anchor outputs in a wallet
- #34156 wallet: fix unnamed legacy wallet migration failure
- #34226 wallet: test: Relative wallet failed migration cleanup
- #34123 wallet: migration, avoid creating spendable wallet from a watch-only legacy wallet
- #34215 wallettool: fix unnamed createfromdump failure walletsdir deletion
- #34370 wallet: Additional cleanups for migration, and fixes for createfromdump with BDB

### Mining

- #33475 bugfix: miner: fix `addPackageTxs` unsigned integer overflow

### Build

- #34227 guix: Fix `osslsigncode` tests

### Documentation

- #33623 doc: document capnproto and libmultiprocess deps in 29.x

### Test

- #33612 test: change log rate limit version gate

### Misc

- #32513 ci: remove 3rd party js from windows dll gha job
- #33508 ci: fix buildx gha cache authentication on forks
- #33581 ci: Properly include $FILE_ENV in DEPENDS_HASH
- #34344 ci: update GitHub Actions versions

Credits
=======

Thanks to everyone who directly contributed to this release:

- Anthony Towns
- Antoine Poinsot
- Ava Chow
- David Gumberg
- Eugene Siegel
- fanquake
- furszy
- Hennadii Stepanov
- ismaelsadeeq
- luke-jr
- m3dwards
- Padraic Slattery
- Pieter Wuille
- SatsAndSports
- sedited
- willcl-ark

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
