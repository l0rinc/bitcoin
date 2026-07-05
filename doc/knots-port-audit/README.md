# Knots Port Audit

Audit date: 2026-07-05

Branch: `codex/knots-current-master`

Source baseline: Bitcoin Knots `29.x-knots`, tag `v29.3.knots20260508`

This note records security-relevant and integration-relevant findings from
replaying Bitcoin Knots changes onto current Bitcoin Core master. It is not a
complete consensus sign-off.

## Replayed Fixes Found During Testing

The RDTS functional tests exposed one missing Knots consensus-path change:

- `38996fb57a` (`script: require empty witness for P2A spends`) was present in
  Knots but absent from the port. Without it, a P2A spend with non-empty witness
  could pass the P2A early return instead of reaching the RDTS unknown-witness
  checks. This is now ported as `24c4e32049`.

Other missing/adapted Knots pieces found during this pass:

- Regtest `reduced_data` now uses the fast 144-block period and 108-block
  threshold expected by the Knots tests under Core's current per-deployment
  versionbits model (`c26c475e4d`).
- The completion audit found Knots policy commits that had not yet been
  replayed: OPNet witness accounting for `-datacarriersize`
  (`2d00c4c315`, ported as `9bfc680467`) and the sub-dust fee penalty stack
  (`3cadc6dcdc`, `20b2e7c4ec`, `640689b0b4`, `484ef9c541`, ported as
  `3f3da51691`, `7d6256aacb`, `1bb6094f8a`, `727af3dfc8`). These add OPNet
  datacarrier coverage, `-subdustfeepenalty`, the GUI option, the default-on
  policy setting, and `mempool_subdust_fee_penalty.py`.
- The test harness now restores compatibility aliases for mutable transaction
  and block hashes and uses the spent-output libconsensus API for taproot script
  vectors (`490d4f78cf`).
- Functional tests were adapted to current Core APIs and Knots policy defaults:
  `create_block(..., ntime=...)`, current binary path lookup, capped
  datacarrier defaults, and sigop tests that isolate bytespersigop policy from
  datacarrier policy (`490d4f78cf`).
- `mempool_accept.py` was further adapted for Knots' data-output policy:
  Core's v30-era unbounded/multiple OP_RETURN expectations are now rejected by
  the port's `scriptpubkey`/`multi-op-return` policy paths (`2635a090c3`).
- The completion audit found Knots' REST parse-error guard for
  `/rest/mempool/transactions/<info|contents>.json?sequence_start=...`
  (`11f97670fe`, ported as `412914b0d9`). The same area exposed an original
  Knots route-registration bug: the transactions endpoint was hidden behind the
  generic `/rest/mempool/` prefix and was also registered without the trailing
  slash expected by its parser. The port fixes that and adds functional coverage
  in `interface_rest.py` (`de98ad9122`).
- The completion audit found Knots' extra transaction weight saturation fix
  (`86cf9d6ee8`, ported as `09cb8ea993`). Without it, extreme input-side
  datacarrier accounting could overflow the `int32_t` return path. The port
  adds a `script_tests` regression that checks both normal accounting and
  saturation at `int32_t` max.
- `feature_init.py` verification exposed a port-side test-framework regression
  in `busy_wait_for_debug_log`: after an early miss, the helper never reset its
  match state and could time out even when the expected log line was present.
  This was not an original Knots defect; Knots' helper resets `found` every
  loop. The port restores that behavior as `01608b0fa4`.
- The completion audit found Knots' negative `-lowmem` clamp
  (`f9f7587b59`, ported as `7985bced24`). Without the clamp, a negative
  user-provided threshold is assigned through the unsigned byte-count path. The
  port adds `feature_init.py` coverage for both `-lowmem=1` and `-lowmem=-1`.
- Knots' external signer fingerprint hardening (`6d2c2259ee`,
  `12eefda89a`, `ee39394ad3`) is already present in this port via the earlier
  wallet/RPC reconciliation commit `e8c2b257ee`, with invalid-fingerprint
  coverage in `rpc_signer.py`. The strengthened coverage now also asserts that
  rejected untrusted fingerprint values are absent from both the RPC error text
  and the new `debug.log` slice.
- The external signer wallet-flag review confirmed Knots' mutable
  `external_signer` flag support (`dc97030bfc`, `6925c383d3`) is present in
  the port and absent from current Core's `setwalletflag` mutable flag set.
  This is a wallet-management compatibility/control change, not consensus or
  network security behavior: Knots allows toggling the flag only for descriptor
  watch-only wallets when external signer support is compiled in, and warns
  that the wallet must be reloaded before the change takes effect.
  `wallet_signer.py` now asserts both the flag state and the reload warning.
- Knots' `bumpfee`/`psbtbumpfee` null-dereference guard for txids absent from
  the wallet (`7e125f8ed2`) is present in the port source: the RBF pre-check
  now tests the `GetWalletTx(...)` result before reading `wtx->tx`, allowing
  the normal `Invalid or non-wallet transaction id` error path to run. This
  pass added direct functional coverage as `eef3aeb7fc`.
- The RPC cookie and wallet-restricted authentication tests exposed missing
  compatibility for Knots-style `add_wallet_options(...)` calls after rebasing
  onto current Core's common `--descriptors` / `--legacy-wallet` parser. This
  was a port-side test-framework issue, fixed as `6f1d551e45`. The same pass
  made the `importmempool` wallet-restriction assertion in `rpc_users.py` pass
  a dummy `filepath`, so the RPC reaches Knots' `EnsureNotWalletRestricted`
  guard instead of the generic argument-count help path.
- The RPC cookie replacement audit confirmed the port has Knots'
  `g_generated_cookie` content check (`50b7a50a61`) and added functional
  coverage that overwrites `.cookie` after startup, stops through the original
  in-memory cookie credentials, and verifies the externally replaced file is
  not deleted during shutdown.
- A legacy-wallet test run exposed a port-introduced RPC assertion crash:
  Knots' older `createwallet descriptors=false` path had overwritten Core
  master's descriptor-only guard, while current Core's wallet creation code
  still asserts that new wallets are descriptor wallets. This was not an
  original Knots crash: an unmodified local Knots build returned RPC `-4`
  (`Compiled without bdb support`) for the same legacy-wallet creation attempt.
  Core master already rejects `descriptors=false`; the port restores that
  behavior as `6cd89c9b09` and covers it in `wallet_createwallet.py`.
- `rpc_getblockfrompeer.py` still used Knots' older mutable block-hash helper
  (`CBlock.calc_sha256()`), while the current port framework exposes block
  hashes through properties (`hash`, `hash_int`, and `sha256`). The port removes
  the stale call as `1486393769`; the test now passes and covers the
  `getblockfileinfo` pruning assertions.
- The `getblockfrompeer` review confirmed Knots' no-header and pruned-node
  future-block behavior (`6c78d40b89`, `aebfd947d2`) is present in the port and
  absent from current Core master. The same review confirmed Knots' restored
  duplicate-request error (`2b67ea465c`, ported as `0946c66e3a`): current Core
  still removes the prior in-flight block request before checking whether the
  selected peer was already asked, while Knots and this port return `Already
  requested from this peer`. The port matches final Knots by allowing requests
  for blocks whose header is not yet known and by disabling Core's prune-mode
  synced-past-height rejection. `rpc_getblockfrompeer.py` covers the
  same-peer duplicate request, the no-header request, a pruned node fetching a
  block it has not seen, and a pruned node fetching a block it has not synced
  past.
- The script verification thread-control review found a port omission in
  Knots' `scriptthreadsinfo` / `setscriptthreadsenabled` RPC surface
  (`daccde46e4` plus doc/fuzz follow-ups): the RPCs were registered and
  reported disabled state, but the port's current-Core `ConnectBlock`
  adaptation still created `CCheckQueueControl` whenever worker threads existed.
  Actual Knots gates parallel script checks on
  `m_script_check_queue_enabled`; the port now does the same. This is runtime
  operator CPU-control behavior, not a consensus-rule change. `rpc_blockchain.py`
  now covers the enable/disable RPC surface.
- `rpc_bind.py` has port-side coverage for Knots' stricter explicit RPC-bind
  behavior, but the expected message was attached to the wrong output stream.
  An unmodified Knots build exits with `Error: Unable to start HTTP server. See
  debug log for details.` on stderr and logs `Unable to bind all endpoints for
  RPC server` in `debug.log`. The port test now asserts both surfaces as
  `afa1750abe`.
- `feature_torcontrol.py` now passes against the current port, covering Tor
  control partial responses, oversized-line disconnects, excessive-line
  disconnects, bind-any onion target mapping, PoW fallback, and the private Tor
  subprocess path that uses `subprocess::close_fds`. The test needed two
  rebase-only adjustments: use a dedicated onion bind to avoid Knots' common
  Tor/local-port warning on stderr, and parse the fake Tor command as
  `LOG -f TORRC`, matching the port's `-torexecute` launch contract
  (`6fe0c50345`). The port now also adds direct `system_tests` coverage for
  the underlying subprocess behavior by opening a parent file descriptor,
  proving a mock child inherits it when `close_fds` is off, and proving it is
  closed when `close_fds` is on.
- The runtime notification review confirmed Knots' support for multiple
  `-startupnotify`, `-blocknotify`, `-alertnotify`, and `-walletnotify`
  commands (`f1e300838a`) is present in the port and absent from current Core.
  This is operator-facing behavior rather than a security or consensus change.
  The port now extends `feature_startupnotify.py` and
  `feature_notifications.py` as `a9ddf9043e` so startup, block, alert, and
  wallet notifications each exercise multiple configured commands.
- Knots-added RPC coverage for `getblocklocations`, `getgeneralinfo`, and
  BIP67 multisig sorting now passes on the port. `rpc_sort_multisig.py` had
  dropped the original Knots `assert_raises_rpc_error` import during the rebase;
  the port restores that test helper import as `dcf97bd63b`.
- Mining priority coverage now passes against Knots' policy defaults. The
  coin-age priority test uses a cache-bypassing `getblocktemplate` request when
  checking immediate `prioritisetransaction` effects, and the shared large
  transaction helper now appends standard P2WSH outputs instead of many
  OP_RETURN outputs so it remains valid under Knots' one-data-output policy
  (`dc0ed81797`). `mining_coin_age_priority.py`,
  `mining_prioritisetransaction.py`, and `feature_maxuploadtarget.py` pass with
  this helper.
- Header-tree DoS coverage exposed a port omission in Knots' reintroduced
  checkpoint enforcement: original Knots retains the testnet3 checkpoint at
  height 546, while the port only had mainnet checkpoint data. This was not an
  original Knots bug and is now restored as `0dbc321ca0`, along with the Knots
  testnet3 header fixture and test framework network magic needed by
  `p2p_dos_header_tree.py`.
- A high-risk review of exact patch-id misses found Knots' codex32 early-return
  fix (`126d6df18d`) was missing from the port. Actual Knots `29.x-knots`
  already contains the fix, so this was a port omission rather than an original
  Knots defect. Without the return, share derivation could continue after
  setting `MISMATCH_K`, `MISMATCH_ID`, `MISMATCH_LENGTH`, or `DUPLICATE_SHARE`;
  when an earlier share was longer than a later share, the interpolation loop
  could read past the later share's data. The port now matches Knots as
  `65c72eb817`, with `codex32_tests` and `wallet_importseed.py` coverage for
  valid decoded shares with inconsistent lengths.
- The same exact-patch review checked Knots' `-consensusrules` / RDTS consent
  commits (`fbfa7482c6`, `e60eea4260`). The port carries the behavior in an
  adapted form: unknown rule names fail startup, `rdts` is accepted, and mainnet
  consent handling is skipped on test chains. `feature_config_args.py` now covers
  the parser/startup surface directly as `31f7b8f005`.
- The mainnet policy-argument review confirmed Knots' opt-in
  `-acceptnonstdtxn` relaxation (`2e2f48f871`) is present in the port and
  absent from current Core. Current Core still treats the option as
  test-network/debug-only and rejects it on main chain; Knots and this port
  allow authenticated operators to disable standardness policy on mainnet while
  keeping the default unchanged. This is mempool/mining policy behavior, not a
  consensus-rule change, but it is a high-visibility Knots-vs-Core divergence.
  `feature_config_args.py` now covers main-chain startup with
  `acceptnonstdtxn=1`.
- The versionbits warning review exposed a port-introduced regression from
  rebasing Knots' stronger unknown-signalling warnings onto current Core's
  BIP323 constant split. The port had kept `VERSIONBITS_NUM_BITS = 5`, so
  Knots' last-100-block warnings and unknown-activation warning cache only
  scanned bits 0-4 even though the restored Knots test signals bits 12 and 13.
  Actual Knots `29.x-knots` still scans the historical 29 BIP9 signal bits and
  passes the same functional test. The port now separates the warning scan
  width from the BIP323 deployment width as `dac70fed98`; `versionbits_tests`
  and `feature_versionbits_warning.py` pass.
