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
  coverage in `rpc_signer.py`.
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
  (`6fe0c50345`).
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
- The follow-up fuzz-build pass found Knots' `wallet_bdb_parser` deterministic
  seeding fix (`30f578a081`) was missing from the port. Actual Knots
  `29.x-knots` already contains the fix, so this was a port test omission rather
  than an original Knots runtime defect. The same fuzz build exposed stale
  current-Core API adaptations in fuzz-only sources: `kernel::CBlockFileInfo`
  qualification, `mini_miner`'s current miner/test helpers, direct mempool
  `Txid` lookups, and move-only `CTxMemPoolEntry` handling. These are now fixed
  as `f1f3f4ae7c`, and a fuzz-enabled compile of the `fuzz` target passes.
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
- `feature_fee_estimates_persist.py` passes, covering the `savefeeestimates`
  RPC and shutdown persistence path.
- Wallet sweep coverage passes on the current descriptor-wallet base:
  `wallet_sweepprivkeys.py` rejects invalid/unfunded keys and sweeps both
  unconfirmed and confirmed P2PKH outputs. Legacy-only Knots tests
  `wallet_dump.py` and `wallet_import_rescan.py` now reach the expected
  current-Core skip path after restoring the ported `AddressType` test helper
  and current `BitcoinTestFramework(__file__)` constructors (`9bfe1fb892`).
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
  same max-feerate decision for a high-sigop, low-weight P2WSH spend.
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
  CJDNS addnode reporting and duplicate CJDNS addnode rejection.
- Knots' user-agent sanitization hardening (`b9d2634b81`) is present in the
  port as `eacc171127`: received peer user agents keep printable punctuation in
  `cleanSubVer`, then percent-escape characters outside the default log-safe
  set when logging the version message. The port now adds focused `util_tests`
  coverage for `SAFE_CHARS_PRINTABLE` and log-style escaping.
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
  The same pass classified the fee-histogram unsigned-decrement fix
  (`85c8d477b0`, ported as `759e1d76b3`) as Knots-surface hardening because
  current Core has no `getmempoolinfo(with_fee_histogram=...)` or REST
  histogram surface. `policyestimator_tests` and `mempool_fee_histogram.py`
  pass with the ported code.
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
  are absent.
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
  port as `bb2d44ee65` through `33a24d08ea`.

- External signer fingerprint hardening:
  `6d2c2259ee`, `12eefda89a`, `ee39394ad3`

  Knots validates signer fingerprints before command use, requires exactly
  eight hex characters, and avoids logging untrusted fingerprint text. This is
  local external-signer hardening, not a remote P2P issue.

- RPC cookie replacement and permission hardening:
  `e49dfac324`, `7140e1f149`, `622b768945`, `50b7a50a61`, `198466d5d3`

  Knots sets temporary cookie permissions before writing, deletes stale temp
  files, deletes before replace, and avoids deleting a cookie replaced by
  another process. These are local RPC-auth file robustness improvements.

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
  `close_fds`, but the Knots/port Tor subprocess path does.

- HTTP RPC bind failure behavior:
  `57becdf59e` plus follow-up listen/bind cleanup commits

  Knots fails initialization when any explicitly requested RPC bind fails,
  while current Core only requires at least one endpoint to bind. This is
  configuration-safety/availability hardening rather than a confirmed
  vulnerability. Direct reproduction on unmodified Knots with one occupied
  `-rpcbind` endpoint and one free endpoint returned exit code 1, the generic
  HTTP startup error on stderr, and the specific bind-all-endpoints error in
  `debug.log`; the port covers this in `rpc_bind.py`.

- CJDNS addnode duplicate detection:
  `28823f30dc`

  Current Core has the earlier CJDNS `GetAddedNodeInfo()` fix, but still
  compares new addnode entries against existing entries using plain IPv6
  resolution in `AddNode()`. Knots flips RFC4193-looking CJDNS addresses before
  comparison and rejects CJDNS duplicates even when the port differs, avoiding
  repeated manual-connection entries to the same CJDNS node.

- DNS seed bootstrap policy:
  `277edb9009`

  Current Core still queries the Peter Todd DNS seeds on mainnet and testnet.
  Knots removes those two seed hostnames. This changes the bootstrap trust and
  availability surface, but the audit does not have evidence that it is a
  vulnerability fix or a consensus issue.

- User-agent sanitization/log escaping:
  `b9d2634b81`

  Core currently strips printable characters outside `SAFE_CHARS_DEFAULT` from
  received peer user agents before storing `cleanSubVer`, and logs that stored
  value directly. Knots preserves the full printable user-agent string for peer
  display and percent-escapes unsafe printable characters at log time. This is
  log/UI integrity hardening against confusing or spoofed peer user-agent text.

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
  callers are internal/tests. The `85c8d477b0` fee-histogram unsigned-decrement
  fix matters for Knots' `getmempoolinfo(with_fee_histogram=...)` and
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
  user-facing fee-limit correctness fix.

