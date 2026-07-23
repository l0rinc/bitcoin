# Review of the 200 most recently active open PRs on bitcoin/bitcoin

Date: 2026-07-22/23. Method: every PR head fetched (refs/remotes/pr/*), diff vs origin/master,
adversarial review for merge-blockers only (consensus, UB/crashes from untrusted input, data/fund loss,
cheap DoS, auth/privacy regressions, broken builds). 'Deep read' = full diff read; 'skim' = diffstat +
targeted read. 6 merge-blockers found (35735, 35387, 27409, 26022, 32387 + 30437 marginal).

## PR 34628 — p2p: Replace per-peer transaction rate-limiting with global rate limits
author: ajtowns | updated: 2026-07-23 | commits: 13

- 349c72ee00 net_processing: Drop unnecessary txid arg from InitiateTxBroadcastToAll
- 12b0dc33c4 doc: Add release note for -txsendrate etc
- 5cde66341a tests: basic functional test for tx rate limiting
- 4842903ac1 rpc: report -txsendrate and bucket info via getnetworkinfo
- 74a47a5207 init: add -txsendrate configuration parameter
- 6307bd034b net_processing: Provide a 30bpm heartbeat log while inv backlog is in use
- df31ee57aa net_processing: add a global delay queue for sending txs
- 7927650e56 util/tokenbucket.h: Provide a generic TokenBucket class
- 749bb447f8 txmempool: Drop CompareMiningScoreWithTopology
- e1b7490fbc net_processing: Replace CompareInvMempoolOrder
- … +3 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32554 — bench: replace embedded raw block with configurable block generator
author: l0rinc | updated: 2026-07-23 | commits: 3

- 83c1df6e59 bench: remove embedded `block413567.raw` fixture
- dbfe817239 bench: switch helper to generated blocks and recipes
- 719ec22a49 bench: route legacy fixture through helper APIs

**Finding:** 🟡 GeomCount infinite loop for geometric_base_prob in (0.99,1) — draw 0-99 always < thresh*100 (src/bench/block_generator.cpp:45-54). Clamp to <=0.99. [yours]

## PR 34683 — rpc: support a formal description of our JSON-RPC interface
author: willcl-ark | updated: 2026-07-23 | commits: 9

- ca9ffb8e12 rpc: add OpenRPC discovery alias
- ef0676f400 rpc: factor getaddressinfo embedded field docs
- 1fb6b60560 test: add functional test for getopenrpcinfo
- 672dd42d14 rpc: add getopenrpcinfo command
- f5116c587f rpc: add placeholder annotation for deprecated params
- 26c221a980 rpc: expose RPC metadata for introspection
- 6a1a66c180 rpc: render Type::ANY in help text instead of aborting
- 06de34a033 rpc: erase empty map entry in removeCommand
- d4d64ae739 rpc: add missing string_view include to server.h

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 29700 — kernel, refactor: return error status on all fatal errors
author: ryanofsky | updated: 2026-07-23 | commits: 23

- c28899d137 test-each-commit: Increase fetch depth
- 6542bfe144 refactor, validation: Return more errors from VerifyDB
- 719b09533f refactor, blockstorage: Return fatal error from ImportBlocks
- 36ede1f7d1 refactor, validation: Return fatal errors from new block functions
- 2999cf7382 refactor, validation: Return fatal errors from activate best chain functions
- 69ef13ead2 refactor, validation: Return fatal errors from block connect functions
- 01de9f6426 refactor, validation: Return fatal errors from assumeutxo snapshot functions
- e0622fd7c0 refactor, validation: Return fatal errors from mempool accept functions
- 6dc2c1376a refactor, validation: Return fatal errors from FlushStateToDisk
- 4704e208e1 refactor, blockstorage: Return fatal error from LoadBlockIndex
- … +13 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33922 — mining: add getMemoryLoad() and track template non-mempool memory footprint
author: Sjors | updated: 2026-07-23 | commits: 16

- 239af38f70 ipc: add getMemoryLoad()
- 1fa2359a7d mining: add GetTemplateMemoryUsage()
- cc6993abc0 mining: track non-mempool memory usage
- 630e18e7d3 refactor: move CTransactionRefComp to util/hasher
- e56d837a97 test: add BlockTemplateManager fuzz test
- ccc5d59ac4 ci: enforce iwyu for block template manager
- eebfebb521 node: remove unused mining interface storage and accessor
- 5361489892 test: create templates via BlockTemplateManager
- c28287b32e rpc: wait for tips via BlockTemplateManager
- 8e7173fed4 rpc: use BlockTemplateManager in generate rpc's
- … +6 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35581 — node: add block template manager and track waitNext fee inflow
author: ismaelsadeeq | updated: 2026-07-23 | commits: 19

- 828a7e3528 miner: use tracked fee inflow for waitNext
- a1c67245db node: add block template staleness detection
- f2acefe725 node: update template snapshots from mempool callbacks
- 4c6af5e055 node: add block template snapshot tracking
- f22cc6a674 miner: track selected chunks in block templates
- 9ee0bd722a test: add unit test for mempool update validation events
- 667a35361e mempool: add and fire MempoolUpdated signal on each mempool update path
- 5673157131 refactor: move-only: extract dependency addition into a separate method
- 35b5ebe6e9 mempool: add SnapshotDiagrams to ChangeSet
- 26001a065a mempool: cache fee rate diagrams as chunks in ChangeSet
- … +9 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35646 — RFC: Separate out runtime errors from BlockValidationState using `util::Expected`
author: yuvicc | updated: 2026-07-23 | commits: 5

- 1811b21e72 test: FatalError type
- 93994bb593 kernel: remove INTERNAL_ERROR validation mode
- 5054e98909 consensus: remove ValidationState error mode
- 18088f226e validation: return fatal errors with `util::Expected`
- fe7d2e86bc kernel: add FatalError type

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35675 — mining: add block template manager
author: ismaelsadeeq | updated: 2026-07-23 | commits: 15

- c2dde7de2c test: fuzz BlockTemplateManager
- 9da8d90a94 ci: enforce iwyu for block template manager
- 4a739e2bba node: remove NodeContext::mining and EnsureMining
- 760b8eda10 test: create templates via BlockTemplateManager
- 63eb429a79 rpc: wait for tips via BlockTemplateManager
- cce9542b57 rpc: build generation templates via BlockTemplateManager
- 3bac51f413 rpc: only copy the header in getblocktemplate
- 6f1b57b041 rpc: do not copy template data in getblocktemplate
- 4872a56210 rpc: build getblocktemplate via BlockTemplateManager
- 28ecc7105a rpc: route getblocktemplate internals through node
- … +5 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32729 — test, refactor: extract script template helpers and expand sigop coverage
author: l0rinc | updated: 2026-07-23 | commits: 18

- eadd2c2b22 fuzz: cover all opcode bytes
- d2d9494ec2 refactor: name base-script opcode checks
- 3c0a28035d test: cover opcode name table
- 39164937d0 refactor: add `IsPayToWitnessPubKeyHash`
- f0698d9904 refactor: extract `IsUncompressedPayToPubKey`
- 00808d1f9e refactor: extract `IsCompressedPayToPubKey`
- 757bdf8750 refactor: extract `IsPayToPubKeyHash`
- ad7c1fc92b refactor: move `IsPayToTaproot` to header
- 5ac399cf9d refactor: move `IsPayToAnchor` to header
- ca7dad28dd refactor: move `IsPayToWitnessScriptHash`
- … +8 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35752 — RFC wallet: handle encryption database write failures
author: l0rinc | updated: 2026-07-23 | commits: 4

- 26c88d8631 wallet: abort failed descriptor key encryption
- e93e0b8057 wallet: reject failed passphrase changes
- a1ece8bb95 wallet: abort encryption after master key write failure
- a623590904 test: characterize wallet encryption write failures

**Finding:** 🟠 encryptwallet I/O failure now hits assert(false) -> node abort on disk-full; m_map_crypted_keys populated before batch write -> spkm un-retryable; WriteCryptedDescriptorKey still discards EraseIC (plaintext+crypted both committed). Fix pattern: local-map accumulate, swap on success, return false. [yours]

## PR 35688 — crypto: accept empty HMAC keys
author: l0rinc | updated: 2026-07-22 | commits: 2

- dc67c4cc39 fuzz: remove stale eval_script empty-input guard
- b80907909c crypto: accept empty HMAC keys

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33324 — blocks: add resumable reobfuscation for existing block files
author: l0rinc | updated: 2026-07-22 | commits: 8

- 22790069f6 gui: report block reobfuscation progress
- f501c89270 blocks: reobfuscate block and undo files
- afa2e3e81e node: log when block file XOR is inactive
- 97f2dcdb20 node: extract block and undo file discovery
- 843f0c7e38 node: extract XOR key file helpers
- 191b116039 util: add `Obfuscation` key helpers
- b550b73a6e util: add non-throwing `Remove` helper
- 3d727c3102 index: remove always-false obfuscation parameter

**Finding:** 🟠 missing DirectoryCommit after Remove(xor.dat) — crash mid-rename can persist renamed new-key files but roll back the unlink -> restart re-migrates converted files (double-XOR corruption of blk/rev). Also -blocksxor=0 trap: no use_xor check, log line invites the fatal combo. [yours]

## PR 35696 — i2p: update leaseset encryption types
author: jpk68 | updated: 2026-07-22 | commits: 1

- 412540e18d i2p: update leaseset encryption types

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35605 — wallet: rpc: Deprecate `removeprunedfunds` RPC
author: davidgumberg | updated: 2026-07-22 | commits: 2

- 44c3268c76 wallet: rpc: deprecate removeprunedfunds
- e5b7785447 test: wallet: resend: avoid internal behavior via removeprunedfunds

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35780 — http: linger-close after parse errors so clients can read the reply
author: b-l-u-e | updated: 2026-07-22 | commits: 2

- 2e991f89e5 http: linger-close after parse errors so clients can read the reply
- 010f606e11 util: add Sock::ShutdownSend() to half-close the send side

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 28690 — build: Introduce internal kernel library
author: sedited | updated: 2026-07-22 | commits: 2

- 9b88b1b64b Update docs and check-deps for util kernel lib
- 9a177d2b75 Introduce internal kernel library

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34400 — wallet: parallel fast rescan (approx 8x speed up with 8 threads)
author: Eunovo | updated: 2026-07-22 | commits: 15

- ae888e0dc9 tests: update wallet_fast_rescan to cover serial fast scan
- 8027e1cef9 wallet/tests: exercise parallel fast-rescan
- eab65562b3 wallet/scan: save and log scan progress every `INTERVAL_TIME`
- 1cfd6a3916 wallet/scan: patch parallel filter verdicts with top-up deltas
- 4534de0811 wallet/scan: check blockfilters in parallel
- 33faeab50c wallet: Add wallet parallel processing threads param
- 3ffb908812 wallet/scan: combine block iteration and filtering in ReadNextBlocks
- b4953b1026 wallet/scan: extract progress tracking helpers from `ChainScanner::Scan`
- 86db854414 wallet/scan: extract QueueNextBlock
- 6b6690e55a wallet/scan: extract block scanning logic to ScanBlock
- … +5 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35751 — validation: use parallel input prevout fetching in TestBlockValidity
author: andrewtoth | updated: 2026-07-22 | commits: 1

- 985d248383 validation: use parallel input prevout fetching in TestBlockValidity

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34808 — cmake, translation: Use native Qt TS file as source for translations on Transifex
author: hebasto | updated: 2026-07-22 | commits: 5

- a434d66025 cmake, translation: Specify English as target language explicitly
- 4097d6d968 cmake, translation: Sort messages within contexts alphabetically
- 312ab8ab0a cmake, translation: Skip source locations in TS files
- 4f553bd0da cmake, translation: Remove TS to XLIFF conversion
- 8c30055458 translation: Switch to Qt TS source file

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35768 — wallet: Reject whitespace-only wallet names
author: vicjuma | updated: 2026-07-22 | commits: 1

- 8464a0e37a wallet: Reject whitespace-only wallet names

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35502 — refactor: extract per-message helpers from ProcessMessage (move-only)
author: w0xlt | updated: 2026-07-22 | commits: 7

- 4fe745f27b move-only: Extract ProcessTx() helper
- 19b613f4d7 move-only: Extract ProcessSendTxRcncl() helper
- 400857c65c move-only: Extract ProcessInv() helper
- fb9216672c move-only: Extract ProcessGetHeaders() helper
- cb03c39cc5 move-only: Extract ProcessGetBlocks() helper
- bc9154a4b2 move-only: Extract ProcessGetDataMessage() helper
- 2afa25ef8e move-only: Extract ProcessGetAddr() helper

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35694 — clusterlin: minor SFL optimizations
author: sipa | updated: 2026-07-22 | commits: 3

- efb4eae338 clusterlin: avoid recomputing intersections in MergeChunks
- 4b91ad149f clusterlin: reserve the suboptimal-chunk queue up front
- e6ca996255 clusterlin: avoid heap allocations in GetLinearization

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35180 — coins: group private cache helpers
author: l0rinc | updated: 2026-07-22 | commits: 1

- c9cedebfff coins: group private cache helpers

**Finding:** 🟠 build break on current master: ResetGuard moved private but CoinsViewOverlay::StartFetching names it. Rebase + keep ResetGuard public/protected. [yours]

## PR 32387 — ipc: add windows support
author: ryanofsky | updated: 2026-07-22 | commits: 52

- aa2892e82a lint: Skip subtree check for src/ipc/libmultiprocess
- c47f97a376 ci: Add missing ipc and tests vcpkg features to Windows fuzz build
- 1b01c61815 ci: Install capnproto via vcpkg and pycapnp for Windows IPC support
- ecb8ced3f1 ci: Enable IPC in Windows cross-compiled CI jobs
- 7edc243370 ipc: enable by default in windows builds
- d3f8630230 cmake: Add add_windows_exe_resources() for Windows exe manifest/version
- de35762333 test: Fix interface_ipc_cli.py error assertions on Windows
- 41376864cd test: Add Windows AF_UNIX asyncio support to ipc_util.py
- e572695e2c test: Enable tool_bitcoin.py test on Windows
- 9da1ce7ba5 bitcoin: Fix IPC child process exec on Windows due to missing .exe extension
- … +42 more commits

**Finding:** 🟠 tip commit aa2892e82a is self-declared DO NOT MERGE (disables libmultiprocess subtree lint + edits subtree directly). Fine as draft, must not merge as-is. Branch also force-pushed to a new topic (Windows IPC) — only skimmed.

## PR 35522 — refactor: Extract per-message helpers from SendMessages() (move-only)
author: pablomartin4btc | updated: 2026-07-22 | commits: 17

- 06741a6f9d move-only: Extract MaybeSendTxMessages() helper
- 1baa4160a2 move-only: Extract ComputeSyncBlocksAndHeaders() helper
- 062920c034 move-only: Extract QueueTxGetData() helper
- 0518a402df move-only: Extract QueueBlocksGetData() helper
- 0164ba0c2b move-only: Extract CheckHeadersSyncTimeout() helper
- 608b92fdac move-only: Extract CheckBlockFlightTimeout() helper
- 2790fecd56 move-only: Extract CheckBlockDownloadStall() helper
- c656841fcc move-only: Extract SendBlockInvFallback() helper
- 6d022c3135 move-only: Extract SendCompactBlockOrHeaders() helper
- 6eb3f3e051 move-only: Extract MaybeSendGetData() helper
- … +7 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35657 — refactor: avoid redundant input lookup in `CheckTxInputs`
author: arejula27 | updated: 2026-07-22 | commits: 3

- 5b02e4b4c2 test: cover missing-input outpoint in CheckTxInputs
- 5021cd02fa consensus: avoid redundant input lookup in CheckTxInputs
- d31c26fd1a bench: add CheckTxInputs benchmark

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35561 — net: move some CNodeState fields to Peer
author: Crypt-iQ | updated: 2026-07-22 | commits: 2

- 765f65971a net: move fPreferredDownload to Peer, make atomic
- 0954ae6696 net: move cmpctblocks fields from CNodeState to Peer

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35440 — wallet: check descriptor cache xpub length before decoding
author: alhudz | updated: 2026-07-22 | commits: 1

- c6076d9100 wallet: check descriptor cache xpub length before decoding

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35665 — psbt: avoid duplicate global xpub keys when merging
author: thomasbuilds | updated: 2026-07-22 | commits: 3

- 6d387af562 psbt: remove write-only global xpub tracking set
- 3b7051c7e3 test: check combinepsbt with conflicting global xpub origins
- 7c632c0e2a psbt: avoid duplicate global xpub keys when merging

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35037 — ipc: support per-address max-connections options on -ipcbind
author: enirox001 | updated: 2026-07-22 | commits: 4

- c6d2cd8d0c doc: add ipcbind max-connections release note
- 2b3c6ddd6c ipc: test per-address connection limiting over unix sockets
- b5c72a4c42 ipc: support per-address max-connections on ipcbind
- 3b9075a0b6 ipc: add default connection limit for ipcbind listeners

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35592 — http: check rpcallowip immediately after accepting connection
author: pinheadmz | updated: 2026-07-22 | commits: 2

- 55d3cd51a4 doc: add release note describing change for forbidden clients
- d1ed2a6e25 http: check rpcallowip immediately after accepting connection

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35775 — scripted-diff: Use C.UTF-8 locale in Guix scripts
author: hebasto | updated: 2026-07-22 | commits: 2

- 80f831494e guix: Fix `glibc` version in comment
- 8916f7967e scripted-diff: Use C.UTF-8 locale in Guix scripts

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 32764 — guix: Build for macOS using LLVM toolchain only
author: hebasto | updated: 2026-07-22 | commits: 2

- 8a90c7cd97 guix: Build for macOS using LLVM toolchain only
- 7e1a750d45 guix, refactor: Use `target` variable instead of hardcoded value

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35773 — test: Suppress implicit-unsigned-integer-truncation:SaltedCoinsCacheHasher::operator()
author: maflcko | updated: 2026-07-22 | commits: 1

- fa7f553781 test: Suppress implicit-unsigned-integer-truncation:SaltedCoinsCacheHasher::operator()

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35762 — test: optionally run functional tests via CTest
author: willcl-ark | updated: 2026-07-22 | commits: 6

- 16c4245b58 ci: exercise functional tests through CTest on macOS
- 19d0706b68 doc: document CTest functional testing
- 2f55235ee7 build: register functional tests with CTest
- 93579583ed test: add direct CTest runner mode
- 532f647643 test: reuse combined log renderer in test runner
- 757fc8f0e0 test: emit ctest manifest

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35662 — script: make txdata non-default-constructible
author: l0rinc | updated: 2026-07-22 | commits: 4

- 92470f76bd script: make txdata construction mandatory
- 48ffc45096 validation: defer txdata construction with optional
- b9bf725181 script: construct txdata at remaining eager callers
- a65853daeb script: construct txdata at simple call sites

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34861 — wallet: Add importdescriptors interface
author: polespinasa | updated: 2026-07-22 | commits: 11

- 2c862726fc wallet: Add an importDescriptors() interface for the wallet
- 3ed46ebe5a wallet: Move ImportDescriptor and ProcessDescriptorsImport to imports.cpp
- b915f495df wallet: rename ProcessDescriptorImport to ImportDescriptor and add ProcessDescriptorsImport
- e65c3cd4b1 wallet: rpc: refactor: ProcessDescriptorImport returns ImportDescriptorResult
- 17772c66a6 wallet: Add ImportError struct and new WalletError codes
- 5cf175032a wallet: rpc: refactor: Extract UniValue processing from ProcessDescriptorImport
- 922e1c5146 wallet: Add ImportDescriptorRequest structs
- 51705a0ae1 wallet: refactor: make is_ranged no longer an optional
- 577974ef7a wallet: lower the minimum timestamp to 0
- 3b8653b9e3 wallet: rpc: Use std::optional in GetImportTimestamp
- … +1 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35501 — wallet: store all witness variants of a transaction
author: achow101 | updated: 2026-07-22 | commits: 10

- fa5cbb8909 uint256: Workaround GCC-14 stringop-overread bug in Compare
- 6c9d76d589 doc: release note for alternate_wtxids in gettransaction
- 99bdcb064c test: compat, ensure downgrade preserves tx witness variants
- ef2afc6a0a test: Test for wallet txs with alternate wtxids
- 2d55c7a74d wallet: Show alternate wtxids in gettransaction
- 0b1af01bd4 wallet: Replace CWalletTx::SetTx with Update
- 56cf27db4d wallet: Store all witness variants of a transaction
- 798ba6d04f wallet: Make CWalletTx::tx private and use CWalletTx::GetTx to access
- 72ebdd6364 wallet: Remove unused CWalletTx CopyFrom and copy constructor
- 19af439bdf wallet: Deserialize directly in CWalletTx's ctor

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35735 — Add state to HTTPRequest
author: pinheadmz | updated: 2026-07-22 | commits: 3

- 94cd5de8a1 test: cover HTTPRequest state machine
- b3f75fd99c Add state to HTTPRequest to avoid duplicate work over I/O cycles
- 5820294a92 http: only read one HTTPRequest at a time per client

**Finding:** 🟠 chunked-encoding regression: zero-chunk trailer spanning two TCP reads wedges the connection (return false after full request) or spurious 400 with a real trailer line — m_chunk_size stays engaged(0) across the I/O boundary (src/httpserver.cpp LoadBody). MUST FIX.

## PR 35713 — Remove boost as a unit test runner
author: rustaceanrob | updated: 2026-07-22 | commits: 17

- 3eed6264ad vcpkg: drop boost-test dependency
- 980553c925 cmake: drop vcpkg Boost Test check
- ab856e002d depends: drop test from Boost libraries
- 145f52b6bf test: Require `stringify` implementation or explicit omission
- 65f9a6f6b3 test: Add `stringify` in tests when applicable
- 2ddf9a3c85 scripted-diff: Migrate tests to header-only framework
- 20e858e081 test: Add header-only framework to util
- e62aab11fe test: Wrap bitwise `&` expressions in macros
- aa8a2b8d4d test: Wrap `||` expressions in macros
- 0ed5397acd test: Unroll `&&` conditions in macros
- … +7 more commits

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35260 — doc: clarify test placement guidance
author: l0rinc | updated: 2026-07-22 | commits: 1

- db74d3390a doc: clarify test placement guidance

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35699 — [31.x] More Backports
author: fanquake | updated: 2026-07-22 | commits: 131

- 57b14acbd8 doc: update release notes for v31.x
- c02ff3e793 p2p: Assume v2transport for addresses from seeds
- 444f2d7965 fuzz, refactor: Remove `Serialize` overload
- 504ded6b0e fuzz: Remove unused `DeserializeFromFuzzingInput` params overload
- ec1123aed1 rpc: define and use new  RPC_LIMIT_EXCEEDED error code
- cd32ebd3f4 Release cs_main between individual private tx re-attempts
- c1a0ed5225 private broadcast: limit outstanding txs to count of 10,000
- ef7542aa29 net: delay stale evaluation and expose time_added in private broadcast
- 0086a236cb net: introduce TxSendStatus internal state container
- b4dd6ba49b chainparams: delete my DNS seed
- … +121 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35531 — txindex: hash keys and pack positions to reduce disk usage
author: andrewtoth | updated: 2026-07-22 | commits: 6

- 20a8aaef4e doc: add release notes for txindex disk usage and stale block lookups
- 693dfa6adf tests: cover txindex hash prefix collisions and legacy fallback
- be3bf13c7a txindex: skip bloom filters and legacy lookups for new databases
- ce62a0f5ea txindex: hash key prefixes and pack block positions
- ef564e622e txindex: use a new block locator for downgrade safety
- 24dfbd081a txindex: make TxIndex::FindTx [[nodiscard]]

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34794 — rest: add Cache-Control headers to REST responses
author: w0xlt | updated: 2026-07-22 | commits: 4

- 22e7805c2d doc: add release note for REST cache-control headers
- 95a66cfff6 doc: document REST cache-control defaults
- fa4c5be123 http: add no-store to unmatched 404s
- 2befe0bd3a rest: add Cache-Control headers to REST responses

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35772 — http: drop connections from RPC clients that are not allowed to connect
author: janb84 | updated: 2026-07-22 | commits: 2

- 72757520d5 http: disconnect rejected RPC clients immediately instead of allowing open connections
- 6d75ebd064 test: check the server disconnects a rejected RPC client

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35759 — fuzz: check http_request body matches framing
author: Ameen-Alam | updated: 2026-07-22 | commits: 1

- 7502b9ddba fuzz: check http_request body matches framing

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 25573 — guix: produce a `-static-pie` bitcoind
author: fanquake | updated: 2026-07-22 | commits: 5

- 88a03c6f3c guix: build x86_64-linux bitcoind & utils statically
- 3059d13cd1 guix: drop -Werror=dev
- ffb539f6d4 guix: adapt symbol and security checks for static ELF
- 3d2b9c5ff5 guix: add glibc 2.44 (master)
- 086b2b1627 guix: split linux toolchain for static builds

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35710 — [30.x] More Backports
author: fanquake | updated: 2026-07-22 | commits: 222

- b872171c31 doc: update release notes for v30.x
- 28234bd827 p2p: Assume v2transport for addresses from seeds
- 3a43bb54f5 chainparams: delete my DNS seed
- 49faec4f87 Merge bitcoin/bitcoin#35668: [30.x] Finalise 30.3
- 3e3c3d66de doc: update manual pages for v30.3
- 0a1d238b25 doc: update release notes for v30.3
- ad5b171a7b build: bump version to v30.3
- 7bc403c1a8 Merge bitcoin/bitcoin#35452: [30.x] 30.3rc1
- d52747d8e7 doc: update manual pages for v30.3rc1
- a1406d6333 build: bump version to v30.3rc1
- … +212 more commits

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34844 — util: Add util::NotNull<SmartPtrType>
author: maflcko | updated: 2026-07-22 | commits: 6

- fa6228d332 refactor: Return util::NotNull<std::unique_ptr<ChangeSet>> from CTxMemPool::GetChangeSet()
- fae7ff51b0 refactor: Use derived move-conversion
- fa2a5253d8 refactor: In CNode use util::NotNull<std::unique_ptr<Transport>> m_transport
- fa1645cc19 refactor: Use NotNull pointer to input fetching pool
- fa551ed729 refactor: Use util::NotNull<std::unique_ptr<LevelDBContext>> m_db_context
- faa79c756b util: Add util::NotNull<SmartPtrType>

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35778 — lint: Skip `libmultiprocess` subtree in `lint-shell-locale.py`
author: hebasto | updated: 2026-07-22 | commits: 1

- 2a669b6bfb lint: Skip `libmultiprocess` subtree in `lint-shell-locale.py`

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35774 — ci: test cross-built macos arm64 binaries
author: willcl-ark | updated: 2026-07-22 | commits: 3

- ab86ef440b ci: test macos cross arm64 binaries
- 672f8f0238 ci: move macos cross into own job
- 138df7d930 ci: enable cross-archive/os caching

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35187 — kernel: Add non-utxo set block validation to API
author: sedited | updated: 2026-07-22 | commits: 4

- a77602ed7d kernel: Add sans utxo set block validation
- d48fe4a4a7 kernel: Add outpoint creation to C header
- 5c9189d48e kernel: Add coin creation to C header
- 644f73541f kernel: Add transaction is coinbase to C header

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34075 — fees: Introduce Mempool Based Fee Estimation to reduce overestimation
author: ismaelsadeeq | updated: 2026-07-22 | commits: 17

- f7ba53dc90 doc: add release notes
- 0ff5c4ead7 test: add mempool estimator i/o fuzz test
- 249ff2a297 fees: persist mempool policy estimator data
- 2c5d733f06 fees: rename and move fee_estimates.dat to fees/block_policy_estimates.dat
- 07d65a526e rpc: add verbosity param to estimatesmartfee to expose mempool coverage stats
- 58a18577d9 fees: only return mempool fee rate estimate when mempool is healthy
- 3cb45dd7c1 validation: pass connected block txs to mempool removal callbacks
- 9a7c2af412 fees: return mempool estimates when it's lower than block policy
- a568fc7ac7 fees: add caching to MemPoolFeeRateEstimator
- 8c9e853f59 fees: add MemPoolFeeRateEstimator class
- … +7 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34681 — wallet: move rescan logic into ChainScanner and wallet/scan
author: Eunovo | updated: 2026-07-22 | commits: 8

- b4953b1026 wallet/scan: extract progress tracking helpers from `ChainScanner::Scan`
- 86db854414 wallet/scan: extract QueueNextBlock
- 6b6690e55a wallet/scan: extract block scanning logic to ScanBlock
- 9eb81e1f73 wallet/scan: extract block filter matching to ShouldFetchBlock
- 9295cab7b7 wallet/scan: move WalletRescanReserver to scan files
- 0f6591dc43 wallet/scan: move RescanFromTime to ChainScanner as ScanFromTime
- 65d9e2fab9 wallet: introduce ChainScanner as a CWallet member
- 217efa59c7 wallet/tests: pin ScanForWalletTransactions behavior

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35750 — addrman: make `m_last_good` network-specific
author: mzumsande | updated: 2026-07-22 | commits: 2

- 31191b5239 addrman: make m_last_good network-specific
- b8840fd1c6 addrman: don't count connection failures before the first success

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35730 — http: limit connected HTTPRemoteClients
author: pinheadmz | updated: 2026-07-22 | commits: 4

- 779e722fba init: do not count file descriptors for HTTPServer if -server=0
- a08f67791b init: account for maximum file descriptors needed by HTTP
- 3680a95b55 http: configure simultaneous connection limit with -rpcmaxconnections
- 548e995e57 http: limit connected clients to 128

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 30342 — kernel, logging: Pass Logger instances to kernel objects
author: ryanofsky | updated: 2026-07-22 | commits: 18

- 6105b072d2 test-each-commit: Increase fetch depth
- 846df5f6f4 kernel: Drop global Logger instance
- 299a6c593b refactor: Log kernel output to local log instances
- 52caf3c63f logging: Add LOG_REQUIRE_CONTEXT option
- 5fa829c394 refactor: Pass Logger instances to kernel objects
- b69266aca1 Merge branch 'pr/klog' into pr/gklog
- 7fd1dfde07 Merge branch 'pr/bclog' into pr/gklog
- e06a5c8737 kernel: Simplify logging API
- 54603d2a4d log refactor: Add support for custom log contexts
- 5e9609717c doc: Add documentation about log levels and macros
- … +8 more commits

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 31260 — scripted-diff: Type-safe settings retrieval
author: ryanofsky | updated: 2026-07-22 | commits: 11

- 0612ffcb49 scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls
- 9fc7b98870 contrib: Add script to replace AddArgs / GetArgs calls with Setting Register / Get calls
- eb5fea3820 cmake: Add univalue, boost, and libevent include directories
- 46563da5c1 lint: Fixes for _settings.h headers
- 019d2d34e0 refactor: Prepare AddHiddenArgs call for scripted-diff
- 56a43d69ea move-only: move node constants to settings header
- 3429229bef move-only: move AddArg default values to headers
- 7172157a87 init, refactor: Prepare AddArg calls for scripted-diff
- f0ee850f57 test: Add test for common::Setting class
- f0fd678653 common: Add Setting class to support typed Settings
- … +1 more commits

**Verdict:** 🟢 no merge-blocker (skim: 3000-line scripted diff, spot-checked + verify script)

## PR 31349 — ci: detect outbound internet traffic generated while running tests
author: vasild | updated: 2026-07-22 | commits: 1

- 84bb496c61 ci: detect outbound internet traffic generated while running tests

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35315 — refactor: Use NodeClock::time_point in more places
author: maflcko | updated: 2026-07-22 | commits: 10

- fab4bd7f2b refactor: Use NodeClock for last GetTime call in net_processing.cpp
- fa58d8f4fb refactor: Use time_point::min()/max() in net_processing
- fa8a149c29 refactor: Use time_point::max() for m_headers_sync_timeout
- fa42ebd0d1 refactor: Use NodeClock::time_point instead of std::chrono::microseconds in net_processing
- fa4d22d06c refactor: Drop HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER "precision"
- fab33fdd92 p2p: Use NodeClock::time_point instead of std::chrono::seconds in node stats and eviction
- fa593c2e6e refactor: Use NodeClock::time_point in txdownloadman/txrequest
- fafd1e3312 p2p: Use NodeClock::time_point for m_last_block_announcement
- fa208c5b24 refactor: Use NodeSeconds for m_best_block_time
- fa562e4c3d refactor: Allow NodeClock::epoch to be used in NodeSeconds context

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32162 — depends: Switch from multilib to platform-specific toolchains
author: hebasto | updated: 2026-07-22 | commits: 1

- de9b436ba3 depends: Switch from multilib to platform-specific toolchains

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35261 — guix: disable LTO in GCC
author: fanquake | updated: 2026-07-22 | commits: 5

- 313ed191e9 guix: pass --disable-tm-clone-registry to base GCC
- 63809be6d1 guix: mirror some arguments from linux-gcc to mingw-w64-gcc
- d8549b449d guix: disable-nls in *-base-gcc
- 4e327f815b guix: disable-lto in *-base-gcc
- b989c31243 guix: modernise style in *-base-gcc

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35737 — test: Move cluster_linearize.h contents into cluster_linearize namespace
author: hebasto | updated: 2026-07-22 | commits: 1

- 21b4b790e4 test: Move cluster_linearize.h contents into cluster_linearize namespace

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35482 — fuzz: exercise the transaction-handling path in process_message(s)
author: HowHsu | updated: 2026-07-22 | commits: 5

- 87b080fe2b fuzz: reset the reused mempool in process_message(s)
- d522fd3196 fuzz: prepare deterministic mempool rebuilds
- b11456386b fuzz: let the test input toggle IBD in the p2p fuzz targets
- 2a29cee684 test: add helper to reset chainman and mempool
- 2a4ef42d34 fuzz: share a single FakeNodeClock in the chainman-resetting fuzz targets

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35641 — kernel: Add script evaluation tracer
author: sedited | updated: 2026-07-22 | commits: 1

- 114e48e968 kernel: Add script tracer

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 29843 — policy: Allow non-standard scripts with -acceptnonstdtxn=1 (test nets only)
author: ajtowns | updated: 2026-07-22 | commits: 5

- 2bf3c6421f validation: Ensure STANDARD_SCRIPT_VERIFY_FLAGS is a superset of GetBlockScriptFlags
- daea759c18 validation: Check only (potential) block script flags when -acceptnonstdtxn is set
- 1eb2d8828f validation: Be explicit about whether CheckInputScripts is for block/mempool
- 4c9a00f565 tests: test tx rejection for both -acceptnonstdtxn=0 and 1
- 9b6342e5d2 tests: in p2p_segwit, check non-mandatory errors with -acceptnonstdtxn=0 node

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33637 — refactor: optimize block index comparisons (1.4-6.8x faster)
author: l0rinc | updated: 2026-07-22 | commits: 7

- 703165e6d3 refactor: optimize `arith_uint256` comparison with spaceship operator
- 8ef4ce2cb3 refactor: inline `arith_uint256` comparison operator
- d053b27c06 refactor: optimize `CBlockIndexWorkComparator` with `std::tie`
- 70d3b99351 fuzz: add equivalence coverage for comparison refactors
- 516e05cd7d move-only: inline `CBlockIndex` comparators to header
- f682c2be38 test: add sorting tests for `CBlockIndexWorkComparator` and `arith_uint256`
- 70c6244b70 bench: add benchmark to measure `CBlockIndexWorkComparator` performance

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33854 — fix assumevalid is ignored during reindex
author: Eunovo | updated: 2026-07-22 | commits: 8

- ce8eeac9be test: script ver is skipped during reindex with -assumevalid
- 6db0136604 test/reindex: only connect minchainwork chain
- 9484be15aa init: keep bg init thread until headers sync is done
- 9a48a6cc39 test: update anti-dos test
- 143c840df4 net: allow headers sync during reindex
- 3cdad27211 notifications: add header_tip mutex
- 54d68c17a6 importblocks: don't connect reindexed low work chains
- dc7aac5e19 importblocks: ensure genesis block is activated

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34729 — Reduce log noise
author: ajtowns | updated: 2026-07-22 | commits: 7

- 8d60c14285 validation: Upgrade log levels for possible block corruption warnings
- 7547e8cd58 txindex: Downgrade log messages and include index name
- a271ac3f90 kernel/coinstats: Downgrade ComputeUTXOStats log level
- 05b429eefc script: Lower level of oversized redeemScript log, push errors to callers
- 74c54e1d11 netbase: Tidy up logging levels
- 94a9c5b416 netbase: Reduce levels of socks5 error logging
- b5689ce8a0 util/log: Add LogWarnThenDebug() helper

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35692 — addrman: remove unreachable tried-collision branch
author: brunoerg | updated: 2026-07-22 | commits: 1

- bc7d905046 addrman: remove unreachable tried-collision branch

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34565 — refactor: extract BlockDownloadManager from PeerManagerImpl
author: w0xlt | updated: 2026-07-22 | commits: 4

- d867ea88fe fuzz: add BlockDownloadManager target
- 78c890442e refactor: migrate block download state and methods into BlockDownloadManager
- bd59d03008 test: add unit tests for BlockDownloadManager
- 6958f98094 refactor: add BlockDownloadManager class with pimpl pattern

**Verdict:** 🟢 no merge-blocker (skim: 1.9k-line move, medium depth on behavior drift)

## PR 35765 — depends: hash local source contents
author: willcl-ark | updated: 2026-07-22 | commits: 1

- b1f79c9fa6 depends: hash local source contents

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34743 — p2p: don't disconnect manual peers for block stalling
author: willcl-ark | updated: 2026-07-22 | commits: 4

- 0b9c9a517e doc: document manual peer stalling behavior
- 93b6443a7b test: cover manual peer block download cooldown
- f81fc10dd5 test: add manual p2p connection helper
- a889acf97c p2p: pause stalling manual block downloads

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35071 — Reindex: save progress to continue after interruption
author: pinheadmz | updated: 2026-07-22 | commits: 2

- 99d0d61cab blockstorage: save reindex progress upon interrupt to resume after restart
- fb7b803dac test: assert current interrupted-reindex behavior: wipe and start over

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35395 — doc: Improve test suite dependencies documentation
author: hebasto | updated: 2026-07-22 | commits: 4

- c14f888996 doc: Document `lsof` test suite dependency
- e25d9c8add doc: Unify Python optional module documentation
- 49f3366623 doc: Improve Python UTF-8 mode note
- 12b010a9d4 doc: Fix header formatting in `test/README.md`

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35570 — refactor: Change some validation.cpp methods to return BlockValidationState
author: optout21 | updated: 2026-07-22 | commits: 13

- e600c4ce02 Internal simplification in TestBlockValidity
- 3764f59254 Refactor AcceptBlock signature
- cd326cd348 Refactor FlushStateToDisk signature
- 020f4ba49a Refactor FatalError signature
- 8aa5f52aaf Refactor ContextualCheckBlock signature
- 36ad0af777 Refactor CheckWitnessMalleation signature
- e333011a92 Refactor CheckBlock signature
- 52204bc760 Refactor CheckMerkleRoot signature
- ce4935253e Change ProcessNewBlockHeaders
- 0d4eac9822 Refactor AcceptBlockHeader signature
- … +3 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35618 — depends: Make tarball creation from local directory reproducible
author: hebasto | updated: 2026-07-22 | commits: 1

- 7e973cce52 depends: Make tarball creation from local directory reproducible

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35558 — p2p: Prefill compact blocks
author: davidgumberg | updated: 2026-07-22 | commits: 16

- 9f36a29393 p2p: prefill our compact blocks with candidates
- 70c2b71d94 p2p: keep track of cmpctblock prefill candidates
- b6111762b1 net: Protect PeerHasHeader from nullptr
- 09579eeb15 net: refactor: Move common logic into SendCompactBlock
- d73d76c9ab p2p: Cache cmpct_block_msg for low bandwidth relay.
- d399ab14cd p2p: refactor: Stuff m_most_recent* into a struct
- 682d222722 net: Add CNode::WindowBytesTotalAndAvailable()
- 4d6e73c887 net: Add GetSendQueueSize()
- fa39ff2029 net: Add Transport::GetMessageSize() for serialized msg sizes
- 91a625170b sock: Add GetOSBytesQueued
- … +6 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35195 — coins: cache UTXO outpoint hash codes
author: l0rinc | updated: 2026-07-21 | commits: 2

- 16e77fdf13 util: allow caching outpoint hash codes
- fb0c207567 Revert "make SaltedOutpointHasher noexcept"

**Finding:** 🟢 no merge-blocker. Mechanically safe but reverts a measured memory optimization (67d99900b0, ~8B/node) — needs benchmark justification; test relies on libstdc++ internals. [yours]

## PR 33585 — cmake: Use builtin support for .manifest files
author: purpleKarrot | updated: 2026-07-21 | commits: 3

- 888fb1721a cmake: Unconditionally add .rc files to sources
- 14240322f4 cmake: Use builtin support for .manifest files
- 5e621f7705 cmake: Unconditionally set WIN32_EXECUTABLE target property

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35716 — wallet: Replace mapWallet and wtxOrdered with a boost::multi_index
author: achow101 | updated: 2026-07-21 | commits: 13

- c56a38e90e wallet: Replace mapWallet with multi index m_txs
- 40a47e7faa wallet: Mark CWalletTx::MarkDirty as const
- b8f9ee6814 wallet, test: Use AddToWallet instead of direct mapWallet.emplace
- 65f5862462 wallet: Replace many direct mapWallet lookups with GetWalletTx
- 37efd9dcad doc: release note for alternate_wtxids in gettransaction
- e21497ee39 test: compat, ensure downgrade preserves tx witness variants
- ef93a48c35 test: Test for wallet txs with alternate wtxids
- ff7beeaddd wallet: Show alternate wtxids in gettransaction
- 863ceee8a4 wallet: Replace CWalletTx::SetTx with Update
- ad2e9656e3 wallet: Store all witness variants of a transaction
- … +3 more commits

**Verdict:** 🟢 no merge-blocker (skim: medium depth on index invariants)

## PR 35368 — tracing: add block header and compact block tracepoints
author: w0xlt | updated: 2026-07-21 | commits: 2

- fbfeb8859c tracing: add compact block reconstruction tracepoint
- c45092fcd6 tracing: add block header tracepoint

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35724 — cmpctblock: Improve logging of `cmpctblock` message reconstruction statistics [part of prefill series]
author: davidgumberg | updated: 2026-07-21 | commits: 3

- cef79baeb2 cmpctblock: log: Print sizes of all tx types and prefill redundancies
- 35b79db9fd cmpctblock: log: Log extrapool separately from mempool
- 7fac2d1655 cmpctblock: log: debuglevel=trace print TXID's of all missing tx'es.

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35729 — refactor: test: Unroll `&&` conditions in macros
author: rustaceanrob | updated: 2026-07-21 | commits: 1

- 490327bf9d refactor: test: Unroll `&&` conditions in macros

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 25722 — refactor: Use util::Result class for wallet loading
author: ryanofsky | updated: 2026-07-21 | commits: 24

- 2bbdb75075 scripted-diff: replace wallet DatabaseStatus with DatabaseError
- 082580fa2c Drop temporary ResultExtract helper for porting to util::Result
- 0afedba86e refactor: Use util::Result class in wallet/test
- e6c4a98d49 refactor: Use util::Result class in wallet/interfaces
- 515737cc9b refactor: Use util::Result class in wallet/rpc
- 610ad9dece refactor: Use util::Result class in wallet/load
- 711b7b6b7a refactor: Use util::Result class in wallet/wallet
- e8da1aa209 refactor: Use util::Result class in wallet/wallettool
- ac3a5145e5 refactor: Use util::Result class in wallet/dump
- f09f74b833 refactor: Use util::Result class in wallet::MakeDatabase
- … +14 more commits

**Verdict:** 🟢 no merge-blocker (skim: 1.4k-line mechanical refactor, spot-checked)

## PR 35763 — util: atomically write banlist.json and rename WriteSettings to WriteJsonUnsafe
author: kevkevinpal | updated: 2026-07-21 | commits: 1

- 07ec5af152 util: atomically write banlist.json and rename WriteSettings to WriteJsonUnsafe

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35569 — Encapsulation for CTransaction
author: purpleKarrot | updated: 2026-07-21 | commits: 4

- a2b339547f CTransaction: Make data members private
- 858899ef52 bitcoin-tidy: Apply tx-observers fixup
- ede288d1c5 bitcoin-tidy: Add tx-observers check
- 2e69721360 CTransaction: Add observer member functions

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 30951 — net: option to disallow v1 connection on ipv4 and ipv6 peers
author: stratospher | updated: 2026-07-21 | commits: 6

- d72df0fc83 doc: add release notes for v2onlyclearnet option
- d0273e9e54 test: Check that v1 connections to clearnet peers don't work
- 7f1f96700b net: disable v1 connections, reconnections on clearnet
- 76e74c0bf5 init: add -v2onlyclearnet config option
- 51ce5ff696 net: add option in CConman to allow only v2 clearnet connections
- 66a312cc59 net: add helper to identify clearnet networks

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33034 — wallet: Store transactions in a separate sqlite table
author: achow101 | updated: 2026-07-21 | commits: 27

- 372e21d7fa walletdb: Load from transactions table and use original tx for upgrade
- 36dcf7f766 wallet: Perform automatic upgrade to using the transactions table
- 3bb591da7c wallet: Also write to the new transactions table
- 0095ce0751 walletdb: Add functions to modify transactions table
- fe187d75f4 sqlite: Iterate the transactions table with SQLiteCursor
- a55ed823b5 sqlite: Add functions and statements for transactions table
- c2808d7d8d sqlite: Create a 'transactions' table if it does not exist
- e37984a619 wallet: Add new serialization format functions for TxState
- 4a95c33801 wallet: Add Un/Serialize to TxState structs
- fd30af2271 sqlite: Add additional blob types to SQLiteStatement::Column
- … +17 more commits

**Finding:** 🟢 no merge-blocker. Corrupt-local-DB edges only: wrong-size txid blob hits new Txid assert instead of graceful CORRUPT; empty state_data blob would optional-deref (no write path produces it). Upgrade path idempotent and crash-safe.

## PR 35084 — ipc: Add nonunix platform support
author: ryanofsky | updated: 2026-07-21 | commits: 10

- 2d3f72fd3f ipc, refactor: Update mp::SpawnProcess call
- e9f19815ca ipc, refactor: Add Stream type alias and use it
- 3859805f05 ipc, refactor: Add SocketId type alias and use it
- 2ee9b69c7a ipc, refactor: Add ProcessId type alias and use it
- 3449797141 ipc: Avoid 'unistd.h' error with MSVC
- dbcc192dce ipc, refactor: fix include order
- 7c86d4834e ipc, refactor: use native path separators in test
- 00287b9a34 ipc, refactor: Change Protocol class field order
- 33d37f3c35 ipc, refactor: Drop connect/listen/serve exe_name parameters
- 794940469e ipc, moveonly: combine ipc_test.cpp and ipc_tests.cpp

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35551 — test: add interface_gui.py to test bitcoin-qt startup
author: ryanofsky | updated: 2026-07-21 | commits: 1

- aa01721c89 test: add interface_gui.py to test bitcoin-gui startup via RPC

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34721 — build: install shell completions via cmake
author: willcl-ark | updated: 2026-07-21 | commits: 1

- 0a2d2cd116 build: install shell completions via cmake

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35754 — ci: pin and verify external inputs
author: l0rinc | updated: 2026-07-21 | commits: 7

- 49cc4e8cab ci: pin GitHub Actions
- 0f0eb35c8b ci: pin container images
- 9bf68d70bc ci: lock test Python dependencies
- 5671b32614 ci: verify downloaded lint tool binaries
- 8ba28acb87 lint: lock Python dependencies with uv
- b4efe448a9 ci: pin Git sources used to build tools
- 3751aa3028 ci: verify cross-build SDK archives

**Finding:** 🟢 no merge-blocker. Enforcement verified. Nits: apt.llvm.org GPG key fetch unpinned; unknown arch fails closed with cryptic message; git --revision needs git>=2.49 (undocumented). [yours]

## PR 35722 — ci: cache BSD sdk sources separately
author: willcl-ark | updated: 2026-07-21 | commits: 3

- cd4b34c58d ci: unify cache path handling
- 732997364b ci: cache BSD SDK sources
- aa040360ce ci: move BSD SDK setup out of image build

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35760 — wallet: make corrupted transaction records fail wallet loading instead of forcing a rescan
author: achow101 | updated: 2026-07-21 | commits: 2

- 78d0f8338d wallet: Remove DBErrors::NEED_RESCAN and rescan_required
- 9d8d6f1cab walletdb: LoadToWallet failure is wallet corruption

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34730 — util/log: Combine the warning/error log levels into a single alert level
author: ajtowns | updated: 2026-07-21 | commits: 2

- 4ce720ba87 scripted-diff: Convert node/blockstorage.cpp to LogAlert
- 7113b17d85 util/log: Replace LogWarning/LogError with LogAlert

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 29278 — Wallet:  Add `maxfeerate` wallet startup option
author: ismaelsadeeq | updated: 2026-07-20 | commits: 9

- d46adaaaaa [doc]: add release notes
- 840e32efee [wallet]: warn when `-maxtxfee` conflicts with `-minrelaytxfee`
- 5973b3a07a [wallet]: enforce `-maxfeerate` on wallet transactions
- 66257be320 [util]: add a new transaction error type
- ba8cf49115 [node]: update `BroadcastTransaction` to check fee rate limit
- bbc034c570 [wallet]: add `maxfeerate` wallet startup option
- 5f1c95548c [wallet]: update `max_fee` to `max_tx_fee`
- 0d68721d46 doc: add missing verb to make sentence readable
- c921662a03 scripted-diff: rename `m_default_max_tx_fee` to `m_max_tx_fee`

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 30343 — wallet, logging: Replace WalletLogPrintf() with LogInfo()
author: ryanofsky | updated: 2026-07-20 | commits: 14

- d908afbad6 test-each-commit: Increase fetch depth
- 1ca4092fd8 wallet, logging: Switch LogInfo to LogWarning/LogError
- 1b8e2f75ce wallet, logging: Replace WalletLogPrintf() with LogInfo()
- 493ee79f51 Merge branch 'pr/bclog' into pr/gwlog
- b5d3925d27 log refactor: Add support for custom log contexts
- 1eedcb9db5 doc: Add documentation about log levels and macros
- aa37de26b0 log refactor: Allow log macros to accept context arguments
- 1ac1be8b15 Merge branch 'pr/relog' into pr/bclog
- f0929e1b78 log refactor: Drop Entry::should_ratelimit field
- 31f9929f3d log refactor: log macro rewrite
- … +4 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32895 — wallet: Prepare for future upgrades by recording versions of last client to open and decrypt
author: achow101 | updated: 2026-07-20 | commits: 4

- 3e4490241f wallet: Set last opened and decrypted features during migration
- edd5225982 wallet: Record the supported features of the last client to decrypt a wallet
- 9867ccac30 wallet: Introduce LastClientFeatures flags and LAST_OPENED_FEATURES record
- e40b36da59 walletdb: Decouple last client record from CLIENT_VERSION

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32784 — wallet: derivehdkey RPC to get xpub at arbitrary path
author: Sjors | updated: 2026-07-20 | commits: 12

- 3ebd59e4c5 doc: use derivehdkey in multisig tutorial
- 1d25ae2d70 test: use derivehdkey in M-of-N multisig demo
- 97e1757ab8 rpc: add derivehdkey
- c8b66d9dba wallet: add GetExtKey helper
- d38eb35f1e wallet: generalize GetActiveHDPubKeys helper
- a0466ae921 refactor: add hardened derivation helper
- a57ee7e151 rpc: ParsePathBIP32 helper
- 84b22f3608 util: reject out-of-range BIP32 keypath indices
- c4a8c49c97 fuzz: check ParseHDKeypath/WriteHDKeypath round-trip
- a88d85ff72 Have ParseHDKeypath handle h derivation marker
- … +2 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35026 — mempool: recalculate stale BIP68 lockpoints with mempool parents in removeForReorg
author: javierpmateos | updated: 2026-07-20 | commits: 1

- c71aa2db62 mempool: recalculate stale BIP68 lockpoints in removeForReorg

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35747 — wallet: Fix FillPSBT failing to sign owned inputs when UTXOs disagree
author: nervana21 | updated: 2026-07-20 | commits: 2

- 0a13744b73 wallet: Select FillPSBT SigningProvider via GetUTXO
- a5c3fd0993 test, doc: Pin PSBTInput::GetUTXO prefer rules

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34909 — wallet, refactor: modularise wallet by extracting out legacy wallet migration
author: rkrux | updated: 2026-07-20 | commits: 5

- 56b532324b wallet, refactor: rename file from migrate to legacybdb
- 675c982620 wallet: extract out migration specific LegacySPKM CWallet methods
- f74a107741 wallet: extract out public facing MigrateLegacyToDescriptor functions
- fde5cead50 wallet: extract out utility functions from wallet/wallet
- e6d1ffd685 wallet: (move-only) extract out migration specific CWallet methods

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35756 — cmake: De-duplicate libraries where possible
author: hebasto | updated: 2026-07-20 | commits: 1

- b7aea2db92 cmake: De-duplicate libraries where possible

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34995 — iwyu: Fix warnings in `src/common` and treat them as errors
author: hebasto | updated: 2026-07-20 | commits: 1

- 6fc28bffc4 ci, iwyu: Enforce warning-free `src/common`

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35731 — Indexes: Harden the flush-error notification invariant
author: arejula27 | updated: 2026-07-20 | commits: 4

- a4c7bf0fbf blockstorage: encapsulate flush+notify in FlushFile
- 42ea4bcccb test: cover FlushBlockFile flush-error notifications
- e1a337ee96 validation: stop writes after flush failure
- 0f04fbee2f test: characterize writes after flush failure

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34486 — net: Reduce local network activity when networkactive=0
author: willcl-ark | updated: 2026-07-20 | commits: 5

- d31aaef595 test: check tor control reconnection in p2p_private_broadcast
- 7b82d530e5 test: add torcontrol networkactive coverage
- a86099b7a0 net: run tor control based on networkactive
- bfce4f989b test: add test for mapport networkactive
- 1bf49e5475 net: wire mapport lifecycle to CConnman

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35738 — coins: parallel input prevout fetching followups
author: andrewtoth | updated: 2026-07-20 | commits: 5

- ae0b64a5d3 fuzz: use per-level fetch scopes in coinscache_sim
- 7b0264a10a doc: improve CoinsViewOverlay documentation
- 7499b121a9 coins: log error reason when prevout fetch submission fails
- 17d64ef1d2 coins: delete Sync and SetBackend on CoinsViewOverlay
- 87cf27fa0c coins: filter coinbase txid from parallel input fetching

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35753 — kernel: handle null mempool on chainstate deletion
author: l0rinc | updated: 2026-07-20 | commits: 1

- a99b27f192 validation: handle null mempool on delete

**Finding:** 🟢 no merge-blocker. Crash was real (assert executes in release); test covers ChainstateManager level, not the literal C API — acceptable. [yours]

## PR 27865 — wallet: Track no-longer-spendable TXOs separately
author: achow101 | updated: 2026-07-19 | commits: 19

- 96f4b4d9c4 wallet: Move definintely unusable TXOs to a separate container
- 4972a22d33 wallet, tests: Have CreateSyncedWallet use CWallet::Create
- c1aeeeed8d wallet: Iterate block txs in reverse on blockDisconnected
- 08b36d7517 wallet: Have IsSpent take a min_conf
- adb8079148 wallet: Remove unused WalletTXO::GetWalletTx()
- 94eb487641 wallet: Include transaction version in WalletTXO
- 5af4f8a983 wallet: Have WalletTXOs also store parent tx time
- f9678cd589 wallet: Store a copy of m_from_me in WalletTXOs and use for "from me"
- 5fc2c2f5ce walletdb: Move ReorderTransactions to immediately after loading txs
- 08bd3e87a3 wallet: Use WalletTXO stored state and coinbase rather than wtx
- … +9 more commits

**Finding:** 🟢 no merge-blocker. Nit: std::optional<bool> negation (!wtx->m_from_me checks has_value, not value) at src/wallet/receive.cpp:213 — behavior-neutral today (shadowed by IsMine guard).

## PR 35477 — test: exercise Schnorr signature cache in txvalidationcache_tests.cpp
author: theStack | updated: 2026-07-19 | commits: 3

- 3ba1bbfa3f test: exercise Schnorr signature cache in txvalidationcache_tests.cpp
- 198b36bc85 test: respect "TAPROOT requires WITNESS" rule in `ValidateCheckInputsForAllFlags`
- e78a2a0d00 test: refactor: simplify tx vin/vout creation in txvalidationcache_tests.cpp

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35671 — mining: add TxCollection to bandwidth-efficiently validate external block templates
author: Sjors | updated: 2026-07-19 | commits: 6

- 9bb89df02f mining: restrict externally generated templates
- 5fdd934905 mining: make TxCollection create a BlockTemplate
- 1611aadbed mining: add coinbase transaction helper
- ee1c43e3ce mining: add TxCollection addMissingTxs
- e7309ab7d2 mining: add TxCollection unknownTxPos
- 19bae00aa1 ipc: add TxCollection scaffold

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35744 — coins: prevent DB resize from invalidating cursors
author: l0rinc | updated: 2026-07-18 | commits: 1

- d3484251b3 coins: block DB resize during cursor iteration

**Finding:** 🟢 no merge-blocker. Correct cursor drain; by design resize now stalls cs_main during long UTXO scans (minutes) — worth a release note. [yours]

## PR 24230 — indexes: Stop using node internal types and locking cs_main, improve sync logic
author: ryanofsky | updated: 2026-07-18 | commits: 19

- 0cb43e15cb Remove direct index -> node dependency
- aa6ba79b75 indexes, refactor: Remove remaining CBlockIndex* pointers from indexing code
- dac7fb40bc indexes, refactor: Remove UndoReadFromDisk calls from indexing code
- ad0a689239 node: NotificationsProxy state enum instead of connected bool
- 12a20204dc indexes: Rewrite chain sync logic, simplify index sync code
- e09026d883 indexes, refactor: Remove SyncWithValidationInterfaceQueue call
- 630301a7da indexes: Move sync thread from index to node
- 19cb17fc0f test: prevent race condition in feature_init.py
- 7861db707b indexes: Add blockfilterindex mutex
- 93019aa23f indexes: Call notification handlers from Sync()
- … +9 more commits

**Finding:** 🟢 no merge-blocker. Nit: ReadBlockData reads undo data twice per block during index sync (perf only).

## PR 34697 — descriptor: fix musig duplicate checks and origin handling
author: shuv-amp | updated: 2026-07-18 | commits: 1

- 5d44a9a253 descriptor: fix musig duplicate checks and origin handling

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35742 — descriptors: check duplicate keys in all multipath Miniscript branches
author: yashbhutwala | updated: 2026-07-18 | commits: 1

- 560cfa24a8 descriptor: check duplicate keys in all multipath branches

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35733 — sign: Remove FillableSigningProvider
author: achow101 | updated: 2026-07-18 | commits: 6

- 632e42a12f doc: Remove reference to FillableSigningProvider
- 64b4a438f6 wallet: Combine FillableSigningProvider into LegacyDataSPKM
- cfdb2cece7 bitcoin-tx: Replace FillableSigningProvider with FlatSigningProvider
- de0afe2dee tests: Replace FillableSigningProvider with FlatSigningProvider
- 5dedc50f53 fuzz: Remove use of FillableSigningProvider
- 703252bf5d test: Use FlatSigningProvider in SetupDummyInputs

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34132 — coins: drop error catcher, centralize fatal read handling
author: l0rinc | updated: 2026-07-18 | commits: 9

- 25369f3dee test: fix dbwrapper fuzz read probe
- 68fa2a6969 coins: make coins view lookups `noexcept`
- ad02a0d707 dbwrapper: route read failures through fatal callback
- 50294f2730 coins: delegate `CCoinsViewDB::HaveCoin` to `GetCoin`
- 21285e99e8 refactor: move coin read-error handling to `CCoinsViewDB`
- b6cc2d6cd9 refactor: plumb `read_error_cb` through init and DB params
- 69023ff34a refactor: replace `CDBWrapper::ExistsImpl` with `::ReadRaw`
- fb4f018dcf refactor: split out `CDBWrapper::ReadRaw` from `CDBWrapper::Read`
- e02461859e test: cover database read errors with dedicated tests

**Finding:** 🟢 no merge-blocker. Intentionally widens abort-on-corruption (typed decode failures fatal everywhere) — flag in release notes; iterator decode paths still return false (35654 narrows). [yours]

## PR 33392 — wallet, rpc: add UTXO set check and incremental rescan to importdescriptors
author: musaHaruna | updated: 2026-07-17 | commits: 6

- 4dacd7c9da test: add importdescriptors `verify_balance` flag behavior
- b1df13b524 rpc: extend importdescriptors with UTXO check and incremental rescan
- f8c32489c5 wallet: extend` RescanFromTime()` with optional endTime to limit rescan range
- 9047880482 wallet/rpc: add `verify_balance` arg & docs for importdescriptors
- 2a96b7c28a wallet: add `GetWalletUTXOSetBalance()` to calculate balance from UTXO set
- 1cc5713b5e node: add and implement `FindCoinsByScript()` for UTXO-set scan by script

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35493 — wallet, descriptor: Fix MuSig private key completeness checks on `importdescriptors`
author: w0xlt | updated: 2026-07-17 | commits: 3

- 0390338692 test: check MuSig import private key warnings
- 5e62fbf09c wallet: check descriptor private key completeness on import
- cd8d01bf47 descriptors: require complete MuSig private keys

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33540 — argsman, cli: GNU-style command-line option parsing (allows options after non-option arguments)
author: pablomartin4btc | updated: 2026-07-16 | commits: 10

- e47c722324 bitcoin-tx: Use parsed command arguments from ArgsManager
- e4f3a44d99 doc: Add release notes for GNU-style option parsing
- be8573a9ef argsman: Handle Windows slash options after commands
- 01a8b03c3a argsman, refactor: Make ParseParameters clearer
- 7349c89522 argsman, refactor: Make ProcessOptionKey clearer
- 5af80d0447 cli, test: Fix parsing of long (--) options
- ba1e886307 argsman, test: Double dashes as RPC named params
- 98dd5a614e cli, refactor, test: Use parsed command args from ArgsManager
- 37f03fac5f argsman, cli, test: Allow options after non-option arguments
- d9bbaf812e argsman, refactor: Factor out ProcessOptionKey from ParseParameters

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35630 — test: Add importdescriptors rpc error test coverage
author: polespinasa | updated: 2026-07-16 | commits: 3

- 84bc7db250 test: test the result order of a multiple import request is correct
- ed2d1ef1b5 test: test invalid or missing timestamp throws importdescriptors
- 07fb58b9ef test: Test a locked wallet rejects an empty importdescriptors request

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35686 — lint: have git-subtree-check warn about backportability
author: Sjors | updated: 2026-07-16 | commits: 1

- 4ace058d85 lint: have git-subtree-check warn about backportability

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35714 — validation: stop writes after flush failure
author: l0rinc | updated: 2026-07-16 | commits: 2

- e1a337ee96 validation: stop writes after flush failure
- 0f04fbee2f test: characterize writes after flush failure

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 17493 — util: Forbid ambiguous multiple assignments in config file
author: ryanofsky | updated: 2026-07-16 | commits: 17

- 0e4d6342ff test-each-commit: Increase fetch depth
- 7f14217c8b test: Extend util_ArgsMerge test to check for "Multiple values specified" errors
- 9a22b9caf6 util: Forbid ambiguous multiple assignments in config file
- e230dd6a58 Merge branch 'pr/wdlist' into pr/wdmult
- da42c7f667 refactor: Always enforce ALLOW_LIST in CheckArgFlags
- d7fb7f07fd refactor: Stop calling GetArg/IsArgSet on ALLOW_LIST options
- 84fda7ff06 Always reject empty -blockfilterindex="" arguments
- 7275db69dd refactor: Fix more ALLOW_LIST arguments
- 71ceb47732 scripted-diff: Add ALLOW_LIST flag to arguments retrieved with GetArgs
- 4a41fdf89a common: Add support for combining ArgsManager flags
- … +7 more commits

**Finding:** 🟢 no merge-blocker. Nit: leftover debug LogInfo('ERROR ...') in src/common/config.cpp.

## PR 17580 — refactor: Add ALLOW_LIST flags and enforce usage in CheckArgFlags
author: ryanofsky | updated: 2026-07-16 | commits: 14

- 6feea349c5 test-each-commit: Increase fetch depth
- da42c7f667 refactor: Always enforce ALLOW_LIST in CheckArgFlags
- d7fb7f07fd refactor: Stop calling GetArg/IsArgSet on ALLOW_LIST options
- 84fda7ff06 Always reject empty -blockfilterindex="" arguments
- 7275db69dd refactor: Fix more ALLOW_LIST arguments
- 71ceb47732 scripted-diff: Add ALLOW_LIST flag to arguments retrieved with GetArgs
- 4a41fdf89a common: Add support for combining ArgsManager flags
- 5e427f4e21 Merge branch 'pr/argcheck' into pr/wdlist
- 50c8a100c9 test: Add test for settings.json parsing with type flags
- b5225d0766 test: Add tests to demonstrate usage of ArgsManager flags
- … +4 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 17581 — refactor: Remove settings merge reverse precedence code
author: ryanofsky | updated: 2026-07-16 | commits: 19

- 7a1537ee07 test-each-commit: Increase fetch depth
- 2ac63340d7 refactor: Remove settings merge reverse precedence code
- 16c31be459 Merge branch 'pr/wdmult' into pr/wdrev
- 7f14217c8b test: Extend util_ArgsMerge test to check for "Multiple values specified" errors
- 9a22b9caf6 util: Forbid ambiguous multiple assignments in config file
- e230dd6a58 Merge branch 'pr/wdlist' into pr/wdmult
- da42c7f667 refactor: Always enforce ALLOW_LIST in CheckArgFlags
- d7fb7f07fd refactor: Stop calling GetArg/IsArgSet on ALLOW_LIST options
- 84fda7ff06 Always reject empty -blockfilterindex="" arguments
- 7275db69dd refactor: Fix more ALLOW_LIST arguments
- … +9 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 17783 — common: Disallow calling IsArgSet() on ALLOW_LIST options
author: ryanofsky | updated: 2026-07-16 | commits: 18

- 4725042d68 test-each-commit: Increase fetch depth
- bc5e499659 common: Disallow calling IsArgSet() on ALLOW_LIST options
- 784daf2c2e init: Stop calling IsArgSet on list args -testactivationheight and -vbparams
- 398cbb070c Normalize inconsistent -noonlynet behavior with -privatebroadcast and -listenonion
- c52297b20f Merge branch 'pr/wdlist' into pr/wdnolist
- da42c7f667 refactor: Always enforce ALLOW_LIST in CheckArgFlags
- d7fb7f07fd refactor: Stop calling GetArg/IsArgSet on ALLOW_LIST options
- 84fda7ff06 Always reject empty -blockfilterindex="" arguments
- 7275db69dd refactor: Fix more ALLOW_LIST arguments
- 71ceb47732 scripted-diff: Add ALLOW_LIST flag to arguments retrieved with GetArgs
- … +8 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35704 — windows: replace deprecated codecvt with fs::u8path
author: kevkevinpal | updated: 2026-07-16 | commits: 1

- abb29836f5 windows: replace deprecated codecvt with fs::u8path

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 32857 — wallet: allow skipping script paths
author: Sjors | updated: 2026-07-16 | commits: 8

- 707b6132af test: remove unused wallet_taproot init_wallet
- 1070ba8c10 doc: add release note for keypath_only
- d04ee7541c test: cover keypath_only in wallet_musig.py
- 0a89ada4d1 rpc: add keypath_only to descriptorprocesspsbt
- 201da29135 rpc: add keypath_only to send and sendall
- 9460402e90 test: cover keypath_only in wallet_taproot.py
- 5e312ffed8 rpc: add keypath_only to walletprocesspsbt
- 3dfe347989 wallet: add option to avoid script path spends

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35578 — net: don’t self advertise tor exit node ip addresses in outbound connections
author: stratospher | updated: 2026-07-16 | commits: 2

- b5d58ee149 doc: use proxy=127.0.0.1:9050=onion in tor.md
- 412aa3d1fd net: don't self-advertise proxy address in outbound connections

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35728 — rpc: Properly throw on internal I/O errors in GetTransaction
author: maflcko | updated: 2026-07-16 | commits: 3

- fac83ef8e9 refactor: Use GetTransaction in ProcessPSBT
- 222207dcc1 rpc: Properly throw on internal I/O errors in GetTransaction
- cccc39d4b6 rpc: Properly throw on internal FindTx IO errors in ProcessPSBT

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34520 — refactor: Add [[nodiscard]] to functions returning bool+mutable ref
author: maflcko | updated: 2026-07-16 | commits: 3

- ee8d96fa43 refactor: Add [[nodiscard]] to functions returning bool+mutable ref
- 6ae45f7795 refactor: Add [[nodiscard]] to GetCachedLastHardenedExtPubKey
- b7cde58e16 refactor: Add [[nodiscard]] to GetOp/GetScriptOp

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34864 — coins: tighten cache entry state invariants
author: l0rinc | updated: 2026-07-16 | commits: 9

- 4812de53e9 coins: require `SpendCoin()` callers to inspect result
- bba737601a coins: preserve cache state on repeated spends
- b4e8af104d coins: derive `DIRTY` from linked list membership
- fefaf41d28 coins: inline `AddFlags()` into `SetDirty()`
- a53b634f33 coins: remove the bare `SetFresh()` helper
- 8fdb9fa33e coins: reject spent `FRESH` entries in `BatchWrite()`
- dea6583cbb coins: assert `BatchWrite()` cursor entries are `DIRTY`
- 99b2346942 test: remove fresh-only states from `coins_tests`
- 18e7474e37 coins: pass freshness through `SetDirty()` in production

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34907 — wallet, test: make wallet_fast_rescan robust
author: rkrux | updated: 2026-07-16 | commits: 1

- f217da5c59 wallet, test: wallet_fast_rescan follow-ups

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34969 — fuzz: several improvements to scriptpubkeyman harness
author: brunoerg | updated: 2026-07-16 | commits: 7

- 08499b1dcb fuzz: spkm: assert a spkm from GetScriptPubKeys() is mine
- 79d0042312 fuzz: spkm: fix early return on updating the wallet descriptor
- 861b8ba144 fuzz: spkm: do not call GetDescriptorString unconditionally
- 070677f02d fuzz: spkm: move expensive ops out of the loop
- bdbd1cefd5 fuzz: spkm: reduce number of created coins
- d64d0a536e fuzz: add a num_coins parameter to ConsumeCoins
- acc8ded486 fuzz: spkm: remove excessive IsMine calls

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34978 — init: reserve file descriptors for IPC connections
author: enirox001 | updated: 2026-07-16 | commits: 3

- c29e28c704 test: extend -ipcmaxconnections coverage to assert FD reservation logging
- 2f731bdee7 init: Reserve file descriptors for IPC connections
- f7e40ee984 test: assert file descriptors available is logged on -ipcbind startup

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35003 — validation: improve block data I/O error handling in P2P paths
author: furszy | updated: 2026-07-16 | commits: 5

- 768014068b net: replace GETBLOCKTXN assert on block read error with graceful shutdown
- 7814d7f00b test: ensure consistent failure behavior in BlockManager reads/writes
- 23205b4b14 test: verify ActivateBestChain fatal errors on I/O failure
- 1acce228e7 FlatFile: do not throw for parent dir creation failure
- beeb248e22 test: add P2P coverage for block disk I/O failures

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35105 — Refactor: Updated TransactionError to use util::Expected and removed TransactionError:OK
author: kevkevinpal | updated: 2026-07-16 | commits: 2

- 9b53586d66 refactor: remove unused TransactionError:OK
- 6866cdfc9b refactor: BroadcastTransaction and HandleATMPError now uses util::Expected

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35307 — blockstorage: keep snapshot base in normal blockfile range
author: shuv-amp | updated: 2026-07-16 | commits: 1

- bbdad237ed blockstorage: keep snapshot base in normal blockfile range

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35516 — rpc: preserve global xpubs and proprietary fields in joinpsbts
author: thomasbuilds | updated: 2026-07-16 | commits: 2

- 461d83abca test: check joinpsbts preserves global xpubs and proprietary fields
- 31f23167b3 rpc: preserve global xpubs and proprietary fields in joinpsbts

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35676 — util: Abort in CheckDiskSpace/FlatFileSeq::Open on rare exceptions
author: maflcko | updated: 2026-07-16 | commits: 2

- fac9306686 util: Abort in CheckDiskSpace on rare exceptions
- faab1e4d3b test: Check handling of I/O erros from CheckDiskSpace (via Allocate)

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 10102 — Multiprocess bitcoin
author: ryanofsky | updated: 2026-07-16 | commits: 19

- b314fb2cff combine_logs: Handle multiprocess wallet log files
- 041c7d93fd doc: Multiprocess misc doc and comment updates
- 9739ae3026 multiprocess: Add debug.log .wallet/.gui suffixes
- 3df11858c5 multiprocess: Make bitcoin-node spawn a bitcoin-wallet process
- f5845c51c5 multiprocess: Make bitcoin-gui spawn a bitcoin-node process
- 0867ab365e multiprocess: Add capnp wrapper for Node interface
- 1465ef5927 multiprocess: Add capnp wrapper for Wallet interface
- 7e5dcf9b27 interfaces, refactor: Change WalletLoader::restoreWallet parameter order
- 3405b53df6 multiprocess: Add capnp serialization code for bitcoin types
- f00eade8aa util: Add util::Result workaround to be compatible with libmultiprocess
- … +9 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 27409 — Make GUI and CLI tools use the same datadir
author: ryanofsky | updated: 2026-07-16 | commits: 3

- 5864569922 Make GUI and CLI tools use the same datadir
- 636cc9aff5 init: Allow bitcoin default datadir to point at an external datadir
- 0545d0d890 bitcoin-wallet: make bitcoin-wallet tool load config file

**Finding:** 🟠 two bugs in the headline datadir-redirect feature: fs::is_directory not negated (valid redirect to existing dir rejected, nonexistent accepted) + comma-operator error string keeps raw %s placeholders (src/common/config.cpp:196). MUST FIX.

## PR 19460 — multiprocess: Add bitcoin-wallet -ipcconnect option
author: ryanofsky | updated: 2026-07-16 | commits: 21

- b39a59767a multiprocess: Add bitcoin-wallet -ipcconnect option
- 4c33b9cc63 Merge branch 'pr/ipc' into pr/ipc-connect
- b314fb2cff combine_logs: Handle multiprocess wallet log files
- 041c7d93fd doc: Multiprocess misc doc and comment updates
- 9739ae3026 multiprocess: Add debug.log .wallet/.gui suffixes
- 3df11858c5 multiprocess: Make bitcoin-node spawn a bitcoin-wallet process
- f5845c51c5 multiprocess: Make bitcoin-gui spawn a bitcoin-node process
- 0867ab365e multiprocess: Add capnp wrapper for Node interface
- 1465ef5927 multiprocess: Add capnp wrapper for Wallet interface
- 7e5dcf9b27 interfaces, refactor: Change WalletLoader::restoreWallet parameter order
- … +11 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 19461 — multiprocess: Add bitcoin-gui -ipcconnect option
author: ryanofsky | updated: 2026-07-16 | commits: 23

- bb4f280f34 multiprocess: Add bitcoin-gui -ipcconnect option
- 9876792cf5 Merge branch 'pr/ipc-connect' into pr/ipc-gui
- b39a59767a multiprocess: Add bitcoin-wallet -ipcconnect option
- 4c33b9cc63 Merge branch 'pr/ipc' into pr/ipc-connect
- b314fb2cff combine_logs: Handle multiprocess wallet log files
- 041c7d93fd doc: Multiprocess misc doc and comment updates
- 9739ae3026 multiprocess: Add debug.log .wallet/.gui suffixes
- 3df11858c5 multiprocess: Make bitcoin-node spawn a bitcoin-wallet process
- f5845c51c5 multiprocess: Make bitcoin-gui spawn a bitcoin-node process
- 0867ab365e multiprocess: Add capnp wrapper for Node interface
- … +13 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 25665 — refactor: Add util::Result failure types and ability to merge result values
author: ryanofsky | updated: 2026-07-16 | commits: 7

- fbd968bc9b ci: Avoid -Wno-error=maybe-uninitialized false positives
- 5524cc310b test: add static test for util::Result memory usage
- 0db7b22129 refactor: Add util::Result multiple error and warning messages
- 8a4be9e254 refactor: Use util::Result class in LoadChainstate and VerifyLoadedChainstate
- 043f3a05b3 refactor: Add util::Result::Update() method
- 9556941d74 wallet: fix clang-tidy warning performance-no-automatic-move
- bd021fcc12 refactor: Add util::Result failure values

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 26022 — Add util::ResultPtr class
author: ryanofsky | updated: 2026-07-16 | commits: 10

- 84c530c73c Use ResultPtr<unique_ptr> instead of Result<unique_ptr>
- 2600b0a222 Add util::ResultPtr class
- 41e35ec71c Merge branch 'pr/bresult2' into pr/bresult-ptr
- fbd968bc9b ci: Avoid -Wno-error=maybe-uninitialized false positives
- 5524cc310b test: add static test for util::Result memory usage
- 0db7b22129 refactor: Add util::Result multiple error and warning messages
- 8a4be9e254 refactor: Use util::Result class in LoadChainstate and VerifyLoadedChainstate
- 043f3a05b3 refactor: Add util::Result::Update() method
- 9556941d74 wallet: fix clang-tidy warning performance-no-automatic-move
- bd021fcc12 refactor: Add util::Result failure values

**Finding:** 🟠 inverted restoreWallet ternary — every successful GUI wallet restore reports failure with an empty error; errored wallet returned as success (src/wallet/interfaces.cpp:588). MUST FIX.

## PR 27052 — test: rpc: add last block announcement time to getpeerinfo result
author: LarryRuane | updated: 2026-07-16 | commits: 5

- 252a6e7275 doc: add release note for 27052
- 8134d74910 test: add functional test for block announcement time tracking
- f94800952c rpc: add last_block_announcement to the getpeerinfo output
- 9e83fd25a8 net: add m_last_block_announcement to CNodeStateStats
- c5b1a485d7 net: change m_last_block_announcement type from int64_t to NodeSeconds

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 29256 — log, refactor: Allow log macros to accept context arguments
author: ryanofsky | updated: 2026-07-16 | commits: 10

- b5d3925d27 log refactor: Add support for custom log contexts
- 1eedcb9db5 doc: Add documentation about log levels and macros
- aa37de26b0 log refactor: Allow log macros to accept context arguments
- 1ac1be8b15 Merge branch 'pr/relog' into pr/bclog
- f0929e1b78 log refactor: Drop Entry::should_ratelimit field
- 31f9929f3d log refactor: log macro rewrite
- 98f3bd6242 log refactor: Ensure categories are not logged at info and higher levels
- 9c509ca53c log test: Add test for all accepted logging arguments
- 3ea9b7cb10 log test: add some test coverage on LogAcceptCategory
- 963ed23bc8 log test: verify log argument evaluation semantics

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 29409 — multiprocess: Add capnp wrapper for Chain interface
author: ryanofsky | updated: 2026-07-16 | commits: 4

- 2eb914c634 multiprocess: Expose Chain interface
- 72c2513992 Add capnp wrapper for Chain interface
- 84e9b8ae2e Add capnp wrapper for Handler interface
- 16a55c7f8a Add capnp serialization code for bitcoin types

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 30437 — ipc: add bitcoin-mine test program
author: ryanofsky | updated: 2026-07-16 | commits: 5

- bffde2f1ff ipc: Add bitcoin-mine to bitcoin wrapper interface
- 20a1ba861c bitcoin-mine: Extend example to mine a block
- 02f9d6e24d test: add interface_ipc_mining.py test calling bitcoin-mine
- f7e6369d07 ipc: Add bitcoin-mine test program
- 4b4b682a1f interface_ui: move from node to common library

**Finding:** 🟡 null-optional deref: tip->hash used unconditionally right after handling the null case (src/bitcoin-mine.cpp). Experimental tool, near-unreachable; trivial fix.

## PR 34775 — kernel: make logging callback global
author: stickies-v | updated: 2026-07-16 | commits: 1

- dafe5eec14 kernel: make logging callback global

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 31672 — rpc: add cpu_load to getpeerinfo
author: vasild | updated: 2026-07-16 | commits: 2

- 52f1efc06a cli: add getpeerinfo#cpu_load to -netinfo
- cb2eab9118 rpc: add cpu_load to getpeerinfo

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33033 — wallet, sqlite: Encapsulate SQLite statements in a RAII class
author: achow101 | updated: 2026-07-15 | commits: 10

- 1682818a7b sqlite: Construct SQLiteStatements when needed
- b79895c87a sqlite: Inline BindBlobToStatement and SpanFromBlob
- ff2bd09814 sqlite: Refactor common writing code from WriteKey and ExecStatement
- 957a8b4bbd sqlite: Replace remaining sqlite3_stmt usage with SQLiteStatement
- e4a36fd4d7 sqlite: Have SQLiteCursor store SQLiteStatement
- b9e40a58c1 sqlite: Use SQLiteStatement in check_main_stmt
- e365c67a56 sqlite: Use SQLiteStatement in PRAGMA integrity_check
- e2f141ac71 sqlite: Refactor ReadPragmaInteger to use SQLiteStatement
- 35706c4b27 sqlite: Make Column template function
- bfa96f8c12 sqlite: Add SQLiteStatement RAII class

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33112 — wallet: relax external_signer flag constraints, add musig2 test (partial)
author: Sjors | updated: 2026-07-15 | commits: 6

- 10aa22325d test: add MuSig2 external signer wallet test
- e13c29fd43 test: move mock signer path helper to the test framework
- 3aa88b42a8 wallet: make external_signer flag mutable
- b66b41c3b3 wallet: make watch-only optional for external signer
- 507292e2eb wallet: avoid signing via createTransaction() with external signer
- 6edefe559b wallet: don't import external keys at creation if blank

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33646 — log: check fclose() results and report safely in logging.cpp
author: cedwies | updated: 2026-07-15 | commits: 1

- 24320452e4 log: check fclose() results and report safely

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33741 — rpc: Optionally print feerates in sat/vb
author: polespinasa | updated: 2026-07-15 | commits: 7

- 513555558e rpc: allow estimaterawfee to return feerates in sat/vB
- 7be9f110a0 rpc: allow estimatesmartfee to return feerates in sat/vb
- 5a21d93dba rpc: allow getnetworkinfo to return feerates in sat/vb
- 628c680d60 rpc: allow getmempoolinfo to return feerates in sat/vb
- 05a788c897 rpc: let MempoolInfoToJSON to return feerates in sat/vb
- e9a4650d0c test: add unit tests for ValueFromFeeRate
- 740b6e803c rpc: Add ValueFromFeeRate function to core_write

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 33847 — kernel: Improve logging API
author: ryanofsky | updated: 2026-07-15 | commits: 1

- 6d370c720a kernel: Simplify logging API

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34872 — wallet: fix mixed-input transaction accounting in history RPCs
author: w0xlt | updated: 2026-07-15 | commits: 13

- eba63bfe77 doc: add mixed-input history RPC release note
- cf1b676c3d test: cover zero-value wallet inputs
- 242acb8af8 test: cover known nonzero foreign inputs
- a4193188c5 test: cover late mixed-input parent import
- 9b422a2237 test: cover zero-value foreign input accounting
- 056ccd7bab wallet,rpc: attribute zero-value foreign mixed inputs
- e04e0906bd wallet: cache zero-value foreign input state
- 46f70e01ed test: cover conservative mixed-input history accounting
- 0d3f1141ef wallet,rpc: report unattributable mixed-input history conservatively
- 40d2e72f16 wallet,rpc: rename transaction entry helper
- … +3 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34162 — net: Avoid undershooting in GetAddressesUnsafe
author: fjahr | updated: 2026-07-15 | commits: 2

- 3f8fe85fae test: Check getnodeaddresses doesn't undershoot if some addresses are banned
- 54896f3c94 net: Avoid undershooting in GetAddressesUnsafe

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34320 — coins: delegate `CCoinsViewDB::HaveCoin` to `GetCoin`
author: l0rinc | updated: 2026-07-15 | commits: 4

- dc4300b4fa coins: delegate `CCoinsViewDB::HaveCoin` to `GetCoin`
- b57a8bd1b3 dbwrapper: have `Read` and `Exists` reuse `ReadRaw`
- 0421796edf refactor: split out `CDBWrapper::ReadRaw` from `CDBWrapper::Read`
- c1b2536163 test: add `HaveInputs` call-path unit tests

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34534 — rpc: Manual prune lock management (Take 2)
author: fjahr | updated: 2026-07-15 | commits: 6

- 9fa4953f01 test: Add prune lock coverage for an unloaded wallet
- ca249270ee doc: Add release note for prune lock RPCs and -prunelockheight
- 730e4b0b9b init: Add -prunelockheight init option
- 8469e171d4 test: Add manual prune lock RPC coverage
- ef3a9e99da rpc: Add setprunelock and listprunelocks
- 63cf16ede5 blockstorage: Add GetPruneLocks to BlockManager

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34405 — wallet: skip APS when no partial spend exists
author: 8144225309 | updated: 2026-07-15 | commits: 1

- b6319c62db wallet: skip APS when no partial spend exists

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34603 — wallet: Fix detection of symlinks on Windows
author: achow101 | updated: 2026-07-15 | commits: 4

- b127cde3f4 test: Test multiwallet symlinks on Windows
- 43283902b0 wallet: Detect Windows symlinks in GetWalletPath
- 759f58eadc wallet: Use IsSymlink to detect symlinks and disable recursing them
- 63fd0550fa fs: Add IsSymlink which can check for Windows symlinks

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34617 — fees: wallet: remove block policy fee estimator internals from wallet
author: ismaelsadeeq | updated: 2026-07-15 | commits: 2

- 3d0bac9b54 fees: move StringForBlockPolicyEstimateReason to block policy estimator
- 0ae13fdd08 fees: split mixed FeeReason into wallet and block policy reasons

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34698 — wallet: handle MiniMiner bump fee calculation failures
author: shuv-amp | updated: 2026-07-15 | commits: 1

- 119c28b0cb wallet: handle MiniMiner bump fee calculation failures

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34778 — logging: rewrite macros to enforce restrictions at compile-time, improve efficiency and usability
author: ryanofsky | updated: 2026-07-15 | commits: 6

- f0929e1b78 log refactor: Drop Entry::should_ratelimit field
- 31f9929f3d log refactor: log macro rewrite
- 98f3bd6242 log refactor: Ensure categories are not logged at info and higher levels
- 9c509ca53c log test: Add test for all accepted logging arguments
- 3ea9b7cb10 log test: add some test coverage on LogAcceptCategory
- 963ed23bc8 log test: verify log argument evaluation semantics

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34824 — net: encapsulate TxRelay state and replace recursive mutexes
author: w0xlt | updated: 2026-07-15 | commits: 6

- e8dbc6e4d5 refactor: use Mutex in TxRelay
- 2a71a1218b fuzz: add node TxRelay target
- baa825d7bc test: add node TxRelay unit tests
- 7eb310923d refactor: hide TxRelay inventory batch storage
- 931f9f3b51 refactor: snapshot TxRelay inventory in SendMessages
- b502b04725 refactor: extract node TxRelay helpers

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35016 — net: deduplicate private broadcast state and snapshot types
author: kenji-yamam0to | updated: 2026-07-15 | commits: 1

- 32e24b1510 net: deduplicate private broadcast snapshot

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35205 — kernel,node: add `dbcache` setter and clarify defaults
author: l0rinc | updated: 2026-07-15 | commits: 9

- 88dd7786b2 test: require `TryGetTotalRam()` detection
- 8496f14c8f node, qt: inline `DEFAULT_DB_CACHE`
- b067e1cfa5 kernel: allow setting chainstate `dbcache`
- ad41d2bbeb kernel, node: colocate dbcache bounds
- 8450203f40 scripted-diff: rename `MIN_DB_CACHE`
- c5d8a90e46 common: cache `TryGetTotalRam()` result
- 24a98928e8 scripted-diff: rename `GetTotalRAM` to `TryGetTotalRam`
- 3531241741 common: move `GetTotalRAM()` to system RAM files
- 215d036247 node, qt: use `1_MiB` for dbcache conversions

**Finding:** 🟢 no merge-blocker. Note: system_ram_tests now hard-fails on <=1 GiB machines (CI/dev inconvenience). [yours]

## PR 35286 — rpc: add testsubmitpackage for 1p1c test submissions
author: instagibbs | updated: 2026-07-15 | commits: 1

- a723ef365c rpc: add testsubmitpackage for 1p1c test submissions

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35606 — script: qa: Improve `Key::Fingerprint` type safety
author: davidgumberg | updated: 2026-07-15 | commits: 1

- c9a70f9338 script: qa: Improve Key::Fingerprint type safety

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35322 — logging: streamline Logger state and drop redundant methods
author: ryanofsky | updated: 2026-07-15 | commits: 14

- addaf64128 logging refactor: drop LogLevel
- 6eb51df21b logging refactor: drop SetCategoryLogLevel(string_view)
- 39e1149722 logging refactor: replace SetLogLevel with SetCategoryLogLevel
- 37a24ebb60 logging refactor: drop EnableCategory/DisableCategory
- 904c1814e3 logging refactor: replace AddCategoryLogLevel with SetCategoryLogLevel
- 7c30371a84 logging refactor: replace DisableCategory with SetCategoryLogLevel
- 7b14eaf39c logging refactor: replace EnableCategory with SetCategoryLogLevel
- d9415af57e logging refactor: add GetLogLevels/SetLogLevels; use them in tests
- 0625adab29 logging refactor: rename node::getLogCategories() to isAnyDebugLoggingEnabled()
- d951898d4a logging refactor: replace DefaultShrinkDebugFile with WillLogCategoryLevel
- … +4 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35351 — net: Disallow invalid HeadersSyncState due to lagging clock
author: hodlinator | updated: 2026-07-15 | commits: 3

- 8b680b2f7c p2p: Shut down if system clock is out of sync with local tip
- 6721134213 refactor(p2p): Extract the max commitments computation from HeadersSyncState()
- 7392ab1e4d test: cover future chain-start MTP boundary in headers presync

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35436 — wallet: Add addHDkey interface
author: pseudoramdom | updated: 2026-07-15 | commits: 5

- f4a84d39fc wallet: Add addhdkey interface
- e9f21573c0 wallet: Use WalletError for AddHDKey failures
- 19eff5d200 wallet: Move addhdkey logic into CWallet
- e6870a1d98 test: expand addhdkey coverage for locked wallet and bad key
- a12a2319b1 wallet: Introduce WalletError with machine-readable error code

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35445 — wallet, descriptor: Revert `StringType::COMPAT` for Miniscript expressions and drop the concept of a Descriptor ID that can be validated
author: achow101 | updated: 2026-07-15 | commits: 8

- 3dd0a0b12e test: Enforce descriptor reimport is an update
- 78ba58769e descriptor: Rename DescriptorID to CompatDescriptorHash
- 7efc166d33 test: Add 31.0 to wallet backwards compatibility test
- 990c3bbd57 wallet, spkm: Treat Descriptor ID as an opaque SPKM ID
- deecc59c13 wallet, export: Include descriptor cache when exporting descriptors
- 243a86b60f spkm: Remove DescriptorSPKM constructor that doesn't take a descriptor
- 334c90a448 test: Add v30.2 and Miniscript to wallet backwards compatibility test
- 7b36ca88d7 miniscript: Don't use StringType::COMPAT

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35492 — wallet: fail dump on incomplete writes
author: OSINTv96 | updated: 2026-07-15 | commits: 1

- bb2a207d08 wallet: fail dump on incomplete writes

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35496 — kernel: add `btck_set_mock_time` for testing time-dependent paths
author: stringintech | updated: 2026-07-15 | commits: 1

- 156f2c6c49 kernel: add `btck_set_mock_time` for testing time-dependent paths

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35557 — kernel, validation: Add btck_chainstate_manager_set_clock_time
author: ryanofsky | updated: 2026-07-15 | commits: 6

- 12a237f9d3 test: Use chainman-scoped clock in utxo_total_supply fuzz target
- 67fe3e361c test: Use chainman-scoped clock in chainstatemanager_ibd_exit_after_loading_blocks
- a3a3b6d251 util/time: Disallow calling now() methods in kernel code
- edabc9ade0 mempool: Use NodeClock::time_point to represent times
- fd2654ddaf kernel, validation: scope clock time to ChainstateManager
- 77043b0c85 test: Fix nondeterministic clock race in chainstatemanager_ibd_exit_after_loading_blocks

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35580 — bugfix:  compare non-adjusted chunk weight against block weight limit
author: ismaelsadeeq | updated: 2026-07-15 | commits: 2

- a9a4f7f2c9 bugfix: compare real chunk weight against block weight limit
- 1754401de4 test: `TestChunkBlockLimits` uses incorrect weight for comparison

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35591 — [DO NOT MERGE] Erlay: bandwidth-efficient transaction relay protocol (Full implementation)
author: sr-gi | updated: 2026-07-15 | commits: 31

- d08f127a77 p2p, test: Add tx reconciliation functional tests
- fa7b5404fe p2p: Handle reconciliation finalization message
- 896aaf07c8 p2p: Add a finalize incoming reconciliation function
- 8c133ac067 p2p: Handle sketch extension
- 25b173bd80 p2p: Respond to sketch extension request
- dd4c7d6e29 p2p: Handle reconciliation extension request
- a82db0abb3 p2p: Keep track of announcements during txrcncl extension
- 53930c2076 p2p: Prepare for sketch extension request
- 0b9de161fe p2p: Be ready to receive sketch extension
- cfb997098b p2p: Request extension if decoding failed
- … +21 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35642 — headersync: do parameter search at runtime
author: sipa | updated: 2026-07-15 | commits: 11

- 7d23abba93 net_processing: compute headers sync parameters at runtime
- be181976e7 headerssync: replace randomized search with better algorithm
- 1637ff907d headerssync: drop find_bufsize optimization (preparation)
- 4dd45aa6a2 headerssync: compute attack_rate in closed form
- 55b040dcb1 headerssync: port the sync-parameter optimizer to C++
- f2c000e46a contrib: add test vectors to headerssync-params.py
- 6b7cc893b4 contrib: drop the ASSUME_CONVEX option from headerssync-params.py
- 89610da63f contrib: break headerssync-params ties deterministically
- bac347a2b7 contrib: parameterize headerssync-params.py computation
- 357e03be97 contrib: drop the RANDOMIZE_OFFSET option from headerssync-params.py
- … +1 more commits

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 16545 — refactor: Implement missing error checking for ArgsManager flags
author: ryanofsky | updated: 2026-07-15 | commits: 6

- 50c8a100c9 test: Add test for settings.json parsing with type flags
- b5225d0766 test: Add tests to demonstrate usage of ArgsManager flags
- d42d26dba5 common: Update ArgManager GetArg helper methods to work better with ALLOW flags
- 933c856094 common: Implement ArgsManager flags to parse and validate settings on startup
- 5c0919d66b doc: Add detailed ArgsManager type flag documention
- 05eeed8067 common: Grammar / formatting tweaks

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 27260 — Enhanced error messages for invalid network prefix during address parsing.
author: portlandhodl | updated: 2026-07-15 | commits: 3

- b3bd890e91 Base58: Errors with expected prefixes
- 1f053163d5 Bech32: Errors with expected prefixes
- 3338bc657a Include Base58 encoded prefixes in chainparams

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 28463 — p2p: Increase inbound capacity for block-relay only connections
author: mzumsande | updated: 2026-07-15 | commits: 7

- c2437c5db1 doc: Update docs that refer to -maxconnections
- 2b15410fd7 test: add test that EvictTxPeerIfFull only evicts tx-relaying peers
- 756f67cedd p2p: trigger possible eviction if we support bloom filters and change a peer to tx relay
- 21068381ee init: make inbound tx relay percentage configurable
- aa53900f33 test: add functional test for inbound maxconnection limits
- f6ded5bf29 net: increase inbound capacity for block-relay-only connections
- 87bca1c2ad net: add options to AttemptToEvictConnection

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 29418 — rpc: provide per message stats for global traffic via new RPC 'getnetmsgstats'
author: vasild | updated: 2026-07-15 | commits: 3

- b2d092efcf test: add functional tests for the new getnetmsgstats RPC
- b001579e43 rpc: visualize global CConnman stats in a new RPC getnetmsgstats
- dca3bb6673 net: count traffic bytes and number of messages globally

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34038 — logging: replace -loglevel with -trace, expose trace logging via RPC
author: ajtowns | updated: 2026-07-15 | commits: 3

- befe8183dc [doc] Release notes for user-visible logging changes
- 5a14f8f5d2 rpc: Update logging RPC to support trace level debugging
- b25dacba1d logging: replace -loglevel arg with -trace

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34264 — fuzz: Extend `spend` coverage
author: Chand-ra | updated: 2026-07-15 | commits: 4

- 1876e77f7e fuzz: Add coverage for `ListCoins()`
- 37d99b6f9e fuzz: Add coverage for `FundTransaction()`
- 2f702af9c9 fuzz: lock inputs to verify correct handling of user-mandated inputs
- 51c7cedea3 fuzz: Refactor `wallet_create_transaction` to support multiple actions

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 34371 — wallet: allow importprunedfunds for spending transactions
author: 8144225309 | updated: 2026-07-15 | commits: 1

- 72186b7697 wallet: allow importprunedfunds for spending transactions

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34502 — wallet: remove most asserts of `WALLET_FLAG_DESCRIPTORS` flag
author: rkrux | updated: 2026-07-15 | commits: 2

- 77778e2559 wallet: move out `UpgradeDescriptorCache()` wallet flag check to the callers
- a3ff41a5a8 wallet: remove few asserts of `WALLET_FLAG_DESCRIPTORS` flag

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 34812 — net: advertise CJDNS addresses when `-externalip` disables discovery
author: w0xlt | updated: 2026-07-15 | commits: 4

- 636a515d7b test: add functional test for CJDNS address with -externalip
- a660afa40d doc: release note for CJDNS address advertising with -externalip
- 9cdf4e21ab test: unit test CJDNS AddLocal() discovery gating
- 361aa7262e net: allow CJDNS addresses when -externalip disables discovery

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35027 — net: add -outboundbind option for outgoing source address
author: 8144225309 | updated: 2026-07-15 | commits: 3

- fa19b8c434 doc: add release notes for -outboundbind
- 1ab4de23ab test: add tests for outbound bind address selection
- 895aee9e0d net: add -outboundbind option for outgoing source address

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35076 — doc: clarify pruning impact on wallet sync
author: MemeticMoney | updated: 2026-07-15 | commits: 1

- 51ee8ca168 doc: clarify pruning impact on wallet sync

**Verdict:** 🟢 no merge-blocker (skim: low-risk area (docs/CI/build/test/gui) or draft)

## PR 35151 — wallet, follow-up: Refactor IsSpent to use HowSpent
author: musaHaruna | updated: 2026-07-15 | commits: 1

- ac774b3184 wallet: Refactor IsSpent to use HowSpent

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35229 — refactor: Use CBlockIndex parameters as reference
author: optout21 | updated: 2026-07-14 | commits: 7

- a1ed86a7e6 refactor: Store reference_wrapper items within CChain class
- b7cc8b99ee refactor: In ProcessBlock change CBlockIndex input to reference
- 91e1110aae refactor: In LookupX methods, change CBlockIndex input to reference
- 555e7e32e2 refactor: In TryDownloadingHistoricalBlocks, change CBlockIndex input to reference
- cec59ef3b6 refactor: Assert and comments to ensure vBlocks never contains null
- 63c9b996f4 refactor: In FindNextBlocks, change CBlockIndex input to reference
- 1c3d6b2c96 refactor: In LastCommonAncestor, change CBlockIndex input to reference

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35294 — wallet: Update tx chain state during loading during AttachChain instead of before
author: achow101 | updated: 2026-07-14 | commits: 2

- cce42a0db6 test: Check all wallet tx states are correct after unclean shutdown
- 1a35042f5a wallet: Update tx chain state after chain notifications are attached

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35317 — wallet: fix ignored subtract_fee_from_outputs option
author: stutxo | updated: 2026-07-14 | commits: 2

- 1b56f3630a wallet: prefer snake_case funding option names in help
- 8ed3107c38 wallet: fix ignored subtract_fee_from_outputs option

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35354 — net: wait for validation queue flush for missing compact filter for an already known block
author: randomlogin | updated: 2026-07-14 | commits: 1

- b68a721e64 net: wait validation queue on cfilter request

**Verdict:** 🟢 no merge-blocker (deep read)

## PR 35387 — logging: make trace logging easily usable
author: ryanofsky | updated: 2026-07-14 | commits: 2

- dffb8cf9e7 rpc: add loglevel RPC
- c3700df020 logging: make -loglevel work standalone without -debug

**Finding:** 🟠 nullopt deref (UB) reachable from RPC input: positional loglevel call with unknown category key derefs *GetLogCategory(cat) unchecked (src/rpc/node.cpp:273). Adjacent logging RPC checks correctly — copy that. MUST FIX.