- The follow-up fuzz-build pass found Knots' `wallet_bdb_parser` deterministic
  seeding fix (`30f578a081`) was missing from the port. Actual Knots
  `29.x-knots` already contains the fix, so this was a port test omission rather
  than an original Knots runtime defect. The same fuzz build exposed stale
  current-Core API adaptations in fuzz-only sources: `kernel::CBlockFileInfo`
  qualification, `mini_miner`'s current miner/test helpers, direct mempool
  `Txid` lookups, and move-only `CTxMemPoolEntry` handling. These are now fixed
  as `f1f3f4ae7c`, and a fuzz-enabled compile of the `fuzz` target passes.
- The BDB hardening review confirmed Knots' overflow-length, BTree-level, and
  last-page LSN checks are present in the port. It also found an original Knots
  fuzz-target gap: the parser can now throw `Overflow record has an impossible
  length` and `Overflow record data is larger than stated size`, but neither
  actual Knots `29.x-knots` nor current Core master lists those malformed-input
  errors in `wallet_bdb_parser`. The port now accepts those expected parser
  failures as `daeaa28b49`.
- The wallet/assumeutxo RPC review found an original Knots bug in the
  Knots-only `confirmations_assumed` reporting surface. `getrawtransaction`,
  `gettxout`, and witness `verifytxoutproof` treated any chainstate with a
  snapshot base as still assumed, but `SnapshotBase()` remains non-null after
  background validation has completed. A temporary test worktree against
  unmodified Knots `29.x-knots` reproduced the raw-transaction failure after
  background validation: the RPC still returned `confirmations: 0` when the
  validated confirmation count was `201`. This is RPC reporting correctness,
  not a consensus rule difference. The port now gates these fields on an
  unvalidated snapshot base and extends `feature_assumeutxo.py` to cover
  `getrawtransaction`, `gettxout`, and witness txoutproof before and after
  background validation (`bf459451a2`).
- The same RPC test pass exposed a port-introduced regression in
  `waitfornewblock(current_tip)`: the help and test still advertised the
  optional `current_tip` argument, but the port's long-poll shutdown adaptation
  ignored `request.params[1]` and always waited on the tip observed when the RPC
  call started. Current Core and actual Knots both retain the `current_tip`
  logic. The port now restores it and updates `rpc_blockchain.py` for the
  current no-UI `Error:` stderr prefix (`bf459451a2`).
- The exact-patch review found Knots' actionable pruned-index startup error
  (`4a4e4e253e`) was still missing after adapting onto the port's newer
  two-stage block/undo-data availability check. Actual Knots already contains
  the fix, so this was a port omission, not an original Knots defect. The port
  now gives index-specific disable instructions for missing pruned block data
  and missing pruned undo data (`881949b28d`), and `feature_index_prune.py`
  asserts the new block-filter and coinstatsindex messages.
- A shared `libbitcoinkernel` build exposed rebase-only kernel/CMake omissions:
  the port had Knots' three-argument `Notifications::warningSet(...)` interface
  but Core master's C API adapter still used the old two-argument override; the
  C API warning/result conversion also lacked Knots' added warning IDs and
  `BLOCK_CHECKPOINT`; the kernel target still referenced a stale
  `kernel_warn_interface`; and `dbwrapper.cpp` needed the LevelDB `memenv`
  helper include path. The same pass restored Knots' missing
  `external_lib_interface` link for `bitcoinkernel` (`31e5d8d6c3`) and the
  sanitizer guard around `--no-undefined` (`f4ee30ffa5`). These are fixed as
  `881949b28d`. Actual Knots already carries the relevant Knots CMake fixes;
  the compile failures were introduced by this port's current-Core kernel API
  adaptation.
- The depends review found Knots' `miniupnpc` package bump (`18a8022ef2`) was
  still missing from the port. Current Core master still packages 2.3.3; Knots
  uses the `2.3.4_pre20260407` commit tarball. The port now uses the Knots
  commit hash, SHA256, and custom extraction path. Exercising that target also
  exposed a port-introduced depends regression: the local `fetch_file` macro had
  one extra closing parenthesis, making `miniupnpc_fetched` and existing
  custom-download packages such as `qrencode_fetched` fail before download.
  Current Core and actual Knots do not have that syntax error, and the port now
  restores the fixed macro.
- The exact-patch review found Knots' dynamic `-dbcache` default series was
  only partially present after rebasing onto current Core. Core master already
  has the simpler 1 GiB high default, but actual Knots `29.x-knots` has the
  later RAM-aware formula, `node/dbcache.{h,cpp}` split, auto-default startup
  logging, platform-dependent help text, GUI migration through
  `node::GetDefaultDBCache()`, and bitcoinkernel default-cache routing
  (`7e1fd61a38`, `54fa06dfd6`, `aa55dd574d`, `be3eb860b8`,
  `595b246cf9`, `5e367d4ba5`, plus follow-ups). This was a port omission, not
  an original Knots defect or consensus issue. The port now matches Knots'
  resource-selection behavior as `96d1c6c8a9`, with unit coverage for the
  formula/warning thresholds and a shared `libbitcoinkernel` build proving the
  new kernel C API dependency is linked.
- The LevelDB exact-patch review found Knots' embedded-LevelDB sanity-check
  guard (`a4fc0050f1`) was missing. On this current Core base the embedded
  LevelDB fork still exposes the runtime version getters, so this was not a
  reproducible build failure or original Knots defect. The port now still
  matches Knots' intended build distinction as `7d4c61ea3a`: system-LevelDB
  builds keep the header/runtime version check, while embedded-LevelDB builds
  define `EMBEDDED_LEVELDB` and skip the redundant runtime comparison.
- The exact-patch review found Knots' BanMan expiry-sweep series was missing:
  exact-expiry removal (`2da7001df1`), scheduler-based expiry sweeps
  (`838fe961ca`), and the helper tidy-up (`9a4431e0e1`). Actual Knots already
  carries these commits, so this was a port omission rather than an original
  Knots defect. The port now matches Knots as `4ac9b06e0d`: bans are removed
  when `now >= banned_until`, UI subscribers start a scheduled sweep loop, and
  earlier-expiring new bans reschedule stale callbacks with a sequence guard.
  `banman_tests` covers exact-expiry removal and scheduled UI notification,
  while `rpc_setban.py` covers the RPC-visible exact-expiry boundary.
- The same review found Knots' `SaltedOutpointHasher` hash-code caching change
  (`6d2d191c0f`, after reverting the Core noexcept optimization in
  `2969207e15`) was missing from the port. This is a runtime
  performance/resource-use divergence for `CCoinsMap` and other outpoint hash
  tables, not a consensus or original Knots defect. The port now matches Knots'
  libstdc++ cache-selection contract as `2c3de42230`, and `hash_tests` asserts
  both the may-throw invocability contract and libstdc++ fast-hash
  classification.
- The GUI/header-sync review found Knots' stale-header progress truncation
  (`823dccc962`) was missing from the port. This is a user-visible logging/UI
  correctness fix, not a consensus issue or original Knots defect: progress is
  truncated to one decimal instead of rounded, avoiding misleading `100.0%`
  header sync displays when headers are still stale. The port now matches Knots
  as `ded4a059d8`. The validation-side logging code was rebuilt with
  `bitcoind`/`test_bitcoin`; the current local build configuration has no Qt
  target, so `src/qt/modaloverlay.cpp` was source-reviewed against Knots but
  not compiled in this build.
- The exact-patch review found Knots' GUI HTTPS linkification for modal
  thread-safe message boxes (`32285d83f4`, `ed0a0f6138`) was still missing.
  This is user-facing GUI behavior, not a security or consensus issue. The port
  now HTML-escapes modal message text, marks it as rich text only after adding
  trusted links, and keeps non-modal notifications plain (`216fa2dd81`).
  `OptionTests::makeHtmlLink` covers plain HTTPS URLs, trailing punctuation,
  rich-text context, and non-HTTPS text. Local Qt execution was not possible:
  the current configured build has no Qt target, and a GUI configure probe
  failed because Qt5 is not installed.
- Compact-block extra-transaction coverage now uses the current P2P test
  framework send helper and hash/wtxid properties (`77d2b2c025`).
  `p2p_compactblocks_extratxs.py`, `p2p_dos_header_tree.py`, and
  `p2p_block_times.py` pass.
- Knots' software-expiry behavior is present and should be treated as a
  visible non-Core divergence: by default the client has an expiry timestamp,
  warns four weeks before expiry, refuses block-template creation after expiry,
  keeps accepting blocks for 144 expired-MTP blocks, then rejects new blocks
  with `node-expired`, and refuses startup after expiry unless overridden.
  `feature_softwareexpiry.py` exposed a port-side no-UI warning regression from
  Core's newer no-caption UI signal API: Knots passed `"Warning"`/`"Error"`
  captions with non-modal icon styles, while the port printed the message
  without prefixes. The port now derives no-UI prefixes from warning/error icon
  bits as `0bbf2e8bb3`.
- `feature_sync_coins_tip_after_chain_sync.py` now uses the current
  `create_block(..., ntime=...)` and P2P send helpers (`0216a33c43`), and the
  post-IBD chainstate disk-sync coverage passes.
- CLI completion coverage exposed stale generated artifacts: `exportasmap` now
  completes as a file argument, and the zsh `bitcoin-cli` completion file from
  Knots had not been carried into the port. The generated bash/zsh completions
  are now refreshed as `c45749ae43`, and `tool_cli_completion.py` passes.
- A final generated-release-artifact review against Knots' tip update
  (`f41f01e1e6`) found `doc/man/bitcoind.1`,
  `doc/man/bitcoin-qt.1`, and `share/examples/bitcoin.conf` were still stale
  for several live port options: Knots' `-maxstaleoutbound`,
  `-consensusrules`, and `-subdustfeepenalty`, plus the current-base
  `-privatebroadcast` and its `-maxconnections` note. The port now refreshes
  those entries as `5fd414d066`. Full manpage regeneration was not possible in
  this local build because `help2man` is unavailable and the configured build
  has no Qt, `bitcoin-tx`, or `bitcoin-util` binaries, so the patch was limited
  to option blocks verified against current `bitcoind --help`; the Qt manpage
  mirrors the same shared node option text.
- The fee-estimator persistence review confirmed Knots' `savefeeestimates` RPC
  (`eefa24ea3e`) is present in the port and absent from current Core master.
  The port matches Knots' `FlushFeeEstimates()` success/failure return path and
  registers the RPC while keeping it out of RPC fuzzing. This is operator
  persistence/control functionality, not consensus or network security
  behavior. `feature_fee_estimates_persist.py` covers RPC success, write
  failure, and equivalence with the normal shutdown dump.
- The runtime mempool-limit review confirmed Knots' `maxmempool` RPC
  (`7445c5b786`, `f3dceb936e`, `771e99c746`) is present in the port and absent
  from current Core's RPC table. The port matches actual Knots by exposing the
  RPC, accepting MB values through the client conversion table, rejecting values
  below the cluster-size-derived minimum, updating `m_opts.max_size_bytes`, and
  calling `LimitMempoolSize(...)` against the active chainstate so a lowered
  limit evicts transactions immediately. This is an RPC-authenticated runtime
  resource-control surface, not consensus behavior or a covert security fix.
  `mempool_limit.py` now covers invalid runtime sizes, visible limit updates,
  and shrink-time eviction.
- The mempool transaction-list review confirmed Knots'
  `listmempooltransactions` RPC and matching REST endpoints
  (`5def0ba762`, `f706635736`) are present in the port and absent from current
  Core. The port matches actual Knots' shared `MempoolTxsToJSON(...)` behavior:
  each result includes the current `mempool_sequence`, filters entries whose
  `entry_sequence` is below `sequence_start`, returns either txids or decoded
  transactions without hex, and exposes the same data through
  `/rest/mempool/transactions/<info|contents>.json`. This is mempool polling
  and observability functionality, not consensus or network-policy behavior.
  `interface_rest.py` now covers non-empty RPC/REST equality, sequence
  filtering, empty future-sequence results, verbose decoded output, and the
  parse-error path.
- The descriptor utility review confirmed Knots' `deriveaddresses` checksum
  option (`4bddd82227`, `4283bc2a2d`, `f8424c940c`) is present in the port and
  absent from current Core, which still hard-requires descriptor checksums for
  this RPC. The port matches actual Knots by defaulting `require_checksum` to
  true, accepting an explicit false value through both the options object and
  named `require_checksum` parameter, and still verifying a checksum when the
  descriptor includes one. This is RPC compatibility for callers deriving from
  descriptors with independent checksums, such as WIF-backed descriptors, not
  consensus or mempool policy. `rpc_deriveaddresses.py` now covers the default
  missing-checksum error, both opt-out calling forms, bad-checksum rejection
  while opted out, and the bitcoin-cli conversion path.