High-signal hardening already present in Core under the same or different
commits and therefore not counted as missing here: secp256k1 ellswift overflow
key handling, `LocalServiceInfo::nScore` saturation, miner `addPackageTxs`
overflow, compact-block witness mutation checks, `LoadChainTip` UB,
`SetStdinEcho` UB, fd-limit overflow/RLIMIT_INFINITY handling, RPC credentials
hashed in memory, PSBT bounds asserts, v2-to-v1 reconnect UAF, feebumper
combined-fee crash, wallet coin-selection boolean amount fix, precomputed
transaction-data lifetime hardening (CVE-2024-52911), Tor-control excessive-line
OOM hardening, I2P SAM `SESSION CREATE` request redaction, BDB overflow data
lengths and btree-level validation, PSBT proprietary-field preservation during
combining, monotonic `uptime`, first-run pruned-disk-space warning rounding,
Windows exclusive `wbx` opens, LevelDB file-size initialization, wallet
`sendall` transaction-size error handling, miniscript assert guards, and most
cpp-subprocess memory/Windows fixes.

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
- `cmake -S . -B /tmp/bitcoin-zmq-build -DWITH_ZMQ=ON -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_GUI=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoin_zmq -j2`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoind bitcoin-cli -j2`
- `cmake -S . -B /tmp/bitcoin-fuzz-wallet-bdb -DBUILD_FUZZ_BINARY=ON
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_GUI=OFF -DWITH_CCACHE=OFF
  -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-fuzz-wallet-bdb --target fuzz -j4`
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
- Original Knots repro build:
  `cmake -S ../knots -B ../knots/build-repro -DRDTS_CONSENT=RUNTIME_WARN`
  and `cmake --build ../knots/build-repro --target bitcoind bitcoin-cli -j4`

Unit tests:

- `build/bin/test_bitcoin --run_test=versionbits_tests`
- `build/bin/test_bitcoin --run_test=script_tests`
- `build/bin/test_bitcoin --run_test=streams_tests`
- `build/bin/test_bitcoin --run_test=chainparams_tests`
- `build/bin/test_bitcoin --run_test=mempool_tests`
- `build/bin/test_bitcoin --run_test=transaction_tests`
- `build/bin/test_bitcoin --run_test=txvalidationcache_tests`
- `build/bin/test_bitcoin --run_test=txvalidation_tests`
- `build/bin/test_bitcoin --run_test=script_p2sh_tests/ValidateInputsStandardness`
- `build/bin/test_bitcoin --run_test=peerman_tests`
- `build/bin/test_bitcoin --run_test=net_tests`
- `build/bin/test_bitcoin --run_test=net_peer_connection_tests`
- `build/bin/test_bitcoin --run_test=rest_tests`
- `build/bin/test_bitcoin --run_test=validation_tests`
- `build/bin/test_bitcoin --run_test=validation_block_tests`
- `build/bin/test_bitcoin --run_test=merkle_tests`
- `build/bin/test_bitcoin --run_test=miner_tests`
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
- `build/bin/test_bitcoin --run_test=wallet_tests/remove_created_wallet_dir_if_empty`
- `build/bin/test_bitcoin --run_test=util_tests/test_sanitize_string_printable_chars`
- `./build/src/secp256k1/bin/tests --target=ellswift_xdh_bad_scalar_tests --iterations=16`
- `./build/src/secp256k1/bin/tests --target=ellswift --iterations=16`

Functional tests:

- `python3 test/functional/feature_rdts.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_utxo_height.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_temporary_deployment.py --configfile build/test/config.ini`
- `python3 test/functional/feature_bip9_max_activation_height.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_accept.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_datacarrier.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_dust.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_subdust_fee_penalty.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini`
- `python3 test/functional/mining_coin_age_priority.py --configfile build/test/config.ini`
- `python3 test/functional/mining_prioritisetransaction.py --configfile build/test/config.ini`
- `python3 test/functional/feature_maxuploadtarget.py --configfile build/test/config.ini`
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini`
- `python3 test/functional/feature_help.py --configfile build/test/config.ini`
- `python3 test/functional/feature_includeconf.py --configfile build/test/config.ini`
- `python3 test/functional/feature_fee_estimates_persist.py --configfile build/test/config.ini`
- `python3 test/functional/feature_index_prune.py --configfile build/test/config.ini`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile build/test/config.ini`
- `python3 test/functional/feature_softwareexpiry.py --configfile build/test/config.ini`
- `python3 test/functional/feature_torcontrol.py --configfile build/test/config.ini`
- `python3 test/functional/interface_bitcoin_cli.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_help.py --configfile build/test/config.ini`
- `python3 test/functional/tool_cli_completion.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_signer.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getrpcwhitelist.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_bind.py --configfile build/test/config.ini`
- `python3 test/functional/interface_zmq.py --configfile /tmp/bitcoin-zmq-build/test/config.ini`
- `python3 test/functional/rpc_getblocklocations.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getgeneralinfo.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_sort_multisig.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_setban.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_rawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_packages.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_fee_histogram.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_mempool_info.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_dos_header_tree.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_block_times.py --configfile build/test/config.ini`
- `python3 test/functional/tool_wallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_multiwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_backup.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_migration.py --configfile build/test/config.ini`
  (skipped: previous releases not available or disabled)
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
- `python3 test/functional/wallet_dump.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_import_rescan.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/feature_rdts.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini`
  (fails on the inherited RDTS `ignore_rejects` internal-bug log described
  above)
