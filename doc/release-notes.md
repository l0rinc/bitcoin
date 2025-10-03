Bitcoin Core version 29.2rc2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.2/test.rc2/>

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

- #32646 p2p: Add witness mutation check inside FillBlock
- #33296 net: check for empty header before calling FillBlock
- #33395 net: do not apply whitelist permissions to onion inbounds

### Mempool

- #33504 mempool: Do not enforce TRUC checks on reorg

### RPC

- #33446 rpc: fix getblock(header) returns target for tip

### CI

- #32989 ci: Migrate CI to hosted Cirrus Runners
- #32999 ci: Use APT_LLVM_V in msan task
- #33099 ci: allow for any libc++ intrumentation & use it for TSAN
- #33258 ci: use LLVM 21
- #33364 ci: always use tag for LLVM checkout

### Doc

- #33484 doc: rpc: fix case typo in `finalizepsbt` help

### Misc

- #33310 trace: Workaround GCC bug compiling with old systemtap
- #33340 Fix benchmark CSV output
- #33482 contrib: fix macOS deployment with no translations

Credits
=======

Thanks to everyone who directly contributed to this release:

- Amisha Chhajed
- Eugene Siegel
- fanquake
- Greg Sanders
- Hennadii Stepanov
- Luke Dashjr
- MarcoFalke
- Martin Zumsande
- Sebastian Falbesoner
- Sjors Provoost
- Vasil Dimov
- Will Clark

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