- The wallet address-activity review confirmed Knots' `getaddressinfo`
  `isactive` field and `CWallet::IsDestinationActive(...)` helper
  (`0dfff334dc`, `514cceac30`, `8eb77c4742`) are present in the port and absent
  from current Core. Rerunning the restored coverage first exposed a port-side
  test drift: the port had Knots' `isactive` assertions in `wallet_keypool.py`
  but had lost Knots' explicit `-keypool=10` setup and matching drain/refill
  counts, so the test derived index 9 while the framework default
  `keypool=1` was in effect. Unmodified Knots passes its own test with that
  setup. The port now restores the controlled keypool setup adapted to the
  descriptor-only current base. This is wallet RPC observability for active
  vs imported or old-seed destinations, not consensus or mempool policy.
- The BIP322 message-signing review confirmed Knots' signing and verification
  stack (`5689da7144`, `b9c6d98dfe`, `5501bfe28e`, `3d056b0314`,
  `5fa31318bd`, `f32c5267ae`, `fa9048636b`, `b7e42cd6b9`, `ef728f7963`,
  `a5375f9c39`, `d07851730b`, `0e79f97800`) is present in the port and absent
  from current Core's wallet/utility RPC behavior. The port matches actual
  Knots by verifying Electrum/BIP137/BIP322 signatures for SegWit addresses,
  supporting BIP322 simple signatures for wallet-owned non-P2PKH destinations,
  preferring legacy message signatures for P2PKH `signmessage`, and throwing
  for inconclusive proof-of-funds or unsupported script cases. This is
  wallet/RPC signature functionality, not transaction or block consensus.
  `wallet_signmessagewithaddress.py` now covers wallet-created BIP322
  signatures for native SegWit and Taproot addresses in addition to the
  existing legacy signing, util-level BIP322 vectors, and
  `rpc_signmessagewithprivkey.py` verification vectors.
- Wallet sweep coverage passes on the current descriptor-wallet base:
  `wallet_sweepprivkeys.py` rejects invalid/unfunded keys and sweeps both
  unconfirmed and confirmed P2PKH outputs. Legacy-only Knots tests
  `wallet_dump.py` and `wallet_import_rescan.py` now reach the expected
  current-Core skip path after restoring the ported `AddressType` test helper
  and current `BitcoinTestFramework(__file__)` constructors (`9bfe1fb892`).
- The wallet backup/export review confirmed Knots' legacy-wallet export
  surfaces are present in the port source: `dumpmasterprivkey` (`e4acb761d4`)
  is registered, and `dumpwallet` writes HD key paths and HD seed ids as
  parseable key parameters instead of hiding them in comments (`2504a07906`,
  `66a0e619dd`). This is backup/restore correctness for legacy wallets, not a
  consensus or network issue. On the current Core base the descriptor-wallet
  `wallet_hd.py` path passes and confirms `dumpmasterprivkey` rejects
  descriptor wallets; the legacy `wallet_dump.py` runtime path still skips
  because new legacy wallets can no longer be created.
- The implicit-SegWit wallet option review confirmed Knots'
  `-walletimplicitsegwit` surface (`9eabba7220`, `2733d2c4ce`) is present in
  the port and absent from current Core. The option controls legacy-wallet
  implicit P2SH-SegWit/P2WPKH script learning and sets `-addresstype=legacy`
  when disabled, so this is legacy wallet recovery/import behavior rather than
  consensus or network security behavior. The direct unit coverage
  `util_tests/outputtype_implicit_segwit` passes; the functional
  `wallet_implicitsegwit.py` path still skips on this base because new legacy
  wallets can no longer be created.
- A later file-presence sweep found the Qt wrapper for the same sweep feature
  was still missing even though the RPC and functional test were already
  ported. The port now restores Knots' `SweepPrivKeyDialog`, wires it through
  `WalletView`, `WalletFrame`, and the File menu (`28b2294989`). This is a
  user-facing wallet GUI omission, not a consensus issue or original Knots
  defect. `wallet_sweepprivkeys.py` was rerun after the GUI restoration; local
  Qt compilation remains unavailable because Qt5 is not installed.
- The high-signal Qt review found Knots' script-verification-thread warning
  (`f23f08cb01`) was still missing from the port. Actual Knots already adds a
  red options-dialog warning when `-par` is set above the CPU core count, while
  current Core master has no matching `threadsWarning` widget. The port now
  restores the label and visibility hook. Local Qt compilation is still
  unavailable, so this pass verified the UI XML parses and the expected widget
  and `GetNumCores()` visibility checks are present in source.
- The same file-presence sweep found Knots' native Windows taskbar progress
  helper (`5f4e34a556`) was missing from the port. The port now replaces the
  Qt5 `WinExtras`/`dwmapi` path with Knots' native COM `WinTaskbarProgress`
  helper and enables taskbar progress for Windows GUI builds with Qt5 or Qt6
  (`677f07f6c1`). This is Windows GUI/build compatibility, not a consensus or
  security issue. The local build is Linux/non-GUI and cannot compile this
  Windows Qt path; source review, `git diff --check`, and a non-GUI
  `test_bitcoin` rebuild were used locally.
- The same sweep found a port-side Qt block visualizer include drift: final
  Knots' `blockview.h` includes `util/transaction_identifier.h`, but current
  Core moved that header to `primitives/transaction_identifier.h`. Original
  Knots is not affected because the old header path still exists there. The
  port now uses the current include path as `d749d3deb1`; local Qt compilation
  remains unavailable because Qt5 is not installed.
- The same pass found the functional test runner still referenced three Knots
  tests that were absent from the port: `rpc_mempool_info.py`,
  `wallet_import_with_label.py`, and `wallet_upgradewallet.py`. Restoring
  `wallet_upgradewallet.py` also exposed the missing Knots BDB dump parser
  helper, `test_framework/bdb.py`, and a half-applied `tool_wallet.py` merge:
  Knots' BDB dump/createfromdump helper calls were present but the helper
  definitions and BDB-only subtest gating were not. This was a port-side
  test-coverage omission, not an original Knots defect. The port restores the
  missing tests/helper and adapts `tool_wallet.py` to current Core's
  command-option validation and unnamed-wallet rejection (`91fc10bc4c`).
  `rpc_mempool_info.py` and `tool_wallet.py` pass on the local build, while
  the restored legacy-wallet tests reach the expected current-Core skip path.
- A follow-up comparison with final Knots' runner restored additional
  legacy-wallet coverage that current Core had removed:
  `wallet_importmulti.py`, `wallet_inactive_hdchains.py`,
  `wallet_pruning.py`, and `wallet_watchonly.py`, plus the `get_key` and
  `get_multisig` wallet test helpers they need (`9b6cd9284d`). These tests
  also reach the expected current-Core skip path in this descriptor-only build.
  The remaining Knots-only `mempool_package_onemore.py` test was checked but
  not restored: it asserts the old ancestor/descendant package carve-out
  behavior, while current Core has replaced that policy surface with cluster
  limits covered by `mempool_package_limits.py`, `mempool_packages.py`, and
  `mempool_cluster.py`.
- The same file-presence sweep found a port-side Berkeley DB build-system
  omission: the port retained `WITH_BDB` and wallet BDB source, but lacked
  Knots' CMake discovery module, depends package, depends BDB patch, wallet
  package selection wiring, and BDB wallet target wiring. The first restoration
  (`ef814e1135`) added the package/discovery files; the stronger BDB compile
  probes then showed that `bitcoin_wallet` had still been SQLite-only and that
  `bdb.cpp`/`salvage.cpp` had stale current-Core database-interface calls. The
  port now restores `WITH_SQLITE`, conditional SQLite/BDB wallet sources,
  `salvage.h`, and current API adaptations as `90c0118d62`. This was not an
  original Knots defect or consensus issue. Full legacy wallet loading/creation
  remains disabled on this current-Core base; the BDB work here restores build
  coverage and the configured BDB backend objects.
- A BDB follow-up review confirmed the port also carries Knots' BDB-specific
  wallet hardening around non-writable directories and environment cleanup:
  `MakeBerkeleyDatabase(...)` catches open/verify exceptions, BDB directory
  errors report the non-writable path, checkpoint/LSN reset failures do not mark
  a database as detached, and BDB cleanup avoids deleting the `database`
  subdirectory on normal shutdown. Current Core master already has the SQLite
  non-writable-directory guard and tests, but has no BDB backend. The port's
  BDB side is covered by `wallet_createwallet.py`, `wallet_startup.py`, and
  `db_tests`.
- The high-signal exact-patch review found Knots' wallet symlink/reparse-point
  guard series (`39f48a142f`, `1f118f18c4`, `ee042e9ad6`) was still missing
  from the port. Actual Knots already carries it, while current Core master
  still lacks `IsSymlink(...)` in the wallet directory scanner and regular-file
  wallet path check. The port now restores the common helper, avoids recursing
  symlink/reparse-point wallet-dir entries, and rejects symlinked regular wallet
  files through `GetWalletPath(...)`. This is local wallet-path hardening, not
  a consensus issue. `wallet_multiwallet.py` now asserts the symlink-scan
  warning, and the same run exposed and fixed a stale missing `Decimal` import
  in that Knots test.
- The same wallet cleanup review found Knots' failed restore/migration cleanup
  guard (`0388bfc6e`) was still missing from the port. Actual Knots already
  checks that newly created wallet directories are still empty before removing
  them; current Core master still has the older unconditional
  `Assume(fs::is_empty(...)); fs::remove(...)` pattern. The port now uses a
  shared wallet cleanup helper, logs and leaves unexpectedly non-empty
  directories alone, and covers both helper branches in `wallet_tests`. Rerunning
  `wallet_backup.py` also exposed a Knots/Core behavior conflict:
  Knots' `restorewallet` coverage (`9ea84c08d7`) expects restoring the unnamed
  default wallet to work, while current Core later rejected empty restore names.
  The port now lets the `restorewallet` RPC pass `allow_unnamed=true`, while
  keeping the lower-level guard available for other callers. The same test run
  adapted stale pruned-wallet error-string literals to the current source text.
- CLI/help verification exposed a port-side bitcoin-cli conversion-table drift:
  current server metadata no longer advertises legacy-only `sethdseed` and
  `addmultisigaddress` conversions, while descriptor-compatible legacy import
  RPCs still advertise their boolean arguments. The client conversion table now
  matches the live `dump_all_command_conversions` metadata as `b2cd725240`.
  `interface_bitcoin_cli.py` also now treats the old `-paytxfee` display line
  as optional when scale-checking `-getinfo` output (`b1c4cd76e5`).
- The wallet `getbalance` review confirmed Knots' current behavior is present
  in the port's adapted `RPCMethod` implementation: the no-dummy path rejects
  `minconf`, legacy `getbalance("*")` still uses the legacy balance path, and
  an avoid-reuse wallet errors if `getbalance("*")` would ignore the flag.
  Functional testing also exposed a port omission, not an original Knots bug:
  `getunconfirmedbalance` was implemented but not registered, and the
  deprecated `getwalletinfo` fields `balance`, `unconfirmed_balance`, and
  `immature_balance` were missing after rebasing onto Core master. Actual Knots
  still registers and returns these compatibility surfaces. The port restores
  them and updates `wallet_balance.py` / `wallet_avoidreuse.py` coverage.
- The wallet funding RPC review confirmed Knots' deprecated `min_conf` option
  for `fundrawtransaction` and `walletcreatefundedpsbt` is present in the port,
  including the negative-value guard and the conflict check against modern
  `minconf`. This is backwards-compatibility behavior, not a security or
  consensus change. `wallet_fundrawtransaction.py` and `rpc_psbt.py` now cover
  positive selection, negative `min_conf`, and `min_conf`/`minconf` conflicts.
- The `setfeerate` review confirmed Knots' sat/vB wallet fee-rate RPC
  (`1d3a37aa64`, with `63fd84f7f1`, `d302fef9a3`, and follow-up test commits)
  is present in the port and absent from current Core's wallet RPC table. This
  is a wallet fee-control compatibility change: it sets the same non-persistent
  per-wallet `m_pay_tx_fee` as `settxfee`, but accepts sat/vB, reports the
  resulting fee rate through `ValueFromFeeRate`, and returns structured
  unchanged-setting errors instead of throwing for range failures.
  `wallet_create_tx.py` and `wallet_bumpfee.py` cover successful setting,
  unsetting, rounding, wallet min fee, relay min fee, wallet max fee, and
  replacement-fee interactions.
- The descriptor-wallet `importaddress` review confirmed Knots'
  descriptor-compatible behavior (`be3ae51ece`) is present in the port. The
  existing restored `wallet_descriptor.py` coverage carried an original Knots
  test bug: actual Knots `29.x-knots` still calls wallet RPCs as
  `recv_wrpc.rpc.importprivkey`, which targets the nonexistent RPC method
  `rpc.importprivkey`. A direct Knots runtime reproduction was attempted in a
  temporary build configured with `RDTS_CONSENT=IMPLICIT`, but the build hit
  `No space left on device` while archiving `libbitcoin_common.a`; the
  source/blame proof still shows this is not port-introduced. The port fixes
  the test calls, adapts removed current-Core wallet methods to
  `Method not found`, and extends coverage for descriptor wallets with private
  keys enabled, `addr(...)` imports, `raw(...)` imports, and the descriptor
  raw-script P2SH rejection (`d208db82c5`).
- The wallet PSBT review confirmed Knots' anti-fee-sniping default for
  `walletcreatefundedpsbt` (`c5448df366`) is present in the port: when the
  caller omits explicit `locktime`, the funded PSBT uses a height-based
  fallback locktime, including PSBTv0 output. Current Core already carries the
  related `send` and `sendall` behavior but still leaves this
  `walletcreatefundedpsbt` default at zero. `rpc_psbt.py` covers the ported
  PSBT behavior.
- The raw-transaction PSBT review confirmed Knots' user-provided previous
  transaction support for `utxoupdatepsbt` and `descriptorprocesspsbt`
  (`bdb4ca4195`, `eea8588f07`) is present in the port and absent from current
  Core master. The port follows the Knots follow-up that exposes
  `descriptorprocesspsbt` `prevtxs` through the options/named-parameter path,
  not a sixth positional argument. `rpc_psbt.py` covers filling and signing a
  child PSBT from provided parent transaction hex, irrelevant prevtxs being a
  no-op, duplicate txid rejection, and too-few-outputs rejection.
- The same wallet RPC review confirmed Knots' case-insensitive fee-estimation
  mode parsing (`8d40addbd2`) and `estimate_mode`/`conf_target` coupling
  validation (`be8ae64b82`) are present in the port's shared fee-estimation
  helper. These are RPC compatibility/argument-validation behaviors, not
  consensus changes. `wallet_send.py` covers mixed-case `economical`, mixed-case
  `unset`, and missing-`conf_target` errors for both positional and options
  object forms.
- The wallet change-output review confirmed Knots' preference guard
  (`70dcdd7aa7`) is present in the port: a wallet does not choose bech32m
  change merely because it is paying a Taproot/bech32m recipient unless the
  wallet's preferred address type is also bech32m. This is wallet privacy and
  compatibility behavior, not consensus. `wallet_address_types.py` now covers
  p2sh-segwit-default and bech32-default wallets sending to a bech32m recipient
  without upgrading change past the user's preferred type.
- Raw transaction, package, and PSBT RPC coverage now passes against the port.
  This covers Knots-touched max burn handling, package max fee/burn arguments,
  and PSBT base64 parameter handling with `=` padding characters.
- Mempool/TRUC verification exposed a port-introduced validation omission:
  the legacy `AcceptToMemoryPool(..., bypass_limits=true)` wrapper did not
  include Knots' `"truc"` ignore token. As a result, disconnected-block
  resurrection could enforce TRUC topology policy and drop transactions from a
  reorged-out block. This was not an original Knots defect; the unmodified
  Knots wrapper already includes `"truc"`. The port now restores it, adds
  `tx_mempool_ignore_truc` unit coverage, and `mempool_truc.py` covers the
  reorg path end to end.
- `mempool_ephemeral_dust.py` exposed a security-shaped policy divergence from
  Core's current test expectations: original Knots does not resurrect an
  unswept ephemeral-dust child after a reorg, but does resurrect the child when
  it sweeps the dust. This was verified by running the unmodified Knots
  functional test locally; the port now matches Knots' behavior.
- `p2p_invalid_tx.py` and `p2p_segwit.py` now match the port's current
  validation surfaces: orphanage overflow is still capped at 100 stored
  orphans, but the Core-current log string is `orphanage count limit`, and
  Knots-style SegWit block failures use mandatory-script reject reasons. The
  SegWit test also restores hash refreshes needed by the current mutable
  transaction helpers.
- The follow-up cherry audit found Knots' `BufferedFile` close-on-destruction
  fix (`88fe778d9d`) only half-applied in the port: the fuzz test expected
  `BufferedFile::fclose()`, but `BufferedFile` still lacked the method and
  destructor. The port now restores both and adds `streams_tests` coverage that
  the wrapped `AutoFile` is closed when `BufferedFile` is destroyed. The same
  pass removed a duplicate `cleanSubVer` assignment left after applying Knots'
  version-message ordering fix.
- The same follow-up confirmed Knots' raw transaction max-feerate accounting
  fix (`4b3cc3d48e`, `1cee5b1ac7`, `335d928d96`) is present in the port:
  `sendrawtransaction` passes a `CFeeRate` into `BroadcastTransaction`, which
  converts it to an absolute fee using the mempool accept result's adjusted
  `m_vsize`. Current Core still precomputes the absolute maximum from plain
  `GetVirtualTransactionSize(*tx)`, so it can disagree with policy-adjusted
  vsize when `-bytespersigop` dominates. `mempool_sigoplimit.py` now covers
  this by requiring `testmempoolaccept` and `sendrawtransaction` to make the
  same max-feerate decision for a high-sigop, low-weight P2WSH spend. The
  isolated max-feerate subtest also passes against unmodified Knots; the full
  cross-run hits an unrelated package-error string drift before that subtest.
- The RPC authentication follow-up confirmed Knots' blank `-rpcauth`,
  `-rpcauthfile`, `-norpcauth`, wallet-restricted rpcauth, and cookie
  permission/replacement hardening is present in the port. Current Core master
  still lacks the `-rpcauthfile` and per-token blank `-rpcauth` behavior. The
  port's `rpc_users.py` coverage exercises multi-entry auth files, blank
  `-rpcauth` around nonblank entries, `-norpcauth`, wallet-restricted auth
  entries, and cookie permission/replacement behavior.
- The CJDNS addnode follow-up confirmed current Core already has the
  `GetAddedNodeInfo()` CJDNS conversion from `f8fec8f26d`, while Knots' later
  `AddNode()` duplicate detection for CJDNS addresses with alternate ports
  (`28823f30dc`) remains a port-only network correctness fix. The port carries
  it as `bcd1387ae6`, and `net_peer_connection_tests` covers both connected
  CJDNS addnode reporting and duplicate CJDNS addnode rejection. The port now
  also covers the RPC-facing `addnode add` path in `rpc_net.py` by restarting
  with `-cjdnsreachable`, adding `[fc00:...]:8333`, and rejecting the same
  CJDNS address on a different port.
- The P2P RPC connection-management follow-up confirmed Knots intentionally
  exposes the hidden `addconnection` RPC outside regtest
  (`2fcf74eb45`, ported as `4feeb3a87b`), while current Core still rejects it
  unless `Params().GetChainType() == REGTEST`. This is a Knots-vs-Core network
  control surface expansion, not a consensus issue or hardening fix:
  RPC-authenticated users on signet/testnet/mainnet can force outbound
  full-relay, block-relay, address-fetch, or feeler connections. The port now
  runs `p2p_add_connections.py` on signet to cover this behavior, and
  `rpc_net.py` also asserts that the port-only `private-broadcast` connection
  type cannot be manually selected through `addnode onetry`.
- The same RPC connection-management review confirmed Knots' `disconnectnode`
  IP-without-port and subnet support (`7e3988fe54`, `6d2bc57f0e`, with
  coverage from `dc36f2b555` and `12a8863b80`) is present in the port. Current
  Core still documents and implements only address/port or node-id matching for
  this RPC. This is another RPC-authenticated network-control expansion rather
  than a consensus issue or hardening fix. The port's `p2p_disconnect_ban.py`
  coverage exercises address+port, portless address, subnet, invalid subnet,
  and node-id paths.
- The ForceInbound eviction review confirmed Knots' trusted-inbound eviction
  behavior (`3544a26256`, `711dadb546`, `067f80e1b5`, `3db935abd1`) is
  present in the port and absent from current Core. It also found an original
  Knots RPC reporting gap: `forceinbound` is accepted by the parser and listed
  in `getpeerinfo` help, but `NetPermissions::ToStrings(...)` did not include
  it in the peer's `permissions` array. The port now reports the permission as
  `e53bce279f`, with unit coverage for `NetPermissionFlags::All` and
  functional coverage in `p2p_eviction.py` and `p2p_permissions.py`.
- The onion-inbound whitelist review confirmed Knots' guard against applying
  address-based whitelist permissions to Tor inbound peers (`61cdc04a83`) is
  already present in current Core and in this port, so it is not a
  Core-missing hardening item. The port now adds `p2p_permissions.py` coverage
  for a real localhost peer connecting through a dedicated `=onion` bind while
  `-whitelist=noban@127.0.0.1` is configured; the peer is reported as
  `network="onion"` and receives no whitelist permissions.
- The block-filter permission review confirmed Knots' `blockfilters`
  whitebind/whitelist permission (`d153093ba2`, `aa2885797e`) is present in
  the port and absent from current Core. The permission lets an explicitly
  trusted peer receive `NODE_COMPACT_FILTERS` and request BIP157 compact
  filters even when global `-peerblockfilters` is off, while startup still
  rejects granting it without `-blockfilterindex`. The port now covers this in
  `p2p_blockfilters.py` by disabling default localhost `bloomfilter`
  permission, whitelisting only `blockfilters`, checking
  `getpeerinfo.permissions`, checking the advertised service bit, and
  requesting a `cfcheckpt`. `p2p_permissions.py` also covers permission
  reporting and merging. This is explicit peer-service control, not consensus
  behavior.
- The invalid-block peer-punishment review confirmed Knots' relaxation
  (`7c7b5839f4`) is present in the port while current Core still routes the
  same block/header validation failures through `Misbehaving(...)`. The port
  also extends Knots' decision matrix to the port-only private-broadcast
  connection type: inbound, manual, feeler, and `noban` peers are tolerated;
  outbound full-relay, block-relay, address-fetch, and private-broadcast peers
  are disconnected without discouraging their address. `net_tests` now covers
  that full matrix.
- The v2-transport privacy review confirmed Knots' randomized Tor
  stream-isolation credential prefix (`10397d85ca`) is already present in
  current Core under different commits, so it is not a Core-missing fix. The
  same review confirmed Knots' `-v2onlyclearnet` option is present in the port
  and actual Knots, while current Core has no matching option. When enabled,
  it skips outbound V1 attempts and V2-to-V1 fallback reconnections only for
  IPv4/IPv6 clearnet peers; V1 onion behavior remains allowed. The existing
  `p2p_v2_encrypted.py` coverage passes for V2 clearnet success, V1 clearnet
  refusal, and V1 onion allowance.
- Knots' user-agent sanitization hardening (`b9d2634b81`) was mostly present in
  the port as `eacc171127`: received peer user agents kept printable
  punctuation in `cleanSubVer`, and `util_tests` covered
  `SAFE_CHARS_PRINTABLE` plus log-style escaping. A later audit found the final
  receive-version log call still used `cleanSubVer` directly after rebasing
  onto current Core's log format. The port now percent-escapes that log value
  and `p2p_handshake.py` checks both surfaces: RPC preserves
  `/User/Agent: test![]{}~/`, while `debug.log` contains
  `/User/Agent: test%21%5B%5D%7B%7D%7E/`.
- Knots' merkle mutation early-exit change (`42b25bbd93`) is present in the
  port while current Core still scans the rest of the level after finding a
  duplicate pair. This is consensus-adjacent but behavior-equivalent: the root
  and `mutated` result are unchanged, and `merkle_tests` still compares the
  current implementation against the legacy merkle-tree implementation.
- The ZMQ follow-up found the Knots notification-failure hardening
  (`1c4d2d54d8`, `268fb1e0e3`, `ba28af94bd`) is present in the port and still
  missing from current Core. A temporary ZMQ-enabled build exposed
  port-introduced API drift: `CZMQNotificationInterface::BlockConnected` needed
  the current `kernel::ChainstateRole` qualifier, wallet ZMQ publishers needed
  `Txid::ToUint256()`, and the wallet-topic checks in `interface_zmq.py` needed
  the current lazy `txid_hex` helper instead of removed `calc_sha256()`. These
  were not original Knots defects; unmodified Knots uses the older
  `ChainstateRole` and transaction-hash APIs. The ZMQ-enabled
  `interface_zmq.py` run now passes.
- The fee-estimator follow-up confirmed Knots' `TxConfirmStats::Read` overflow
  guard (`163d3e5c13`, ported as `aeaf84b7d5`) is present while current Core
  still multiplies `scale * maxPeriods` before checking the one-week bound.
  The port now adds `policyestimator_tests` coverage that serializes a
  corrupt `fee_estimates.dat`-style record with an impossible scale and
  verifies that `CBlockPolicyEstimator::Read(...)` rejects it cleanly.
  The same pass classified the fee-histogram unsigned-decrement fix
  (`85c8d477b0`, ported as `759e1d76b3`) as Knots-surface hardening because
  current Core has no `getmempoolinfo(with_fee_histogram=...)` or REST
  histogram surface. `policyestimator_tests` and `mempool_fee_histogram.py`
  pass with the ported code.
- The mempool-entry RPC review confirmed Knots' transaction-serialization
  `hash` field in mempool entry output (`2f7b38db86`) is present in the port
  and absent from current Core's `entryToJSON(...)`. This is RPC compatibility
  and observability rather than a security or consensus change. The port now
  restores the original Knots-style `feature_segwit.py` coverage as
  `6166d553d1`, asserting that `hash` and `wtxid` both match the witness hash
  for witness and non-witness mempool transactions.
- The `validateaddress` RPC compatibility review confirmed Knots' hidden
  deprecated `address_type` parameter and deprecated `error_index` result
  (`5ed6ea5f31`, `48c91aabdb`, `7115f632a2`) are present in the port and
  absent from current Core master. This is compatibility for older Knots RPC
  clients, not consensus or network security behavior. Existing
  `rpc_invalid_address_message.py` coverage asserts accepted `address_type`,
  `error_index`, invalid address-type values, and invalid address-type JSON
  types.
- The wallet witness-only follow-up confirmed Knots' null-provider guard for
  `fundrawtransaction(..., {"segwit_inputs_only": true})` (`2a09a34129`) is
  present in the port. Current Core does not expose the `segwit_inputs_only`
  coin-control option, so this is Knots-only wallet RPC crash hardening rather
  than a Core-missing fix. It was not introduced by this port, and current
  unmodified Knots `29.x-knots` already contains the fix. The port now adds
  `wallet_fundrawtransaction.py` coverage for an unsolvable native-segwit
  watch-only output: the RPC returns `Insufficient funds` instead of
  dereferencing a null signing provider.
- The DNS seed follow-up found Knots' removal of the Peter Todd DNS seeds
  (`277edb9009`) was still missing from the port. Current Core master still
  includes `seed.btc.petertodd.net.` and `seed.tbtc.petertodd.net.` in the
  mainnet and testnet DNS seed lists. The port now matches Knots as
  `0899f88da9`, with `chainparams_tests` coverage asserting those seed names
  are absent. The focused `chainparams_tests/dns_seed_removals` target passes
  on the port; the local Knots tree has no matching test file, but source
  inspection confirms the seed names are absent there too.
- The custom signet review confirmed Knots' `-signetblocktime` option
  (`d8434da3c1`) is present in the port and absent from current Core. This
  changes the proof-of-work target spacing only for a custom signet that also
  sets `-signetchallenge`; it is not a mainnet/testnet consensus change. The
  port now adds `chainparams_tests` coverage as `3316d65fcc` for the default
  600-second signet spacing, a 30-second custom spacing, and the missing
  challenge / non-positive value error paths.
- The `ignore_rejects` follow-up found an original Knots RDTS policy-bypass
  edge in `PolicyScriptVerifyFlags()`: the broad
  `non-mandatory-script-verify-flag` ignore and the grouped
  `non-mandatory-script-verify-flag-upgradable` ignore could remove
  `REDUCED_DATA_MANDATORY_VERIFY_FLAGS` from the policy script check. The
  later consensus script check still rejected the transaction, so this was not
  a mempool consensus bypass, but it did trigger the internal
  `BUG! PLEASE REPORT THIS! CheckInputScripts failed against latest-block but
  not STANDARD flags` log path. The port now keeps RDTS flags enforced through
  those legacy bypass names, and `feature_rdts.py` covers both the broad
  PUSHDATA path and grouped unknown-witness path while asserting the consensus
  fallback log is not hit.
- While rerunning focused units, Core's newer
  `script_p2sh_tests/ValidateInputsStandardness` expectations were still
  written for Core's generic `bad-txns-nonstandard-inputs` reason. The ported
  Knots helper returns specific ignore-reject names such as
  `bad-txns-input-scriptcheck-sigops`, and intentionally defers a non-push-only
  P2SH scriptSig after `IsStandardTx()` is ignored so consensus script checks
  catch it. The unit now matches the ported Knots behavior.
- The RDTS consensus-equivalence follow-up compared the port against Knots'
  `interpreter`, `tx_verify`, `validation`, `deploymentstatus`, `versionbits`,
  and chain-parameter paths. The script, output-size, generation-transaction,
  per-input old-UTXO exemption, and temporary-deployment state-machine logic
  match Knots after adapting from Knots' global BIP9 window/threshold fields to
  Core master's per-deployment `period`/`threshold` fields. The only material
  API adaptation in `ConnectBlock()` is replacing Knots'
  `StateSinceHeight(pindex->pprev, ...)` activation-height query with Core's
  `Info(*pindex, ...).active_since`; the focused RDTS activation tests cover
  that replacement at activation, expiry, and activation-boundary reorg points.
- The RDTS P2P-service review found a port-introduced handshake cleanup issue
  after adapting Knots' preferential RDTS peering and `-maxstaleoutbound`
  behavior (`7f57236043`, `44d7e88dba`) onto current Core's newer VERSION
  handling. The port had retained Core's earlier peer-service/tx-relay setup
  and also inserted Knots' later setup block after the RDTS stale-peer gate, so
  accepted transaction-relay peers called `Peer::SetTxRelay()` twice and hit
  its `Assume(!m_tx_relay)` invariant. Unmodified Knots sets this state once
  after the RDTS gate, and current Core has no RDTS gate. The port now removes
  the duplicate setup as `d10d97fd54`, updates stale fixed-limit comments, and
  extends `peerman_tests` / `p2p_handshake.py` to cover `-maxstaleoutbound`
  parsing and the zero-tolerance non-BIP110 path.
- The versionbits RPC review confirmed Knots' `period_start` field for
  BIP9 signalling statistics (`5e04731447`) is present in the port and covered
  by `rpc_blockchain.py`. Current Core master still reports `period`,
  `elapsed`, and `count` without `period_start`. This is RPC observability
  rather than a security or consensus change.
- Earlier exact patch-id sweeps were useful for finding simple omissions, but
  they are not a complete proof because many Knots changes are adapted,
  squashed, or present through current Core under different commits. After the
  codex32 follow-up above, the remaining manually reviewed 2026 exact-patch
  misses include `33f9de6b91` and `c4c558b2c4`; both are historical reverts,
  not final Knots tree deltas. The Windows taskbar progress support restored
  by later Knots merges is now present in this port, and Knots' miner code uses
  the same overflow-safe `nBlockWeight + BLOCK_FULL_ENOUGH_WEIGHT_DELTA` check
  as this port/current Core.
- Extending the same review to non-merge Knots commits since 2025-01-01 also
  covered those two historical reverts, a historical CI-only `MAKEJOBS` revert
  (`5f7f1cf181`) superseded by Knots' later Cirrus runner workflow, and
  archived 28.1 release notes (`bb5f76ee01`) that are already present
  byte-for-byte in this tree but were intentionally excluded from the patch-id
  evidence set with other historical release notes. The CI mismatch is not a
  runtime/client delta; the port retains current Core's newer CI workflow on
  the rebased base.
- Extending the same review to Knots non-merge commits since 2024-01-01 covered
  historical archived release notes for Core 25.2, 26.1, 26.2, 27.0, 27.1,
  27.2, and 28.0, plus old release-note fragments intentionally omitted from
  the patch-id evidence set. No additional runtime/client omission has been
  identified from that historical-release-note bucket.
- A follow-up exact-patch mismatch pass rechecked several older and 2025
  runtime buckets that still do not match by patch-id because they were adapted
  to current Core or squashed with surrounding port work. The reviewed surfaces
  are present: help-option negation is rejected through
  `ArgsManager::DISALLOW_NEGATION`; Knots' libevent HTTP bind tolerance/cleanup
  series is superseded by the current `Sock`-based `HTTPServer` listener with
  `IsIgnoredDefaultBindError()` and RAII socket ownership; prune-lock RPCs,
  dynamic `maxmempool`, fee histograms, `listmempooltransactions`,
  `getrpcwhitelist`, script-thread RPCs, BIP322 signing/verification,
  `deriveaddresses(require_checksum)`, wallet implicit-segwit controls,
  restored `-blockmaxsize`, named-pipe/human-readable `dumptxoutset`,
  AutoFile explicit-close checks, low-memory cache flushing, GUI settings/RPC
  history, traffic graph tooltips, mempool stats, and the Network Watch GUI
  are all present in the current tree. These were not new omissions or original
  Knots defects from this pass.
- The remaining source-looking paths from a final file-presence sweep were
  classified against current Core history and live references rather than
  restored mechanically. The old `core_write.cpp` module is merged into
  `core_io`; `policy/fees*` is now
  `policy/fees/block_policy_estimator*`; `txorphanage.*` is under
  `src/node`; `transaction_identifier.h` is under `src/primitives`;
  `support/events.h`, `FindLibevent.cmake`, `libevent.mk`, and
  `raii_event_tests.cpp` disappeared with the current Core HTTP/libevent
  removal; `policy_fee_tests.cpp` is `feerounder_tests.cpp`; the old
  compiler-bug test and `epochguard.h` were removed upstream; and Knots'
  `test/util/str.cpp` `CaseInsensitiveEqual` helper is now covered by
  `util/strencodings` and `util_string_tests`. The functional runner lists
  resolve to existing files after the legacy-wallet test restoration, and no
  additional live runtime/client omission was found in this sweep.

## Original Knots Defects Confirmed

The `addnode` RPC crash was confirmed on an unmodified local build of Knots
`29.x-knots`:

```text
bitcoin-cli -regtest addnode 127.0.0.1:18445 onetry false inbound
```

Result on original Knots:

- `bitcoind` aborted with exit code 134.
- Assertion: `net.cpp:3058: ... OpenNetworkConnection(...): Assertion 'conn_type != ConnectionType::INBOUND' failed.`

This was not introduced by the port. The port rejects this input with
`RPC_INVALID_PARAMETER`, and `rpc_net.py` now covers the regression path.

The REST mempool-transactions route bug was also confirmed on an unmodified
local build of Knots `29.x-knots`:

```text
/rest/mempool/transactions/info.json
/rest/mempool/transactions/info.json?sequence_start=bad
```

Both requests were handled by the generic mempool endpoint and returned:

```text
Invalid URI format. Expected /rest/mempool/<info|info/with_fee_histogram|contents>.json
```

This was not introduced by the port. The port now registers the longer
`/rest/mempool/transactions/` prefix before `/rest/mempool/`, and
`interface_rest.py` covers both a successful request and the malformed
`sequence_start` parse-error path.

The RDTS `ignore_rejects` internal-bug log was confirmed on an unmodified
local build of Knots `29.x-knots` by running the port's strengthened RDTS
functional test against Knots' binaries:

```text
python3 /mnt/my_storage/bitcoin/test/functional/feature_rdts.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini
```

Result on original Knots:

- The test failed during
  `ignore_rejects=["non-mandatory-script-verify-flag"]`.
- `testmempoolaccept` still rejected the RDTS-invalid transaction, so no
  mempool consensus bypass was observed.
- `debug.log` contained:
  `BUG! PLEASE REPORT THIS! CheckInputScripts failed against latest-block but
  not STANDARD flags ... mempool-script-verify-flag-failed (Push value size
  limit exceeded)`.

The port preserves rejection but rejects during the policy script check, before
the internal consensus-fallback bug log can be triggered.

The ForceInbound `getpeerinfo.permissions` omission was confirmed on an
unmodified local build of Knots `29.x-knots` by running the port's strengthened
eviction functional test against Knots' binaries:

```text
python3 /mnt/my_storage/bitcoin/test/functional/p2p_eviction.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_eviction_forceinbound_repro
```

Result on original Knots:

- ForceInbound still worked well enough for the whitebind peer to connect.
- The test failed because the ForceInbound peer's `getpeerinfo` permissions
  array omitted `forceinbound`.

This was not introduced by the port. The port now includes `forceinbound` in
`NetPermissions::ToStrings(...)`, so RPC output matches the accepted permission
and help text.

The external signer duplicate-fingerprint enumeration bug was confirmed on an
unmodified local build of Knots `29.x-knots` with the mock signer returning
`00000001`, duplicate `00000001`, then valid `00000002`:

```text
bitcoin-cli -regtest enumeratesigners
```

Result on original Knots:

- Only `00000001` was returned; `00000002` was silently skipped.
- Source inspection explains the behavior: Knots uses `if (duplicate) break;`
  in `ExternalSigner::Enumerate(...)`, while current Core and this port use
  `continue`.

This was not introduced by the port. The port inherits current Core's
`continue` behavior, and `rpc_signer.py` covers the duplicate-then-unique
enumeration case.

## Core-Missing Hardening Candidates

The following items are the strongest remaining security-shaped candidates
found in Knots after filtering out changes already present in current Core
under different commits. They are not all proven exploitable.

- AllocateFileRange data-corruption hardening:
  `cab5e8d861`, `4a9f8c78f6`, `8e9c114fa`, `476861713`, `2c113ebc2`

  Core master still has the older macOS, POSIX, Windows, and fallback
  preallocation behavior. Knots disables the buggy macOS path, calls
  `posix_fallocate` only on the intended range, avoids Windows truncation, and
  makes the fallback preserve existing bytes. These are already present in this
  port as `bb2d44ee65` through `33a24d08ea`. Source comparison against
  unmodified Knots confirmed this was not introduced by the port; the port now
  also has `fs_tests/allocate_file_range_preserves_existing_bytes`
  (`4ecd895b33`) to assert the public helper does not truncate or clobber
  existing bytes and still extends the requested range with zeroes.

- External signer fingerprint hardening:
  `6d2c2259ee`, `12eefda89a`, `ee39394ad3`

  Knots validates signer fingerprints before command use, requires exactly
  eight hex characters, and avoids logging untrusted fingerprint text. This is
  local external-signer hardening, not a remote P2P issue. A minimal
  unmodified Knots startup with `not-a-secret-fingerprint` returned the generic
  invalid-fingerprint error without echoing that value in either RPC output or
  `debug.log`; the port's `rpc_signer.py` now checks the same non-echo
  invariant.

- RPC cookie replacement and permission hardening:
  `e49dfac324`, `7140e1f149`, `622b768945`, `50b7a50a61`, `198466d5d3`

  Knots sets temporary cookie permissions before writing, deletes stale temp
  files, deletes before replace, and avoids deleting a cookie replaced by
  another process. These are local RPC-auth file robustness improvements. A
  minimal unmodified Knots startup preserved
  `__cookie__:replaced-by-another-process` after shutdown when the node was
  stopped with the original generated credentials; the port now covers the same
  shutdown invariant in `rpc_users.py`.

- RPC auth-file and blank-token handling:
  `9f6d3fbe78`, `39bde96b6`, `51588287fb`, `a7a205dc7d`, `edb3686495`,
  `a34ee591e3`, `0545fb9215`, `a5e2475758`, `adccd25c27`, `1369150c00`

  Knots supports loading rpcauth entries from files, multiple auth entries per
  file, wallet-restricted rpcauth entries, and explicit disabling with
  `-norpcauth`. It also treats a blank `-rpcauth` token as a no-op without
  disrupting other nonblank `-rpcauth` tokens. Current Core's `httprpc.cpp`
  still parses only direct `-rpcauth` values and errors on blank entries. This
  is local configuration hardening, not a remote bypass by itself.

- Wallet symlink/reparse-point path hardening:
  `39f48a142f`, `1f118f18c4`, `ee042e9ad6`

  Current Core master still uses ordinary `std::filesystem` symlink checks in
  the wallet directory scan and regular-file wallet path compatibility case.
  Knots adds an `IsSymlink(...)` helper that detects Windows reparse points,
  avoids recursing such entries while scanning `listwalletdir`, and prevents a
  symlink/reparse point from being accepted as a legacy top-level wallet file.
  This is local path-safety hardening around wallet discovery/loading.

- Wallet failed-cleanup directory hardening:
  `0388bfc6e`

  Current Core master still assumes newly created restore/migration wallet
  directories are empty before removing them during failure cleanup. Knots logs
  and leaves the directory alone if it is unexpectedly non-empty, avoiding an
  unintended removal attempt against a path that no longer has the shape the
  cleanup code expected. This is local wallet data-safety hardening, not a
  consensus issue.

- Subprocess fd cleanup before exec:
  `214047ecd3`, `ed5a3b3604`

  Knots implements `close_fds` cleanup with `/proc/self/fd`, then replaces it
  with `close_range`/fallback closing to avoid non-async-signal-safe work after
  `fork()`. Core's current `RunCommandParseJSON` path does not request
  `close_fds`, but the Knots/port Tor subprocess path does. The port now has a
  focused `system_tests/subprocess_close_fds` regression that verifies file
  descriptors are inherited without `close_fds` and closed with it.

- Port mapping disabled when not listening:
  `95c8a63102`

  Knots force-disables explicit `-upnp=1` and `-natpmp=1` values when parameter
  interaction leaves the node with `-listen=0`. Current Core no longer exposes
  UPnP, but its NAT-PMP interaction still uses a soft disable, so an explicit
  `-listen=0 -natpmp=1` can survive until `StartMapPort(...)`. This is local
  network-surface/privacy hardening, not a consensus issue. A minimal
  unmodified Knots startup with `-listen=0 -upnp=1 -natpmp=1` logged both
  forced-disable messages, confirming this is not port-introduced. The port now
  covers the direct explicit-argument case in `feature_config_args.py`, in
  addition to the existing `-connect=0` / `-noconnect` interaction coverage.

- HTTP RPC bind failure behavior:
  `57becdf59e` plus follow-up listen/bind cleanup commits

  Knots fails initialization when any explicitly requested RPC bind fails,
  while current Core only requires at least one endpoint to bind. This is
  configuration-safety/availability hardening rather than a confirmed
  vulnerability. Direct reproduction on unmodified Knots with one occupied
  `-rpcbind` endpoint and one free endpoint returned exit code 1, the generic
  HTTP startup error on stderr, and the specific bind-all-endpoints error in
  `debug.log`; the port covers this in `rpc_bind.py`.

- Invalid-block peer punishment relaxation:
  `7c7b5839f4`

  Current Core still marks peers as misbehaving/discouraged for several invalid
  block/header paths in `MaybePunishNodeForBlock(...)`. Knots instead routes
  those paths through `CNode::PunishInvalidBlocks()`: inbound, manual, feeler,
  and `noban` peers are tolerated, while outbound full-relay, block-relay,
  address-fetch, and this port's private-broadcast peers are simply
  disconnected rather than discouraged. This is network partition/availability
  hardening rather than a consensus-rule change. Current Core already carries
  the related transaction-relay cleanup (`drop MaybePunishNodeForTx`) and the
  single script-check path and onion-inbound whitelist permission suppression;
  the remaining Core difference is invalid-block peer-punishment behavior.

- ForceInbound trusted-inbound eviction:
  `3544a26256`, `711dadb546`, `067f80e1b5`, `3db935abd1`

  Current Core does not expose Knots' `forceinbound` P2P permission. Knots and
  this port can mark selected inbound peers as trusted enough to force an
  eviction attempt when inbound slots are full, choosing a random unprotected
  inbound peer if the regular protection logic would otherwise leave no
  candidate. This is local operator availability/DoS hardening, not a
  consensus change.

- CJDNS addnode duplicate detection:
  `28823f30dc`

  Current Core has the earlier CJDNS `GetAddedNodeInfo()` fix, but still
  compares new addnode entries against existing entries using plain IPv6
  resolution in `AddNode()`. Knots flips RFC4193-looking CJDNS addresses before
  comparison and rejects CJDNS duplicates even when the port differs, avoiding
  repeated manual-connection entries to the same CJDNS node. The focused
  `rpc_net.py` CJDNS duplicate test passes on the port and as an isolated
  cross-check against unmodified Knots.

- V2-only clearnet outbound option:
  `cbfbefd8f3`, `7c06acab77`, `f4fc3f03f4`, `5891703ba0`

  Current Core supports BIP324/v2 transport but does not expose Knots'
  debug-only `-v2onlyclearnet` policy switch. Knots and this port can refuse
  outbound V1 and V2-to-V1 fallback connections on IPv4/IPv6 while still
  allowing V1 on non-clearnet networks such as onion. This is default-off
  transport-policy hardening, not a consensus change.

- Persistent unexpected block-version signalling warnings:
  `78d5cb210b`, `771ee9fbb4`, `e94eba4e03`, `c17e9d41d5`

  Current Core warns on unknown versionbits activation for BIP323-available
  bits, but does not keep Knots' additional persistent last-100-block warnings
  for unknown version schemas, individual unexpected versionbits, or BIP320
  reserved-bit signalling thresholds. This is operator/security visibility
  hardening around possible soft-fork signalling, not a consensus-rule change.

- RPC multi-warning string-mode visibility:
  `e4e4a81317`

  Current Core's deprecated `getblockchaininfo`, `getmininginfo`, and
  `getnetworkinfo` warning string mode returns only the last active warning.
  Knots and this port join all active warnings with newlines, matching the
  non-deprecated array mode's visibility instead of silently dropping earlier
  warnings. This is operator/security visibility hardening for deprecated RPC
  clients, not a consensus change.

- DNS seed bootstrap policy:
  `277edb9009`

  Current Core still queries the Peter Todd DNS seeds on mainnet and testnet.
  Knots removes those two seed hostnames. This changes the bootstrap trust and
  availability surface, but the audit does not have evidence that it is a
  vulnerability fix or a consensus issue. The port pins this with
  `chainparams_tests/dns_seed_removals`.

- User-agent sanitization/log escaping:
  `b9d2634b81`

  Core currently strips printable characters outside `SAFE_CHARS_DEFAULT` from
  received peer user agents before storing `cleanSubVer`, and logs that stored
  value directly. Knots preserves the full printable user-agent string for peer
  display and percent-escapes unsafe printable characters at log time. This is
  log/UI integrity hardening against confusing or spoofed peer user-agent text.
  The strengthened `p2p_handshake.py` user-agent test passes against
  unmodified Knots and the fixed port, confirming the earlier missing log
  escape was port-introduced.

- ZMQ notification resilience and read-block logging:
  `1c4d2d54d8`, `268fb1e0e3`, `ba28af94bd`

  Current Core still shuts down and removes a ZMQ notifier after one failed
  notification and still routes raw-block disk-read failures through
  `zmq_strerror(errno)`. Knots keeps notifiers active after transient send/read
  failures and logs the failing block hash without using unrelated ZMQ errno
  text. This is notification availability and diagnostics hardening, not a
  consensus issue.

- Fee-estimator file read overflow guard:
  `163d3e5c13`

  Current Core still computes `scale * maxPeriods` before checking whether a
  serialized fee-estimates file tracks more than one week of confirmations.
  Knots checks `scale > 1008 / maxPeriods` first and only multiplies after the
  corrupt-file bound has passed. This is local corrupt-file hardening for
  `fee_estimates.dat`, not remote network exposure.

- External or Knots-only surfaces:
  `d637873230` fixes `GetBlockFileInfo` bounds handling, but the obvious
  RPC-facing caller is Knots' `getblockfileinfo`. Current Core's corresponding
  callers are internal/tests. Actual Knots and the port return `nullptr` when
  the block-file info vector is empty; current Core's older `size() - 1` check
  can underflow before calling `.at(n)`. The port now covers the empty lookup in
  `blockmanager_tests/blockmanager_get_block_file_info_empty`. The
  `85c8d477b0` fee-histogram unsigned-decrement fix matters for Knots'
  `getmempoolinfo(with_fee_histogram=...)` and
  `/rest/mempool/info/with_fee_histogram`, but current Core has no
  corresponding histogram surface. The wallet `2a09a34129` null-provider guard
  matters for Knots' `segwit_inputs_only` coin-control option, but current Core
  has no matching option. The BDB cleanup/data-loss fixes matter for this port
  because BDB support is retained, but current Core master no longer has the
  same BDB write-environment files.

- Ephemeral-dust reorg policy hardening:
  Knots' mempool policy keeps an unswept ephemeral-dust child from re-entering
  the mempool after its containing block is invalidated, while still allowing
  re-entry when the child sweeps the dust. Core's current test expected the
  unswept child to resurrect. This looks like mempool policy hardening rather
  than a consensus issue, and it was confirmed on an unmodified local Knots
  build before updating the port test.

- Raw transaction max-feerate accounting with policy-adjusted vsize:
  `4b3cc3d48e`, `1cee5b1ac7`, `335d928d96`

  Core's current `sendrawtransaction` still turns the user-supplied
  `maxfeerate` into an absolute fee limit with plain
  `GetVirtualTransactionSize(*tx)`. Knots passes the `CFeeRate` into
  `BroadcastTransaction` and uses the test-accept result's `m_vsize`, so
  `-bytespersigop` and other non-weight vsize adjustments are included in the
  same way as `testmempoolaccept` and mempool policy. This is wallet/RPC
  self-protection rather than remote consensus risk, but it is a real
  user-facing fee-limit correctness fix. The focused port run and an isolated
  unmodified Knots run both accept the high-sigop transaction under a
  `maxfeerate` that would be too low if plain transaction vsize were used.

- Descriptor-wallet `importaddress` compatibility:
  `be3ae51ece`

  Current Core master no longer registers the legacy `importaddress` wallet
  RPC. Knots and this port keep it available for descriptor watch-only wallets
  by translating addresses into `addr(...)` descriptors and hex scripts into
  `raw(...)` descriptors, while rejecting descriptor wallets with private keys
  enabled and descriptor raw-script P2SH imports. This is RPC compatibility and
  watch-only wallet ergonomics, not consensus or network security hardening.

- `walletcreatefundedpsbt` anti-fee-sniping:
  `c5448df366`

  Current Core master already discourages fee sniping for `send` and `sendall`
  when no explicit locktime is supplied, but `walletcreatefundedpsbt` still
  constructs default PSBTs with zero fallback locktime. Knots and this port
  apply the same height-based anti-fee-sniping default to funded PSBTs, and the
  port also covers PSBTv0 locktime output. This is wallet privacy and
  miner-incentive hardening, not a consensus-rule change.

- PSBT previous-transaction injection for RPC updating/signing:
  `bdb4ca4195`, `eea8588f07`

  Current Core master can populate PSBT input UTXOs from the UTXO set, mempool,
  and txindex, but `utxoupdatepsbt` and `descriptorprocesspsbt` do not accept
  caller-provided previous transaction hex. Knots and this port can use those
  provided transactions to fill and sign dependent PSBT chains that are not yet
  in the mempool or UTXO set. This is transaction-construction functionality,
  not consensus behavior.

- `getblockfrompeer` without a known header:
  `6c78d40b89`, `aebfd947d2`, `2b67ea465c`

  Current Core master still requires the block header before
  `getblockfrompeer` can request the block, and rejects future-height requests
  in prune mode. Core also still drops the old in-flight block request before
  checking whether the same peer was already asked, so it does not preserve
  Knots' duplicate-request error. Knots and this port can request an arbitrary
  block hash from a selected peer, including in pruned-node scenarios where the
  node has not seen or synced past the header yet, and report repeated same-peer
  requests as `Already requested from this peer`. This is RPC/operator recovery
  functionality, not consensus behavior.

High-signal hardening already present in Core under the same or different
commits and therefore not counted as missing here: secp256k1 ellswift overflow
key handling, `LocalServiceInfo::nScore` saturation, miner `addPackageTxs`
overflow, compact-block witness mutation checks, `LoadChainTip` UB,
reindex-chainstate periodic dbcache flushes (`ac7c0590ef`, rebased from Core
`84820561dc`; current Core carries this through `1d4e3d1b18`),
requested-block `ReadBlock(..., expected_hash)` checks in net processing,
`SetStdinEcho` UB, fd-limit overflow/RLIMIT_INFINITY handling, RPC credentials
hashed in memory, PSBT bounds asserts, v2-to-v1 reconnect UAF, randomized Tor
stream-isolation credential prefixes, feebumper combined-fee crash, wallet
coin-selection boolean amount fix, precomputed transaction-data lifetime
hardening (CVE-2024-52911), Tor-control excessive-line OOM hardening, I2P SAM
`SESSION CREATE` request redaction, BDB overflow data lengths and btree-level
validation, PSBT proprietary-field preservation during combining, monotonic
`uptime`, first-run pruned-disk-space warning rounding, Windows exclusive `wbx`
opens, LevelDB file-size initialization, wallet `sendall` transaction-size
error handling, miniscript assert guards, and most cpp-subprocess
memory/Windows fixes.

## Open Risks

- Legacy-wallet creation is a non-consensus divergence from Knots on this
  current-Core base. Core master no longer creates new legacy wallets, and this
  port now preserves Core's explicit RPC error instead of crashing. Ported
  legacy-only wallet tests are skipped by the framework when `--legacy-wallet`
  mode is selected.
- BIP-110/RDTS consensus equivalence remains the main consensus-risk area
  because it is an intentional soft-fork divergence from Core. The current pass
  mapped the Knots RDTS subjects to replayed/adapted port commits, compared the
  consensus-critical source paths against Knots, and reran the focused RDTS,
  UTXO-height, temporary-deployment, versionbits, script, tx-validation, and
  P2P handshake tests. That is focused regression evidence and source-level
  adaptation evidence, not a full independent consensus-equivalence proof or
  exhaustive cross-client block corpus.

## Verification

Source/manifest checks:

- `comm -23 <(git ls-tree -r --name-only knots/29.x-knots | sort)
  <(git ls-files | sort)`
- `rg -n "FindLibevent|libevent|mempool-limits|system_ram\\.h|core_write\\.cpp|policy/fees|fees_args|support/events\\.h|compilerbug_tests|policy_fee_tests|raii_event_tests|test/util/index|test/util/str|txorphanage\\.h|txorphanage\\.cpp|epochguard|transaction_identifier\\.h|mempool_package_onemore|rpcauth-test" . -g '!test/cache/**' -g '!build/**' -g '!depends/work/**' -g '!depends/built/**' -g '!depends/sources/**'`
- `git log origin/master --follow --oneline -- <remaining source-looking
  missing path>` for old `core_write`, fee, libevent, orphanage, transaction
  identifier, epochguard, and test-helper paths
- Direct import of `test/functional/test_runner.py` with
  `test/functional` on `sys.path` returned `missing_count 0`

Builds:

- `cmake -B build -DRDTS_CONSENT=RUNTIME_WARN`
- `cmake --build build --target bitcoind bitcoin-cli test_bitcoin -j4`
- `cmake --build build --target bitcoind -j4`
- `cmake --build build --target bitcoin-cli -j4`
- `cmake --build build --target bitcoind bitcoin-cli -j4`
- `cmake -S . -B /tmp/bitcoin-zmq-build -DWITH_ZMQ=ON -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_GUI=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoin_zmq -j2`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoind bitcoin-cli -j2`
- `cmake -S . -B /tmp/bitcoin-fuzz-wallet-bdb -DBUILD_FUZZ_BINARY=ON
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_GUI=OFF -DWITH_CCACHE=OFF
  -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-fuzz-wallet-bdb --target fuzz -j4`
- `cmake -S . -B build-fuzz-check -GNinja -DBUILD_FOR_FUZZING=ON
  -DBUILD_FUZZ_BINARY=ON -DBUILD_GUI=OFF -DBUILD_BENCH=OFF
  -DBUILD_KERNEL_LIB=OFF`
- `ninja -C build-fuzz-check
  src/test/fuzz/CMakeFiles/fuzz.dir/__/__/wallet/test/fuzz/wallet_bdb_parser.cpp.o`
- `cmake --build build --target bitcoind test_bitcoin -j4`
- `cmake -S . -B /tmp/bitcoin-kernel-after -DBUILD_KERNEL_LIB=ON
  -DBUILD_SHARED_LIBS=ON -DBUILD_DAEMON=OFF -DBUILD_CLI=OFF
  -DBUILD_BITCOINCONSENSUS_LIB=OFF -DBUILD_TX=OFF -DBUILD_UTIL=OFF
  -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_WALLET_TOOL=OFF -DBUILD_GUI=OFF
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF
  -DENABLE_WALLET=OFF -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-kernel-after --target bitcoinkernel -j4`
- `cmake -S . -B /tmp/bitcoin-kernel-dbcache -DBUILD_KERNEL_LIB=ON
  -DBUILD_SHARED_LIBS=ON -DBUILD_DAEMON=OFF -DBUILD_CLI=OFF
  -DBUILD_BITCOINCONSENSUS_LIB=OFF -DBUILD_TX=OFF -DBUILD_UTIL=OFF
  -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_WALLET_TOOL=OFF -DBUILD_GUI=OFF
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF
  -DENABLE_WALLET=OFF -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-kernel-dbcache --target bitcoinkernel -j4`
- `cmake -S . -B /tmp/bitcoin-kernel-leveldb -DBUILD_KERNEL_LIB=ON
  -DBUILD_SHARED_LIBS=ON -DBUILD_DAEMON=OFF -DBUILD_CLI=OFF
  -DBUILD_BITCOINCONSENSUS_LIB=OFF -DBUILD_TX=OFF -DBUILD_UTIL=OFF
  -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_WALLET_TOOL=OFF -DBUILD_GUI=OFF
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF
  -DENABLE_WALLET=OFF -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-kernel-leveldb --target bitcoinkernel -j4`
- `cmake --build build --target test_bitcoin -j4`
- `cmake --build build --target bitcoind -j4`
- `TMPDIR=/mnt/my_storage/tmp cmake -S . -B /tmp/bitcoin-asan-consensus-after
  -DBUILD_BITCOINCONSENSUS_LIB=ON -DBUILD_SHARED_LIBS=ON
  -DSANITIZERS=address -DBUILD_DAEMON=OFF -DBUILD_CLI=OFF -DBUILD_TX=OFF
  -DBUILD_UTIL=OFF -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_KERNEL_LIB=OFF
  -DBUILD_WALLET_TOOL=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF
  -DENABLE_WALLET=OFF -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `TMPDIR=/mnt/my_storage/tmp cmake --build /tmp/bitcoin-asan-consensus-after
  --target bitcoinconsensus -j1`
- `git diff --check`
- `cmake --build build --target test_bitcoin -j2`
- `cmake --build build --target help | rg -n
  "(qt|bitcoin-qt|test_bitcoin-qt|test_bitcoin_qt)"` returned no configured Qt
  target
- `rg -n "util/transaction_identifier\\.h" src test -g '!test/cache/**'`
  returned no matches after the Qt block visualizer include update
- `python3 -c "import xml.etree.ElementTree as ET;
  ET.parse('src/qt/forms/optionsdialog.ui')"`
- `rg -n "threadsWarning|Using more threads than CPU cores|value >
  GetNumCores\\(\\)" src/qt/forms/optionsdialog.ui src/qt/optionsdialog.cpp`
- `cmake -S . -B /tmp/bitcoin-qt-check -DBUILD_GUI=ON -DBUILD_TESTS=ON
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_CCACHE=OFF
  -DRDTS_CONSENT=IMPLICIT` failed with `Could NOT find Qt (missing: Qt5_DIR
  Qt5_FOUND)`
- `make -C depends print-bdb_packages print-bdb_packages_ print-wallet_packages_
  NO_QT=1 NO_ZMQ=1 NO_UPNP=1 NO_USDT=1`
- `make -C depends print-upnp_packages print-packages NO_QT=1 NO_ZMQ=1
  NO_USDT=1`
- `make -C depends print-miniupnpc_download_path
  print-miniupnpc_download_file print-miniupnpc_file_name
  print-miniupnpc_sha256_hash print-miniupnpc_source
  print-miniupnpc_all_sources NO_QT=1 NO_ZMQ=1 NO_USDT=1`
- `make -C depends miniupnpc_fetched V=1 NO_QT=1 NO_ZMQ=1 NO_USDT=1`
- `make -C depends miniupnpc_extracted V=1 NO_QT=1 NO_ZMQ=1 NO_USDT=1`
- `make -C depends miniupnpc_preprocessed V=1 NO_QT=1 NO_ZMQ=1 NO_USDT=1`
- `make -C depends miniupnpc_built V=1 NO_QT=1 NO_ZMQ=1 NO_USDT=1`
- `cmake -S . -B /tmp/bitcoin-bdb-sqlite-probe -DWITH_BDB=ON
  -DWARN_INCOMPATIBLE_BDB=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_CCACHE=OFF
  -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-bdb-sqlite-probe --target bitcoin_wallet -j2`
- `cmake -S . -B /tmp/bitcoin-bdb-only-probe -DWITH_SQLITE=OFF
  -DWITH_BDB=ON -DWARN_INCOMPATIBLE_BDB=OFF -DBUILD_GUI=OFF
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-bdb-only-probe --target bitcoin_wallet -j2`
- `cmake --build /tmp/bitcoin-bdb-only-probe --target bitcoind -j2`
- `cmake -S . -B /tmp/bitcoin-bdb-only-tests-probe -DWITH_SQLITE=OFF
  -DWITH_BDB=ON -DWARN_INCOMPATIBLE_BDB=OFF -DBUILD_GUI=OFF
  -DBUILD_TESTS=ON -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-bdb-only-tests-probe --target test_util
  bitcoin_wallet -j2`
- `cmake --build build --target test_bitcoin -j2`
- `cmake --build build --target bitcoind -j2`
- `cmake --build build --target bitcoind test_bitcoin -j4`
- `cmake --build build --target bitcoind bitcoin-cli bitcoin-wallet -j4`
- `build/bin/bitcoind --help | rg -n
  "maxstaleoutbound|consensusrules|privatebroadcast|subdustfeepenalty" -C 2`
- `rg -n "maxstaleoutbound|consensusrules|privatebroadcast|subdustfeepenalty"
  doc/man/bitcoind.1 doc/man/bitcoin-qt.1 share/examples/bitcoin.conf`
- `BUILDDIR=$PWD/build contrib/devtools/gen-manpages.py
  --skip-missing-binaries` failed after skipping the disabled `bitcoin`,
  `bitcoin-tx`, `bitcoin-util`, and `bitcoin-qt` binaries because `help2man` is
  not installed locally
- Original Knots repro build:
  `cmake -S ../knots -B ../knots/build-repro -DRDTS_CONSENT=RUNTIME_WARN`
  and `cmake --build ../knots/build-repro --target bitcoind bitcoin-cli -j4`

Unit tests:

- `build/bin/test_bitcoin --run_test=versionbits_tests`
- `build/bin/test_bitcoin --run_test=script_tests`
- `build/bin/test_bitcoin --run_test=streams_tests`
- `build/bin/test_bitcoin --run_test=chainparams_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=chainparams_tests/dns_seed_removals`
- `build/bin/test_bitcoin --run_test=mempool_tests`
- `build/bin/test_bitcoin --run_test=transaction_tests`
- `build/bin/test_bitcoin --run_test=txvalidationcache_tests`
- `build/bin/test_bitcoin --run_test=txvalidation_tests`
- `build/bin/test_bitcoin --run_test=script_p2sh_tests/ValidateInputsStandardness`
- `build/bin/test_bitcoin --run_test=peerman_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=net_tests`
- `build/bin/test_bitcoin --run_test=net_tests/cnode_punish_invalid_blocks
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_get_block_file_info_empty`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_readblock_hash_mismatch
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=blockmanager_tests`
- `build/bin/test_bitcoin --run_test=netbase_tests/netpermissions_test
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=net_peer_connection_tests`
- `build/bin/test_bitcoin --run_test=rest_tests`
- `build/bin/test_bitcoin --run_test=validation_tests`
- `build/bin/test_bitcoin --run_test=validation_chainstatemanager_tests,wallet_tests
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=validation_block_tests`
- `build/bin/test_bitcoin --run_test=merkle_tests`
- `build/bin/test_bitcoin --run_test=miner_tests`
- `build/bin/test_bitcoin
  --run_test=policyestimator_tests/read_rejects_fee_estimates_with_oversized_scale`
- `build/bin/test_bitcoin --run_test=policyestimator_tests`
- `build/bin/test_bitcoin --run_test=codex32_tests`
- `build/bin/test_bitcoin --run_test=caches_tests`
- `build/bin/test_bitcoin --run_test=sanity_tests`
- `build/bin/test_bitcoin --run_test=banman_tests`
- `build/bin/test_bitcoin --run_test=hash_tests`
- `build/bin/test_bitcoin --run_test=blockfilter_index_tests`
- `build/bin/test_bitcoin --run_test=txindex_tests,txospenderindex_tests,coinstatsindex_tests`
- `build/bin/test_bitcoin --run_test=db_tests,walletdb_tests,wallet_tests`
- `build/bin/test_bitcoin --run_test=fs_tests,walletdb_tests,wallet_tests`
- `build/bin/test_bitcoin --run_test=fs_tests/allocate_file_range_preserves_existing_bytes`
- `build/bin/test_bitcoin --run_test=db_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=wallet_tests/remove_created_wallet_dir_if_empty`
- `build/bin/test_bitcoin --run_test=util_tests`
- `build/bin/test_bitcoin --run_test=util_tests/test_sanitize_string_printable_chars`
- `build/bin/test_bitcoin --run_test=util_tests/outputtype_implicit_segwit`
- `build/bin/test_bitcoin --run_test=system_tests/subprocess_close_fds`
- `build/bin/test_bitcoin --run_test=system_tests`
- `build/bin/test_bitcoin --run_test=node_warnings_tests
  --catch_system_error=no --log_level=nothing --report_level=no`
- `./build/src/secp256k1/bin/tests --target=ellswift_xdh_bad_scalar_tests --iterations=16`
- `./build/src/secp256k1/bin/tests --target=ellswift --iterations=16`

Functional tests:

- `python3 test/functional/feature_rdts.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_utxo_height.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_temporary_deployment.py --configfile build/test/config.ini`
- `python3 test/functional/feature_bip9_max_activation_height.py --configfile build/test/config.ini`
- `python3 test/functional/feature_versionbits_warning.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_versionbits_warning`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_handshake_ua_escape_3`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_handshake_rdts_gate_fixed3`
- `python3 test/functional/p2p_eviction.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_eviction_forceinbound`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_permissions_onion_whitelist`
- `python3 test/functional/p2p_blockfilters.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_blockfilters_permission_2`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_permissions_blockfilters`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_net_cjdns_addnode_3`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_net_addconnection`
- `python3 test/functional/p2p_add_connections.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_add_connections_signet`
- `python3 test/functional/mempool_accept.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_datacarrier.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_dust.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_subdust_fee_penalty.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_mempool_sigoplimit_maxfeerate`
- `python3 test/functional/mempool_limit.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_mempool_limit_maxmempool_rpc`
- `python3 test/functional/mining_coin_age_priority.py --configfile build/test/config.ini`
- `python3 test/functional/mining_prioritisetransaction.py --configfile build/test/config.ini`
- `python3 test/functional/feature_maxuploadtarget.py --configfile build/test/config.ini`
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini`
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_interface_rest_listmempooltransactions`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_config_args_port_mapping_2`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_config_args_acceptnonstd_main_3`
- `python3 test/functional/feature_help.py --configfile build/test/config.ini`
- `python3 test/functional/feature_includeconf.py --configfile build/test/config.ini`
- `python3 test/functional/feature_fee_estimates_persist.py --configfile build/test/config.ini`
- `python3 test/functional/feature_fee_estimates_persist.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_fee_estimates_persist_save_rpc`
- `python3 test/functional/feature_segwit.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_segwit_mempool_hash`
- `python3 test/functional/feature_startupnotify.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_startupnotify_multi`
- `python3 test/functional/feature_notifications.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_notifications_multi`
- `python3 test/functional/feature_index_prune.py --configfile build/test/config.ini`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile build/test/config.ini`
- `python3 test/functional/feature_softwareexpiry.py --configfile build/test/config.ini`
- `python3 test/functional/feature_torcontrol.py --configfile build/test/config.ini`
- `python3 test/functional/interface_bitcoin_cli.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_help.py --configfile build/test/config.ini`
- `python3 test/functional/tool_cli_completion.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_signer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_signer_fingerprint_2`
- `python3 test/functional/wallet_signer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_signer_warning`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_users_cookie_replace`
- `python3 test/functional/rpc_getrpcwhitelist.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_bind.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_current_tip`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_period_start`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_scriptthreads`
- `python3 test/functional/interface_zmq.py --configfile /tmp/bitcoin-zmq-build/test/config.ini`
- `python3 test/functional/p2p_v2_encrypted.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_v2_encrypted`
- `python3 test/functional/rpc_getblocklocations.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getgeneralinfo.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_sort_multisig.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_deriveaddresses.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_deriveaddresses_checksum`
- `python3 test/functional/rpc_deriveaddresses.py --usecli --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_deriveaddresses_checksum_cli`
- `python3 test/functional/rpc_setban.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_disconnect_ban.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_disconnect_ban_ip_subnet`
- `python3 test/functional/rpc_rawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_invalid_address_message.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_invalid_address_validateaddress_compat`
- `python3 test/functional/rpc_txoutproof.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_txoutproof`
- `python3 test/functional/rpc_packages.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_min_conf`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_anti_fee_sniping`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_prevtxs`
- `python3 test/functional/mempool_fee_histogram.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_getblockfrompeer_no_header`
- `python3 test/functional/rpc_mempool_info.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_dos_header_tree.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_block_times.py --configfile build/test/config.ini`
- `python3 test/functional/feature_block.py --configfile build/test/config.ini
  --skipreorg --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_block_skip`
- `python3 test/functional/feature_assumeutxo.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_assumeutxo_after_blockchain`
- `python3 test/functional/tool_wallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_startup.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_assumeutxo.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_assumeutxo_after_fix`
- `python3 test/functional/wallet_address_types.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_address_types_change_pref`
- `python3 test/functional/wallet_balance.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_balance_getbalance_4`
- `python3 test/functional/wallet_avoidreuse.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_avoidreuse_getbalance_4`
- `python3 test/functional/wallet_descriptor.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_descriptor_importaddress_4`
- `python3 test/functional/wallet_create_tx.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_create_tx_setfeerate`
- `python3 test/functional/wallet_bumpfee.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_bumpfee_setfeerate`
- `python3 test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_fundrawtransaction_min_conf`
- `python3 test/functional/wallet_multiwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_backup.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_send.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_send_fee_mode`
- `python3 test/functional/wallet_migration.py --configfile build/test/config.ini`
  (skipped: previous releases not available or disabled)
- `python3 test/functional/wallet_keypool.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_keypool_isactive_fixed`
- `python3 test/functional/wallet_signmessagewithaddress.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_signmessage_bip322`
- `python3 test/functional/rpc_signmessagewithprivkey.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_signmessage_bip322`
- `python3 test/functional/wallet_sweepprivkeys.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_importseed.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_import_with_label.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_importmulti.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_upgradewallet.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_implicitsegwit.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_inactive_hdchains.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_pruning.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_watchonly.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_watchonly.py --configfile build/test/config.ini --usecli --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_hd.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_hd_dumpmaster`
- `python3 test/functional/wallet_dump.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_dump.py --configfile build/test/config.ini
  --legacy-wallet --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_dump_hd_metadata_legacy`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_import_rescan.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/feature_rdts.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini`
  (fails on the inherited RDTS `ignore_rejects` internal-bug log described
  above)
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/p2p_eviction.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_eviction_forceinbound_repro`
  (fails on unmodified Knots because the ForceInbound peer's
  `getpeerinfo.permissions` array omits `forceinbound`)
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/rpc_signer.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_rpc_signer_fingerprint`
  (the invalid-fingerprint redaction checks pass first, then unmodified Knots
  fails the duplicate-then-unique signer enumeration case by returning only
  `00000001`)
- Original Knots cross-check:
  minimal startup with `../knots/build-repro/bin/bitcoind -regtest`, replacing
  `regtest/.cookie` with `__cookie__:replaced-by-another-process`, then
  stopping via `bitcoin-cli -rpcuser=__cookie__ -rpcpassword=<generated>`
  preserved the replacement (`cookie_after_stop=__cookie__:replaced-by-another-process`)
- Original Knots cross-check:
  `python3 /mnt/my_storage/bitcoin/test/functional/p2p_handshake.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_handshake_ua_escape`
  (passes on unmodified Knots, confirming the missing user-agent log escape was
  introduced by the port)
- Original Knots cross-check:
  a one-off subclass of `mempool_sigoplimit.py` running only
  `test_sendrawtransaction_maxfeerate_uses_sigop_adjusted_vsize` with
  `--configfile /mnt/my_storage/knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_mempool_sigoplimit_maxfeerate_only2`
  passed on unmodified Knots
- Original Knots cross-check:
  a one-off subclass of `rpc_net.py` running only
  `test_addnode_cjdns_duplicate` with
  `--configfile /mnt/my_storage/knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_rpc_net_cjdns_addnode_only` passed on
  unmodified Knots
- Original Knots cross-check:
  `python3 /mnt/my_storage/bitcoin/test/functional/feature_versionbits_warning.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_feature_versionbits_warning_check`
  (passes on unmodified Knots, confirming the earlier warning-range failure was
  introduced by the port's current-Core BIP323 adaptation)
- Original Knots cross-check:
  `python3 ../knots/test/functional/wallet_keypool.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_wallet_keypool_isactive_repro`
  (passes on unmodified Knots, confirming the local `wallet_keypool.py`
  failure was port-side test drift from losing Knots' explicit keypool setup)
- Original Knots expected-failure repro with a temporary
  `/mnt/my_storage/knots-assumeutxo-repro` worktree and the port's added
  post-validation raw-transaction assertion:
  `python3 test/functional/feature_assumeutxo.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_feature_assumeutxo_rawtx_repro`
  (fails on unmodified Knots with `AssertionError: not(0 == 201)`, confirming
  the inherited stale `confirmations_assumed` reporting bug)

The full `feature_block.py` run reached the large-reorg section but failed
because `/tmp` was full and the node shut down with `Disk space is too low!`;
the `--skipreorg` rerun above passed on a temp directory under
`/mnt/my_storage`.
