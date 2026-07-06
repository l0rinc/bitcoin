# Knots Port Audit

Audit date: 2026-07-06

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
- `feature_chain_tiebreaks.py` now restores current Core's stronger from-disk
  equal-work regression test. A stale Knots duplicate `test_chain_split_from_disk`
  method had been left later in the class, so Python executed the older two-way
  restart loop instead of Core's three-way post-restart extension test. This was
  a port-side test coverage defect, not an original Knots consensus bug, and is
  fixed as `5c6196c636`.
- `mempool_accept.py` was further adapted for Knots' data-output policy:
  Core's v30-era unbounded/multiple OP_RETURN expectations are now rejected by
  the port's `scriptpubkey`/`multi-op-return` policy paths (`2635a090c3`).
- The same test now covers Knots' `-spkreuse=0` inter-transaction policy
  conflict path (`2748eaf91f`, `49c780eae8`, `5847aee675`): duplicate outputs
  inside one transaction are rejected, while a higher-fee transaction claiming
  a scriptPubKey already claimed by an opt-in replaceable mempool transaction
  is handled through the replacement path.
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
- The `-minrelaymaturity` / `-minrelaycoinblocks` review found an original
  Knots local-configuration footgun. Actual Knots `29.x-knots` accepts
  negative values for both options; the coin-block threshold is stored as
  `uint64_t`, so `-minrelaycoinblocks=-1` becomes an effectively unreachable
  relay threshold. This is not consensus behavior or a remote trigger, but it
  can silently disable local relay/mining acceptance for an operator. The port
  now rejects negative values as `8c192ed1c8`, with unit and functional
  startup coverage.
- The same low-memory review confirmed the underlying Knots dbcache flush
  pressure series is present in the port and absent from current Core:
  `SystemNeedsMemoryReleased()` checks Windows and Linux available-memory
  signals, `FlushStateToDisk(IF_NEEDED)` treats memory pressure like a critical
  coins-cache condition, the threshold is operator-configurable through
  `-lowmem=<MiB>`, and deterministic tests can disable it by setting the
  threshold to zero. This is local availability/resource hardening, not
  consensus behavior or a remote trigger. Focused `feature_init.py`
  `init_lowmem_test` runs pass against both the port and unmodified Knots. The
  port now adds direct unit coverage for the threshold predicate, including
  saturated `free+buffer` accounting, and for `FlushStateToDisk(IF_NEEDED)`
  emptying a small coins cache only when the memory-pressure probe fires
  (`d2db337c78`).
- The `GetArg` / `GetBoolArg` numeric settings review confirmed Knots'
  `577c04c80e` is present in the port and still absent from current Core
  master. Core's `SettingToBool(...)` still falls through to `value.get_str()`
  for non-bool values, so numeric JSON values in `settings.json` throw where
  string values are accepted. Knots and the port switch on `UniValue` type and
  call `getValStr()` for both strings and numbers. This is local configuration
  robustness, not consensus behavior, and is already pinned by
  `getarg_tests/setting_args`.
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
  made the wallet-restriction assertions in `rpc_users.py` pass dummy
  filepaths and already-loaded wallet names, so the RPCs reach Knots'
  `EnsureNotWalletRestricted` / wallet-name guards instead of generic
  argument-count or file-access paths.
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
  The crash is fixed as `6cd89c9b09` and covered in
  `wallet_createwallet.py`, but the later `wallet_undeprecate_legacy-29`
  source review confirmed a remaining port-completeness gap: actual Knots
  source permits `descriptors=false` when compiled with BDB, while this
  current-Core port still forces `WALLET_FLAG_DESCRIPTORS` and rejects legacy
  creation before any BDB-backed path can run. This is wallet compatibility,
  not consensus behavior or a remote security issue.
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
  past. The same RPC compatibility pass confirmed Knots' legacy `nodeid`
  named-argument alias for `peer_id` (`ef1c43e51d`) is present in the port and
  absent from current Core, including bitcoin-cli's named-argument JSON
  conversion entry. This is CLI/RPC compatibility for older Knots callers, not
  consensus behavior or network hardening; `rpc_getblockfrompeer.py` now covers
  both the server-side `nodeid` alias and the `bitcoin-cli -named` conversion.
- The script verification thread-control review found a port omission in
  Knots' `scriptthreadsinfo` / `setscriptthreadsenabled` RPC surface
  (`daccde46e4` plus doc/fuzz follow-ups): the RPCs were registered and
  reported disabled state, but the port's current-Core `ConnectBlock`
  adaptation still created `CCheckQueueControl` whenever worker threads existed.
  Actual Knots gates parallel script checks on
  `m_script_check_queue_enabled`; the port now does the same. This is runtime
  operator CPU-control behavior, not a consensus-rule change. `rpc_blockchain.py`
  now covers the enable/disable RPC surface.
- The same script-thread-control surface exposed a port-only error-reporting
  mismatch after Knots' `be0857745a` single-script-check backport. Current Core
  reports block script failures as `block-script-verify-flag-failed`, and the
  port's parallel script-check path already did the same, but the inline path
  reached when `setscriptthreadsenabled(false)` was active still returned the
  older Knots/Core `mandatory-script-verify-flag-failed` label. This was not a
  consensus-rule difference or an original Knots defect; the port now aligns the
  inline label with current Core as `2feca940f4`, and `feature_dersig.py`
  disables script threads to cover the branch.
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
  (`6fe0c50345`). A later source check also confirmed the port has Knots/Core
  Tor-control PoW enablement and fallback (`6df8bf7673`, `d4e12697e2`,
  `fca97c11b3`), excessive-line OOM defense (`4110843327`), completed-line
  length enforcement and reconnect (`3eb50531a5`, `0d747bbc55`), and empty
  `AUTHCHALLENGE` logging (`ce574bd1e6`). The port now also adds direct
  `system_tests` coverage for the underlying subprocess behavior by opening a
  parent file descriptor, proving a mock child inherits it when `close_fds` is
  off, and proving it is closed when `close_fds` is on.
- The per-network proxy review confirmed Knots' `-proxy=...=tor` alias is
  present in the port and actual Knots while current Core's parser still
  accepts only `onion` despite help text that advertises `tor`. Rerunning
  `feature_proxy.py` exposed a port-side test drift: the test used
  `p2p_port(...)` for Knots' common Tor/local-port warning assertion but had
  lost the helper import during the rebase. Actual Knots' test already imports
  it, so this was not an original Knots defect. The port restores the import,
  and the full proxy functional test now passes against both the port and an
  unmodified Knots build.
- The I2P SAM redaction review confirmed Knots/Core's `SESSION CREATE` private
  key redaction (`6f9467ff97`) is present in the port. The port now adds
  `i2p_tests/session_create_error_redacts_private_key` (`0bd1f283ae`) to force
  a `SESSION CREATE` failure after loading a known private key and assert the
  resulting log/error path contains `SESSION CREATE ...` without the encoded
  private key or `DESTINATION=` request body.
- The runtime notification review confirmed Knots' support for multiple
  `-startupnotify`, `-blocknotify`, `-alertnotify`, and `-walletnotify`
  commands (`f1e300838a`) is present in the port and absent from current Core.
  This is operator-facing behavior rather than a security or consensus change.
  The port now extends `feature_startupnotify.py` and
  `feature_notifications.py` as `a9ddf9043e` so startup, block, alert, and
  wallet notifications each exercise multiple configured commands. A follow-up
  check also confirmed the port and actual Knots carry `99bd4320f8`'s
  `-alertnotify` double-quote substitution for Windows `cmd.exe`, while current
  Core still substitutes the sanitized alert text inside single quotes.
- Knots-added RPC coverage for `getblocklocations`, `getgeneralinfo`, and
  BIP67 multisig sorting now passes on the port. The `getblocklocations`
  review confirmed the port carries Knots' locking, help/example,
  result-schema, and fuzz-registration fixes (`91c9e14639`, `32c2b6e326`,
  `e56f5e7255`, `e1a10af807`). Current Core master has no
  `getblocklocations` RPC, so these are Knots-only operator/debugging RPC
  fixes rather than Core-missing hardening. `rpc_sort_multisig.py` had dropped
  the original Knots `assert_raises_rpc_error` import during the rebase; the
  port restores that test helper import as `dcf97bd63b`.
- The `getblock` / `getrawtransaction` fixup review confirmed Knots'
  user-facing RPC help and result-documentation cleanups (`20130089e3`,
  `8765a4ce0e`, `35cdcb3309`) are present after adapting them to the current
  shared `RPCMethod`/`TxDoc` helpers and the port-only `coinbase_tx` and
  `confirmations_assumed` fields. This is RPC documentation and result-shape
  metadata, not consensus behavior; `rpc_blockchain.py` and
  `rpc_rawtransaction.py` pass with the adapted metadata.
- The `scanblocks` status review confirmed Knots' in-progress
  `relevant_blocks` reporting (`4c9dc4bbe6`) is present in the port and absent
  from current Core master. The review also exposed a port-introduced bug:
  rebasing the Knots `scanblocks` implementation onto the current `RPCMethod`
  helper dropped the final invalid-action `else`, so `scanblocks "foobar"`
  returned an empty object and triggered the RPC result checker's internal-bug
  error instead of `Invalid action 'foobar'`. Actual Knots still has the
  invalid-action throw, and the strengthened `rpc_scanblocks.py` passes against
  unmodified Knots. The port restores the throw and now also covers status-time
  `relevant_blocks` by running a concurrent block-filter scan and checking
  `scanblocks "status"` before aborting the scan.
- Mining priority coverage now passes against Knots' policy defaults. The
  coin-age priority test uses a cache-bypassing `getblocktemplate` request when
  checking immediate `prioritisetransaction` effects, and the shared large
  transaction helper now appends standard P2WSH outputs instead of many
  OP_RETURN outputs so it remains valid under Knots' one-data-output policy
  (`dc0ed81797`). `mining_coin_age_priority.py`,
  `mining_prioritisetransaction.py`, and `feature_maxuploadtarget.py` pass with
  this helper.
- Knots' reintroduced checkpoint enforcement (`75b826c729`, port
  `2dd00e5000`) is a Core-missing validation hardening rather than a
  port-introduced behavior: it rejects a header whose height exactly matches a
  configured checkpoint but whose hash differs, even when the node has not
  first accepted the honest chain through that checkpoint. `p2p_dos_header_tree`
  covers both the older-than-last-known-checkpoint rejection and this
  height-specific `checkpoint-mismatch` branch, and passes against unmodified
  Knots. The same review exposed a port omission in the supporting testnet3
  checkpoint data: original Knots retains the testnet3 checkpoint at height 546,
  while the port only had mainnet checkpoint data. This was not an original
  Knots bug and is now restored as `0dbc321ca0`, along with the Knots testnet3
  header fixture and test framework network magic needed by
  `p2p_dos_header_tree.py`. A later checkpoint-data pass also confirmed the
  port carries Knots' updated mainnet checkpoint table (`bb1949adc6`),
  including the final checkpoint at height `908765` with hash
  `00000000000000000001b64acb5fe4b40b84092159b6406a6244f46a37fa6c6b`;
  current Core master has no equivalent checkpoint table. This remains local
  validation hardening, not a consensus rule difference. `checkpoint_sanity`
  now pins the latest mainnet checkpoint height and hash.
- A high-risk review of exact patch-id misses found Knots' codex32 early-return
  fix (`126d6df18d`) was missing from the port. Actual Knots `29.x-knots`
  already contains the fix, so this was a port omission rather than an original
  Knots defect. Without the return, share derivation could continue after
  setting `MISMATCH_K`, `MISMATCH_ID`, `MISMATCH_LENGTH`, or `DUPLICATE_SHARE`;
  when an earlier share was longer than a later share, the interpolation loop
  could read past the later share's data. Current Core master has no codex32
  implementation, so this is Knots-only wallet/import hardening rather than a
  Core shortcoming. The port now matches Knots as `65c72eb817`, with
  `codex32_tests` and `wallet_importseed.py` coverage for valid decoded shares
  with inconsistent lengths.
- The same exact-patch review checked Knots' `-consensusrules` / RDTS consent
  commits (`fbfa7482c6`, `e60eea4260`). The port carries the behavior in an
  adapted form: unknown rule names fail startup, `rdts` is accepted, and mainnet
  consent handling is skipped on test chains. `feature_config_args.py` now covers
  the parser/startup surface directly as `31f7b8f005`.
- A follow-up RDTS consent audit confirmed two later Knots build-time consent
  modes are present in the port. `RDTS_CONSENT=RUNTIME_WARN` (`dca6d162e1`,
  ported as `ba57ec2c31`) lets daemon builds enforce RDTS while warning hourly
  if the operator has not configured `consensusrules=rdts`; the GUI coerces
  this mode back to runtime-check because the user is shown an explicit
  consent/exit prompt. `RDTS_CONSENT=UNSUPPORTED_UNSAFE_NO_ENFORCEMENT`
  (`2f0e5090a7`, ported as `603f4958d0`) is consensus-significant: it is an
  explicit unsupported build mode that disables the reduced-data deployment and
  clears `NODE_REDUCED_DATA` unless `-consensusrules=rdts` is supplied. A
  daemon-only probe build verified both runtime surfaces: without
  `-consensusrules=rdts`, `getnetworkinfo.localservicesnames` omits
  `REDUCED_DATA?` and RPC warnings say RDTS is not enabled; with
  `-consensusrules=rdts`, the same binary advertises `REDUCED_DATA?` and the
  RDTS-disabled warning is absent. This is not a Core shortcoming; it is a
  Knots-specific consensus escape hatch that should be called out explicitly.
- The mainnet policy-argument review confirmed Knots' opt-in
  `-acceptnonstdtxn` relaxation (`2e2f48f871`) is present in the port and
  absent from current Core. Current Core still treats the option as
  test-network/debug-only and rejects it on main chain; Knots and this port
  allow authenticated operators to disable standardness policy on mainnet while
  keeping the default unchanged. This is mempool/mining policy behavior, not a
  consensus-rule change, but it is a high-visibility Knots-vs-Core divergence.
  `feature_config_args.py` now covers main-chain startup with
  `acceptnonstdtxn=1`.
- The pruning-argument review confirmed Knots' `-pruneduringinit=0` fix
  (`63ec908ea6`) is present in the port. Actual Knots converts `0` to manual
  pruning during init instead of treating it as disabled pruning, and the port
  carries the same `PRUNE_TARGET_MANUAL` adaptation through current Core's
  block-manager argument parser. Current Core master has no `-pruneduringinit`
  option. `blockmanager_tests/blockmanager_args_prune_during_init` covers the
  `0`, `1`, minimum-target, disable, and invalid-value cases.
- The corrupt-data startup recovery review confirmed Knots' `-reindex=auto`
  feature (`35698ab7f4`) is present in the port and absent from current Core.
  Knots and this port treat `-reindex=auto` as false for the initial
  `GetBoolArg("-reindex", false)` check, then if chainstate loading fails they
  bypass the GUI/user prompt and retry once with full reindexing. This is local
  availability/recovery behavior for corrupted block-index or chainstate data,
  not consensus behavior or a remote trigger. `feature_init.py` now deletes a
  block-index LevelDB file and verifies that `-reindex=auto` logs
  `Automatically running a reindex.` and restarts successfully at the original
  height; the same focused method passes on both the port and unmodified Knots.
- The first-startup disk-space warning review confirmed Knots'
  `4f06564f36` is present in the port and absent from current Core. Knots and
  this port treat `AssumedBlockchainSize()` as decimal GB, matching the
  user-facing `GB` wording and Qt's `GB_BYTES` constant, and use the same
  decimal divisor when reporting the warning. Current Core still checks
  `AssumedBlockchainSize() * 1_GiB` but reports the raw decimal-size chainparam
  value, so the warning threshold is about 7.4% higher than the displayed
  estimate. This is local operator warning/resource-estimation correctness, not
  consensus behavior or a remote security issue.
- The chain-parameter sync review checked Knots' later
  `defaultAssumeValid`/`nMinimumChainWork` update (`cb074d52a87`) and its
  historical testnet4 assumeutxo addition (`80f8f9a38b`). Current Core and this
  port are already ahead of Knots `29.x-knots`: mainnet, testnet3, testnet4,
  and signet use newer `nMinimumChainWork` and `defaultAssumeValid` values,
  and testnet4 has assumeutxo entries at heights `90'000` and `120'000`.
  Current unmodified Knots still has older values, and its current testnet4
  assumeutxo table is empty because the historical height-90000 addition was
  later reverted by `f1cc17e0f0` (`Revert test chain parameter changes etc`).
  This is not an original Knots consensus bug: `nMinimumChainWork`,
  `defaultAssumeValid`, and assumeutxo snapshot metadata affect local sync
  gating and snapshot acceptance, not block validity. The port intentionally
  keeps current Core's newer data, and `chainparams_tests` now pins these
  values so a future rebase cannot silently downgrade them to older Knots
  parameters.
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
- The shutdown/wait RPC review confirmed Knots' retained
  `m_tip_block_cv` wake-up from `node.shutdown_request` (`d2c1bd10db`, ported
  as `1ba5009294`) is present in the port and still absent from current Core's
  direct `node.shutdown_request` path. Core already wakes waiters from
  `Interrupt()`, but Knots and the port also cover GUI-triggered shutdowns that
  call `shutdown_request` directly. This is RPC/shutdown hardening, not
  consensus. Rerunning the strengthened port `feature_shutdown.py` exposed a
  port-side test starvation bug: the framework defaults to `rpcthreads=2`, and
  the test occupied both workers with `waitfornewblock` and
  `waitforblockheight` before polling `getrpcinfo`. Actual Knots' own shutdown
  test starts only one long call and passes. The port test now sets
  `-rpcthreads=4` as `29e4c5fb39`.
- The exact-patch review found Knots' actionable pruned-index startup error
  (`4a4e4e253e`) was still missing after adapting onto the port's newer
  two-stage block/undo-data availability check. Actual Knots already contains
  the fix, so this was a port omission, not an original Knots defect. The port
  now gives index-specific disable instructions for missing pruned block data
  and missing pruned undo data (`881949b28d`), and `feature_index_prune.py`
  asserts the new block-filter and coinstatsindex messages.
- The prune-lock review confirmed the port has Knots' public prune-lock RPCs
  and persistence support (`0b4bd4e134`, `4822c21812`,
  `1d1c92d40d`, `162f0dba2f`), adapted to current Core's void-returning
  `CDBWrapper` write API. Current Core still has only internal index prune
  locks, no `listprunelocks`/`setprunelock` RPCs, and no persisted
  `DB_PRUNE_LOCK` entries. The port now has a focused `rpc_prunelocks.py`
  functional test covering the `temporary` field, restart persistence,
  temporary non-persistence, persistent-to-temporary disk erasure, delete-all,
  and invalid wildcard usage.
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
- The UTXO LevelDB batch-size review found a port omission from adapting
  Knots' `2104df3209` onto current Core's later `DEFAULT_DB_CACHE_BATCH`
  location. Actual Knots defaults `-dbbatchsize` to 64 MiB, while current Core
  now defaults it to 32 MiB. The port had a stale `nDefaultDbBatchSize = 64 <<
  20` in `txdb.h`, but the live `CoinsViewOptions` default and help text still
  used Core's `DEFAULT_DB_CACHE_BATCH{32_MiB}`. This was not an original Knots
  defect. The port now sets `DEFAULT_DB_CACHE_BATCH{64_MiB}` in
  `kernel/caches.h`, removes the unused stale constant, and pins the live
  default in `caches_tests/default_db_batch_size`.
- The LevelDB exact-patch review found Knots' embedded-LevelDB sanity-check
  guard (`a4fc0050f1`) was missing. On this current Core base the embedded
  LevelDB fork still exposes the runtime version getters, so this was not a
  reproducible build failure or original Knots defect. The port now still
  matches Knots' intended build distinction as `7d4c61ea3a`: system-LevelDB
  builds keep the header/runtime version check, while embedded-LevelDB builds
  define `EMBEDDED_LEVELDB` and skip the redundant runtime comparison.
- The same sanity-check review confirmed Knots' LLVM 96267 compiler
  optimization check (`8fbdf93878`, port `25a78ddab0`) is present in the port
  and absent from current Core. This is a startup/build-environment safety check
  rather than consensus behavior: if the compiler misoptimizes the reproducer,
  `SanityChecks()` aborts startup with `Compiler optimization sanity check
  failure`. The port's `sanity_tests` covers both the LevelDB sanity check and
  the LLVM reproducer. The unmodified Knots source carries the same check and
  test, but its `build-repro` directory only contains `bitcoind` and
  `bitcoin-cli`, so there was no Knots `test_bitcoin` binary to run directly.
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
- The GUI review found Knots' RPC console `/clearhistory` command
  (`aafe2f8d9`) and status-bar sync progress expansion (`c3aa589495`) were
  still missing from the port. These are user-facing Qt behavior changes, not
  consensus or network security changes. The port now carries `/clearhistory`
  as `e1445679c3`, the expanded progress bar as `7db09de6d9`, and Qt test
  coverage for the clear-history command as `2e0ff4068b`. The local build still
  has `BUILD_GUI=OFF`, so the Qt test target could not be executed here.
- The same Qt source pass found port-introduced GUI compile drift, not original
  Knots defects: `src/qt/transactionfilterproxy.cpp` had duplicated naked
  filter-change fragments after the current-Core Qt 6.10 modernization, and
  `src/qt/rpcconsole.h` had two identical `PlainCopyTextEdit` class
  definitions. Actual Knots `29.x-knots` has only one `PlainCopyTextEdit` and
  does not have the current-Core transaction-filter fragment duplication. The
  port now fixes these as `e94cbbbe9c` and `9d032f4b21`.
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
- Knots' post-IBD chainstate disk-sync hardening (`2f1bf089f7`) is present in
  the port and absent from current Core: once IBD is over, the scheduler waits
  for stable height and no validated block downloads, then calls
  `CoinsTip().Sync()` to reduce data-loss exposure after initial sync. This is
  not consensus behavior, but it is a storage-safety divergence. The port's
  adapted `feature_sync_coins_tip_after_chain_sync.py` uses the current
  `create_block(..., ntime=...)` and P2P send helpers (`0216a33c43`), and the
  coverage passes against both the port and unmodified Knots.
- The P2P block-read hash-check audit of Knots `15805060ec` did not reveal a
  current Core shortcoming: current Core master, actual Knots, and this port all
  pass the requested `inv.hash`/`req.blockhash` into `BlockManager::ReadBlock`
  before serving non-witness blocks or `getblocktxn` responses. The port keeps
  the same behavior while adapting the call sites to its low-priority block-file
  read API. `blockmanager_tests` now also asserts the raw-`FlatFilePos`
  `expected_hash` path that these P2P call sites rely on.
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
  `a6db4bde2f`,
  `5fa31318bd`, `f32c5267ae`, `fa9048636b`, `b7e42cd6b9`, `ef728f7963`,
  `a5375f9c39`, `d07851730b`, `0e79f97800`) is present in the port and absent
  from current Core's wallet/utility RPC behavior. The port matches actual
  Knots by verifying Electrum/BIP137/BIP322 signatures for SegWit addresses,
  supporting BIP322 simple signatures for wallet-owned non-P2PKH destinations,
  preferring legacy message signatures for P2PKH `signmessage`, and throwing
  for inconclusive proof-of-funds or unsupported script cases. This is
  wallet/RPC signature functionality, not transaction or block consensus. The
  signature-checker `require_sighash_all` flag is only enabled by this message
  verifier path, where it rejects otherwise-valid BIP322 proofs signed with
  non-`SIGHASH_ALL` hash types. `script_tests/require_sighash_all` covers the
  low-level ECDSA invariant, and `wallet_signmessagewithaddress.py` covers
  wallet-created BIP322 signatures for native SegWit and Taproot addresses in
  addition to the
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
- The `bitcoin-wallet importfromcoldcard` review confirmed the Knots-only
  command (`e3d45ed809`, `0b7664c038`, `f72496a06a`) is present in the port and
  absent from current Core. Malformed Coldcard export files exposed an original
  Knots crash: both the port and an actual `../knots` `29.x-knots`
  `bitcoin-wallet` build aborted with exit 134 and `std::out_of_range` when no
  `importdescriptors '[{...}]'` line was found. The port now keeps the command
  but fails cleanly before creating a wallet (`730764bc9f`). This is a local
  wallet-tool crash/DoS in a Knots-only command, not a consensus or network
  issue and not a Core-missing covert fix. Coverage in `tool_wallet.py` now
  checks missing, nonexistent, and malformed dump files plus a successful
  Coldcard-style descriptor import. Verification run: `cmake --build build
  --target bitcoin-wallet -j4`, `python3 test/functional/tool_wallet.py
  --configfile build/test/config.ini --test_methods test_importfromcoldcard
  --tmpdir=/mnt/my_storage/tmp_tool_wallet_coldcard`, and the default
  `tool_wallet.py` run with `--tmpdir=/mnt/my_storage/tmp_tool_wallet_full`.
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
- The wallet-tool dump/createfromdump cleanup review checked Knots'
  `cc324aa2be` failed-restore cleanup and `afd2785f2c` BDB wallet-id warnings.
  Current Core master already avoids the old `fs::remove_all(wallet_path)`
  cleanup by removing only `wallet->GetDatabase().Files()` plus the named
  wallet path, so this is not Core-missing data-loss hardening. Knots and the
  port additionally keep unnamed `createfromdump` compatibility and warn that
  BDB dumps/restores do not preserve the wallet id; that warning is tied to
  Knots' retained BDB backend. A fresh `tool_wallet.py` run passes with the
  ported behavior.
- The BDB read-only parser review confirmed Knots' last-page LSN check
  (`2069130d80`) is present in the port and already present in current Core
  master: the parser scans through `outer_meta.last_page` inclusively before
  accepting a flushed Berkeley DB file. This was not introduced by the port and
  was not a remaining Core-missing fix, but it is now pinned by
  `db_tests/berkeley_ro_checks_final_page_lsn` with a minimal hand-built BDB
  fixture whose final page has a dirty LSN.
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
- The wallet metadata compatibility review confirmed Knots'
  `5321a6a55a` is present in the port and absent from current Core master:
  version-2 `CKeyMetadata` records contain one unsupported key-origin flag byte
  between create time and later HD metadata, and Knots preserves that byte even
  though the flag semantics are no longer supported. Current Core still
  deserializes only version/create-time for this legacy version and would leave
  the byte unread/drop it on rewrite. This is old wallet metadata preservation,
  not consensus behavior. The port now pins it with
  `walletdb_tests/key_metadata_preserves_unsupported_flags`.
- A follow-up wallet directory review confirmed Knots' node-data skip list
  (`283cd1f065`) is present in the port and absent from current Core master.
  Current Core has the later iterator error-handling shape, but its
  `ListDatabases(...)` still recurses through `blocks`, `chainstate`,
  `indexes`, and network subdirectories when `-walletdir` points at the chain
  datadir. The port matches actual Knots by disabling recursion into those
  top-level node data directories. `wallet_multiwallet.py` now writes a fake
  SQLite wallet marker under an ordinary directory and another under
  `blocks/`; `listwalletdir` lists only the ordinary marker.
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
  The port also carries Knots' legacy-balance conflict and `findBlock`
  hardening (`14bf4726c4`, `cf5ec99cb0`): conflicting unconfirmed wallet
  transactions are included in `getbalance("*", 0)` only when they are in the
  node's mempool, and the legacy locktime/MTP lookup uses `CHECK_NONFATAL`
  instead of `Assume(...)`. Current Core master removed `GetLegacyBalance`, so
  these are Knots compatibility fixes rather than remaining Core-missing
  hardening.
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
- The wallet confirmation-target review confirmed Knots' one-day default
  confirmation target (`a8b4eb70c0`) is present in the port and absent from
  current Core: `DEFAULT_TX_CONFIRM_TARGET` is `144` in Knots and this port,
  while current Core still defaults to `6`. This is a quiet wallet fee-policy
  default change, not consensus behavior or a covert security hardening. The
  port now pins the constructed wallet default in
  `wallet_tests/default_confirm_target_is_one_day`; port and unmodified Knots
  `bitcoind -help-debug` both advertise `-txconfirmtarget` default `144`.
- Knots' address-usage tracking series (`fc7954a148`, `022887d933`,
  `a00bc6f395`, `82908e28a5`) is present in the port and absent from current
  Core: wallets keep in-memory script-use metadata and `getaddressinfo` returns
  `use_txids` for wallet transactions that received to the queried address.
  `wallet_basic.py` covers empty and reused-address `use_txids`, and a manual
  unmodified-Knots RPC check under
  `/mnt/my_storage/tmp_knots_use_txids_manual` confirmed that a mined-to wallet
  address reports a non-empty `use_txids` array. While rerunning this area, the
  test exposed a port-only integration bug from carrying Knots' older generic
  wallet chain-limit error across current Core's cluster-mempool interface:
  `-walletrejectlongchains` returned `mempool policy limits exceeded` instead
  of Core's detailed `too many unconfirmed transactions in cluster`. Actual
  Knots cannot reproduce that exact path because it lacks the current
  `-limitclustercount` option. The port now restores the detailed current-Core
  rejection string.
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
  PSBT behavior, and actual Knots' own `rpc_psbt.py` now passes against the
  local unmodified Knots build as a same-repo cross-check.
- The raw-transaction PSBT review confirmed Knots' user-provided previous
  transaction support for `utxoupdatepsbt` and `descriptorprocesspsbt`
  (`bdb4ca4195`, `eea8588f07`) is present in the port and absent from current
  Core master. The port follows the Knots follow-up that exposes
  `descriptorprocesspsbt` `prevtxs` through the options/named-parameter path,
  not a sixth positional argument. `rpc_psbt.py` covers filling and signing a
  child PSBT from provided parent transaction hex, irrelevant prevtxs being a
  no-op, duplicate txid rejection, and too-few-outputs rejection. The latest
  full `rpc_psbt.py` reruns against both the port and actual Knots exercised
  these assertions before the funded-PSBT locktime checks.
- The same PSBT review confirmed Knots' options-object compatibility for
  `walletprocesspsbt` (`e5160731d2`) and `descriptorprocesspsbt`
  (`a22be508da`) is present in the port and absent from current Core. Both RPCs
  still accept the older positional arguments, while named/options calls can set
  `sighashtype`, `bip32derivs`, and `finalize`; `walletprocesspsbt` also
  accepts `sign` in that object. This is RPC client compatibility, not
  consensus behavior. `rpc_psbt.py` covers equality between positional and
  options-object calls for both RPCs.
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
  compatibility behavior, not consensus. Current Core master still chooses
  bech32m change whenever a bech32m internal descriptor exists and any recipient
  is Taproot, even when the wallet default is only bech32. The new
  `spend_tests/change_type_avoids_newer_default` unit pins the helper directly:
  default bech32m selects bech32m change for a Taproot recipient, default bech32
  keeps bech32 change, and an explicit bech32m change type still overrides. The
  existing `wallet_address_types.py` coverage also exercises p2sh-segwit-default
  and bech32-default wallets sending to a bech32m recipient without upgrading
  change past the user's preferred type, and that functional check passes on
  unmodified Knots.
- The wallet/IPC shutdown review confirmed Knots/Core's validation-interface
  drain on wallet notification disconnect (`41d1ce75e3`) and chain-client
  `ipc::Exception` handling for disconnected `bitcoin-wallet` children
  (`a9f11b76fb`) are present in the port. Wallet unload and failed
  chain-attach paths call `CWallet::DisconnectChainNotifications()`, which
  disconnects the handler and waits for pending notifications, and `Shutdown()`
  catches IPC disconnect exceptions while stopping chain clients. This is
  shutdown robustness rather than consensus or wallet-balance behavior.
- Raw transaction, package, and PSBT RPC coverage now passes against the port.
  This covers Knots-touched max burn handling, package max fee/burn arguments,
  and PSBT base64 parameter handling with `=` padding characters.
- Knots' `signrawtransaction*` fee-result extension (`83c29e9702`,
  `6ea56f2c1a`) is present in the port and absent from current Core. When all
  signed inputs expose known amounts and witness data, the RPC result includes
  `fee` as input amounts minus outputs. Existing `feature_segwit.py` covers the
  wallet RPC path; the port now also asserts the deterministic
  `signrawtransactionwithkey` P2SH-P2WSH fee result, and the strengthened test
  passes against unmodified Knots. This is wallet/RPC metadata, not consensus
  behavior.
- Wallet anchor verification exposed a port-introduced spendability divergence:
  `wallet_anchor.py` passed on unmodified Knots but failed in the port because
  `AvailableCoins(...)` treated `ISMINE_SPENDABLE` outputs in
  disabled-private-key wallets as spendable only when watch-only spending was
  allowed and the output was solvable. Actual Knots keeps spendable outputs
  selectable regardless of the disabled-private-key flag, while watch-only
  outputs still require `fAllowWatchOnly` and solvability. The port now matches
  Knots, so anchor outputs reach the intended unsolvable-size error path rather
  than being filtered out as generic insufficient funds. This is wallet/RPC
  behavior and zero-value/anchor accounting, not consensus behavior or an
  original Knots defect. `wallet_anchor.py`, `wallet_listtransactions.py`, and
  `wallet_tests` pass after the fix; the same `wallet_anchor.py` run passes on
  unmodified Knots.
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
  functional test locally; a fresh full rerun against both the port and
  unmodified Knots reaches the reorg section and passes. The port now matches
  Knots' behavior.
- `p2p_invalid_tx.py` and `p2p_segwit.py` now match the port's current
  validation surfaces: orphanage overflow is still capped at 100 stored
  orphans, but the Core-current log string is `orphanage count limit`, and
  SegWit block failures use current Core's `block-script-verify-flag-failed`
  reject reasons while mempool failures use `mempool-script-verify-flag-failed`.
  A follow-up `p2p_segwit.py` run caught stale mandatory-script block-label
  expectations from the older Knots test baseline; this was a port-side test
  mismatch, not an original Knots consensus bug. The same check confirmed
  Knots' witness-stripping retry coverage is already present in current Core,
  actual Knots, and the port. The SegWit test also restores hash refreshes
  needed by the current mutable transaction helpers.
- The follow-up cherry audit found Knots' `BufferedFile` close-on-destruction
  fix (`88fe778d9d`) only half-applied in the port: the fuzz test expected
  `BufferedFile::fclose()`, but `BufferedFile` still lacked the method and
  destructor. The port now restores both and adds `streams_tests` coverage that
  the wrapped `AutoFile` is closed when `BufferedFile` is destroyed. The same
  pass removed a duplicate `cleanSubVer` assignment left after applying Knots'
  version-message ordering fix.
- The later `BufferedFile` page-cache advice commit (`97130ac516`) was also
  only half present in the port. The `AdviseSequential()` and
  `CloseAndUncache()` helpers existed in `util/fs_helpers`, but the rebased
  `BufferedFile` constructor and close path did not call them. Actual Knots
  does call `AdviseSequential()` when wrapping the source file and
  `CloseAndUncache()` on close. The port now matches Knots and extends
  `streams_buffered_file_closes_source` to cover both destructor close and
  explicit close/idempotence. This was a port omission, not an original Knots
  defect. Current Core lacks this sequential-read/page-cache-drop behavior.
- The version-message ordering review confirmed Knots'
  `df874f848a` is present in the port as `9cb0591f30`: the peer's sanitized
  `cleanSubVer` is stored under `m_subver_mutex` before `nVersion` is published
  as nonzero. Current Core master still assigns `nVersion` first, leaving a
  narrow window where readers that use nonzero `nVersion` as the handshake
  marker can observe an empty `cleanSubVer`. This is peer-display/RPC state
  hardening, not consensus behavior. Existing `p2p_handshake.py` coverage
  checks the visible user-agent preservation path; the ordering itself is
  verified by source comparison because the race is not deterministic enough
  for a reliable functional assertion.
- The same follow-up confirmed Knots' raw transaction max-feerate accounting
  fix (`4b3cc3d48e`, `1cee5b1ac7`, `335d928d96`) is present in the port:
  `sendrawtransaction` passes a `CFeeRate` into `BroadcastTransaction`, which
  converts it to an absolute fee using the mempool accept result's adjusted
  `m_vsize`. Current Core still precomputes the absolute maximum from plain
  `GetVirtualTransactionSize(*tx)`, so it can disagree with policy-adjusted
  vsize when `-bytespersigop` dominates. `mempool_sigoplimit.py` now covers
  this by requiring `testmempoolaccept` and `sendrawtransaction` to make the
  same max-feerate decision for a high-sigop, low-weight P2WSH spend. The
  focused max-feerate subtest also passes directly against unmodified Knots;
  the full cross-run hits an unrelated package-error string drift before that
  subtest.
  A later focused rerun exposed only a port-side test-method isolation issue:
  the subtest assumed `run_test()` had already created `self.wallet`. The port
  now initializes the MiniWallet lazily as `e14bfdfd03`, so the documented
  focused command exercises the intended path directly.
- The RPC authentication follow-up confirmed Knots' blank `-rpcauth`,
  `-rpcauthfile`, `-norpcauth`, wallet-restricted rpcauth, and cookie
  permission/replacement hardening is present in the port. Current Core master
  still lacks the `-rpcauthfile` and per-token blank `-rpcauth` behavior. The
  port's `rpc_users.py` coverage exercises multi-entry auth files, blank
  `-rpcauth` around nonblank entries, `-norpcauth`, wallet-restricted auth
  entries, and cookie permission/replacement behavior. A later cookie-permission
  compatibility check confirmed Knots' octal `-rpccookieperms` parser and
  legacy control values (`0`, empty/`1`, and `-norpccookieperms`) are present in
  the port and absent from current Core, which still accepts only `owner`,
  `group`, or `all`. This is local RPC-auth file compatibility and permission
  control, not consensus behavior or remote network exposure. The lingering test
  `FIXME` about reviving an earlier command-line `-rpcauth=*` after
  `-norpcauth` is inherited from actual Knots and was not introduced by the
  port; the test asserts the current Knots behavior around config-file auth
  restoration.
- The JSON-RPC compatibility review confirmed Knots' nonstandard-version
  tolerance (`6cada87972`) is present in the port and absent from current Core:
  only exact string `"2.0"` selects JSON-RPC 2.0 response semantics, while
  numeric `jsonrpc` values and unknown strings such as `"3.0"` are treated as
  legacy JSON-RPC 1.x requests. Current Core rejects those malformed markers
  with HTTP 400 / `RPC_INVALID_REQUEST`. This is RPC client compatibility, not
  consensus behavior or an auth bypass. `interface_rpc.py` covers numeric
  `"1.0"` compatibility, unknown-version batch requests, and direct numeric or
  unknown `jsonrpc` HTTP requests; the same test passes on unmodified Knots.
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
  type cannot be manually selected through `addnode onetry`. The same
  coverage now exercises Knots' old positional `addnode ... onetry
  <connection_type>` compatibility slot and confirms the port rejects
  `inbound` there before reaching `OpenNetworkConnection`.
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
  Follow-up review of the temporary Knots/Core cap
  (`2b3e19e9e8`) and Knots' later removal (`a2225405a5`) confirmed final
  Knots intentionally has no 8-peer cap on forced inbound replacements. The
  port matches that behavior, and `p2p_eviction.py` now includes a dedicated
  `test_forceinbound_nocap` method that creates nine forced inbound peers
  while preserving the stronger default permission-reporting assertions for
  the port.
- The `getpeerinfo` metadata review confirmed Knots' `last_block_announcement`
  (`a4e4e17feb`) and optional per-peer `cpu_load` (`9084ea96ac`) fields are
  present in the port and absent from current Core. These are diagnostic RPC
  fields, not policy or consensus behavior; `rpc_net.py` accounts for both
  fields, including the fact that `cpu_load` is absent before the version
  handshake completes.
- The `getpeerinfo` misbehavior-score compatibility review confirmed Knots'
  restored deprecated `misbehavior_score` field is present in the port and
  absent from current Core. Knots and the port report `0` normally and `100`
  only when the peer's internal `m_should_discourage` flag is set, and the RPC
  help marks the field deprecated. This is RPC compatibility/diagnostics, not
  consensus behavior, peer punishment policy, or a covert hardening change.
  The existing `rpc_net.py` no-version-peer expectation asserts the field on
  both the port and unmodified Knots.
- The onion-inbound whitelist review confirmed Knots' guard against applying
  address-based whitelist permissions to Tor inbound peers (`61cdc04a83`) is
  already present in current Core and in this port, so it is not a
  Core-missing hardening item. The port now adds `p2p_permissions.py` coverage
  for a real localhost peer connecting through a dedicated `=onion` bind while
  `-whitelist=noban@127.0.0.1` is configured; the peer is reported as
  `network="onion"` and receives no whitelist permissions.
- The outgoing whitelist-permission review confirmed Knots'
  `-whitelist=...,out@...` automatic-outbound behavior (`a9f6f721aa`) is
  present in the port and absent from current Core. Current Core only applies
  outgoing whitelist permissions to manual connections in `ConnectNode(...)`;
  Knots and this port apply the configured outgoing ranges to every outgoing
  connection type. This is operator permission semantics, not consensus
  behavior. `p2p_permissions.py` now covers both sides directly: an automatic
  `outbound-full-relay` peer receives `noban`/`download` when the whitelist is
  `noban,out@127.0.0.1`, and receives no such permissions when the whitelist is
  incoming-only.
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
- The compact-filter index default review confirmed Knots' `b85232d7462` is
  present in the port and absent from current Core. Bare `-blockfilterindex`
  and `-blockfilterindex=1` enable only the selected default filter set
  (currently `basic`) in Knots and the port, while current Core still expands
  those forms to `AllBlockFilterTypes()`, currently including `v0`. This is
  index resource-control behavior, not consensus or P2P protocol behavior. The
  port now pins it in `rpc_getblockfilter.py` by proving `v0` is a known filter
  type but is not enabled by bare `-blockfilterindex`; the same test file also
  passes against unmodified Knots.
- The feefilter, local-only bloom, and filtered-witness-block review confirmed
  Knots' restored `-feefilter` option (`8fb8c3a1f7`), localhost-only BIP37
  bloom-filter default (`ae3270624f`), and
  `MSG_FILTERED_WITNESS_BLOCK` handling (`9eaa8b5350`) are present in the
  port and absent from current Core master. Current Core has no `-feefilter`
  argument, keeps BIP37 off by default without Knots' implicit localhost
  `bloomfilter` permission, and still comments `MSG_FILTERED_WITNESS_BLOCK`
  out as unused. The port now covers `-nofeefilter` in `p2p_feefilter.py`.
  `p2p_filter.py` now verifies that a default node does not advertise global
  `BLOOM`, a localhost peer nevertheless receives the `bloomfilter` permission
  and can send `filterload`, `-peerbloomfilters=0` disables that exception, and
  a filtered witness block returns the matched transaction with its
  witness-preserving `wtxid`. The strengthened tests pass against unmodified
  Knots, confirming these are native Knots behaviors rather than port-introduced
  changes. This is P2P relay/service compatibility and operator control, not
  consensus behavior or a remote crash.
- The invalid-block peer-punishment review confirmed Knots' relaxation
  (`7c7b5839f4`) is present in the port while current Core still routes the
  same block/header validation failures through `Misbehaving(...)`. The port
  also extends Knots' decision matrix to the port-only private-broadcast
  connection type: inbound, manual, feeler, and `noban` peers are tolerated;
  outbound full-relay, block-relay, address-fetch, and private-broadcast peers
  are disconnected without discouraging their address. `net_tests` now covers
  that full matrix, and `p2p_invalid_block.py` now runs with an ordinary
  inbound peer to prove non-mutated consensus-invalid blocks stay connected.
  The same functional test also documents the separate mutated-block pre-check:
  Knots and the port still disconnect an ordinary inbound peer before that
  path reaches `MaybePunishNodeForBlock(...)`. A fresh rerun of the same
  functional test against unmodified `../knots` passed, confirming the behavior
  is inherited from Knots rather than port-created.
- The compact-block duplicate-`blocktxn` review found Knots' empty partial
  header guard (`569ceb0df4`) missing from the port, even though current Core
  master and unmodified Knots both carry it. A failed compact-block
  reconstruction clears the `PartiallyDownloadedBlock` header while leaving the
  partial-block pointer available for fallback block download; a duplicate
  `blocktxn` response from the same peer then reached the port's
  `Assume(LookupBlockIndex(partialBlock.header.hashPrevBlock))` path with an
  empty header. The port now removes the request and marks the peer
  misbehaving before using the empty header. The same pass restored the active
  duplicate-`blocktxn` functional coverage by removing a stale duplicate test
  definition and dropped a current-Core disconnect expectation that conflicts
  with Knots' invalid-block punishment relaxation.
- The compact-block witness-mutation review confirmed Knots' `FillBlock`
  mutation check (`9b95ab5e9d`, from Core PR 32646) is present in the port and
  already present in current Core, so it is not a Core-missing hardening item.
  The new `p2p_compactblocks.py` `test_witness_mutated_blocktxn` method covers
  the consensus-adjacent relay path directly: a compact block omits the
  coinbase, the peer replies with a `blocktxn` coinbase whose txid merkle root
  still matches but whose witness commitment is mutated, reconstruction fails
  with full-block fallback, and the later valid full block is still accepted.
  The same focused method passes on unmodified Knots, confirming this behavior
  was not introduced by the port.
- The v2-transport privacy review confirmed Knots' randomized Tor
  stream-isolation credential prefix (`10397d85ca`) is already present in
  current Core under different commits, so it is not a Core-missing fix. The
  same review confirmed Knots' `-v2onlyclearnet` option is present in the port
  and actual Knots, while current Core has no matching option. When enabled,
  it skips outbound V1 attempts and V2-to-V1 fallback reconnections only for
  IPv4/IPv6 clearnet peers; V1 onion behavior remains allowed. The existing
  `p2p_v2_encrypted.py` coverage passes for V2 clearnet success, V1 clearnet
  refusal, and V1 onion allowance, and `feature_config_args.py` now covers the
  startup guard that rejects `-v2onlyclearnet=1` when v2 transport is disabled.
  The same focused startup-guard test and the full V2 transport functional test
  both pass against unmodified Knots, so this is native Knots behavior rather
  than a port-introduced validation change.
- Knots' user-agent sanitization hardening (`b9d2634b81`) was mostly present in
  the port as `eacc171127`: received peer user agents kept printable
  punctuation in `cleanSubVer`, and `util_tests` covered
  `SAFE_CHARS_PRINTABLE` plus log-style escaping. A later audit found the final
  receive-version log call still used `cleanSubVer` directly after rebasing
  onto current Core's log format. The port now percent-escapes that log value
  and `p2p_handshake.py` checks both surfaces: RPC preserves
  `/User/Agent: test![]{}~/`, while `debug.log` contains
  `/User/Agent: test%21%5B%5D%7B%7D%7E/`. A source comparison shows current
  Core still stores `SanitizeString(strSubVer)` and logs that value directly,
  while actual Knots and the port store `SAFE_CHARS_PRINTABLE` output and
  escape the receive-version log value.
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
  `interface_zmq.py` run now passes, including a fresh rerun from
  `/mnt/my_storage/build-zmq-audit`, a separate tree configured with
  `-DWITH_ZMQ=ON`; the latest check rebuilt `bitcoind`/`bitcoin-cli` there
  before rerunning the functional suite. The same pass confirmed Knots'
  `ipc://` ZMQ URI compatibility (`be5ba1bc7e`, merged as `9bbe4d26fc`) is
  present in the port and absent from current Core's startup argument
  pre-check, which still accepts only the Core `unix:` alias in
  `CheckHostPortOptions(...)`. This is local ZMQ configuration compatibility,
  not consensus behavior or network hardening; `interface_zmq.py` now reaches
  and passes a native `ipc://` notifier startup check on both the port and
  unmodified Knots ZMQ builds.
- The fee-estimator follow-up confirmed Knots' `TxConfirmStats::Read`
  pre-multiplication bound check (`163d3e5c13`, ported as `aeaf84b7d5`) is
  present while current Core still multiplies `scale * maxPeriods` before
  checking the one-week bound. Current Core master has independently widened
  the temporary product to `uint64_t` (`fa1d17d56c`), so this is no longer a
  demonstrated current-Core overflow; it is stricter corrupt-file validation
  ordering in Knots and the port. The port now adds `policyestimator_tests`
  coverage that serializes a corrupt `fee_estimates.dat`-style record with an
  impossible scale and verifies that `CBlockPolicyEstimator::Read(...)` rejects
  it cleanly.
  The same pass classified the fee-histogram unsigned-decrement fix
  (`85c8d477b0`, ported as `759e1d76b3`) as Knots-surface hardening because
  current Core has no `getmempoolinfo(with_fee_histogram=...)` or REST
  histogram surface. `policyestimator_tests` and `mempool_fee_histogram.py`
  pass with the ported code; the functional test now includes the historical
  edge shape where the mempool has a transaction below the only requested
  histogram floor. A same-test run against unmodified Knots confirms the
  fee-histogram behavior is inherited rather than port-created.
- The mempool/orphan IBD performance review checked Knots' `8990a80618`
  `removeForBlock` empty-map guard and `fc5361a515` orphanage empty-pool
  guard. Both are present in the port and current Core master, so they are not
  Core-missing covert DoS hardening. They are local CPU/allocation reductions
  during block connection and orphan cleanup, not consensus behavior. Focused
  `mempool_tests` and `orphanage_tests` pass with the port.
- The block-storage performance review checked Knots' bulk serialization and
  low-priority I/O stream (`6f9c3445bc`, `6f34614262`, `e55171aa6c`,
  `a737874af8`, `cc8111592c`). Current Core master already carries the bulk
  block/undo read-write and expected-hash pieces through different hashes
  (`520965e293`, `8d801e3efb`, `09ee8b7f27`, `9341b5333a`), but does not
  carry Knots' `ioprio` layer. The port keeps Knots' lower-priority reads for
  peer block serving, startup verification, rollback, and external block import
  while preserving Core's current `ReadRawBlockResult` API. This is local
  I/O-scheduling hardening/performance behavior, not consensus behavior.
  Focused `streams_tests` and `blockmanager_tests` pass with the port.
- The wallet best-block persistence review checked Knots' follow-up series
  `3c20072d93`, `36f643e680`, `5b371fb621`, and `ad19b1efff`. Current Core
  already carries the same best-block-in-memory/write-through behavior for
  block connect, block disconnect, wallet loading, and post-rescan state. This
  is wallet reorg/rescan bookkeeping, not consensus behavior and not a
  Knots-only hardening gap. Focused `wallet_tests` and
  `wallet_reorgsrestore.py` pass with the port.
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
  fallback log is not hit. A fresh cross-run with the port's strengthened test
  against unmodified Knots still fails on the broad bypass and prints the
  internal-bug log, while Knots' own narrower RDTS test passes because it only
  exercises the exact `mempool-script-verify-flag-failed` ignore string.
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
  A later exact-patch audit mapped the current Knots activation-boundary commits
  to adapted port commits: the UTXO-height test (`224af28c3c`) is
  `56b84e44fd`, the per-input old-UTXO exemption and cache-safety changes
  (`0e066c15eb`, `5524704ab0`) are `87c701dc1d`, the upgradable-witness /
  taproot / OP_SUCCESS consensus enforcement (`e7344dff45`) is `671097f199`,
  the output-size rule stack (`ff9a3348e8`, `d6c354e30d`, `b5ba93ec1b`) is
  present through `0f63eae265`, and the generation-transaction output-size
  check (`198386d383`) is present in `Consensus::CheckOutputSizes(...)` plus the
  `ConnectBlock()` coinbase check. The focused UTXO-height test now passes on
  both the port and an unmodified Knots build, including the
  activation-boundary reorg/cache-poisoning scenario.
  `feature_reduced_data_temporary_deployment.py` was later strengthened to
  cover the script-flag side of temporary expiry too: a post-activation P2WSH
  spend with a 300-byte witness is rejected while RDTS is active, then accepted
  once the test deployment expires at height 576. The strengthened test passes
  against both the port and unmodified Knots, so this confirms native temporary
  soft-fork behavior rather than a newly found consensus bug.
- The libbitcoinconsensus review confirmed Knots intentionally restores the
  shared `libbitcoinconsensus` surface removed from current Core, but keeps
  `BUILD_BITCOINCONSENSUS_LIB` defaulted to `OFF`. This is compatibility and
  packaging policy, not an extra consensus rule or Core-missing hardening fix.
  The local audit build has the option enabled so the target can be checked:
  `cmake --build build --target bitcoinconsensus -j2` was already up to date,
  and `script_tests` pass; the only warning is the expected skipped
  `script_assets_test` when `DIR_UNIT_TEST_DATA` is unset.
- The RDTS P2P-service review found a port-introduced handshake cleanup issue
  after adapting Knots' preferential RDTS peering and `-maxstaleoutbound`
  behavior (`7f57236043`, `44d7e88dba`, `c146ef8c7a`) onto current Core's
  newer VERSION handling. The port had retained Core's earlier
  peer-service/tx-relay setup and also inserted Knots' later setup block after
  the RDTS stale-peer gate, so accepted transaction-relay peers called
  `Peer::SetTxRelay()` twice and hit its `Assume(!m_tx_relay)` invariant.
  Unmodified Knots sets this state once after the RDTS gate, and current Core
  has no RDTS gate. The port now removes the duplicate setup as `d10d97fd54`,
  updates stale fixed-limit comments, and extends `peerman_tests` /
  `p2p_handshake.py` to cover `-maxstaleoutbound` parsing and the
  zero-tolerance non-BIP110 path. A follow-up check confirmed the later Knots
  default of eight stale outbound peers is present as
  `DEFAULT_MAXSTALEOUTBOUND{8}`.
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
- A focused miner review rechecked Knots' restored serialized-size mining cap
  and follow-up bugfixes (`b835402650`, `e5bf9721fa`, `b392070a38`,
  `e2212c8cf8`, `c95d04c8a8`) against this port and current Core. The port
  matches actual Knots' intent under the current Core block assembler:
  `-blockmaxsize`/GBT `blockmaxsize` account serialized transaction size only
  when the cap is below `MAX_BLOCK_SERIALIZED_SIZE`; setting only size leaves the
  weight cap unbounded at `MAX_BLOCK_WEIGHT`; near-maximum size caps no longer
  silently disable the size cap or lower the weight cap; and the "full enough"
  stop heuristic compares against the configured size cap using a fixed
  `BLOCK_FULL_ENOUGH_SIZE_DELTA`, not the reserved coinbase/header size.
  Current Core master has no `-blockmaxsize` surface, so this is a restored
  Knots miner/operator policy difference, not a consensus-rule change. Blocks
  above a local mining cap remain valid. Coverage added/adapted in
  `30a6e24931` keeps the restored startup and RPC paths live under current
  cluster-mempool and data-carrier policy. Proof: `build/bin/test_bitcoin
  --run_test=miner_tests/blockmaxsize_mining_options --catch_system_error=no
  --log_level=error --report_level=short`, `python3
  test/functional/mining_basic.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mining_basic_blockmaxsize_review10
  --portseed=7419`, and comparison run `python3
  ../knots/test/functional/mining_basic.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_mining_basic_blockmaxsize_review
  --portseed=7420` all passed. The functional-test issues found during this
  pass were port/test adaptation issues: the RPC `minfeerate` helper must not
  expect `getmininginfo().blockmintxfee` to change, the synthetic large
  transactions need explicit non-standard relay permission under Knots' stricter
  data-carrier defaults, and the RPC mempool fixture needs the real
  `-limitclustersize` knob instead of deprecated ancestor/descendant size
  options. Actual Knots' mining functional test passed, so this did not expose
  an original Knots consensus or remote-crash bug.
- The policy-default/corepolicy review confirmed a non-obvious Core/Knots
  configuration split. Current Core master defaults `-minrelaytxfee` and
  `-incrementalrelayfee` to 100 sat/kvB and `-blockmintxfee` to 1 sat/kvB.
  Knots restored stricter public defaults of 1000 sat/kvB for all three
  (`48ada82ee9`, `8cc55d6dc4`, `7e020a528f`; ported as `c30b6101f8`,
  `4c24f71e5b`, `7dda581441`) while adding `-corepolicy=1` to soft-set Core's
  lower fee defaults and several less restrictive data-carrier/policy knobs.
  A runtime regtest check showed the port's default
  `getmempoolinfo`/`getmininginfo` values as `0.00001000`, `0.00001000`,
  `0.00001000`, and the same node with `-corepolicy=1` as `0.00000100`,
  `0.00000100`, `0.00000001`. This is a policy/config divergence, not a
  consensus-rule difference.
- The full non-GUI unit run exposed three integration issues from combining
  Knots behavior with current Core tests/APIs. `node_init_tests/init_test`
  aborted because current Core's init test never initialized Knots'
  `bitcoin_rw.conf` path; the port now runs `ReadConfigFiles()` before
  `AppInitMain` as `f41907fc25`. `rpc_tests/rpc_convert_values_dumptxoutset`
  exposed a port-introduced `bitcoin-cli -named dumptxoutset ... separator=:`
  conversion bug caused by current Core's string-positional heuristic; the
  port now registers `separator` as a string parameter as `cc59b7ea37`.
  Unmodified Knots `29.x-knots` accepted the same named command and wrote a
  colon-delimited human-readable dump after generating a regtest UTXO, so this
  was not an original Knots defect. The RBF feerate-diagram test still carried
  Knots 29.x expectations that larger conflict topologies were uncalculable;
  current Core's TxGraph-backed mempool can calculate those diagrams, so the
  port adapts the coverage and fixes the grandchild lookup typo as
  `73e8138d49`.
- The RBF service-bit review confirmed Knots' `b3b512dc6e` behavior is present:
  the port keeps the compatibility `NODE_REPLACE_BY_FEE` constant/display name
  for peers that advertise it, but no longer advertises the bit locally even
  when `-mempoolreplacement=fee,-optin` makes the local policy full-RBF. Current
  Core master no longer has the service bit at all. This is network-policy
  signaling, not consensus behavior.
- The same RBF pass found a real port omission in the anti-DoS replacement
  limit. Current Core's cluster-mempool RBF code limits replacements by the
  number of affected clusters; actual Knots `29.x-knots` also keeps the older
  BIP125/Rule-5 bound on potential transactions evicted, counted conservatively
  with descendants. The port had inherited Core's looser cluster-only behavior,
  so a replacement evicting more than 100 transactions across fewer than 100
  clusters was accepted. This was not an original Knots bug: unmodified Knots'
  `feature_rbf.py` rejects the default-mempool case. The port now enforces both
  bounds and covers the cluster-bound, transaction-bound, and overlapping
  descendant overcount cases in `rbf_tests`, with functional coverage in
  `feature_rbf.py`. This is mempool anti-DoS policy hardening missing from
  current Core, not a consensus rule difference.
- Exercising that area also exposed port-side test harness drift: the rebased
  `mempool_util.py` lacked current Core's cluster/TRUC constants and the
  `random` import used by orphan helpers, and the RBF functional test still had
  stale current-Core message expectations plus deprecated ancestor/descendant
  size args. These were port/test issues, not original Knots client defects.
- A refreshed 2026 exact-patch mismatch pass found four final-tree omissions
  that were not suspicious but should be carried for release parity:
  Knots' pruning help text warning that wallets and indexes must stay active
  while pruning is enabled (`5a75f172bb`, ported as `dbccc5fe0d`), the Debian
  copyright update (`d0ddf66a2c`, ported as `6bd131f112`), and the Boost 1.91
  multi-index workaround (`4d22aa2b70`, ported as `c9af9b3a49`), and the Guix
  Darwin linker flag for identical-code-folding parity (`bea657ca5b`, ported
  as `39792fee03`). The Boost
  commit was adapted because current Core's cluster-mempool miner no longer
  has Knots' old `CTxMemPoolModifiedEntry_Indices` container; the live
  `CTxMemPoolEntry` and `Announcement` containers now use the Knots
  version-gated alias form. The same pass checked Knots'
  precomputed-transaction-data lifetime fix (`29b4e281a7`, CVE-2024-52911)
  and confirmed the port/current Core source already declares `txsdata` before
  `CCheckQueueControl`, so the check queue is destroyed first.
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
- The older positional compatibility form
  `bitcoin-cli -regtest addnode 127.0.0.1:18445 onetry '"inbound"'`
  reaches the same assertion in unmodified Knots.

This was not introduced by the port. The port rejects this input with
`RPC_INVALID_PARAMETER`, and `rpc_net.py` covers both the named/new argument
path and the old positional compatibility path.

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
python3 /mnt/my_storage/bitcoin/test/functional/feature_rdts.py \
  --configfile /mnt/my_storage/knots/build-repro/test/config.ini \
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_knots_with_port_test \
  --portseed=7424
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

The same strengthened test passes on the port:

```text
python3 test/functional/feature_rdts.py --configfile build/test/config.ini \
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_port_recheck --portseed=7423
```

Knots' native test still passes because it does not exercise the broad legacy
script-flag bypass names:

```text
python3 ../knots/test/functional/feature_rdts.py \
  --configfile ../knots/build-repro/test/config.ini \
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_knots_own_recheck --portseed=7425
```

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

The related no-cap behavior was also confirmed on unmodified Knots with the
permission assertion disabled:

```text
python3 /mnt/my_storage/bitcoin/test/functional/p2p_eviction.py \
  --configfile /mnt/my_storage/knots/build-repro/test/config.ini \
  --cachedir /mnt/my_storage/bitcoin/test/cache \
  --test_methods test_forceinbound_nocap \
  --tmpdir=/mnt/my_storage/tmp_knots_p2p_eviction_forceinbound_nocap_method \
  --portseed=32663
```

Result on original Knots: passed; nine ForceInbound peers remained connected,
so the no-cap behavior was not introduced by the port.

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

- Automatic reindex on corrupted block data:
  `35698ab7f4`

  Current Core master still offers only boolean `-reindex` and an interactive
  GUI retry prompt after chainstate-load failure. Knots and this port accept
  `-reindex=auto`; if block-index or chainstate loading fails, they skip the
  prompt and retry once with full reindexing. This is local
  availability/recovery hardening for corrupt on-disk chain data, not consensus
  behavior. The port's `feature_init.py` now covers deletion of a block-index
  LevelDB file and confirms that `-reindex=auto` automatically reindexes back
  to the same height; the same method passes on unmodified Knots.

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
  is local configuration hardening, not a remote bypass by itself. Source
  comparison confirmed the port carries Knots' `-rpcauthfile` argument,
  wallet-restriction propagation through `JSONRPCRequest`, and blank-token
  skip behavior. The full `rpc_users.py` auth coverage passes on both the port
  and unmodified Knots, including auth files with one entry, multiple entries,
  blank lines, no trailing newline, wallet restrictions, blank direct
  `-rpcauth`, and `-norpcauth` interactions.

  Knots' follow-up to store cookie and `-rpcuser`/`-rpcpassword` credentials
  hashed in memory (`f06169f019`) was rechecked separately because it looks
  like a covert-security candidate. It is present in unmodified Knots and in
  this port's adapted wallet-restriction-aware auth path, but current Core
  master already has equivalent hashing from bitcoin/bitcoin#32423, so it is
  not a Core-missing hardening candidate. The remaining Core-missing surface in
  this auth area is Knots' auth-file, wallet restriction, and blank-token
  handling described above.

  A follow-up test pass also covered Knots' method-level wallet-restriction
  hardening (`5edd9495d8`, `52db62f6fa`, `78b8b5f465`, `32ce5f22c0`):
  wallet-restricted users cannot call `importmempool`, `dumptxoutset`,
  `loadtxoutset`, `restorewallet`, wallet import/export/backup/restore, or
  `migratewallet`; and `createwallet`, `loadwallet`, and `unloadwallet` cannot
  operate on a different loaded wallet. The strengthened `rpc_users.py` passes
  on both the port and unmodified Knots, so this did not expose an original
  Knots defect.

- Numeric `settings.json` boolean handling:
  `577c04c80e`

  Current Core master still implements `SettingToBool(...)` as null/bool/string
  handling with `value.get_str()` for every non-bool value, and its
  `getarg_tests/setting_args` expectations still require numeric boolean
  settings to throw. Numeric `settings.json` values therefore throw in
  `GetBoolArg`, even though equivalent string settings are interpreted. Knots
  and this port treat `UniValue::VNUM` like `UniValue::VSTR` by passing
  `getValStr()` to `InterpretBool(...)`, so `99` and `3.25` are true while `0`
  is false. This is local configuration robustness and removes a surprising
  abort-on-access path for numeric settings; focused `getarg_tests/setting_args`
  coverage passes with the port.

- First-startup block-storage size warning units:
  `4f06564f36`, `4c62285bb9`, `58ceddd389`

  Current Core master still converts `AssumedBlockchainSize()` to bytes with
  `1_GiB`, but the chainparam value and warning text are decimal GB; it also
  still reports the full assumed chain size even when the warning is about a
  lower prune target. Knots and this port use `1'000'000'000` for the
  disk-space threshold, report the selected full-chain-or-prune byte count, and
  round up the displayed decimal GB value. This avoids a confusing
  false-positive warning window where Core can say approximately `N GB` will be
  stored but require enough free space for `N GiB`, and avoids under-reporting a
  fractional pruned target as `0 GB`. This is local operator
  warning/resource-estimation correctness, not consensus or network security
  behavior. The port now factors the calculation for
  `node_init_tests/block_storage_space_warning_units` coverage.

- UTXO LevelDB write-batch default:
  `2104df3209`

  Current Core master now defaults `-dbbatchsize` to 32 MiB after merging the
  upstream version of the UTXO flush optimization. Knots keeps the more
  aggressive 64 MiB default, which reduces the number of LevelDB write batches
  during large UTXO flushes at the cost of a larger temporary memory peak. This
  is performance/resource-selection behavior, not consensus behavior. The port
  now applies the Knots value at Core's current `DEFAULT_DB_CACHE_BATCH`
  location so `CoinsViewOptions`, `bitcoind -help-debug`, and the unit test all
  agree with unmodified Knots.

- Low-memory-triggered dbcache flushing:
  `10e7dc80ee`, `d95f134d11`, `ebf7df30c2`, `da690b5a69`, `f9f7587b59`

  Current Core still flushes the coins cache during `IF_NEEDED` only when the
  cache itself crosses its size limit. Knots and this port can also flush when
  the host is under memory pressure, using Windows memory status and Linux
  `sysinfo` data, and expose `-lowmem=<MiB>` so operators can choose the
  threshold or disable the feature with `0`. This is availability hardening for
  memory-constrained nodes and avoids keeping dirty UTXO cache entries in RAM
  while the OS is likely to swap. It is not consensus behavior and not remotely
  triggerable by itself. This pass verified the behavior against unmodified
  `29.x-knots`; the port did not invent the feature. The port now also covers
  the pure threshold decision, overflow-safe `free+buffer` accounting, explicit
  `-lowmem=0` disable handling, and the validation `IF_NEEDED` flush trigger.

- Buffered block-file page-cache advice:
  `97130ac516`

  Current Core still wraps sequential block-file reads in `BufferedFile`
  without telling the OS about the access pattern or dropping file-backed pages
  afterward. Knots and this port call `posix_fadvise(..., WILLNEED)` /
  `POSIX_FADV_SEQUENTIAL` through `AdviseSequential()` and close with
  `CloseAndUncache()` so the kernel can avoid retaining large sequentially read
  block-file pages. This is local resource/performance hardening for reindex
  and other sequential reads, not consensus behavior and not a remote trigger.
  This pass verified the port against unmodified `29.x-knots` and current Core:
  actual Knots and the port expose `AdviseSequential(...)` and
  `CloseAndUncache(...)`, while current Core still lacks those hooks. The port
  now has direct `fs_tests/file_advice_helpers_keep_file_semantics` coverage
  asserting the advice helper preserves the current file position and the close
  wrapper still flushes/closes the file (`3e75471507`).

- Prune-lock reorg rollback and persistence:
  `8ee1214157`, `0b4bd4e134`, `4822c21812`

  Current Core master still moves internal prune locks backward only when their
  first protected height is above `pindexDelete->nHeight - 1`, setting the lock
  directly to that height. Knots and this port instead decrement locks at or
  above that boundary, so a lock at the disconnected tip boundary gets moved
  back one block and still has a chance to survive a reorg. Knots also exposes
  and persists operator-created prune locks; current Core has no public
  prune-lock RPCs or `DB_PRUNE_LOCK` persistence. The port's existing
  `feature_index_prune.py` asserts the reorg movement log, and the new
  `rpc_prunelocks.py` covers RPC persistence and temporary-lock semantics.

- Wallet symlink/reparse-point path hardening:
  `39f48a142f`, `1f118f18c4`, `ee042e9ad6`

  Current Core master still uses ordinary `std::filesystem` symlink checks in
  the wallet directory scan and regular-file wallet path compatibility case.
  Knots adds an `IsSymlink(...)` helper that detects Windows reparse points,
  avoids recursing such entries while scanning `listwalletdir`, and prevents a
  symlink/reparse point from being accepted as a legacy top-level wallet file.
  This is local path-safety hardening around wallet discovery/loading. Source
  comparison confirmed the port matches Knots' scanner shape while current Core
  lacks the helper call and recursion guard. The full `wallet_multiwallet.py`
  run passes and covers the symlink-recursion warning plus symlinked wallet
  load rejection on platforms where symlink checks are enabled.

- Wallet node-data directory scan skip:
  `283cd1f065`

  Current Core master still recurses through node data directories when
  `-walletdir` is pointed at the chain datadir. Knots skips top-level node
  storage paths such as `blocks`, `chainstate`, `coins`, `database`, `indexes`,
  and network subdirectories during wallet discovery. This reduces accidental
  wallet detection and expensive traversal of non-wallet storage, and is local
  wallet-discovery hardening rather than a consensus change. The current port
  and actual Knots both carry the skip list in `ListDatabases(...)`, while
  current Core's corresponding scanner still has no node-data skip list. The
  fresh `wallet_multiwallet.py` run covers this by placing a fake wallet marker
  under `blocks/` and proving `listwalletdir` ignores it.

- Wallet failed-cleanup directory hardening:
  `0388bfc6e`

  Current Core master still assumes newly created restore/migration wallet
  directories are empty before removing them during failure cleanup. Knots logs
  and leaves the directory alone if it is unexpectedly non-empty, avoiding an
  unintended removal attempt against a path that no longer has the shape the
  cleanup code expected. This is local wallet data-safety hardening, not a
  consensus issue. The port factors this into
  `RemoveCreatedWalletDirIfEmpty(...)`; `wallet_tests` now asserts the empty
  directory removal path and the non-empty sentinel-preservation path.

- Subprocess fd cleanup before exec:
  `214047ecd3`, `ed5a3b3604`

  Knots implements `close_fds` cleanup with `/proc/self/fd`, then replaces it
  with `close_range`/fallback closing to avoid non-async-signal-safe work after
  `fork()`. This is not port-introduced: unmodified Knots carries the
  `close_fds` option, the Tor subprocess call passes `subprocess::close_fds`
  on `29.x-knots`, and the port's `src/util/subprocess.cpp` matches Knots for
  the child-side `close_range`/fallback helper. Core master has no
  `close_fds` option or `src/util/subprocess.cpp`; its current
  `RunCommandParseJSON` path does not request descriptor cleanup. The port now
  has a focused `system_tests/subprocess_close_fds` regression that verifies
  file descriptors are inherited without `close_fds` and closed with it.

- Port mapping disabled when not listening:
  `95c8a63102`

  Knots force-disables explicit `-upnp=1` and `-natpmp=1` values when parameter
  interaction leaves the node with `-listen=0`. Current Core no longer exposes
  UPnP, but its NAT-PMP interaction still uses a soft disable, so an explicit
  `-listen=0 -natpmp=1` can survive until `StartMapPort(...)`. This is local
  network-surface/privacy hardening, not a consensus issue. A minimal
  unmodified Knots startup with `-listen=0 -upnp=1 -natpmp=1` logged both
  forced-disable messages, confirming this is not port-introduced; rerunning
  the port's direct functional method against `../knots` also passes. The port
  now covers the direct explicit-argument case in `feature_config_args.py`, in
  addition to the existing `-connect=0` / `-noconnect` interaction coverage.

- Per-network proxy `tor` alias:
  `77f1a82318`

  Knots and this port accept both `-proxy=...=tor` and `-proxy=...=onion` for
  the Tor/onion network selector. Current Core's help text says the network can
  be `ipv4`, `ipv6`, `tor`, or `cjdns`, but the parser branch still checks only
  `net_str == "onion"` before falling through to `Unrecognized network`. This
  is a configuration compatibility/UX shortcoming rather than a remote security
  issue: the failure is startup-time and explicit, but an operator following
  help text or Knots-compatible config syntax gets a surprising rejection. The
  existing `feature_proxy.py` "Test overriding the Tor proxy" path covers the
  `=tor` spelling and now passes against both the port and unmodified Knots.

- Tor local-address registration without outbound Tor reachability:
  `eab454304e`

  Knots and this port allow `AddLocal()` for Tor/onion local addresses even
  when `NET_ONION` is not currently reachable, because onion inbound service
  advertisement and outbound onion reachability are not necessarily symmetric.
  Current Core still requires `g_reachable_nets.Contains(addr)` for all local
  address networks, so `-onlynet=ipv4 -listenonion=0 -externalip=<onion>` drops
  the explicit onion local address instead of reporting it in
  `getnetworkinfo.localaddresses`. This is operator/network-surface behavior,
  not consensus or remote crash risk. The port now pins the rule with
  `net_tests/LocalAddress_TorDoesNotRequireOutboundReachability`; a runtime
  startup/RPC check also confirms both the port and unmodified Knots report the
  onion `-externalip` in `localaddresses` while `onion.reachable` is false.

- PCP/NAT-PMP explicit-warning preservation:
  `00354e1161`

  Current Core already downgrades repeated PCP/NAT-PMP `NOT_AUTHORIZED`
  failures to debug after the first warning, which avoids noisy logs on routers
  that disable port mapping. Knots adds a small but operator-relevant hardening
  nuance: if the user explicitly requested NAT-PMP/PCP, or enables it through
  the node interface, repeated `NOT_AUTHORIZED` results stay at warning level
  instead of being silently downgraded. This is operator visibility rather than
  a consensus or remote crash issue. The port adapts Knots' global flag to the
  current Core `common/pcp.cpp` split and uses an atomic flag because the port
  mapping code can be toggled from the node interface. The new
  `pcp_tests/pcp_not_authorized_explicit_warning` unit test asserts both sides
  of the behavior: one warning by default, and repeated warnings when the
  explicit-warning flag is set.

- HTTP RPC bind failure behavior:
  `57becdf59e` plus follow-up listen/bind cleanup commits

  Knots fails initialization when any explicitly requested RPC bind fails,
  while current Core only requires at least one endpoint to bind. This is
  configuration-safety/availability hardening rather than a confirmed
  vulnerability. Direct reproduction on unmodified Knots with one occupied
  `-rpcbind` endpoint and one free endpoint returned exit code 1, the generic
  HTTP startup error on stderr, and the specific bind-all-endpoints error in
  `debug.log`; rerunning the port's current `rpc_bind.py` against both the
  port and `../knots` passes, including the explicit partial-bind failure case.

- Invalid-block peer punishment relaxation:
  `7c7b5839f4`

  Current Core still marks peers as misbehaving/discouraged for several invalid
  block/header paths in `MaybePunishNodeForBlock(...)`. Knots instead routes
  non-mutated invalid-block validation results through
  `CNode::PunishInvalidBlocks()`: inbound, manual, feeler, and `noban` peers
  are tolerated, while outbound full-relay, block-relay, address-fetch, and
  this port's private-broadcast peers are simply disconnected rather than
  discouraged. The direct mutated-block pre-check is still handled as ordinary
  misbehavior and can disconnect an inbound peer before
  `MaybePunishNodeForBlock(...)` runs. This is network partition/availability
  hardening rather than a consensus-rule change. Current Core already carries
  the related transaction-relay cleanup (`drop MaybePunishNodeForTx`) and the
  single script-check path and onion-inbound whitelist permission suppression;
  the remaining Core difference is invalid-block peer-punishment behavior. The
  port pins the connection-type decision matrix in
  `net_tests/cnode_punish_invalid_blocks`, and `p2p_invalid_block.py` covers the
  ordinary-inbound non-mutated-invalid-block tolerance plus the still-disconnect
  mutated-block pre-check.

- ForceInbound trusted-inbound eviction:
  `3544a26256`, `711dadb546`, `067f80e1b5`, `3db935abd1`

  Current Core does not expose Knots' `forceinbound` P2P permission. Knots and
  this port can mark selected inbound peers as trusted enough to force an
  eviction attempt when inbound slots are full, choosing a random unprotected
  inbound peer if the regular protection logic would otherwise leave no
  candidate. This is local operator availability/DoS hardening, not a
  consensus change. The port's latest `p2p_eviction.py`, `p2p_permissions.py`,
  and `netbase_tests/netpermissions_test` runs cover the forced-inbound
  connection path plus the port-side fix for Knots' missing
  `getpeerinfo.permissions` string.

- Outgoing whitelist permissions for automatic outbound peers:
  `a9f6f721aa`

  Current Core still applies `-whitelist=...,out@...` permissions only when
  `ConnectNode(...)` is opening a manual connection. Knots and this port apply
  the outgoing whitelist ranges to automatic outbound connection types as well,
  so operator-granted permissions such as `noban`/`download` are not silently
  limited to `addnode`/manual peers. This is local network permission semantics,
  not a consensus change. The isolated
  `p2p_permissions.py --test_methods check_automatic_outbound_permissions`
  run passes against both the port and unmodified Knots.

- Implicit whitelist `addr` permission:
  `9a79815097`

  Current Core expands bare implicit whitelist entries such as
  `-whitelist=127.0.0.1` to relay/mempool/noban/download-style permissions but
  still omits `addr`. Knots and this port include `addr`, so trusted implicit
  whitelist peers can request uncached, up-to-date address responses and are
  allowed the corresponding address-relay behavior without requiring the
  operator to spell `addr@...` explicitly. This is network permission
  semantics for trusted peers, not consensus behavior or a remote crash. The
  port's `p2p_permissions.py` covers the default, `-whitelistrelay=0`, and
  `-whitelistforcerelay` implicit cases; unmodified Knots' own
  `p2p_permissions.py` passes with the same expectations.

- Compact-block extra-transaction memory cap:
  `390d5f80e6`, `43a4bcd7f5`, `9b78a13aea`

  Current Core retains only a count limit for the non-mempool extra
  transactions used during compact block reconstruction: the default is 100
  entries, and there is no size-based cap. Knots and this port raise the count
  default to 32,768 but add `-blockreconstructionextratxnsize`, defaulting to
  10 MB, plus a 100 KB per-transaction insertion ceiling. This makes the cache
  more useful for reconstruction without leaving memory use unbounded by large
  rejected/replaced transactions. This is P2P memory/resource hardening, not
  consensus behavior. The port's focused functional test covers count
  disabling, count wraparound, zero size, 1 MB versus 2 MB size-limit eviction,
  exact-boundary eviction, 0.1 MB fractional parsing, and a single transaction
  larger than the configured limit; the same test passes against unmodified
  Knots. `peerman_tests/peerman_args_block_reconstruction_extra_txn` covers the
  parser conversion for fractional megabytes and negative values.

- Configurable orphan-transaction count cap:
  Knots' `-maxorphantx` option is present in the port and absent from current
  Core. Current Core has modern orphanage memory/latency limits, but no
  operator-facing count knob; Knots keeps the historical default of 100 stored
  orphan transactions and lets operators lower the count to zero or raise it.
  Negative values are clamped to zero by the shared peerman argument parser.
  This is P2P memory/resource-control behavior, not consensus behavior or peer
  punishment policy. The new `p2p_maxorphantx.py` functional test starts one
  node with `-maxorphantx=3` and another with `-maxorphantx=0`, then verifies
  the visible orphanage size through `getorphantxs`; the same test passes
  against unmodified Knots. Existing unit coverage checks parser clamping and
  the txdownload orphan-count limit.

- Unknown/future witness output policy switch:
  `584798d402`, `82b2c8372e`

  Current Core relays standard transactions that create unknown/future witness
  outputs and exposes no policy knob to reject them. Knots and this port keep
  the permissive default (`-acceptunknownwitness=1`) but add
  `-acceptunknownwitness=0` and a GUI setting to reject transactions sending to
  `TxoutType::WITNESS_UNKNOWN` with
  `scriptpubkey-unknown-witnessversion`. Spending an unknown witness program
  remains non-standard in both Core and Knots, and consensus behavior is
  unchanged; this is an operator-controlled relay/mining policy switch for
  future-version output creation. `transaction_tests/test_IsStandard` toggles
  the mempool option and covers both rejection and default acceptance. The new
  `mempool_acceptunknownwitness.py` functional test proves the externally
  visible behavior: `-acceptunknownwitness=0` rejects a transaction creating a
  future witness output from mempool, but the same transaction is accepted in a
  block relayed from a permissive peer. The same functional test passes against
  unmodified Knots, confirming this is inherited Knots policy rather than a
  port-only divergence.

- Minimum relay input age / coin-block policy:
  `fa0267d631`, `482bd5382e`

  Current Core has no `-minrelaymaturity` or `-minrelaycoinblocks` policy
  knobs. Knots and this port keep both defaults at zero but let operators
  reject transactions spending too-recent confirmed outputs, either by block
  depth or by value-weighted coin-block age. This is relay/mining policy and
  local spam/rate-limit control, not consensus behavior: a block containing an
  otherwise-valid transaction that violates these local thresholds remains a
  valid block. The port's `mempool_minrelay.py` mines a fresh confirmed output,
  proves `-minrelaymaturity=2` rejects its immediate spend with
  `bad-txns-input-immature-depth` until one more block is mined, and proves
  `-minrelaycoinblocks=7500000000` rejects the same one-confirmation shape
  with `bad-txns-input-immature-coinblocks` until the output has enough
  coin-block age. The same test passes against unmodified Knots. The port also
  fixes an inherited Knots argument-validation bug by rejecting negative values
  for both options at startup.

- CJDNS addnode duplicate detection:
  `28823f30dc`

  Current Core has the earlier CJDNS `GetAddedNodeInfo()` fix, but still
  compares new addnode entries against existing entries using plain IPv6
  resolution in `AddNode()`. Knots flips RFC4193-looking CJDNS addresses before
  comparison and rejects CJDNS duplicates even when the port differs, avoiding
  repeated manual-connection entries to the same CJDNS node. The focused
  `rpc_net.py --test_methods test_addnode_cjdns_duplicate` run passes on the
  port and as an isolated cross-check against unmodified Knots.

- V2-only clearnet outbound option:
  `cbfbefd8f3`, `7c06acab77`, `f4fc3f03f4`, `5891703ba0`

  Current Core supports BIP324/v2 transport but does not expose Knots'
  debug-only `-v2onlyclearnet` policy switch. Knots and this port can refuse
  outbound V1 and V2-to-V1 fallback connections on IPv4/IPv6 while still
  allowing V1 on non-clearnet networks such as onion. This is default-off
  transport-policy hardening, not a consensus change. The port's focused
  startup-guard run and full `p2p_v2_encrypted.py` run both pass, and the same
  tests pass against unmodified Knots.

- Persistent unexpected block-version signalling warnings:
  `78d5cb210b`, `771ee9fbb4`, `e94eba4e03`, `c17e9d41d5`,
  `9fd3694e76`

  Current Core warns on unknown versionbits activation for BIP323-available
  bits, but does not keep Knots' additional persistent last-100-block warnings
  for unknown version schemas, individual unexpected versionbits, or BIP320
  reserved-bit signalling thresholds. Knots also logs a per-block
  `Miner violated version bit protocol` warning for BIP320 reserved-bit abuse
  without promoting that transient warning into RPC `warnings` output. This is
  operator/security visibility hardening around possible soft-fork signalling,
  not a consensus-rule change. The port keeps current Core's BIP323 deployment
  width at
  `VERSIONBITS_NUM_BITS = 5` while scanning the historical 29 BIP9 signal bits
  through `VERSIONBITS_NUM_WARNING_BITS`, and the refreshed
  `feature_versionbits_warning.py` run passes against both the port and
  unmodified Knots.

- RPC multi-warning string-mode visibility:
  `e4e4a81317`

  Current Core's deprecated `getblockchaininfo`, `getmininginfo`, and
  `getnetworkinfo` warning string mode returns only the last active warning.
  Knots and this port join all active warnings with newlines, matching the
  non-deprecated array mode's visibility instead of silently dropping earlier
  warnings. This is operator/security visibility hardening for deprecated RPC
  clients, not a consensus change. The source delta is isolated to
  `node::GetWarningsForRpc(...)`: current Core still returns
  `all_messages.back().original` in deprecated mode, while Knots and this port
  use `util::Join(all_messages, Untranslated("\n")).original`. The port's
  `node_warnings_tests` now asserts both the array form and the newline-joined
  deprecated string form.

- DNS seed bootstrap policy:
  `277edb9009`

  Current Core still queries the Peter Todd DNS seeds on mainnet and testnet.
  Knots removes those two seed hostnames. This changes the bootstrap trust and
  availability surface, but the audit does not have evidence that it is a
  vulnerability fix or a consensus issue. The port pins this with
  `chainparams_tests/dns_seed_removals`; source comparison shows Core still
  lists `seed.btc.petertodd.net.` and `seed.tbtc.petertodd.net.`, while both
  Knots and the port omit them.

- User-agent sanitization/log escaping:
  `b9d2634b81`

  Core currently strips printable characters outside `SAFE_CHARS_DEFAULT` from
  received peer user agents before storing `cleanSubVer`, and logs that stored
  value directly. Knots preserves the full printable user-agent string for peer
  display and percent-escapes unsafe printable characters at log time. This is
  log/UI integrity hardening against confusing or spoofed peer user-agent text.
  The strengthened `p2p_handshake.py` user-agent test passes against
  unmodified Knots and the fixed port, confirming the earlier missing log
  escape was port-introduced. The latest rerun used the full handshake test
  because the user-agent assertion is inline in `run_test`.

- User-agent append/spoof controls:

  Knots adds network-visible `-uaappend=<fragment>` and debug-only
  `-uaspoof=<ua>` startup controls that current Core does not have. By
  default Knots advertises the normal `/Satoshi:.../` fragment followed by an
  added `/Knots:.../` fragment. `-uaappend` appends a literal BIP14-style
  fragment after that formatting. `-uaspoof=<ua>` replaces the entire
  subversion string, while the boolean form `-uaspoof=1` is a surprising
  identity-hiding shortcut: it keeps the base `/Satoshi:.../` user agent and
  comments but suppresses the `/Knots:.../` suffix. This is not consensus or
  remote-crash exposure, and it is not a Core-missing hardening fix; it is a
  Knots-only operator/network identity surface that can make a Knots node look
  less obviously like Knots unless local config/debug options are inspected.
  `feature_uacomment.py` now pins the boolean modes, and the same strengthened
  test passes against unmodified Knots.

- Mempool statistics subsystem and RPC:

  Knots adds a `src/stats` subsystem, `-statsenable`,
  `-statsmaxmemorytarget=<bytes>`, and an authenticated `getmempoolstats` RPC
  that current Core master does not have. `bitcoind` leaves collection off by
  default, while the Qt startup path sets `-statsenable=1` unless the operator
  overrides it. When enabled, validation records memory-capped mempool samples
  on mempool admission and block connect/disconnect, including an initial
  zero-mempool sample during startup. This is not consensus behavior and not a
  remote unauthenticated exposure; it is a Knots-only memory/RPC surface and a
  GUI-default behavioral difference from Core. `rpc_mempoolstats.py` now covers
  the default-disabled daemon path and the enabled RPC/sample path, and the
  same test passes against unmodified Knots.

- ZMQ notification resilience and read-block logging:
  `1c4d2d54d8`, `268fb1e0e3`, `ba28af94bd`

  Current Core still shuts down and removes a ZMQ notifier after one failed
  notification and still routes raw-block disk-read failures through
  `zmq_strerror(errno)`. Knots keeps notifiers active after transient send/read
  failures and logs the failing block hash without using unrelated ZMQ errno
  text; the helper's name is stale in the port, but its body now matches
  Knots' "call all notifiers and do not erase failed ones" behavior. This is
  notification availability and diagnostics hardening, not a consensus issue.

- Fee-estimator file read bound-ordering guard:
  `163d3e5c13`

  Current Core still computes `scale * maxPeriods` before checking whether a
  serialized fee-estimates file tracks more than one week of confirmations, but
  current Core master uses `uint64_t` temporaries after `fa1d17d56c`, so this is
  not a confirmed current-Core overflow with the present serialized
  `unsigned int` scale and vector-size limits. Knots checks
  `scale > 1008 / maxPeriods` first and only multiplies after the corrupt-file
  bound has passed. This is local corrupt-file hardening/order-of-validation
  cleanup for `fee_estimates.dat`, not remote network exposure or consensus
  behavior. Source comparison confirmed actual Knots carries the same guard in
  its older `src/policy/fees.cpp`, while the port carries it in current Core's
  split `src/policy/fees/block_policy_estimator.cpp`; the port also has
  `policyestimator_tests/read_rejects_fee_estimates_with_oversized_scale`
  coverage.

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
  build before updating the port test. The latest verification reran the full
  functional test because the reorg helper depends on setup from `run_test`.

- Ephemeral-output policy knobs:
  Current Core has a hard-coded standardness allowance for at most one dust
  output. Knots adds `-permitbareanchor` and `-permitephemeral=<options>` to
  split that allowance into P2A-anchor, ordinary-send, and nonzero-dust pieces.
  The default Knots mode is `anchor,-send,-dust`, so ordinary P2TR zero-value
  dust is rejected with `dust-nonanchor`; `send,-dust` permits zero-value
  ordinary dust but still rejects nonzero dust with `dust-nonzero`; and
  `send,dust` permits nonzero ordinary dust when the package spends it. The
  `-corepolicy=1` interaction soft-sets the broader `anchor,send,dust` mode.
  The parser is deliberately permissive like Knots: unknown tokens are ignored
  and leave the default `anchor,-send,-dust` state. This is relay/mining
  standardness policy rather than consensus, but the functional test forces
  `-acceptnonstdtxn=0` so the option bits are actually exercised instead of
  being masked by regtest's usual nonstandard-transaction allowance. The
  strengthened unit and functional tests pass on the port, and the focused
  functional option test passes against unmodified Knots.

- Dynamic dust policy:
  Knots' `-dustdynamic=off|[<multiplier>*]target:<blocks>|[<multiplier>*]mempool:<kB>`
  is present in the port and absent from current Core. It keeps
  `dustrelayfeefloor` as the configured floor, then periodically raises
  `dustrelayfee` either from fee-estimator targets or from the fee rate found
  at a configured depth in this node's mempool. The option is a relay/mining
  standardness policy that can make low-value outputs non-standard when fee
  pressure is high; it is not a consensus rule, and old blocks/transactions are
  not made invalid. The parser rejects target values below two blocks, target
  values above the estimator horizon, mempool positions below one kB, and
  zero/invalid multipliers. Runtime checks on both the port and unmodified
  Knots with `-dustdynamic=mempool:250 -dustrelayfee=0.00001000` reported
  `dustdynamic: "3*mempool:250"` and preserved `dustrelayfeefloor` at
  `0.00001000`. Existing functional coverage also checks the RPC string and
  scheduler-updated dust feerate for both target- and mempool-based modes.

- TRUC policy modes:
  Knots' `-mempooltruc` option is present in the port and absent from current
  Core. It lets the operator choose `reject`, `accept`, or `enforce` for
  version-3/TRUC transactions. The actual Knots default is `accept`, so TRUC
  transactions are handled like ordinary transactions unless the operator opts
  into stricter policy. `-mempooltruc=enforce` or `-mempooltruc=1` enforces the
  TRUC topology and size limits; `-mempooltruc=reject` or `0` rejects TRUC
  transactions entirely; and `-mempooltruc=accept` or `-enforce` accepts them
  without imposing the TRUC-specific limits. The `-corepolicy=1` interaction
  soft-sets `-mempooltruc=enforce`. This is relay/mining policy, not
  consensus. The functional framework starts current binaries with
  `-corepolicy`, so the test's no-extra-args case reports `enforce`; explicit
  `-corepolicy=0` coverage now verifies the user-facing Knots default remains
  `accept`.

- Legacy mempool.dat compatibility:
  Knots' temporary `-persistmempoolv1` option is present in the port and absent
  from current Core. Current mempool.dat version 2 includes an obfuscation key
  and writes the rest of the file through that obfuscation stream. With
  `-persistmempoolv1=1`, Knots and the port write version 1 and skip the
  obfuscation key so older clients can read the file. This is local disk-format
  compatibility, not network or consensus behavior. The upstream-style
  `mempool_compatibility.py` test is present but skipped in this checkout
  because previous-release binaries are not available. Direct runtime checks
  using `savemempool` on empty mempools showed the first serialized uint64 is
  `1` for both the port and unmodified Knots when `-persistmempoolv1=1`.

- Sub-dust effective-fee penalty:
  Knots' `-subdustfeepenalty` is present in the port and absent from current
  Core. When enabled, every output below its dust threshold subtracts the
  missing amount from the transaction's modified fee. This makes sub-dust
  outputs pay enough extra fee to compensate for their future spend cost when
  nonstandard relay/mining is otherwise permitted. `OP_RETURN` outputs have a
  zero dust threshold and are not penalized. This is relay/mining fee policy,
  not consensus. The strengthened functional test checks the boundary
  rejection/acceptance behavior and now also admits a transaction to verify
  `getmempoolentry.fees.modified` is reduced by the sub-dust penalty.

  That strengthened test exposed a port-only invariant bug: the initial port
  updated the staged `CTxMemPoolEntry` modified fee directly, but current
  Core's `TxGraph` cache still held the pre-penalty fee. On regtest, where
  mempool consistency checks run by default, `sendrawtransaction` hit
  `CTxMemPool::check` with
  `diagram.back().fee == check_total_modified_fee`. Unmodified Knots passed
  the same strengthened test, so this was not an actual Knots bug. The port now
  routes staged modified-fee changes through `ChangeSet::UpdateModifiedFee`,
  which updates both the staged entry and the graph fee cache.

- ScriptPubKey-reuse mempool policy:
  Knots' `-spkreuse=0` mode is present in the port and absent from current Core.
  The port carries the later Knots pointer-based `mapUsedSPK` representation
  and routes inter-transaction scriptPubKey conflicts through the same
  replacement machinery as opt-in input conflicts, while keeping
  same-transaction twin outputs and input/change reuse as direct policy
  rejections. This is mempool policy for address/script reuse and spam
  reduction, not consensus. The strengthened port test covers both the direct
  twin-output rejection and the replacement-style inter-transaction conflict.
  Source comparison shows unmodified Knots has the same `m_conflicts_incl_policy`
  and `mapUsedSPK` handling; the full current port `mempool_accept.py` cannot
  be run unchanged against that Knots build because it expects a newer
  `getmempoolinfo.permitbaremultisig` result field before reaching the
  `-spkreuse` section.

- Maximum script-size relay/mining policy:
  Knots' `-maxscriptsize` mode is present in the port and absent from current
  Core. The default limit is 1650 bytes, and Knots applies it to output
  scriptPubKeys, spent output scriptPubKeys, P2SH redeem scripts, and the
  serialized aggregate witness stack. `-acceptnonstdtxn=1` soft-sets the limit
  to `uint32_t` max unless the operator explicitly supplies `-maxscriptsize`,
  so nonstandard relay intentionally disables this filter by default. This is
  relay/mining spam-filtering policy, not consensus: blocks can still contain
  larger valid scripts where consensus permits them. The new
  `mempool_maxscriptsize.py` test covers the externally visible
  `scriptpubkey-size`, `bad-txns-input-script-size`, and
  `bad-witness-witness-size` rejection paths, plus the
  `-acceptnonstdtxn=1` escape hatch. The same test passes against unmodified
  Knots, confirming these are inherited Knots semantics rather than a
  port-only behavior.

- Overlay-protocol reject filters:
  Knots' `-rejectparasites` (`3c732178d0`) and `-rejecttokens`
  (`2e7ea254c0`) modes are present in the port and absent from current Core.
  With `-rejectparasites=1`, transactions using locktime 21 are rejected with
  `parasite-cat21`. With `-rejecttokens=1`, Runes-style
  `OP_RETURN OP_13 <push>` outputs are rejected with `tokens-runes`, and
  OLGA-style data hidden behind P2WSH-looking outputs is rejected with
  `tokens-olga` when enough adjacent outputs are present. This is explicit
  relay/mining spam-filtering policy rather than consensus, and it should be
  described as a Knots-vs-Core policy divergence rather than a Core security
  bug. Existing `transaction_tests` cover the standardness helpers, and the
  new `mempool_reject_filters.py` functional test covers the RPC-visible
  default-allowed and flag-rejected cases. The same functional test passes
  against unmodified Knots.

- Data-carrier format and bare-carrier policy:
  Knots' `-acceptnonstddatacarrier`, `-datacarrierfullcount`, and
  `-permitbaredatacarrier` switches are present in the port and absent from
  current Core. They separate ordinary `OP_RETURN` size policy from
  non-`OP_RETURN` carrier detection and from transactions that contain only
  data-carrier outputs. The actual Knots default rejects non-standard carrier
  injection, applies `-datacarriersize` to all detected carrier methods, and
  rejects bare carrier-only transactions with `bare-datacarrier`.
  `-corepolicy=1` relaxes all three by soft-setting
  `-acceptnonstddatacarrier=1`, `-datacarrierfullcount=0`, and
  `-permitbaredatacarrier=1`; the functional framework starts current binaries
  with `-corepolicy`, so the strengthened test explicitly uses
  `-corepolicy=0` when checking the user-facing Knots defaults. This is
  relay/mining standardness policy, not consensus behavior: blocks can still
  include otherwise-consensus-valid carrier forms unless RDTS output limits are
  active. The strengthened `mempool_datacarrier.py` test now covers default
  non-standard OPNet rejection, full-count size rejection, and bare-carrier
  rejection/override, and the full test passes against both the port and
  unmodified Knots. The same cross-run confirmed an RPC-surface difference:
  the port exposes current-Core `getmempoolinfo` fields such as
  `maxdatacarriersize` and `permitbaremultisig`, while actual Knots does not,
  so those field assertions are gated on availability.

- Strict bytes-per-sigop policy:
  Knots' `-bytespersigopstrict` (`63934093bc`, side commit `81720c0870`) is
  present in the port and absent from current Core. Unlike `-bytespersigop`,
  which can raise a transaction's policy-adjusted vsize, the strict option
  rejects transactions whose accurate sigop cost is too high for their vsize
  with `bad-txns-too-many-sigops`. `-acceptnonstdtxn=1` soft-sets the strict
  threshold to zero unless the operator explicitly overrides it. This is
  relay/mining DoS-control policy, not consensus: the strengthened
  `mempool_sigoplimit.py` mines the same signed P2SH `CHECKSIG` transaction
  directly in a block after proving `-bytespersigopstrict=1000` rejects it from
  mempool. The focused strict-policy subtest passes against both the port and
  unmodified Knots.

- Data-carrier cost accounting:
  Knots' `-datacarriercost` (`d60de5916f`) is present in the port and absent
  from current Core. It does not change consensus validity and does not simply
  reject data outputs; instead it adds policy-only extra weight to mempool
  entries for bytes classified as embedded data, so fee-rate, ancestor/descendant
  size, eviction, mining, and PSBT fee sizing see a larger effective vsize.
  The default cost is one virtual byte per data byte; `-acceptnonstdtxn=1`
  soft-sets it to `0.25`, matching witness-discounted raw weight unless the
  operator overrides it. The strengthened `mempool_datacarrier.py` test proves
  that a standard OP_RETURN transaction has raw `vsize` under the default, but
  with `-datacarriercost=2` its `getmempoolentry.vsize` increases by one
  virtual byte per carrier byte. The focused method passes against both the
  port and unmodified Knots, confirming inherited Knots behavior.

- Bare pubkey relay option shadowed by output-size policy:
  Knots' `-permitbarepubkey` (`bc3479044b`, side commit `9259ac22ef`) is
  present in the port and absent from current Core, but the current Knots
  output-size mempool check rejects compressed P2PK outputs first. A compressed
  bare-pubkey script is 35 bytes, while Knots' reduced-data output-size limit
  is 34 bytes for non-`OP_RETURN` outputs, so `testmempoolaccept` returns
  `bad-txns-vout-script-toolarge` even with `-permitbarepubkey=1` or
  `-acceptnonstdtxn=1`. The same transaction can still be mined directly on
  ordinary regtest before RDTS activation, so this is not a pre-activation
  consensus rule; after RDTS activation, the output-size limit is consensus and
  the option cannot make such outputs valid. The new `mempool_bare_pubkey.py`
  test passes against both the port and unmodified Knots, confirming this is
  inherited Knots behavior rather than a port-introduced regression. The audit
  classification is a surprising option interaction / stale policy surface, not
  a remote crash or Core security fix.

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
  `maxfeerate` that would be too low if plain transaction vsize were used; the
  latest unmodified-Knots check uses the direct `--test_methods` form rather
  than the older one-off subclass.

- Legacy-sigop transaction standardness:
  `204b965915`, `538182b27e`

  Current Core now carries the BIP54-style policy limit on potentially
  executed non-witness sigops per transaction. The remaining Knots/port
  divergence is the exposed `-maxtxlegacysigops` threshold and the specific
  reject/ignore surface: `bad-txns-input-sigops-toomany-overall` with debug
  text `non-witness sigops exceed bip54 limit`, rather than Core's generic
  `bad-txns-nonstandard-inputs` reason. This is relay/mining policy
  configurability and diagnostics, not a consensus rule: the functional test
  rejects the over-limit transaction from mempool, then mines the same
  transaction directly in a block.

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
  miner-incentive hardening, not a consensus-rule change. The current source
  comparison shows Core funds the PSBT and immediately constructs the returned
  PSBT, while Knots and the port assign `rawTx = CMutableTransaction(*txr.tx)`
  and call `MaybeDiscourageFeeSniping2(...)` when the locktime parameter was
  omitted.

- PSBT previous-transaction injection for RPC updating/signing:
  `bdb4ca4195`, `eea8588f07`

  Current Core master can populate PSBT input UTXOs from the UTXO set, mempool,
  and txindex, but `utxoupdatepsbt` and `descriptorprocesspsbt` do not accept
  caller-provided previous transaction hex. Knots and this port can use those
  provided transactions to fill and sign dependent PSBT chains that are not yet
  in the mempool or UTXO set. This is transaction-construction functionality,
  not consensus behavior. The current source comparison shows Knots and the
  port parse `prevtxs`, reject duplicate provided txids, and search those
  transactions before txindex/mempool, while current Core still exposes only
  descriptors for `utxoupdatepsbt` and has no `descriptorprocesspsbt` `prevtxs`
  option.

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
  functionality, not consensus behavior. Fresh source comparison shows Core
  still throws `Block header missing` before calling `FetchBlock`, while Knots
  and the port pass the hash and optional block index through to peerman.
  Peerman-side comparison also shows Core calls `RemoveBlockRequest(...)`
  before duplicate detection, while Knots and the port check
  `IsBlockRequestedFromPeer(...)` first.

High-signal hardening already present in Core under the same or different
commits and therefore not counted as missing here: secp256k1 ellswift overflow
key handling, `LocalServiceInfo::nScore` saturation, miner `addPackageTxs`
overflow, compact-block witness mutation checks and repeated-`blocktxn`
empty-header guard, `LoadChainTip` UB,
reindex-chainstate periodic dbcache flushes (`ac7c0590ef`, rebased from Core
`84820561dc`; current Core carries this through `1d4e3d1b18`),
requested-block `ReadBlock(..., expected_hash)` checks in net processing,
equal-work active-chain tie-break persistence across restart and complex
reorgs (`5689ba8fde`, `b1378e3f48`, `dbca0cc4d3`, and `24ffe06d2f`),
BaseIndex rewind no-commit state persistence (`16b1710d97`) and stale
`current_tip == m_best_block_index` assert removal (`c4de297c26`),
`SetStdinEcho` UB, fd-limit overflow/RLIMIT_INFINITY handling, RPC credentials
hashed in memory, PSBT bounds asserts, v2-to-v1 reconnect UAF, randomized Tor
stream-isolation credential prefixes, feebumper combined-fee crash, wallet
coin-selection boolean amount fix, precomputed transaction-data lifetime
hardening (CVE-2024-52911), the CVE-2025-46598 validation/script-cache and
transaction-punishment cleanup cluster, Tor-control excessive-line OOM
hardening, I2P SAM
`SESSION CREATE` request redaction, BDB overflow data lengths, btree-level
validation, and final-page LSN validation, PSBT proprietary-field preservation
during combining, monotonic
`uptime`, first-run pruned-disk-space warning rounding, Windows exclusive `wbx`
opens, LevelDB file-size initialization, wallet `sendall` transaction-size
error handling, miniscript assert guards, and most cpp-subprocess
memory/Windows fixes, witness-stripped SegWit reject-filter handling,
miniscript `FindChallenges` stack-overflow avoidance in tests, P2P
`-capturemessages` option caching, single-timepoint inactivity checks, and
clean success exit status for initialization interrupted by shutdown. The old
Knots/libevent HTTP listen-socket cleanup fix is structurally avoided on this
current-Core base because the port uses Core's native `HTTPServer` with RAII
`Sock` ownership rather than `evhttp_accept_socket_with_handle`. A follow-up
patch-id audit also found that current Core already
has the MiniMiner negative-fee assumption removal, peer/peeraddr log comma
restoration through `CNode::LogPeer()`, and lazy `decodepsbt` result-doc
initialization. The Knots `getblock_vin` lazy-init follow-up is structurally
avoided in current Core and this port because `getblock()` is registered as an
`RPCMethod` factory, rather than by namespace-scope `RPCResult` construction.

The precomputed transaction-data lifetime item was rechecked because Knots
backports it as `29b4e281a7` and its commit message identifies
CVE-2024-52911: an invalid block could cause queued parallel script checks to
read freed `PrecomputedTransactionData`. Current Core master already carries
the upstream fix as `1ed799fb21`, and this port inherits the same shape in
`ConnectBlock()`: `txsdata` is constructed before the check-queue control, so
it is destructed after queued checks drain. Actual Knots has the equivalent
29.x backport. The focused
`txvalidationcache_tests --catch_system_error=no --log_level=error
--report_level=short` run passed on the port.

The CVE-2025-46598 backport cluster was also rechecked because Knots merged the
Bitcoin Core PR 33788 fixes through `6e7ea3cf2a` and earlier 29.x merge
commits: witness-stripping detection (`97088fa75a`, `56626300b8`,
`020ed613be`), sighash midstate caching and tests, single script-check
validation (`be0857745a`), and removal of transaction-relay peer punishment
(`65bcbbc538`). Current Core master, actual Knots, and the port all carry the
same live behavior: a witness-stripped spend is classified as
`TX_WITNESS_STRIPPED` without rerunning script checks, `MaybePunishNodeForTx`
is absent, and the sighash cache tests are present. This is not Core-missing
hardening. Focused port verification passed with `build/bin/test_bitcoin
--run_test=sighash_tests/sighash_caching --catch_system_error=no
--log_level=error --report_level=short` and `build/bin/test_bitcoin
--run_test=transaction_tests/spends_witness_prog --catch_system_error=no
--log_level=error --report_level=short`.

The coins-cache fuzz P2SH fixture fix is also already present from current
Core rather than replayed as the Knots hash. Knots backports it as
`aa46f48dae`, while this branch inherits upstream Core `ac58e6c53c`; both
commits move the generated P2SH script's final `OP_EQUAL` byte to index 22 of
the 23-byte scriptPubKey. The port, current Core, and unmodified Knots all have
that final-tree shape. This is fuzz coverage correctness, not a runtime
consensus or policy behavior change. A throwaway fuzz build compiled
`coinscache_sim.cpp.o` with `BUILD_FOR_FUZZING=ON`.

The mempool/orphanage empty-state iteration optimizations are also already
present in current Core and therefore are not Core-missing hardening. Knots
backports the `removeForBlock` IBD fast path as `8990a80618` and the orphanage
empty-check as `fc5361a515`; current Core carries them as `41ad2be434` and
`249889bee6`. The port has the same behavior and `75027d3139` only tidies the
rebased `removeForBlock` guard's formatting/comment around Knots'
priority-update code. This is IBD/performance hardening, not consensus or
mempool policy semantics. Focused `mempool_tests`, `txvalidation_tests`, and
`orphanage_tests` passed after the cleanup.

The `LoadChainTip` entry above covers Knots `33329f812e` and its follow-up
`ee42cf3e`. Knots first mitigated the comparator UB by erasing/reinserting
candidate entries around `nSequenceId` mutation, then fixed that mitigation to
erase/reinsert `target` rather than the loop's `tip`. Current Core carries a
stronger successor (`854a6d5a9a` plus test extension `20ae9b98ea`): all
chainstates load their tips while `setBlockIndexCandidates` is empty, then
`PopulateBlockIndexCandidates()` rebuilds the candidate sets after the
`nSequenceId` mutations are complete. The port matches that source structure
and now matches Core's functional regression test after removing the stale
duplicate method noted above.

## Open Risks

- Legacy-wallet creation is a non-consensus divergence from Knots on this
  current-Core base. Knots source still supports new legacy wallet creation
  when compiled with BDB; Core master no longer creates new legacy wallets, and
  this port now preserves Core's explicit RPC error instead of crashing. Ported
  legacy-only wallet tests are skipped by the framework when `--legacy-wallet`
  mode is selected, so BDB-enabled legacy creation remains unverified and
  unported here.
- BIP-110/RDTS consensus equivalence remains the main consensus-risk area
  because it is an intentional soft-fork divergence from Core. The current pass
  mapped the Knots RDTS subjects to replayed/adapted port commits, compared the
  consensus-critical source paths against Knots, and reran the focused RDTS,
  UTXO-height, temporary-deployment, versionbits, script, tx-validation, and
  P2P handshake tests. That is focused regression evidence and source-level
  adaptation evidence, not a full independent consensus-equivalence proof or
  exhaustive cross-client block corpus.
- `RDTS_CONSENT=UNSUPPORTED_UNSAFE_NO_ENFORCEMENT` is an explicit unsupported
  build-time escape hatch carried from Knots. It can produce a node that does
  not enforce RDTS and does not advertise `NODE_REDUCED_DATA` unless the
  operator also supplies `consensusrules=rdts`. This is intentional Knots
  behavior, not a port-introduced bug, but it is consensus-divergent from the
  default Knots/RDTS configuration and should not be hidden in a release note.

## Verification

Source/manifest checks:

- `comm -23 <(git ls-tree -r --name-only knots/29.x-knots | sort)
  <(git ls-files | sort)`
- `rg -n "FindLibevent|libevent|mempool-limits|system_ram\\.h|core_write\\.cpp|policy/fees|fees_args|support/events\\.h|compilerbug_tests|policy_fee_tests|raii_event_tests|test/util/index|test/util/str|txorphanage\\.h|txorphanage\\.cpp|epochguard|transaction_identifier\\.h|mempool_package_onemore|rpcauth-test" . -g '!test/cache/**' -g '!build/**' -g '!depends/work/**' -g '!depends/built/**' -g '!depends/sources/**'`
- `rg -n "Clang_IndVarSimplify_Bug_SanityCheck|Compiler optimization sanity check|leveldb_major_version|EMBEDDED_LEVELDB" ../knots/src/kernel/checks.cpp ../knots/src/kernel/checks.h ../knots/src/test/sanity_tests.cpp ../knots/src/dbwrapper.cpp ../knots/src/dbwrapper.h ../knots/src/CMakeLists.txt ../knots/cmake -g '!**/build*/**'`
- `ls -la ../knots/build-repro/bin` showed only `bitcoind` and `bitcoin-cli`,
  so `../knots/build-repro/bin/test_bitcoin --run_test=sanity_tests` was not
  available.
- `git show --stat --patch --minimal 603f4958d0`,
  `git -C ../knots show --stat --patch --minimal 2f0e5090a7`, and `rg -n
  "UNSUPPORTED_UNSAFE_NO_ENFORCEMENT|g_enable_rdts|NODE_REDUCED_DATA|RDTS is not enabled|fake or fraudulent"
  CMakeLists.txt src/init.cpp src/kernel/chainparams.* src/validation.cpp`
  show the port and actual Knots both carry the explicit unsupported build mode
  that disables RDTS deployment and drops `NODE_REDUCED_DATA` until
  `consensusrules=rdts` is configured.
- `git show --stat --oneline 56b84e44fd 87c701dc1d 671097f199
  0f63eae265 404e44d108 ba57ec2c31 538f4eda40` maps the current port's
  adapted RDTS activation, service, consent, and bypass-hardening commits.
- `rg -n
  "reduced_data_start_height|flags_per_input|CheckOutputSizes\\(|generation tx|SCRIPT_VERIFY_REDUCED_DATA|DISCOURAGE_UPGRADABLE|maxstaleoutbound"
  src/validation.cpp src/consensus/tx_verify.cpp
  src/test/txvalidationcache_tests.cpp test/functional/feature_rdts.py
  test/functional/feature_reduced_data_utxo_height.py
  test/functional/p2p_handshake.py` shows the current source/test coverage for
  the RDTS activation-boundary and BIP110 service-limit paths.
- `git -C ../knots show --stat --patch --minimal 15805060ec`,
  `rg -n "ReadBlock\\(.*inv\\.hash|ReadBlock\\(.*req\\.blockhash"
  src/net_processing.cpp`, `git show origin/master:src/net_processing.cpp |
  rg -n "ReadBlock\\(.*inv\\.hash|ReadBlock\\(.*req\\.blockhash"`, and the
  equivalent `../knots` check show that the P2P block-serving expected-hash
  guard is present in current Core, actual Knots, and the port rather than
  being a remaining Core-missing hardening item. `build/bin/test_bitcoin
  --run_test=blockmanager_tests --catch_system_error=no --log_level=error
  --report_level=short` covers the raw-`FlatFilePos` mismatch path used by
  those call sites.
- `git log HEAD --oneline --regexp-ignore-case --extended-regexp
  --grep='bulk serialization|low I/O|IO priority|recomputing block hash|LoadExternalBlockFile'
  -- src/node/blockstorage.cpp src/node/blockstorage.h src/streams.h
  src/streams.cpp src/util/ioprio.cpp src/util/ioprio.h src/validation.cpp
  src/net_processing.cpp src/test/blockmanager_tests.cpp
  src/test/streams_tests.cpp`, the equivalent `origin/master` and
  `../knots 29.x-knots` checks, and `git grep -n
  "ioprio\\|lowprio\\|ReadBlock("` across those source paths map Knots'
  bulk/read-hash/low-I/O stream to the current port. `origin/master` has the
  bulk read/write and expected-hash commits but no `ioprio` or `lowprio`
  matches; actual Knots and the port both lower priority for peer-served block
  reads, startup verification, rollback, and `LoadExternalBlockFile`.
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=streams_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=blockmanager_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_readblock_hash_mismatch
  --catch_system_error=no --log_level=error --report_level=short`
- `git show --stat --patch --minimal 577c04c80e`,
  `git grep -n
  "SettingToBool\\|GetBoolArg\\|VNUM\\|getValStr\\|setting_args" HEAD
  origin/master -- src/common/args.cpp src/common/args.h
  src/test/getarg_tests.cpp`, and the equivalent `../knots 29.x-knots` grep
  show that actual Knots and the port interpret numeric `settings.json`
  booleans through `getValStr()`, while current Core still throws on numeric
  values in `SettingToBool(...)` and pins that behavior in `getarg_tests`.
- `build/bin/test_bitcoin --run_test=getarg_tests/setting_args
  --catch_system_error=no --log_level=error --report_level=short`
- `git show --stat --patch --minimal 1f40813aac ceefc86bcf fc538327e0`
  and `git -C ../knots show --stat --patch --minimal 4f06564f36 4c62285bb9
  58ceddd389` map the port's first-start block-storage warning series to
  actual Knots. `git grep -n
  "AssumedBlockchainSize\\|assumed_chain_bytes\\|additional_bytes_needed\\|may not accommodate"
  HEAD origin/master -- src/init.cpp src/kernel/chainparams.h` and the
  equivalent `../knots 29.x-knots` grep show that current Core still uses
  `1_GiB` for the warning threshold and reports
  `chainparams.AssumedBlockchainSize()`, while Knots and the port use decimal
  GB, report the selected full/pruned byte count, and round up.
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=node_init_tests/block_storage_space_warning_units
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=node_init_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=util_tests/ceil_div_test
  --catch_system_error=no --log_level=error --report_level=short`
- `git show --stat --patch --minimal 10e7dc80ee d95f134d11 ebf7df30c2
  da690b5a69 f9f7587b59`, the equivalent `../knots 29.x-knots` show, and
  `git grep -n
  "g_low_memory_threshold\\|SystemNeedsMemoryReleased\\|-lowmem\\|mempressure"
  HEAD origin/master -- src test cmake CMakeLists.txt` show that actual Knots
  and the port carry the low-memory dbcache flush series while current Core does
  not.
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=validation_flush_tests
  --catch_system_errors=no`
- `build/bin/test_bitcoin --run_test=validation_chainstate_tests
  --catch_system_errors=no`
- `test/functional/feature_init.py --configfile=build/test/config.ini
  --cachedir=test/cache`
- `git show --stat --patch --minimal 97130ac516`,
  `git -C ../knots show --stat --patch --minimal 97130ac516`, and
  `git grep -n
  "AdviseSequential\\|CloseAndUncache\\|BufferedFile\\|posix_fadvise\\|FADV"
  HEAD origin/master -- src test cmake CMakeLists.txt` show that actual Knots
  and the port advise/uncache `BufferedFile` block-file reads while current Core
  has no corresponding helper or hook.
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=fs_tests --catch_system_errors=no`
- `build/bin/test_bitcoin --run_test=streams_tests --catch_system_errors=no`
- `git show --stat --patch --minimal 1ba5009294 8fad5801e0 d2c1bd10db`,
  `git show origin/master:src/init.cpp | rg -n
  "shutdown_request|Interrupt\\(|m_tip_block_cv|notify_all" -C 5`,
  `git -C ../knots show 29.x-knots:src/init.cpp | rg -n
  "shutdown_request|Interrupt\\(|m_tip_block_cv|notify_all" -C 5`, and
  `rg -n
  "shutdown_request|m_tip_block_cv|waitTipChanged|waitfornewblock|waitforblockheight|rpcthreads"
  src/init.cpp src/node/interfaces.cpp src/rpc/blockchain.cpp
  test/functional/feature_shutdown.py test/functional/feature_init.py
  test/functional/test_framework/util.py` show the port and actual Knots retain
  the direct `shutdown_request` wait-wakeup while current Core only wakes these
  long-poll waiters through the later `Interrupt()` path.
- `git show origin/master:src/init.cpp | rg -n
  "proxy=.*\\[=<network>|net_str == \"tor\"|net_str == \"onion\"" -C 3`
  shows current Core advertises per-network `-proxy` selectors including
  `tor`, but the parser branch accepts only `onion`; `rg -n
  "proxy=.*\\[=<network>|net_str == \"tor\"|net_str == \"onion\"|proxy=127\\.2\\.2\\.2:2222=tor"
  src/init.cpp test/functional/feature_proxy.py ../knots/src/init.cpp
  ../knots/test/functional/feature_proxy.py` shows the port and actual Knots
  accept `tor || onion` and cover the `=tor` spelling in the functional test.
- `git show origin/master:src/wallet/db.cpp | sed -n '20,70p'` and
  `git -C ../knots show 29.x-knots:src/wallet/db.cpp | sed -n '20,75p'`
  show that current Core lacks Knots' `ignore_paths` skip list in
  `ListDatabases(...)`.
- `git show --stat --patch --minimal dbca0cc4d3e 5689ba8fde b1378e3f48
  24ffe06d2f`, `rg -n
  "SEQ_ID_BEST_CHAIN_FROM_DISK|SEQ_ID_INIT_FROM_DISK|feature_chain_tiebreaks|setBlockIndexCandidates|nSequenceId"
  src/chain.h src/node/blockstorage.cpp src/validation.cpp
  test/functional/feature_chain_tiebreaks.py test/functional/test_runner.py`,
  and equivalent `origin/master` and `../knots` source checks show the
  equal-work chain-tip tie-break persistence across restarts and complex
  reorgs is already carried by current Core, actual Knots, and the port. This
  is a reorg/restart safety check, not a remaining Core shortcoming.
- `git -C ../knots show --stat --patch --minimal
  33329f812e1a159d0b6209fc826050e90d2bf4a3
  ee42cf3ec5cf97d31836789e778a89cb341af600`, `git show --stat --patch
  --minimal 854a6d5a9a 20ae9b98ea`, `rg -n
  "PopulateBlockIndexCandidates|setBlockIndexCandidates|SEQ_ID_BEST_CHAIN_FROM_DISK|LoadChainTip"
  src/validation.cpp src/validation.h src/node/chainstate.cpp
  src/test/validation_chainstatemanager_tests.cpp
  test/functional/feature_chain_tiebreaks.py`, and `git diff --exit-code
  origin/master -- test/functional/feature_chain_tiebreaks.py` show the port
  uses Core's stronger LoadChainTip/candidate-set fix and now executes Core's
  matching functional regression test. Before `5c6196c636`, a stale duplicate
  Knots method in the port made Python select the older two-block disk test
  instead.
- `git -C ../knots show --stat --patch --minimal be0857745a5a0154d89a2aa9ddaa2a84e912598a`,
  `git show origin/master:src/validation.cpp | rg -n
  "mempool-script-verify-flag-failed|block-script-verify-flag-failed|mandatory-script-verify-flag-failed"
  -C 2`, and `rg -n
  "mempool-script-verify-flag-failed|block-script-verify-flag-failed|mandatory-script-verify-flag-failed|setscriptthreadsenabled"
  src/validation.cpp test/functional/feature_dersig.py
  test/functional/rpc_blockchain.py` show current Core and the port both use
  the single mempool script-check path, but only the port's disabled-thread
  inline block path had retained the older Knots `mandatory-script` label before
  `2feca940f4`.
- `git -C ../knots show --stat --patch --minimal
  97088fa75aa0af5355587ce3522320f459e35204`, `rg -n
  "TX_WITNESS_STRIPPED|SpendsNonAnchorWitnessProg|Witness program was passed an empty witness|with_witness=False"
  src/validation.cpp src/node/txdownloadman_impl.cpp
  test/functional/p2p_segwit.py`, and equivalent `origin/master` and
  `../knots` checks show the witness-stripped SegWit retry tests and
  reject-filter handling are present in current Core, actual Knots, and the
  port. The focused rerun
  `python3 test/functional/p2p_segwit.py --configfile build/test/config.ini --tmpdir=/mnt/my_storage/tmp_p2p_segwit_witness_stripping_2 --portseed=7394`
  passed after updating the port's stale block-failure label expectations to
  current Core's `block-script-verify-flag-failed`.
- `git -C ../knots show --stat --patch --minimal
  fbe185ce7a76e3c8d36042df27f9a34ec9a95cff`,
  `git show origin/master:src/httpserver.cpp`, and `rg -n
  "BindAndStartListening|unique_ptr<Sock>|m_listen|evhttp_accept_socket_with_handle"
  src/httpserver.cpp src/httpserver.h` show Knots' older libevent fix for
  closing a listen socket when `evhttp_accept_socket_with_handle` fails is not
  a remaining port gap: current Core and the port hold the socket in a
  `std::unique_ptr<Sock>` until successful bind/listen registration.
- `git -C ../knots show --stat --patch --minimal
  5da98f931d777fa13b3e4804dc01f3e84f1a32c1`, `git show
  origin/master:src/test/miniscript_tests.cpp`, and `rg -n
  "FindChallenges|std::vector stack" src/test/miniscript_tests.cpp` show the
  miniscript challenge traversal stack-overflow avoidance is already present in
  current Core and the port's test code. Focused verification:
  `build/bin/test_bitcoin --run_test=httpserver_tests --catch_system_error=no --log_level=error --report_level=short`
  and `build/bin/test_bitcoin --run_test=miniscript_tests --catch_system_error=no --log_level=error --report_level=short`
  passed.
- `git -C ../knots show --stat --patch --minimal
  204b96591542373dc75c6a6401b477f4b6615e69
  538182b27e81556a3c72fbc61be1db60938edda6`, `rg -n
  "maxtxlegacysigops|MAX_TX_LEGACY_SIGOPS|CheckSigopsBIP54|bad-txns-input-sigops-toomany-overall|non-witness sigops exceed"
  src test/functional src/test`, and equivalent `origin/master` checks show the
  port and actual Knots carry the configurable BIP54-style legacy-sigop
  standardness limit and specific reject reason, while current Core has the
  fixed 2,500-sigop policy check but no `-maxtxlegacysigops` option. Focused
  verification:
  `build/bin/test_bitcoin --run_test=mempool_tests/MempoolMaxTxLegacySigopsParse --catch_system_error=no --log_level=error --report_level=short`,
  `build/bin/test_bitcoin --run_test=transaction_tests/max_standard_legacy_sigops --catch_system_error=no --log_level=error --report_level=short`,
  and `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini --test_methods test_legacy_sigops_stdness --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_legacy_sigops --portseed=7395`
  passed.
- `git show --stat --patch --minimal 16b1710d97` plus `rg -n
  "BaseIndex::Rewind|committed index state must never be ahead|SetBestBlockIndex\\(new_tip\\)|Commit\\("
  src/index/base.cpp` and equivalent `origin/master` and `../knots` checks
  show Core, Knots, and the port already avoid committing index state inside
  `BaseIndex::Rewind`, preventing the index state from being persisted ahead
  of the flushed chainstate after reorgs. `git show --stat --patch --minimal
  c4de297c26`, `git show origin/master:src/index/base.cpp | rg -n
  "assert\\(current_tip == m_best_block_index\\)|BaseIndex::Rewind" -C 3`, and
  the matching port/Knots checks show the older stale
  `current_tip == m_best_block_index` assert is also absent in all three trees.
- `rg -n
  "nMinimumChainWork|defaultAssumeValid|m_assumeutxo_data|height = 90|height = 120|hash_serialized|m_chain_tx_count|blockhash = uint256"
  src/kernel/chainparams.cpp`,
  `git show origin/master:src/kernel/chainparams.cpp | rg -n
  "nMinimumChainWork|defaultAssumeValid|m_assumeutxo_data|height = 90|height = 120|hash_serialized|m_chain_tx_count|blockhash = uint256"`,
  and `git -C ../knots show 29.x-knots:src/kernel/chainparams.cpp | rg -n
  "nMinimumChainWork|defaultAssumeValid|m_assumeutxo_data|height = 90|height = 120|hash_serialized|m_chain_tx_count|blockhash = uint256"`
  show the port matches current Core's newer chain security parameters and
  testnet4 assumeutxo data while unmodified Knots `29.x-knots` remains on
  older values with an empty testnet4 assumeutxo table. `git -C ../knots log
  --oneline --grep="assumeutxo" -- src/kernel/chainparams.cpp` shows the
  historical Knots height-90000 addition, and `git -C ../knots log --oneline
  --grep="test chain parameter" -- src/kernel/chainparams.cpp` shows the later
  `f1cc17e0f0` revert.
- `git -C ../knots show --stat --patch --minimal bb1949adc6`,
  `rg -n "checkpointData|908765|CheckBlock\\(908765|checkpoint_sanity"
  src/kernel/chainparams.cpp src/test/validation_tests.cpp`,
  `git -C ../knots show 29.x-knots:src/kernel/chainparams.cpp | rg -n
  "checkpointData|908765|546" -C 4`, and
  `git show origin/master:src/kernel/chainparams.cpp | rg -n
  "checkpointData|908765|546" -C 4` show the port matches actual Knots'
  latest mainnet checkpoint table and testnet3 checkpoint while current Core
  lacks the restored checkpoint data.
- `git show --stat --patch --minimal 99bd4320f8`,
  `git show origin/master:src/node/kernel_notifications.cpp | sed -n
  '28,48p'`, `git -C ../knots show 29.x-knots:src/node/kernel_notifications.cpp
  | sed -n '28,48p'`, and `sed -n '28,48p'
  src/node/kernel_notifications.cpp` show current Core still wraps
  `-alertnotify`'s `%s` substitution in single quotes, while actual Knots and
  the port use double quotes for Windows `cmd.exe` compatibility.
- `git show origin/master:src/net_processing.cpp | rg -n
  "previous compact block reconstruction attempt failed|header.IsNull"` and
  `git -C ../knots show 29.x-knots:src/net_processing.cpp | rg -n
  "previous compact block reconstruction attempt failed|header.IsNull"` show
  that current Core and unmodified Knots both carry the repeated-`blocktxn`
  empty-header guard that was missing from the port.
- `git show origin/master:src/net.cpp | sed -n '512,522p'`,
  `git show knots/29.x-knots:src/net.cpp | sed -n '536,544p'`, and
  `sed -n '529,536p' src/net.cpp` show that current Core gates outgoing
  whitelist permission application on `ConnectionType::MANUAL`, while actual
  Knots and the port pass `vWhitelistedRangeOutgoing` for every outgoing
  connection.
- `git show origin/master:src/net.cpp | sed -n '580,610p'`,
  `git -C ../knots show 29.x-knots:src/net.cpp | rg -n
  "NetPermissionFlags::Implicit|NetPermissionFlags::Addr" -C 6`, and
  `sed -n '588,612p' src/net.cpp` show current Core clears implicit whitelist
  permissions after adding relay/mempool/noban, while actual Knots and the port
  also add `NetPermissionFlags::Addr`.
- `git show origin/master:src/net_processing.h origin/master:src/node/peerman_args.cpp
  | rg -n "BLOCK_RECONSTRUCTION_EXTRA_TXN|blockreconstructionextratxnsize|max_extra_txs_size" -C 4`,
  `git -C ../knots show 29.x-knots:src/net_processing.h 29.x-knots:src/node/peerman_args.cpp
  | rg -n "BLOCK_RECONSTRUCTION_EXTRA_TXN|blockreconstructionextratxnsize|max_extra_txs_size" -C 4`,
  and `rg -n "BLOCK_RECONSTRUCTION_EXTRA_TXN|blockreconstructionextratxnsize|max_extra_txs_size"
  src/net_processing.h src/node/peerman_args.cpp src/test/peerman_tests.cpp
  test/functional/p2p_compactblocks_extratxs.py` show current Core has only
  the 100-entry count default, while actual Knots and the port have the
  32,768-entry default, 10 MB default size cap, fractional-MB parser, and
  functional coverage.
- `git show origin/master:src/init.cpp origin/master:src/node/peerman_args.cpp
  2>/dev/null | rg -n "maxorphantx|max_orphan_txs"` returns no matches, while
  `rg -n "maxorphantx|max_orphan_txs|DEFAULT_MAX_ORPHAN_TRANSACTIONS"
  src/init.cpp src/node/peerman_args.cpp src/net_processing.h
  ../knots/src/init.cpp ../knots/src/node/peerman_args.cpp
  ../knots/src/net_processing.h` shows the option, parser, and 100-transaction
  default in both the port and unmodified Knots.
- `git show origin/master:src/kernel/mempool_options.h origin/master:src/policy/policy.cpp
  | rg -n "acceptunknownwitness|scriptpubkey-unknown-witnessversion|WITNESS_UNKNOWN" -C 4`,
  `git -C ../knots show 29.x-knots:src/kernel/mempool_options.h 29.x-knots:src/policy/policy.cpp 29.x-knots:src/test/transaction_tests.cpp
  | rg -n "acceptunknownwitness|scriptpubkey-unknown-witnessversion|WITNESS_UNKNOWN" -C 4`,
  and `rg -n "acceptunknownwitness|scriptpubkey-unknown-witnessversion|WITNESS_UNKNOWN"
  src/kernel/mempool_options.h src/policy/policy.cpp src/node/mempool_args.cpp
  src/test/transaction_tests.cpp` show current Core lacks the option while
  actual Knots and the port expose and test the unknown-witness output policy
  toggle.
- `git show origin/master:src/validation.cpp origin/master:src/kernel/mempool_options.h
  origin/master:src/init.cpp origin/master:src/node/mempool_args.cpp | rg -n
  "minrelaymaturity|minrelaycoinblocks|GetCoinAge|bad-txns-input-immature" -C 4`,
  `git -C ../knots show 29.x-knots:src/validation.cpp 29.x-knots:src/kernel/mempool_options.h
  29.x-knots:src/init.cpp 29.x-knots:src/node/mempool_args.cpp | rg -n
  "minrelaymaturity|minrelaycoinblocks|GetCoinAge|bad-txns-input-immature" -C 4`,
  and `rg -n "minrelaymaturity|minrelaycoinblocks|GetCoinAge|bad-txns-input-immature"
  src/validation.cpp src/kernel/mempool_options.h src/init.cpp
  src/node/mempool_args.cpp test/functional/mempool_minrelay.py` show current
  Core lacks Knots' minimum relay input-age knobs while actual Knots and the
  port share the same validation rejection paths and functional coverage.
- `git -C ../knots show 29.x-knots:src/node/mempool_args.cpp
  29.x-knots:src/kernel/mempool_options.h | rg -n
  "minrelaycoinblocks|minrelaymaturity|uint64_t|GetIntArg" -C 4`,
  `sed -n '145,162p' src/node/mempool_args.cpp`, and
  `rg -n "test_minrelay_age_args|MempoolMinRelayAgeParse"
  src/test/mempool_tests.cpp test/functional/feature_config_args.py` show
  actual Knots assigns signed `GetIntArg` values into the unsigned
  `minrelaycoinblocks` option, while the port now rejects negative values for
  both minrelay age options.
- `git show --stat --patch --minimal b85232d7462`, `git show
  origin/master:src/init.cpp | rg -n
  "blockfilterindex_value|AllBlockFilterTypes|BlockFilterType::BASIC|certain indexes|indexes for all known types"
  -C 5`, `git -C ../knots show 29.x-knots:src/init.cpp | rg -n
  "blockfilterindex_value|AllBlockFilterTypes|BlockFilterType::BASIC|certain indexes|indexes for all known types"
  -C 5`, and `rg -n
  "blockfilterindex_value|AllBlockFilterTypes|BlockFilterType::BASIC|certain indexes|indexes for all known types"
  src/init.cpp test/functional/rpc_getblockfilter.py` show bare
  `-blockfilterindex` still enables all known filter types in current Core, but
  only the selected default set (`basic`) in actual Knots and the port.
- `git show origin/master:src/init.cpp | rg -n
  "peerbloomfilters|bloomfilter@127|whitelist_opts|NODE_BLOOM" -C 4`,
  `git -C ../knots show 29.x-knots:src/init.cpp | rg -n
  "peerbloomfilters|bloomfilter@127|whitelist_opts|NODE_BLOOM" -C 4`, and
  `rg -n "peerbloomfilters|bloomfilter@127|whitelist_opts|NODE_BLOOM"
  src/init.cpp test/functional/p2p_filter.py` show current Core only toggles
  global `NODE_BLOOM`, while actual Knots and the port add implicit localhost
  `bloomfilter` permissions when `-peerbloomfilters` is unspecified and global
  `NODE_BLOOM` is off.
- `git show origin/master:src/wallet/migrate.cpp | rg -n
  "for \\(uint32_t i = 0; i <= outer_meta.last_page|LSNs are not reset"` and
  `git -C ../knots show 29.x-knots:src/wallet/migrate.cpp | rg -n
  "for \\(uint32_t i = 0; i <= outer_meta.last_page|LSNs are not reset"` show
  that current Core, actual Knots, and the port all scan the final Berkeley DB
  page when checking LSN cleanliness.
- `git show origin/master:src/common/args.cpp | rg -n -C 12
  "SettingToBool"` and `git show origin/master:src/test/getarg_tests.cpp |
  rg -n -C 8 "set_foo\\(99\\)|set_foo\\(3\\.25\\)|set_foo\\(0\\)"`
  show current Core still uses the older `value.get_str()` numeric-setting
  path and expects numeric `GetBoolArg` values to throw, while Knots
  `577c04c80e` changes those cases to `InterpretBool(value.getValStr())`.
- `git -C ../knots show --stat --patch --minimal 4f06564f36`,
  `git show origin/master:src/init.cpp | rg -n
  "AssumedBlockchainSize|assumed_chain_bytes|1_GiB|1'000'000'000|Disk space for" -C 4`,
  `git -C ../knots show 29.x-knots:src/init.cpp | rg -n
  "AssumedBlockchainSize|assumed_chain_bytes|1_GiB|1'000'000'000|Disk space for" -C 4`,
  and `rg -n
  "AssumedBlockchainSize|assumed_chain_bytes|1_GiB|1'000'000'000|Disk space for"
  src/init.cpp` show current Core still uses `1_GiB` for the first-startup
  block-storage warning threshold while actual Knots and the port use decimal
  `1'000'000'000` bytes.
- `git -C ../knots show --stat --patch --minimal 35698ab7f4`,
  `git show origin/master:src/init.cpp | rg -n -C 5 --
  "-reindex|do_reindex|Automatically running|reindex_after_failure|auto"`,
  `git -C ../knots show 29.x-knots:src/init.cpp | rg -n -C 5 --
  "-reindex|do_reindex|Automatically running|reindex_after_failure|auto"`,
  and `rg -n --
  "-reindex|do_reindex|Automatically running|reindex_after_failure|init_auto_reindex_test"
  src/init.cpp test/functional/feature_init.py` show Knots and the port accept
  `-reindex=auto` and retry chainstate loading with `do_reindex=true` after a
  startup failure, while current Core still has only boolean `-reindex` plus
  the GUI/user-prompt retry path.
- `git -C ../knots show --stat --patch --minimal 2104df3209`,
  `git show origin/master:src/kernel/caches.h origin/master:src/txdb.h
  origin/master:src/init.cpp | rg -n
  "DEFAULT_DB_CACHE_BATCH|dbbatchsize|batch_write_bytes|32_MiB|64_MiB" -C 3`,
  `git -C ../knots show 29.x-knots:src/txdb.h
  29.x-knots:src/node/coins_view_args.cpp 29.x-knots:src/init.cpp | rg -n
  "nDefaultDbBatchSize|dbbatchsize|batch_write_bytes|64 << 20" -C 4`, and
  `rg -n "DEFAULT_DB_CACHE_BATCH|nDefaultDbBatchSize|dbbatchsize"
  src/kernel/caches.h src/txdb.h src/test/caches_tests.cpp src/init.cpp` show
  actual Knots uses a 64 MiB `-dbbatchsize` default, current Core uses 32 MiB,
  and the port now carries the 64 MiB default at Core's current constant
  location without the stale unused `nDefaultDbBatchSize`.
- `git -C ../knots show --stat --patch --minimal 10e7dc80ee`,
  `git show origin/master:src/init.cpp origin/master:src/validation.cpp
  origin/master:src/common/system.cpp origin/master:src/common/system.h | rg
  -n "lowmem|SystemNeedsMemoryReleased|HAVE_LINUX_SYSINFO|memory pressure|FlushStateToDisk\\(state, FlushStateMode::IF_NEEDED\\)|g_low_memory_threshold"
  -C 5`, `rg -n
  "g_low_memory_threshold|SystemNeedsMemoryReleased|AvailableMemory|lowmem|freeram|bufferram|GlobalMemoryStatusEx|sysinfo"
  src cmake CMakeLists.txt test/functional/feature_init.py src/test`, and
  the matching actual-Knots source checks show that Knots and the port carry
  configurable low-memory-triggered dbcache flushing, while current Core lacks
  both the `-lowmem` option and the `SystemNeedsMemoryReleased()` flush hook.
- `git -C ../knots show --stat --patch --minimal
  97130ac516ffe729153122aacd0b8a23e0650100`,
  `git show origin/master:src/util/fs_helpers.cpp
  origin/master:src/util/fs_helpers.h origin/master:src/streams.h | rg -n
  "AdviseSequential|CloseAndUncache|posix_fadvise|POSIX_FADV|BufferedFile\\(AutoFile|~BufferedFile|int fclose\\(" -C 5`,
  `git -C ../knots show 29.x-knots:src/util/fs_helpers.cpp
  29.x-knots:src/util/fs_helpers.h 29.x-knots:src/streams.h | rg -n
  "AdviseSequential|CloseAndUncache|posix_fadvise|POSIX_FADV|BufferedFile\\(AutoFile|~BufferedFile|int fclose\\(" -C 5`,
  and `rg -n
  "AdviseSequential|CloseAndUncache|posix_fadvise|BufferedFile\\(AutoFile|int fclose\\(\\)|streams_buffered_file_closes_source"
  src/streams.h src/util/fs_helpers.cpp src/util/fs_helpers.h
  src/test/streams_tests.cpp` show actual Knots and the port wire the
  sequential-read and close-and-uncache helpers into `BufferedFile`, while
  current Core lacks those helpers and call sites.
- `git show origin/master:src/zmq/zmqnotificationinterface.cpp | rg -n
  "TryForEachAndRemoveFailed|notifier->Shutdown|notifiers.erase" -C 4`,
  `git -C ../knots show 29.x-knots:src/zmq/zmqnotificationinterface.cpp |
  rg -n "TryForEachAndRemoveFailed|notifier->Shutdown|notifiers.erase" -C 4`,
  and `rg -n "TryForEachAndRemoveFailed|notifier->Shutdown|notifiers.erase"
  src/zmq/zmqnotificationinterface.cpp -C 4` show current Core still shuts
  down and erases failed ZMQ notifiers, while Knots and the port keep the
  notifier list intact.
- `git show origin/master:src/zmq/zmqpublishnotifier.cpp | rg -n
  "Can't read block|zmqError" -C 3`, `git -C ../knots show
  29.x-knots:src/zmq/zmqpublishnotifier.cpp | rg -n
  "Can't read block|zmqError" -C 3`, and `rg -n
  "Can't read block|zmqError" src/zmq/zmqpublishnotifier.cpp -C 3` show
  current Core still logs raw-block disk-read failures via `zmqError`, while
  Knots and the port log the failing block hash directly.
- `git show origin/master:src/init.cpp | rg -n
  "zmqpub|ADDR_PREFIX_UNIX|ipc:" -C 4`,
  `git -C ../knots show 29.x-knots:src/init.cpp | rg -n
  "zmqpub|ADDR_PREFIX_UNIX|ipc:" -C 4`, and `rg -n
  "zmqpub|ADDR_PREFIX_UNIX|ipc:" src/init.cpp test/functional/interface_zmq.py
  -C 4` show current Core's `CheckHostPortOptions(...)` accepts only the
  `unix:` alias for ZMQ Unix-domain sockets, while Knots and the port also
  allow native libzmq `ipc://` URIs.
- `git show origin/master:src/net_processing.cpp | rg -n
  "cleanSubVer = SanitizeString|receive version message" -C 2`,
  `git -C ../knots show 29.x-knots:src/net_processing.cpp | rg -n
  "SAFE_CHARS_PRINTABLE|SanitizeString\\(cleanSubVer|receive version message"
  -C 2`, and `rg -n
  "SAFE_CHARS_PRINTABLE|log_subver|receive version message"
  src/net_processing.cpp src/util/strencodings.* test/functional/p2p_handshake.py`
  show that current Core strips user-agent printable punctuation before
  storing/logging, while Knots and the port preserve it for RPC display and
  escape it at receive-version log time.
- `git -C ../knots show 29.x-knots:src/node/blockmanager_args.cpp | rg -n
  "pruneduringinit|PRUNE_TARGET_MANUAL"` confirms actual Knots converts
  `-pruneduringinit=0` to manual pruning during init.
- `git show origin/master:src/node/transaction.cpp | sed -n '32,105p'`,
  `git show origin/master:src/rpc/mempool.cpp | sed -n '110,158p'`,
  `git -C ../knots show 29.x-knots:src/node/transaction.cpp | sed -n
  '34,100p'`, and `sed -n '32,92p' src/node/transaction.cpp && sed -n
  '136,154p' src/rpc/mempool.cpp` show current Core precomputes the raw
  transaction max-fee as a plain-vsize absolute amount, while Knots and the
  port pass `CFeeRate` into `BroadcastTransaction` and convert it with the
  mempool accept result's `m_vsize`.
- `git -C ../knots show --patch --minimal --no-ext-diff
  b3b512dc6ee6fc148c4d3255c4cf21de1aefca79 -- src/init.cpp
  test/functional/feature_rbf.py test/functional/p2p_node_network_limited.py`
  shows Knots removed local `NODE_REPLACE_BY_FEE` advertisement and changed the
  functional tests to assert the bit is absent.
- `git show origin/master:src/init.cpp origin/master:src/protocol.h
  origin/master:src/protocol.cpp origin/master:test/functional/feature_rbf.py
  origin/master:test/functional/p2p_node_network_limited.py
  origin/master:src/bitcoin-cli.cpp | rg -n
  "NODE_REPLACE_BY_FEE|REPLACE_BY_FEE|rbf_policy == RBFPolicy::Always|g_local_services.*REPLACE|expected_services"`
  shows current Core no longer carries the RBF service bit.
- `git -C ../knots show 29.x-knots:src/init.cpp 29.x-knots:src/protocol.h
  29.x-knots:src/protocol.cpp 29.x-knots:test/functional/feature_rbf.py
  29.x-knots:test/functional/p2p_node_network_limited.py
  29.x-knots:src/bitcoin-cli.cpp | rg -n
  "NODE_REPLACE_BY_FEE|REPLACE_BY_FEE|rbf_policy == RBFPolicy::Always|g_local_services.*REPLACE|expected_services"`
  and `rg -n
  "NODE_REPLACE_BY_FEE|REPLACE_BY_FEE|rbf_policy == RBFPolicy::Always|g_local_services.*REPLACE|expected_services"
  src/init.cpp src/protocol.h src/protocol.cpp test/functional/feature_rbf.py
  test/functional/p2p_node_network_limited.py src/bitcoin-cli.cpp` show Knots
  and the port keep the compatibility constant/display name but do not
  advertise it.
- `git show origin/master:src/policy/policy.h | rg -n
  "DEFAULT_BLOCK_MIN_TX_FEE|DEFAULT_INCREMENTAL_RELAY_FEE|DEFAULT_MIN_RELAY_TX_FEE"`
  and `git -C ../knots show 29.x-knots:src/policy/policy.h | rg -n
  "DEFAULT_BLOCK_MIN_TX_FEE|DEFAULT_INCREMENTAL_RELAY_FEE|CORE_INCREMENTAL_RELAY_FEE|DEFAULT_MIN_RELAY_TX_FEE"`
  show current Core's lower default fee constants and Knots' restored stricter
  defaults plus `CORE_INCREMENTAL_RELAY_FEE`.
- `build/bin/bitcoind -help | rg -n
  "corepolicy|incrementalrelayfee|blockmintxfee|minrelaytxfee|acceptnonstddatacarrier|datacarriercost|subdustfeepenalty"
  -C 2` shows the port exposes `-corepolicy` and the stricter default help
  values.
- `git show origin/master:src/init.cpp | rg -n
  "acceptnonstddatacarrier|datacarrierfullcount|permitbaredatacarrier|datacarriercost"`
  and the same search in `origin/master:src/node/mempool_args.cpp` return no
  matches, while `rg -n
  "acceptnonstddatacarrier|datacarrierfullcount|permitbaredatacarrier|datacarriercost"
  src/init.cpp ../knots/src/init.cpp` shows the switches and `-corepolicy`
  soft-sets in both the port and unmodified Knots.
- Manual runtime check: start `build/bin/bitcoind -regtest` in a clean datadir
  and query `getmempoolinfo`/`getmininginfo`; the default port returned
  `minrelaytxfee=0.00001000`, `incrementalrelayfee=0.00001000`, and
  `blockmintxfee=0.00001000`, while the same check with `-corepolicy=1`
  returned `0.00000100`, `0.00000100`, and `0.00000001`.
- Manual runtime check: start `build/bin/bitcoind -regtest` in a clean datadir
  with `-dustdynamic=mempool:250 -dustrelayfee=0.00001000` and query
  `getmempoolinfo`; the port returned `dustdynamic="3*mempool:250"`,
  `dustrelayfee=0.00001000`, and `dustrelayfeefloor=0.00001000`. The same
  command using `../knots/build-repro/bin/bitcoind` returned the same three
  values on unmodified Knots, except for unrelated result fields omitted by the
  older Knots RPC response.
- `python3 test/functional/mempool_compatibility.py --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_compat_persistv1_port
  --portseed=32870` skipped because previous-release binaries are unavailable
  in this checkout.
- Manual runtime check: start `build/bin/bitcoind -regtest` with
  `-persistmempoolv1=1`, run `savemempool`, and inspect
  `od -An -t u8 -N8 regtest/mempool.dat`; the port printed `1`. The same check
  with `../knots/build-repro/bin/bitcoind` also printed `1`.
- Manual runtime check: start `build/bin/bitcoind -regtest` in isolated
  datadirs and RPC ports with no `-corepolicy`, with `-corepolicy=0`, and with
  `-corepolicy=1`; `getmempoolinfo.truc_policy` returned `accept`, `accept`,
  and `enforce`. The same sequential check with
  `../knots/build-repro/bin/bitcoind` returned the same values. Earlier
  parallel attempts were discarded because multiple regtest daemons contended
  for the same default RPC port.
- `git show origin/master:src/policy/rbf.cpp origin/master:src/policy/rbf.h |
  rg -n "GetUniqueClusterCount|too many conflicting clusters|too many potential replacements|MAX_REPLACEMENT_CANDIDATES"`
  and `git -C ../knots show 29.x-knots:src/policy/rbf.cpp
  29.x-knots:src/policy/rbf.h | rg -n
  "GetCountWithDescendants|too many potential replacements|MAX_REPLACEMENT_CANDIDATES"`
  show current Core limits by affected cluster count, while Knots also checks
  potential replacement entries with descendants.
- `rg -n
  "GetUniqueClusterCount|CalculateDescendantData|too many conflicting clusters|too many potential replacements|MAX_REPLACEMENT_CANDIDATES"
  src/policy/rbf.cpp src/policy/rbf.h src/test/rbf_tests.cpp
  test/functional/feature_rbf.py` shows the port now enforces and tests both
  limits.
- `git show origin/master:src/wallet/rpc/spend.cpp | sed -n '1757,1815p'`,
  `git -C ../knots show 29.x-knots:src/wallet/rpc/spend.cpp | sed -n
  '1901,1940p'`, and `sed -n '1968,2002p' src/wallet/rpc/spend.cpp` show
  current Core's `walletcreatefundedpsbt` path returns the funded transaction
  as a PSBT without applying the default anti-fee-sniping locktime, while Knots
  and the port apply it when the explicit locktime argument is omitted.
- `git show origin/master:src/rpc/rawtransaction.cpp | rg -n
  "prevtxs|utxoupdatepsbt|descriptorprocesspsbt|ProcessPSBT" -C 5`,
  `git -C ../knots show 29.x-knots:src/rpc/rawtransaction.cpp | rg -n
  "prevtxs|utxoupdatepsbt|descriptorprocesspsbt|ProcessPSBT|prev_tx_map"
  -C 5`, and `rg -n
  "prevtxs|utxoupdatepsbt|descriptorprocesspsbt|ProcessPSBT|prev_tx_map"
  src/rpc/rawtransaction.cpp test/functional/rpc_psbt.py` show current Core's
  PSBT update/signing RPCs lack caller-provided previous transaction injection,
  while Knots and the port support it and test duplicate/irrelevant/short
  previous-transaction cases.
- `git show origin/master:src/rpc/blockchain.cpp | sed -n '524,585p'`,
  `git -C ../knots show 29.x-knots:src/rpc/blockchain.cpp | sed -n
  '499,545p'`, `sed -n '550,600p' src/rpc/blockchain.cpp`,
  `git show origin/master:src/rpc/client.cpp | rg -n
  "getblockfrompeer|peer_id|nodeid" -C 3`,
  `git -C ../knots show 29.x-knots:src/rpc/client.cpp | rg -n
  "getblockfrompeer|peer_id|nodeid" -C 3`,
  `git show origin/master:src/net_processing.cpp | sed -n '1987,2010p'`,
  `git -C ../knots show 29.x-knots:src/net_processing.cpp | sed -n
  '1888,1910p'`, and `sed -n '2055,2085p' src/net_processing.cpp` show
  current Core still requires `getblockfrompeer` callers to have the header and
  drops prior in-flight state before same-peer duplicate detection, and also
  lacks Knots' `nodeid` named-argument conversion entry, while Knots and the
  port can fetch by hash without a known header, preserve the duplicate
  same-peer error, and accept the `nodeid` alias.
- `git log origin/master --follow --oneline -- <remaining source-looking
  missing path>` for old `core_write`, fee, libevent, orphanage, transaction
  identifier, epochguard, and test-helper paths
- Direct import of `test/functional/test_runner.py` with
  `test/functional` on `sys.path` returned `missing_count 0`
- `bash -n contrib/guix/libexec/build.sh`
- `rg -n "DisconnectChainNotifications|ipc::Exception|client->stop"
  src/wallet src/init.cpp`

Builds:

- `cmake -B build -DRDTS_CONSENT=RUNTIME_WARN`
- `cmake --build build --target bitcoind bitcoin-cli test_bitcoin -j4`
- `TMPDIR=/mnt/my_storage/tmp cmake -S . -B
  /mnt/my_storage/tmp/bitcoin-rdts-unsafe-probe -DBUILD_DAEMON=ON
  -DBUILD_CLI=ON -DBUILD_TX=OFF -DBUILD_UTIL=OFF
  -DBUILD_UTIL_CHAINSTATE=OFF -DBUILD_KERNEL_LIB=OFF
  -DBUILD_WALLET_TOOL=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DENABLE_WALLET=OFF
  -DWITH_ZMQ=OFF -DWITH_USDT=OFF -DWITH_CCACHE=ON
  -DRDTS_CONSENT=UNSUPPORTED_UNSAFE_NO_ENFORCEMENT` configured successfully
  and emitted the expected unsupported/insecure warning.
- `TMPDIR=/mnt/my_storage/tmp cmake --build
  /mnt/my_storage/tmp/bitcoin-rdts-unsafe-probe --target bitcoind bitcoin-cli
  -j4`
- `bitcoind` from the unsafe probe build started on a fresh mainnet datadir
  without `-consensusrules=rdts`; `bitcoin-cli getnetworkinfo` showed no
  `REDUCED_DATA?` in `localservicesnames`, `getnetworkinfo` and
  `getblockchaininfo` warned that RDTS is not enabled, and `debug.log` included
  `This node will NOT enforce them`.
- The same unsafe probe build started on a fresh mainnet datadir with
  `-consensusrules=rdts`; `bitcoin-cli getnetworkinfo` showed
  `REDUCED_DATA?`, the RDTS-disabled warning was absent, and `debug.log`
  included `User already consented to 'rdts' consensus rules (in config)`.
- `cmake --build build --target bitcoind -j4`
- `cmake --build build --target bitcoin-cli -j4`
- `cmake --build build --target bitcoind bitcoin-cli -j4`
- `cmake -S . -B /tmp/bitcoin-zmq-build -DWITH_ZMQ=ON -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_GUI=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoin_zmq -j2`
- `cmake --build /tmp/bitcoin-zmq-build --target bitcoind bitcoin-cli -j2`
- `cmake -S . -B build-zmq -DWITH_ZMQ=ON -DBUILD_TESTS=OFF
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DBUILD_GUI=OFF
  -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build build-zmq --target bitcoind bitcoin-cli -j4`
- `cmake -S . -B /mnt/my_storage/build-zmq-audit -DWITH_ZMQ=ON
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF
  -DBUILD_GUI=OFF -DWITH_CCACHE=OFF -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /mnt/my_storage/build-zmq-audit --target bitcoind bitcoin-cli -j4`
- `cmake -S ../knots -B ../knots/build-zmq-audit -DWITH_ZMQ=ON
  -DRDTS_CONSENT=RUNTIME_WARN -DBUILD_GUI=OFF -DWITH_CCACHE=OFF`
- `cmake --build ../knots/build-zmq-audit --target bitcoind bitcoin-cli -j4`
- `cmake -S . -B /tmp/bitcoin-fuzz-wallet-bdb -DBUILD_FUZZ_BINARY=ON
  -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_GUI=OFF -DWITH_CCACHE=OFF
  -DRDTS_CONSENT=IMPLICIT`
- `cmake --build /tmp/bitcoin-fuzz-wallet-bdb --target fuzz -j4`
- `cmake -S . -B build-fuzz-check -GNinja -DBUILD_FOR_FUZZING=ON
  -DBUILD_FUZZ_BINARY=ON -DBUILD_GUI=OFF -DBUILD_BENCH=OFF
  -DBUILD_KERNEL_LIB=OFF`
- `ninja -C build-fuzz-check
  src/test/fuzz/CMakeFiles/fuzz.dir/__/__/wallet/test/fuzz/wallet_bdb_parser.cpp.o`
- `cmake -S . -B /tmp/bitcoin-fuzz-coinscache -GNinja
  -DBUILD_FOR_FUZZING=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_GUI=OFF
  -DBUILD_BENCH=OFF -DBUILD_KERNEL_LIB=OFF -DENABLE_WALLET=OFF
  -DWITH_ZMQ=OFF -DWITH_USDT=OFF -DWITH_CCACHE=OFF`
- `ninja -C /tmp/bitcoin-fuzz-coinscache
  src/test/fuzz/CMakeFiles/fuzz.dir/coinscache_sim.cpp.o`
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
- `cmake -LAH build | rg -n
  "BUILD_BITCOINCONSENSUS_LIB|BUILD_KERNEL_LIB|BUILD_SHARED_LIBS"`
- `cmake --build build --target bitcoinconsensus -j2`
- `build/bin/test_bitcoin --run_test=script_tests --catch_system_error=no
  --log_level=warning --report_level=short`
- `git diff --check`
- `cmake -LA -N build | rg -n
  "BUILD_GUI|BUILD_GUI_TESTS|WITH_QT|ENABLE_WALLET"` reported
  `BUILD_GUI=OFF`, `ENABLE_WALLET=ON`, and `WITH_QT_VERSION=5`
- `build/bin/bitcoind -help | sed -n '147,160p'`
- `cmake --build build --target test_bitcoin -j2`
- `cmake --build build --target bitcoin-qt -j4` failed with
  `ninja: error: unknown target 'bitcoin-qt'`
- `cmake --build build --target test_bitcoin-qt -j4` failed with
  `ninja: error: unknown target 'test_bitcoin-qt'`
- `cmake --build build --target help | rg -n
  "(qt|bitcoin-qt|test_bitcoin-qt|test_bitcoin_qt)"` returned no configured Qt
  target
- `rg -n "/clearhistory|ClearCommandHistory|Console Commands|Command history
  and console output cleared" src/qt/rpcconsole.cpp src/qt/rpcconsole.h
  src/qt/test/apptests.cpp`
- `rg -n "progressBar->setSizePolicy|statusBar\\(\\)->addWidget\\(progressBar,
  1\\)" src/qt/bitcoingui.cpp`
- `rg -n "class PlainCopyTextEdit" src/qt/rpcconsole.h` returned a single
  class definition
- `nl -ba src/qt/transactionfilterproxy.cpp | sed -n '51,127p'`
- `rg -n "dumptxoutset.*separator|separator.*dumptxoutset"
  src/rpc/client.cpp src/test/rpc_tests.cpp src/rpc/blockchain.cpp`
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
- `rg -n
  "rpcauthfile|rpcauth|wallet_restriction|m_wallet_restriction|Method not available for wallet-restricted"
  src/init.cpp src/httprpc.cpp src/rpc/request.h src/rpc/util.cpp
  src/rpc/util.h src/wallet/rpc/util.cpp test/functional/rpc_users.py`
- `rg -n
  "rpccookieperms|InterpretPermString|StringToOctal|ConvertPermsToOctal|norpccookieperms"
  src/httprpc.cpp src/util/fs_helpers.cpp test/functional/rpc_users.py`,
  `git show origin/master:src/httprpc.cpp origin/master:src/util/fs_helpers.cpp
  | rg -n
  "rpccookieperms|InterpretPermString|StringToOctal|ConvertPermsToOctal|norpccookieperms"
  -C 4`, and `git -C ../knots show 29.x-knots:src/httprpc.cpp
  29.x-knots:src/util/fs_helpers.cpp | rg -n
  "rpccookieperms|InterpretPermString|StringToOctal|ConvertPermsToOctal|norpccookieperms"
  -C 4`
- `git grep -n
  "rpcauthfile|wallet_restriction|m_wallet_restriction|Method not available for wallet-restricted"
  origin/master -- src/init.cpp src/httprpc.cpp src/rpc/request.h
  src/rpc/util.cpp src/rpc/util.h src/wallet/rpc/util.cpp
  test/functional/rpc_users.py` returned no matches for the Knots-only
  auth-file/wallet-restriction surface.
- `git -C ../knots grep -n
  "rpcauthfile|wallet_restriction|m_wallet_restriction|Method not available for wallet-restricted"
  29.x-knots -- src/init.cpp src/httprpc.cpp src/rpc/request.h
  src/rpc/util.cpp src/rpc/util.h src/wallet/rpc/util.cpp
  test/functional/rpc_users.py`
- `rg -n
  "CheckUserAuthorized|strRPCUserColonPass|user_colon_pass|GenerateAuthCookie|g_rpcauth.push_back|CHMAC_SHA256"
  src/httprpc.cpp src/rpc/request.cpp src/rpc/request.h`, `git show
  origin/master:src/httprpc.cpp | rg -n
  "CheckUserAuthorized|strRPCUserColonPass|user_colon_pass|GenerateAuthCookie|g_rpcauth.push_back|CHMAC_SHA256"
  -C 4`, and `git -C ../knots show 29.x-knots:src/httprpc.cpp | rg -n
  "multiUserAuthorized|CheckUserAuthorized|strRPCUserColonPass|user_colon_pass|GenerateAuthCookie|g_rpcauth.push_back|CHMAC_SHA256"
  -C 4` show cookie and `-rpcuser` credentials are hashed before storage in
  the port, current Core, and unmodified Knots; Knots' old
  `strRPCUserColonPass` spelling is only a local variable in
  `InitRPCAuthentication()`.
- `git show origin/master:src/common/pcp.cpp | rg -n
  "g_pcp_warn_for_unauthorized|NOT_AUTHORIZED|already_warned|Mapping failed"
  -C 4`, `git show origin/master:src/node/interfaces.cpp | rg -n
  "mapPort|g_pcp" -C 3`, `git -C ../knots show
  29.x-knots:src/common/pcp.cpp | rg -n
  "g_pcp_warn_for_unauthorized|NOT_AUTHORIZED|already_warned|Mapping failed"
  -C 4`, `git -C ../knots show
  29.x-knots:src/node/interfaces.cpp | rg -n "mapPort|g_pcp" -C 3`, and
  `rg -n
  "g_pcp_warn_for_unauthorized|NOT_AUTHORIZED|already_warned|Mapping failed|mapPort\\("
  src/common/pcp.cpp src/common/pcp.h src/init.cpp src/node/interfaces.cpp
  src/test/pcp_tests.cpp` show current Core has the repeated-`NOT_AUTHORIZED`
  downgrade but not Knots' explicit-user warning override, while Knots and the
  port carry the override.
- `git show origin/master:src/node/mini_miner.cpp | rg -n
  "Don't check fees|GetModFeesWithAncestors\\(\\) >=|GetSizeWithAncestors"
  -C 3` and `rg -n
  "Don't check fees|GetModFeesWithAncestors\\(\\) >=|GetSizeWithAncestors"
  src/node/mini_miner.cpp src/test/miniminer_tests.cpp` show current Core and
  the port no longer assert ancestor fees are greater than self fees, so Knots'
  negative-fee MiniMiner fix is already present.
- `git show origin/master:src/net.cpp | rg -n "peeraddr|LogPeer" -C 3` and
  `rg -n "peeraddr|LogPeer|peer=0, peeraddr" src/net.cpp
  src/net_processing.cpp test/functional/feature_logging.py` show current Core
  and the port format peer log prefixes through `CNode::LogPeer()`, preserving
  the peer/peeraddr comma that older Knots fixed directly.
- `git show origin/master:src/rpc/rawtransaction.cpp | rg -n
  "DecodePSBTInputs|DecodePSBTOutputs|decodepsbt_inputs|decodepsbt_outputs"
  -C 4`, `git show origin/master:src/rpc/blockchain.cpp | rg -n
  "GetBlockVin|getblock_vin|RPCMethod getblock|RPCHelpMan getblock" -C 4`,
  `git -C ../knots show 29.x-knots:src/rpc/blockchain.cpp | rg -n
  "GetBlockVin|getblock_vin|RPCMethod getblock|RPCHelpMan getblock" -C 4`,
  and `rg -n
  "DecodePSBTInputs|DecodePSBTOutputs|GetBlockVin|getblock_vin|RPCMethod getblock|RPCHelpMan getblock"
  src/rpc/rawtransaction.cpp src/rpc/blockchain.cpp` show current Core and the
  port already lazy-initialize the `decodepsbt` result docs and avoid the
  `getblock_vin` namespace-static pattern via the newer `RPCMethod` factory
  registration shape; actual Knots carries the explicit `GetBlockVin()` helper.
- `git show origin/master:src/node/warnings.cpp | rg -n
  "all_messages\\.back|Join\\(all_messages" -C 3`
- `git -C ../knots show 29.x-knots:src/node/warnings.cpp | rg -n
  "all_messages\\.back|Join\\(all_messages" -C 3`
- `rg -n "all_messages\\.back|Join\\(all_messages|warning 1"
  src/node/warnings.cpp src/test/node_warnings_tests.cpp`
- `sed -n '421,475p' src/policy/fees/block_policy_estimator.cpp`
- `git show origin/master:src/policy/fees/block_policy_estimator.cpp |
  sed -n '421,475p'`
- `git -C ../knots grep -n
  "maxConfirms|maxPeriods = confAvg.size|scale >" 29.x-knots -- src`
- `sed -n '20,95p' src/wallet/db.cpp && sed -n '430,452p'
  src/util/fs_helpers.cpp && sed -n '95,116p' src/wallet/test/wallet_tests.cpp`
- `git show origin/master:src/wallet/db.cpp | sed -n '20,95p' &&
  git show origin/master:src/wallet/wallet.cpp | sed -n '520,534p'`
- `git -C ../knots show 29.x-knots:src/wallet/db.cpp | sed -n '20,95p'
  && git -C ../knots show 29.x-knots:src/wallet/wallet.cpp |
  sed -n '548,560p'`
- `rg -n
  "descriptors argument must|Only descriptor wallets can be created|require_format = DatabaseFormat::SQLITE"
  src/wallet/rpc/wallet.cpp src/wallet/wallet.cpp`,
  `git show origin/master:src/wallet/rpc/wallet.cpp
  origin/master:src/wallet/wallet.cpp | rg -n
  "descriptors argument must|Only descriptor wallets can be created|require_format = DatabaseFormat::SQLITE"
  -C 3`, and `git -C ../knots show
  29.x-knots:src/wallet/rpc/wallet.cpp 29.x-knots:src/wallet/wallet.cpp |
  rg -n
  "Setting to \\\"false\\\"|Compiled without bdb support|require_format = DatabaseFormat::SQLITE|Only descriptor wallets"
  -C 3` show the port still follows Core's descriptor-only wallet creation
  guard, while Knots allows legacy creation when BDB is compiled and only
  returns the BDB-disabled error under non-BDB builds.
- `rg -n "DEFAULT_TX_CONFIRM_TARGET|m_confirm_target"
  src/wallet/wallet.h src/wallet/test/wallet_tests.cpp`,
  `git show origin/master:src/wallet/wallet.h | rg -n
  "DEFAULT_TX_CONFIRM_TARGET|m_confirm_target" -C 2`, and
  `git -C ../knots show 29.x-knots:src/wallet/wallet.h | rg -n
  "DEFAULT_TX_CONFIRM_TARGET|m_confirm_target" -C 2`
- `BUILDDIR=$PWD/build contrib/devtools/gen-manpages.py
  --skip-missing-binaries` failed after skipping the disabled `bitcoin`,
  `bitcoin-tx`, `bitcoin-util`, and `bitcoin-qt` binaries because `help2man` is
  not installed locally
- Original Knots repro build:
  `cmake -S ../knots -B ../knots/build-repro -DRDTS_CONSENT=RUNTIME_WARN`
  and `cmake --build ../knots/build-repro --target bitcoind bitcoin-cli -j4`

Unit tests:

- `build/bin/test_bitcoin --run_test=versionbits_tests`
- `build/bin/test_bitcoin --run_test=versionbits_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=script_tests`
- `build/bin/test_bitcoin --run_test=streams_tests`
- `build/bin/test_bitcoin
  --run_test=streams_tests/streams_buffered_file_closes_source
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=streams_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `cmake --build build --target test_bitcoin && build/bin/test_bitcoin
  --run_test=chainparams_tests --catch_system_error=no --log_level=error
  --report_level=short`
- `build/bin/test_bitcoin --run_test=chainparams_tests/dns_seed_removals`
- `build/bin/test_bitcoin --run_test=chainparams_tests/dns_seed_removals
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=mempool_tests`
- `build/bin/test_bitcoin --run_test=mempool_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=mempool_tests/MempoolDustDynamicParse
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=mempool_tests/MempoolPermitEphemeralParse
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=mempool_tests,txrequest_tests,miner_tests
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=orphanage_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=transaction_tests`
- `build/bin/test_bitcoin --run_test=transaction_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=txvalidationcache_tests`
- `build/bin/test_bitcoin --run_test=txvalidationcache_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin
  --run_test=txvalidationcache_tests/checkinputs_flags_per_input_cache_safety
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=txvalidation_tests`
- `build/bin/test_bitcoin --run_test=txvalidation_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `cmake --build build --target test_bitcoin && build/bin/test_bitcoin
  --run_test=validation_tests/checkpoint_sanity --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=script_p2sh_tests/ValidateInputsStandardness`
- `build/bin/test_bitcoin --run_test=peerman_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=net_tests`
- `build/bin/test_bitcoin
  --run_test=net_tests/LocalAddress_TorDoesNotRequireOutboundReachability
  --catch_system_errors=no`
- `build/bin/test_bitcoin --run_test=net_tests --catch_system_errors=no`
- Port runtime check:
  `bitcoind -regtest -datadir=<tmp> -daemonwait -listen=0 -listenonion=0 -onlynet=ipv4 -externalip=pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion -fallbackfee=0.0001`
  followed by `bitcoin-cli -regtest -datadir=<tmp> getnetworkinfo`; the result
  reported `onion.reachable=false` and included the onion address in
  `localaddresses`.
- `build/bin/test_bitcoin --run_test=net_tests/cnode_punish_invalid_blocks
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=net_tests/cnode_punish_invalid_blocks
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=net_tests/cnode_punish_invalid_blocks
  --catch_system_errors=no`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_get_block_file_info_empty`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_readblock_hash_mismatch
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=blockmanager_tests`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/blockmanager_args_prune_during_init
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=netbase_tests/netpermissions_test
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=netbase_tests/netpermissions_test
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=net_peer_connection_tests`
- `build/bin/test_bitcoin --run_test=pcp_tests/pcp_not_authorized_explicit_warning`
- `build/bin/test_bitcoin --run_test=pcp_tests`
- `build/bin/test_bitcoin --run_test=rest_tests`
- `build/bin/test_bitcoin --run_test=validation_tests`
- `build/bin/test_bitcoin --run_test=validation_chainstatemanager_tests,wallet_tests
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=validation_chainstatemanager_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=validation_block_tests`
- `build/bin/test_bitcoin --run_test=merkle_tests`
- `build/bin/test_bitcoin --run_test=miner_tests`
- `build/bin/test_bitcoin --run_test=miniminer_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin
  --run_test=policyestimator_tests/read_rejects_fee_estimates_with_oversized_scale`
- `build/bin/test_bitcoin
  --run_test=policyestimator_tests/read_rejects_fee_estimates_with_oversized_scale
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=policyestimator_tests`
- `build/bin/test_bitcoin --run_test=policyestimator_tests
  --catch_system_errors=no`
- `build/bin/test_bitcoin --run_test=codex32_tests`
- `build/bin/test_bitcoin --run_test=codex32_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=caches_tests`
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=caches_tests/default_db_batch_size
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=sanity_tests`
- `build/bin/test_bitcoin --run_test=banman_tests`
- `build/bin/test_bitcoin --run_test=hash_tests`
- `build/bin/test_bitcoin --run_test=blockencodings_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=blockfilter_index_tests`
- `build/bin/test_bitcoin --run_test=txindex_tests,txospenderindex_tests,coinstatsindex_tests`
- `build/bin/test_bitcoin --run_test=db_tests,walletdb_tests,wallet_tests`
- `build/bin/test_bitcoin --run_test=fs_tests,walletdb_tests,wallet_tests`
- `build/bin/test_bitcoin --run_test=fs_tests/allocate_file_range_preserves_existing_bytes`
- `build/bin/test_bitcoin --run_test=db_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=db_tests/berkeley_ro_checks_final_page_lsn
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=wallet_tests/remove_created_wallet_dir_if_empty`
- `build/bin/test_bitcoin --run_test=wallet_tests/remove_created_wallet_dir_if_empty
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=wallet_tests/default_confirm_target_is_one_day
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/bitcoind -regtest -help-debug | sed -n '512,516p'`
- `../knots/build-repro/bin/bitcoind -regtest -help-debug | sed -n '505,509p'`
- `build/bin/test_bitcoin --run_test=getarg_tests/setting_args
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=util_tests`
- `build/bin/test_bitcoin --run_test=util_tests/test_sanitize_string_printable_chars`
- `build/bin/test_bitcoin --run_test=util_tests/outputtype_implicit_segwit`
- `build/bin/test_bitcoin --run_test=system_tests/subprocess_close_fds
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=system_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=i2p_tests/session_create_error_redacts_private_key
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=i2p_tests --catch_system_error=no
  --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=node_warnings_tests
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=node_warnings_tests
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=node_init_tests/init_test
  --catch_system_error=no --log_level=nothing --report_level=no`
- `build/bin/test_bitcoin --run_test=rbf_tests/calc_feerate_diagram_rbf
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=rbf_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `cmake --build build --target bitcoind test_bitcoin`
- `cmake --build build --target test_bitcoin && build/bin/test_bitcoin
  --run_test=rbf_tests --catch_system_error=no --log_level=error
  --report_level=short`
- `cmake --build build --target test_bitcoin -j4`
- `build/bin/test_bitcoin --run_test=mempool_tests/MempoolMinRelayAgeParse
  --catch_system_errors=no --log_level=error --report_level=short`
- `cmake --build build --target bitcoind -j4`
- `build/bin/test_bitcoin --run_test=rpc_tests/rpc_convert_values_dumptxoutset
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --catch_system_error=no --log_level=error
  --report_level=short` passed with all assertions successful
- `cmake --build build --target bitcoind -j4`
- `build/bin/bitcoind -help-debug | rg -n "dbbatchsize" -C 1`
- `../knots/build-repro/bin/bitcoind -help-debug | rg -n "dbbatchsize" -C 1`
- `./build/src/secp256k1/bin/tests --target=ellswift_xdh_bad_scalar_tests --iterations=16`
- `./build/src/secp256k1/bin/tests --target=ellswift --iterations=16`

Functional tests:

- `python3 test/functional/feature_proxy.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_proxy_tor_alias_port_2
  --portseed=27622`
- `python3 test/functional/feature_proxy.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_proxy_tor_alias_knots_2
  --portseed=27623`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini
  --test_methods init_lowmem_test
  --tmpdir=/mnt/my_storage/tmp_feature_init_lowmem_port --portseed=27630`
- `python3 test/functional/feature_init.py --configfile
  ../knots/build-repro/test/config.ini --test_methods init_lowmem_test
  --tmpdir=/mnt/my_storage/tmp_feature_init_lowmem_knots --portseed=27631`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --test_methods test_addnode_getaddednodeinfo
  --tmpdir=/mnt/my_storage/tmp_rpc_net_addnode_guard_port
  --portseed=31910`
- `python3 test/functional/feature_rbf.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_rbf_service_port_7 --portseed=32290`
- `python3 ../knots/test/functional/feature_rbf.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_rbf_service_knots --portseed=32220`
- `python3 test/functional/p2p_node_network_limited.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_node_network_limited_service_port_2
  --portseed=32300`
- `python3 ../knots/test/functional/p2p_node_network_limited.py
  --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_node_network_limited_service_knots
  --portseed=32230`
- `python3 -m py_compile test/functional/test_framework/mempool_util.py
  test/functional/feature_rbf.py test/functional/mempool_packages.py
  test/functional/mempool_cluster.py test/functional/mempool_updatefromblock.py
  test/functional/wallet_v3_txs.py test/functional/p2p_orphan_handling.py
  test/functional/p2p_opportunistic_1p1c.py`
- `python3 test/functional/feature_rdts.py --configfile build/test/config.ini`
- `python3 test/functional/feature_rdts.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_port_recheck --portseed=7423`
- `python3 test/functional/feature_reduced_data_utxo_height.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_utxo_height.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_reduced_data_utxo_height_port
  --portseed=27510`
- `python3 ../knots/test/functional/feature_reduced_data_utxo_height.py
  --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_reduced_data_utxo_height_knots
  --portseed=27511`
- `python3 test/functional/feature_reduced_data_temporary_deployment.py --configfile build/test/config.ini`
- `python3 test/functional/feature_reduced_data_temporary_deployment.py
  --configfile=build/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rdts_temp_deployment_witness_port
  --portseed=32300`
- `python3 test/functional/feature_bip9_max_activation_height.py --configfile build/test/config.ini`
- `python3 test/functional/feature_versionbits_warning.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_versionbits_warning`
- `python3 test/functional/feature_versionbits_warning.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_versionbits_warning_review_port
  --portseed=26442`
- `python3 test/functional/feature_versionbits_warning.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_versionbits_warning_bip320_recheck
  --portseed=7429`
- `python3 test/functional/feature_versionbits_warning.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_versionbits_warning_review_knots
  --portseed=26443`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_handshake_ua_escape_3`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_handshake_ua_escape_review_port2
  --portseed=26444`
- `python3 test/functional/p2p_handshake.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_handshake_ua_escape_review_knots2
  --portseed=26445`
- `python3 test/functional/feature_logging.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_feature_logging_peeraddr
  --portseed=27501`
- `python3 test/functional/p2p_handshake.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_handshake_rdts_gate_fixed3`
- `python3 test/functional/p2p_handshake.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_p2p_handshake_bip110_port
  --portseed=27512`
- `python3 ../knots/test/functional/p2p_handshake.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_handshake_bip110_knots --portseed=27513`
- `python3 test/functional/p2p_eviction.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_eviction_forceinbound`
- `python3 test/functional/p2p_eviction.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_eviction_forceinbound_review_port
  --portseed=26436`
- `python3 test/functional/p2p_eviction.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_eviction_forceinbound_nocap_port2
  --portseed=32662`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_permissions_onion_whitelist`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_permissions_forceinbound_review_port
  --portseed=26437`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_permissions_outbound_auto`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --test_methods check_automatic_outbound_permissions
  --tmpdir=/mnt/my_storage/tmp_p2p_permissions_outbound_auto_review_port
  --portseed=26438`
- `python3 test/functional/p2p_permissions.py --configfile
  ../knots/build-repro/test/config.ini
  --test_methods check_automatic_outbound_permissions
  --tmpdir=/mnt/my_storage/tmp_p2p_permissions_outbound_auto_review_knots
  --portseed=26439`
- `python3 test/functional/p2p_blockfilters.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_blockfilters_permission_2`
- `python3 test/functional/p2p_permissions.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_permissions_blockfilters`
- `python3 test/functional/p2p_permissions.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_permissions_implicit_addr_port
  --portseed=32610`
- `python3 ../knots/test/functional/p2p_permissions.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_permissions_implicit_addr_knots_native
  --portseed=32612`
- `python3 test/functional/rpc_getblockfilter.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_getblockfilter_basic_default_port
  --portseed=27610`
- `python3 test/functional/rpc_getblockfilter.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_getblockfilter_basic_default_knots
  --portseed=27611`
- `build/test/functional/p2p_feefilter.py`
- `build/test/functional/p2p_filter.py`
- `python3 test/functional/p2p_filter.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_filter_local_bloom_port
  --portseed=32600`
- `python3 test/functional/p2p_filter.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_filter_local_bloom_knots
  --portseed=32601`
- `python3 test/functional/mempool_minrelay.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_minrelay_port
  --portseed=32630`
- `python3 test/functional/feature_config_args.py --configfile=build/test/config.ini
  --cachedir=test/cache --test_methods test_minrelay_age_args
  --tmpdir=/mnt/my_storage/tmp_feature_config_minrelay_args_port_2
  --portseed=32641`
- `python3 test/functional/mempool_minrelay.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_minrelay_port_after_guard
  --portseed=32642`
- `python3 test/functional/mempool_minrelay.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_minrelay_knots
  --portseed=32631`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_net_cjdns_addnode_3`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --test_methods test_addnode_cjdns_duplicate
  --tmpdir=/mnt/my_storage/tmp_rpc_net_cjdns_addnode_review_port
  --portseed=26440`
- `python3 test/functional/rpc_net.py --configfile
  ../knots/build-repro/test/config.ini
  --test_methods test_addnode_cjdns_duplicate
  --tmpdir=/mnt/my_storage/tmp_rpc_net_cjdns_addnode_review_knots
  --portseed=26441`
- `python3 test/functional/rpc_net.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_net_addconnection`
- `build/test/functional/rpc_net.py`
- `python3 test/functional/rpc_net.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_net_misbehavior_score_port
  --portseed=32694`
- `python3 ../knots/test/functional/rpc_net.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_net_misbehavior_score_knots
  --portseed=32695`
- `python3 test/functional/p2p_add_connections.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_add_connections_signet`
- `python3 test/functional/mempool_accept.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_accept.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_accept_spkreuse_port_2
  --portseed=32010`
- `python3 test/functional/mempool_accept.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_accept_spkreuse_knots
  --portseed=32020` stopped before the `-spkreuse` section because
  unmodified Knots' `getmempoolinfo` result lacked the current port test's
  expected `permitbaremultisig` field.
- `python3 test/functional/mempool_datacarrier.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_dust.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_truc.py --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_truc_policy_port
  --portseed=32880 --test_methods test_truc_policy_option`
- `python3 test/functional/mempool_truc.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_truc_policy_knots
  --portseed=32881 --test_methods test_truc_policy_option`
- `python3 test/functional/mempool_ephemeral_dust.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_dust_review_port
  --portseed=26451`
- `python3 test/functional/mempool_ephemeral_dust.py
  --configfile=build/test/config.ini --test_methods
  test_permitephemeral_options
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_perm_port
  --portseed=32850`
- `python3 test/functional/mempool_ephemeral_dust.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_perm_full_port
  --portseed=32852`
- `python3 test/functional/mempool_ephemeral_dust.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_dust_review_knots
  --portseed=26452`
- `python3 test/functional/mempool_ephemeral_dust.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --test_methods test_permitephemeral_options
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_perm_knots
  --portseed=32851`
- `python3 test/functional/mempool_ephemeral_dust.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_ephemeral_perm_full_knots
  --portseed=32853`
- `python3 test/functional/mempool_subdust_fee_penalty.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_subdust_fee_penalty.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_subdust_modified_port_2
  --portseed=32862`
- `python3 test/functional/mempool_subdust_fee_penalty.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_subdust_modified_knots
  --portseed=32861`
- `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini
  --test_methods test_sendrawtransaction_maxfeerate_uses_sigop_adjusted_vsize
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_maxfeerate_3`
- `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini
  --test_methods test_sendrawtransaction_maxfeerate_uses_sigop_adjusted_vsize
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_maxfeerate_review_port
  --portseed=26449`
- `python3 test/functional/mempool_sigoplimit.py --configfile
  ../knots/build-repro/test/config.ini --test_methods
  test_sendrawtransaction_maxfeerate_uses_sigop_adjusted_vsize
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_maxfeerate_review_knots
  --portseed=26450`
- `python3 test/functional/mempool_sigoplimit.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_full`
- `python3 test/functional/mempool_sigoplimit.py
  --configfile=build/test/config.ini --test_methods
  test_bytespersigopstrict_policy
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_strict_port_2
  --portseed=32821`
- `python3 test/functional/mempool_sigoplimit.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --test_methods test_bytespersigopstrict_policy
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_strict_knots
  --portseed=32822`
- `python3 test/functional/mempool_sigoplimit.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_full_strict_port
  --portseed=32823`
- `python3 test/functional/mempool_datacarrier.py
  --configfile=build/test/config.ini --test_methods
  test_datacarriercost_adjusted_vsize
  --tmpdir=/mnt/my_storage/tmp_mempool_datacarrier_cost_method_port
  --portseed=32843`
- `python3 test/functional/mempool_datacarrier.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --test_methods test_datacarriercost_adjusted_vsize
  --tmpdir=/mnt/my_storage/tmp_mempool_datacarrier_cost_method_knots
  --portseed=32844`
- `python3 test/functional/mempool_datacarrier.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_datacarrier_cost_port_3
  --portseed=32845`
- `python3 test/functional/mempool_datacarrier.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_datacarrier_policy_port_5
  --portseed=32896`
- `python3 test/functional/mempool_datacarrier.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_datacarrier_policy_knots_3
  --portseed=32897`
- `build/bin/test_bitcoin --run_test=transaction_tests/test_IsStandard
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=script_tests/script_DataCarrierBytes
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin
  --run_test=script_tests/calculate_extra_tx_weight_saturates
  --catch_system_error=no --log_level=error --report_level=short`
- `python3 test/functional/mempool_maxscriptsize.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_maxscriptsize_port_2
  --portseed=32801`
- `python3 test/functional/mempool_maxscriptsize.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_maxscriptsize_knots
  --portseed=32802`
- `python3 test/functional/mempool_bare_pubkey.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_bare_pubkey_port
  --portseed=32830`
- `python3 test/functional/mempool_bare_pubkey.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_bare_pubkey_knots
  --portseed=32831`
- `python3 test/functional/mempool_reject_filters.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_reject_filters_port
  --portseed=32810`
- `python3 test/functional/mempool_reject_filters.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_reject_filters_knots
  --portseed=32811`
- `python3 test/functional/mempool_limit.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_mempool_limit_maxmempool_rpc`
- `python3 test/functional/mining_coin_age_priority.py --configfile build/test/config.ini`
- `python3 test/functional/mining_prioritisetransaction.py --configfile build/test/config.ini`
- `python3 test/functional/feature_maxuploadtarget.py --configfile build/test/config.ini`
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini`
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_interface_rest_listmempooltransactions`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_init_wait_port --portseed=27522`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_init_reindex_auto_port_5
  --portseed=32100 --test_methods init_auto_reindex_test`
- `python3 test/functional/feature_init.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_init_reindex_auto_knots_3
  --portseed=32110 --test_methods init_auto_reindex_test`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_init_full_reindex_auto_port
  --portseed=32120`
- `python3 test/functional/feature_chain_tiebreaks.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_feature_chain_tiebreaks_port
  --portseed=27530`
- `python3 test/functional/feature_chain_tiebreaks.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_chain_tiebreaks_loadchaintip_ub_fixed
  --portseed=7382`
- `python3 ../knots/test/functional/feature_chain_tiebreaks.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_chain_tiebreaks_knots --portseed=27531`
- `python3 ../knots/test/functional/feature_shutdown.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_shutdown_wait_knots --portseed=27521`
- `python3 test/functional/feature_shutdown.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_shutdown_wait_port2 --portseed=27523`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_config_args_port_mapping_2`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --test_methods test_port_mapping_disabled_when_not_listening
  --tmpdir=/mnt/my_storage/tmp_feature_config_port_mapping_direct
  --portseed=26431`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --test_methods test_connect_with_seednode
  --tmpdir=/mnt/my_storage/tmp_feature_config_port_mapping_connect
  --portseed=26432`
- `python3 test/functional/feature_config_args.py --configfile
  ../knots/build-repro/test/config.ini
  --test_methods test_port_mapping_disabled_when_not_listening
  --tmpdir=/mnt/my_storage/tmp_knots_feature_config_port_mapping_direct
  --portseed=26433`
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
- `python3 test/functional/feature_dersig.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_dersig_scriptthreads_inline_3
  --portseed=7393`
- `python3 test/functional/rpc_signrawtransactionwithkey.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_signrawtransactionwithkey_fee`
- `python3 test/functional/rpc_signrawtransactionwithkey.py --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_rpc_signrawtransactionwithkey_fee`
- `python3 test/functional/feature_startupnotify.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_startupnotify_multi`
- `python3 test/functional/feature_notifications.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_notifications_multi`
- `python3 test/functional/feature_index_prune.py --configfile build/test/config.ini`
- `python3 test/functional/feature_index_prune.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_index_prune_prunelock_review`
- `python3 test/functional/rpc_prunelocks.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_prunelocks`
- `build/bin/test_bitcoin --run_test=blockmanager_tests/prune_lock_update_and_delete
  --catch_system_error=no --log_level=error --report_level=short`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile build/test/config.ini`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_sync_coins_tip_after_chain_sync`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_sync_coins_tip_after_chain_sync_port_2
  --portseed=7390`
- `python3 test/functional/feature_sync_coins_tip_after_chain_sync.py --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_sync_coins_tip_after_chain_sync`
- `python3 test/functional/feature_softwareexpiry.py --configfile build/test/config.ini`
- `python3 test/functional/feature_torcontrol.py --configfile build/test/config.ini`
- `test/functional/feature_torcontrol.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_torcontrol_audit`
- `python3 test/functional/interface_bitcoin_cli.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_help.py --configfile build/test/config.ini`
- `python3 test/functional/tool_cli_completion.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_signer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_signer_fingerprint_2`
- `python3 test/functional/wallet_signer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_signer_warning`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_users_cookie_replace`
- `python3 test/functional/interface_rpc.py --configfile=build/test/config.ini
  --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_interface_rpc_weirdversions_port
  --portseed=32692`
- `python3 test/functional/interface_rpc.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_interface_rpc_weirdversions_knots
  --portseed=32693`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_users_auth_review`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_users_rpcauth_review_port --portseed=26411`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_users_hashed_auth_review_port
  --portseed=31930`
- `python3 test/functional/rpc_whitelist.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_whitelist_hashed_auth_review_port
  --portseed=31940`
- `python3 test/functional/rpc_users.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_users_wallet_restricted_matrix_port
  --portseed=31980`
- `python3 test/functional/rpc_users.py --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_users_wallet_restricted_matrix_knots
  --portseed=31990`
- `python3 test/functional/rpc_users.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_users_cookieperms_octal_port
  --portseed=32698`
- `python3 test/functional/rpc_users.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_users_cookieperms_octal_knots
  --portseed=32699`
- `python3 test/functional/rpc_getrpcwhitelist.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getrpcwhitelist.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_getrpcwhitelist_auth_review_port
  --portseed=26412`
- `python3 test/functional/rpc_bind.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_bind.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_bind_explicit_failure_port
  --portseed=26434`
- `python3 test/functional/rpc_bind.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_bind_explicit_failure_knots
  --portseed=26435`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_current_tip`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_period_start`
- `python3 test/functional/rpc_blockchain.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_blockchain_scriptthreads`
- `build/test/functional/rpc_blockchain.py`
- `python3 test/functional/interface_zmq.py --configfile /tmp/bitcoin-zmq-build/test/config.ini`
- `python3 test/functional/interface_zmq.py --configfile build-zmq/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_interface_zmq_review`
- `python3 test/functional/p2p_v2_encrypted.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_v2_encrypted`
- `python3 test/functional/feature_config_args.py --configfile
  build/test/config.ini --test_methods
  test_v2onlyclearnet_requires_v2transport
  --tmpdir=/mnt/my_storage/tmp_feature_config_v2onlyclearnet_port
  --portseed=26401`
- `python3 test/functional/feature_config_args.py --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_feature_config_args_v2only_full
  --portseed=26404`
- `python3 test/functional/p2p_v2_encrypted.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_v2onlyclearnet_port --portseed=26403`
- `python3 test/functional/feature_config_args.py --configfile build/test/config.ini
  --cachedir=test/cache --test_methods
  test_v2onlyclearnet_requires_v2transport
  --tmpdir=/mnt/my_storage/tmp_feature_config_v2onlyclearnet_port_latest
  --portseed=32220`
- `python3 test/functional/p2p_v2_encrypted.py --configfile build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_v2onlyclearnet_port_latest --portseed=32221`
- `python3 test/functional/rpc_getblocklocations.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_getblocklocations_review`
- `python3 test/functional/rpc_getgeneralinfo.py --configfile build/test/config.ini`
- `build/test/functional/rpc_scanblocks.py`
- `python3 test/functional/rpc_sort_multisig.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_deriveaddresses.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_deriveaddresses_checksum`
- `python3 test/functional/rpc_deriveaddresses.py --usecli --configfile
  build/test/config.ini --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_deriveaddresses_checksum_cli`
- `python3 test/functional/rpc_setban.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_disconnect_ban.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_disconnect_ban_ip_subnet`
- `python3 test/functional/rpc_rawtransaction.py --configfile build/test/config.ini`
- `build/test/functional/rpc_rawtransaction.py`
- `python3 test/functional/rpc_invalid_address_message.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_invalid_address_validateaddress_compat`
- `python3 test/functional/rpc_txoutproof.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_txoutproof`
- `python3 test/functional/rpc_packages.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_options_review`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_min_conf`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_anti_fee_sniping`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_psbt_anti_fee_sniping_review_port
  --portseed=26453`
- `python3 ../knots/test/functional/rpc_psbt.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_psbt_anti_fee_sniping_review_knots
  --portseed=26454`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_psbt_prevtxs`
- `python3 test/functional/mempool_fee_histogram.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_fee_histogram.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_fee_histogram_review_port
  --portseed=26447`
- `python3 test/functional/mempool_fee_histogram.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_fee_histogram_review_knots
  --portseed=26448`
- `python3 test/functional/mempool_fee_histogram.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_fee_histogram_underflow_port
  --portseed=32650`
- `python3 test/functional/mempool_fee_histogram.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_fee_histogram_underflow_knots
  --portseed=32651`
- `build/bin/test_bitcoin --run_test=mempool_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=orphanage_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=wallet_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `python3 test/functional/wallet_reorgsrestore.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_wallet_reorgsrestore_bestblock --portseed=7396`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_rpc_getblockfrompeer_no_header`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_rpc_getblockfrompeer_review_port
  --portseed=26455`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_getblockfrompeer_nodeid_alias_port
  --portseed=32696`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_rpc_getblockfrompeer_nodeid_alias_knots
  --portseed=32697`
- `python3 test/functional/rpc_mempool_info.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_extratxs_size_port
  --portseed=32620`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_extratxs_size_knots
  --portseed=32621`
- `build/bin/test_bitcoin --run_test=peerman_tests/peerman_args_block_reconstruction_extra_txn
  --catch_system_errors=no --log_level=error --report_level=short`
- `python3 test/functional/p2p_maxorphantx.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_maxorphantx_port_3
  --portseed=32912`
- `python3 test/functional/p2p_maxorphantx.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_maxorphantx_knots
  --portseed=32913`
- `build/bin/test_bitcoin --run_test=txdownload_tests/max_orphan_txs_limit
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=transaction_tests/test_IsStandard
  --catch_system_errors=no --log_level=error --report_level=short`
- `python3 test/functional/mempool_acceptunknownwitness.py
  --configfile=build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_mempool_acceptunknownwitness_port
  --portseed=32900`
- `python3 test/functional/mempool_acceptunknownwitness.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_acceptunknownwitness_knots
  --portseed=32901`
- `python3 test/functional/p2p_compactblocks.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_compactblocks_header_guard_final`
- `python3 test/functional/p2p_compactblocks.py --configfile=build/test/config.ini
  --cachedir=test/cache --test_methods test_witness_mutated_blocktxn
  --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_witness_mutation_port3
  --portseed=32676`
- `python3 test/functional/p2p_compactblocks.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_witness_mutation_full_port4
  --portseed=32678`
- `python3 test/functional/p2p_invalid_block.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_p2p_invalid_block_inbound_punish2`
- `test/functional/p2p_invalid_block.py --configfile=build/test/config.ini
  --cachedir=test/cache`
- `python3 test/functional/interface_zmq.py --configfile
  /mnt/my_storage/build-zmq-audit/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_interface_zmq_audit_rerun`
- `python3 test/functional/interface_zmq.py --configfile
  /mnt/my_storage/build-zmq-audit/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_interface_zmq_review_rerun
  --portseed=26446`
- `python3 test/functional/interface_zmq.py --configfile=/mnt/my_storage/build-zmq-audit/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_interface_zmq_ipc_port
  --portseed=32700`
- `python3 test/functional/interface_zmq.py --configfile=../knots/build-zmq-audit/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_interface_zmq_ipc_knots
  --portseed=32701`
- `python3 test/functional/p2p_dos_header_tree.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_dos_header_tree.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_p2p_dos_header_tree_checkpoint`
- `python3 test/functional/p2p_dos_header_tree.py --configfile ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_p2p_dos_header_tree_checkpoint`
- `python3 test/functional/p2p_block_times.py --configfile build/test/config.ini`
- `python3 test/functional/feature_block.py --configfile build/test/config.ini
  --skipreorg --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_block_skip`
- `python3 test/functional/feature_assumeutxo.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_feature_assumeutxo_after_blockchain`
- `python3 test/functional/tool_wallet.py --configfile build/test/config.ini`
- `python3 test/functional/tool_wallet.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_tool_wallet_cleanup_review --portseed=7399`
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_startup.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_assumeutxo.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_assumeutxo_after_fix`
- `python3 test/functional/wallet_address_types.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_address_types_change_pref`
- `build/bin/test_bitcoin --run_test=spend_tests/change_type_avoids_newer_default
  --catch_system_error=no --log_level=error --report_level=short`
- `python3 test/functional/wallet_address_types.py --configfile=build/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_wallet_address_types_change_newer_port
  --portseed=32690`
- `python3 test/functional/wallet_address_types.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_wallet_address_types_change_newer_knots
  --portseed=32691`
- `cmake --build build --target bitcoind bitcoin-cli test_bitcoin -j4`
- `python3 ../knots/test/functional/wallet_anchor.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_wallet_anchor_isfromme --portseed=7403`
- `python3 test/functional/wallet_anchor.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_wallet_anchor_isfromme_after_fix --portseed=7404`
- `python3 test/functional/wallet_listtransactions.py --configfile
  build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_wallet_listtransactions_isfromme_after_fix
  --portseed=7405`
- `build/bin/test_bitcoin --run_test=wallet_tests --catch_system_error=no
  --log_level=error --report_level=short`
- `python3 test/functional/wallet_balance.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_wallet_balance_legacy_conflict_review`
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
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_fundrawtransaction_witness_options_review`
- `python3 test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_fundrawtransaction_min_conf`
- `python3 test/functional/wallet_basic.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_basic_use_txids_fixed`
- `python3 test/functional/wallet_multiwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_multiwallet.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_multiwallet_skip_node_dirs_control`
- `python3 test/functional/wallet_multiwallet.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_wallet_multiwallet_path_hardening --portseed=26421`
- Manual Knots walletdir check: start
  `../knots/build-repro/bin/bitcoind -regtest -daemon
  -datadir=/mnt/my_storage/tmp_knots_walletdir_skip_manual_control
  -walletdir=/mnt/my_storage/tmp_knots_walletdir_skip_manual_control/regtest
  -nowallet`, place SQLite-looking `wallet.dat` markers under
  `ordinary_sqlite_marker` and `blocks/skipped_wallet`, then
  `../knots/build-repro/bin/bitcoin-cli -regtest
  -datadir=/mnt/my_storage/tmp_knots_walletdir_skip_manual_control
  listwalletdir`; actual Knots returned only `ordinary_sqlite_marker`.
- `python3 test/functional/wallet_backup.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_send.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_send_fee_mode`
- `python3 test/functional/wallet_migration.py --configfile build/test/config.ini`
  (skipped: previous releases not available or disabled)
- `python3 test/functional/wallet_keypool.py --configfile build/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_bitcoin_wallet_keypool_isactive_fixed`
- `build/bin/test_bitcoin --run_test=script_tests/require_sighash_all
  --catch_system_error=no --log_level=error --report_level=short`
- `build/bin/test_bitcoin --run_test=script_tests --catch_system_error=no
  --log_level=error --report_level=short`
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
  `python3 /mnt/my_storage/bitcoin/test/functional/feature_rdts.py
  --configfile /mnt/my_storage/knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_knots_with_port_test
  --portseed=7424`
  (fails on the inherited RDTS `ignore_rejects` internal-bug log described
  above)
- Original Knots cross-check:
  `python3 ../knots/test/functional/feature_rdts.py --configfile
  ../knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_feature_rdts_knots_own_recheck
  --portseed=7425`
  passed on unmodified Knots, confirming Knots' native test does not cover the
  broader legacy script-flag bypass strings.
- Original Knots cross-check:
  `python3 test/functional/feature_reduced_data_temporary_deployment.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_rdts_temp_deployment_witness_knots --portseed=32301`
  passed on unmodified Knots, including the strengthened witness-script expiry
  check at the RDTS active-to-expired boundary.
- Original Knots cross-check:
  `../knots/build-repro/bin/bitcoind -regtest -datadir=<tmp> -daemonwait -listen=0 -listenonion=0 -onlynet=ipv4 -externalip=pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion -fallbackfee=0.0001`
  followed by `../knots/build-repro/bin/bitcoin-cli -regtest -datadir=<tmp>
  getnetworkinfo` passed on unmodified Knots; the result reported
  `onion.reachable=false` and still included the onion address in
  `localaddresses`.
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/p2p_eviction.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_eviction_forceinbound_repro`
  (fails on unmodified Knots because the ForceInbound peer's
  `getpeerinfo.permissions` array omits `forceinbound`)
- Original Knots cross-check:
  `python3 test/functional/p2p_eviction.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --test_methods test_forceinbound_nocap --tmpdir=/mnt/my_storage/tmp_knots_p2p_eviction_forceinbound_nocap_method --portseed=32663`
  passed on unmodified Knots, confirming final Knots also allows more than
  eight forced inbound replacements.
- Original Knots cross-check:
  `python3 ../knots/test/functional/p2p_permissions.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_p2p_permissions_implicit_addr_knots_native --portseed=32612`
  passed on unmodified Knots, including Knots' native expectations that bare
  implicit whitelist entries include the `addr` permission.
- Original Knots expected-failure repro:
  `python3 test/functional/p2p_permissions.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_p2p_permissions_implicit_addr_knots --portseed=32611`
  reached the implicit whitelist checks but later failed on the separate
  already documented `forceinbound` `getpeerinfo.permissions` gap.
- Original Knots expected-failure repro:
  `python3 /mnt/my_storage/bitcoin/test/functional/rpc_signer.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_rpc_signer_fingerprint`
  (the invalid-fingerprint redaction checks pass first, then unmodified Knots
  fails the duplicate-then-unique signer enumeration case by returning only
  `00000001`)
- Original Knots cross-check:
  `python3 test/functional/rpc_users.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_rpc_users_rpcauth_review --portseed=26413`
  passed on unmodified Knots, confirming the port's auth-file, wallet-restricted
  auth, blank `-rpcauth`, and `-norpcauth` behavior is inherited Knots
  behavior rather than port-introduced.
- Original Knots cross-check:
  minimal startup with `../knots/build-repro/bin/bitcoind -regtest`, replacing
  `regtest/.cookie` with `__cookie__:replaced-by-another-process`, then
  stopping via `bitcoin-cli -rpcuser=__cookie__ -rpcpassword=<generated>`
  preserved the replacement (`cookie_after_stop=__cookie__:replaced-by-another-process`)
- Original Knots cross-check:
  `python3 test/functional/p2p_handshake.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_p2p_handshake_ua_escape_review_knots2 --portseed=26445`
  (passes on unmodified Knots, confirming the missing user-agent log escape was
  introduced by the port)
- Original Knots cross-check:
  `python3 test/functional/mempool_sigoplimit.py --configfile ../knots/build-repro/test/config.ini --test_methods test_sendrawtransaction_maxfeerate_uses_sigop_adjusted_vsize --tmpdir=/mnt/my_storage/tmp_mempool_sigoplimit_maxfeerate_review_knots --portseed=26450`
  passed on unmodified Knots
- Original Knots cross-check:
  `python3 ../knots/test/functional/rpc_psbt.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_rpc_psbt_anti_fee_sniping_review_knots --portseed=26454`
  passed on unmodified Knots, including Knots' native
  `walletcreatefundedpsbt` anti-fee-sniping assertion
- Original Knots cross-check:
  `python3 ../knots/test/functional/rpc_getblockfrompeer.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_rpc_getblockfrompeer_review_knots --portseed=26456`
  passed on unmodified Knots, including no-header fetches, duplicate same-peer
  request errors, and pruned-node refetch coverage.
- Original Knots cross-check:
  a one-off subclass of `rpc_net.py` running only
  `test_addnode_cjdns_duplicate` with
  `--configfile /mnt/my_storage/knots/build-repro/test/config.ini
  --tmpdir=/mnt/my_storage/tmp_knots_rpc_net_cjdns_addnode_only` passed on
  unmodified Knots
- Original Knots cross-check:
  `test/functional/p2p_feefilter.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_feefilter_option`
  passed on unmodified Knots, including the new `-nofeefilter` assertion.
- Original Knots cross-check:
  `python3 test/functional/feature_config_args.py --configfile ../knots/build-repro/test/config.ini --test_methods test_v2onlyclearnet_requires_v2transport --tmpdir=/mnt/my_storage/tmp_feature_config_v2onlyclearnet_knots --portseed=26402`
  passed on unmodified Knots, confirming the `-v2onlyclearnet=1` plus
  `-v2transport=0` startup guard is inherited Knots behavior.
- Original Knots cross-check:
  `python3 test/functional/feature_config_args.py --configfile ../knots/build-repro/test/config.ini --cachedir=test/cache --test_methods test_v2onlyclearnet_requires_v2transport --tmpdir=/mnt/my_storage/tmp_feature_config_v2onlyclearnet_knots_latest --portseed=32222`
  passed on unmodified Knots, confirming the inherited startup guard with the
  current test tree.
- Original Knots cross-check:
  `python3 test/functional/p2p_v2_encrypted.py --configfile ../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_p2p_v2onlyclearnet_knots_latest --portseed=32223`
  passed on unmodified Knots, confirming the inherited V2 clearnet success, V1
  clearnet refusal, and V1 onion allowance behavior.
- Original Knots cross-check:
  `test/functional/p2p_filter.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_filter_filtered_witness`
  passed on unmodified Knots, including the new
  `MSG_FILTERED_WITNESS_BLOCK` witness-preservation assertion.
- Original Knots cross-check:
  `python3 test/functional/p2p_filter.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_p2p_filter_local_bloom_knots --portseed=32601`
  passed on unmodified Knots, including the localhost-only BIP37 default,
  explicit `-peerbloomfilters=0` disable path, and filtered-witness block
  assertion.
- Original Knots cross-check:
  `python3 test/functional/p2p_compactblocks_extratxs.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_extratxs_size_knots --portseed=32621`
  passed on unmodified Knots, including `-blockreconstructionextratxnsize`
  zero, fractional, boundary, and eviction behavior.
- Original Knots cross-check:
  `python3 test/functional/mempool_minrelay.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_mempool_minrelay_knots --portseed=32631`
  passed on unmodified Knots, confirming that `-minrelaymaturity=2` and
  `-minrelaycoinblocks=7500000000` reject fresh confirmed spends until one more
  block provides enough age.
- Original Knots bug cross-check:
  `timeout 3s ../knots/build-repro/bin/bitcoind -regtest -datadir="$tmpdir" -minrelaycoinblocks=-1 -noconnect -listen=0 -server=0 -printtoconsole=0`
  and the same command with `-minrelaymaturity=-1` both returned `124`, meaning
  unmodified Knots stayed running until killed by `timeout` instead of rejecting
  the invalid negative value during startup.
- Original Knots source cross-check:
  `git -C ../knots show 29.x-knots:src/test/transaction_tests.cpp | rg -n "acceptunknownwitness|scriptpubkey-unknown-witnessversion" -C 4`
  shows unmodified Knots' unit test has the same unknown-witness output
  acceptance/rejection expectation as the port. The later
  `python3 test/functional/mempool_acceptunknownwitness.py
  --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_mempool_acceptunknownwitness_knots
  --portseed=32901` run also passed on unmodified Knots, including block
  acceptance after mempool rejection.
- Original Knots cross-check:
  `python3 test/functional/p2p_compactblocks.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_compactblocks_header_guard`
  reached and passed the repeated-`blocktxn` section on unmodified Knots,
  logging `previous compact block reconstruction attempt failed`, then failed
  later on an unrelated invalid-`sendcmpct` disconnect expectation.
- Original Knots cross-check:
  `python3 test/functional/p2p_compactblocks.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --test_methods test_witness_mutated_blocktxn --tmpdir=/mnt/my_storage/tmp_p2p_compactblocks_witness_mutation_knots2 --portseed=32677`
  passed on unmodified Knots, confirming the witness-mutated `blocktxn`
  fallback is inherited from Knots/current Core rather than introduced by the
  port.
- Original Knots cross-check:
  `python3 test/functional/p2p_invalid_block.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_p2p_invalid_block_inbound_punish`
  passed on unmodified Knots after removing the automatic `noban` whitelist,
  confirming that ordinary inbound peers are tolerated for non-mutated
  consensus-invalid blocks and disconnected for the separate mutated-block
  pre-check.
- Original Knots cross-check:
  `test/functional/p2p_invalid_block.py --configfile=../knots/build-repro/test/config.ini
  --cachedir=test/cache
  --tmpdir=/mnt/my_storage/tmp_knots_p2p_invalid_block_inbound_punish_latest
  --portseed=32210` passed on unmodified Knots with the current port test.
- Original Knots cross-check:
  `test/functional/rpc_scanblocks.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_rpc_scanblocks_invalid_action`
  passed on unmodified Knots, including the new in-progress
  `relevant_blocks` status assertion and the existing invalid-action error
  check.
- Manual port/Knots legacy-wallet cross-check:
  starting fresh regtest nodes and calling
  `bitcoin-cli -regtest -named createwallet wallet_name=legacy descriptors=false`
  returned RPC `-4` with
  `descriptors argument must be set to "true"; it is no longer possible to
  create a legacy wallet.` on the port, while unmodified Knots built without
  BDB returned RPC `-4` with
  `Compiled without bdb support (required for legacy wallets)`. This confirms
  the port blocks the request before the Knots BDB-availability branch.
- Original Knots expected-failure repro:
  foreground `../knots/build-repro/bin/bitcoind -regtest` plus
  `bitcoin-cli -regtest addnode 127.0.0.1:<port> onetry false inbound`
  under `/mnt/my_storage/tmp_knots_addnode_false_inbound.yTqHQn` aborted with
  exit code 134 and stderr
  `Assertion 'conn_type != ConnectionType::INBOUND' failed.`
  The positional JSON-string compatibility form
  `bitcoin-cli -regtest addnode 127.0.0.1:<port> onetry '"inbound"'` also
  aborted under `/mnt/my_storage/tmp_knots_addnode_crash_fg.TM8tLP`.
- Original Knots cross-check:
  `python3 /mnt/my_storage/bitcoin/test/functional/feature_versionbits_warning.py --configfile /mnt/my_storage/knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_feature_versionbits_warning_check`
  (passes on unmodified Knots, confirming the earlier warning-range failure was
  introduced by the port's current-Core BIP323 adaptation)
- Original Knots cross-check:
  `python3 ../knots/test/functional/wallet_keypool.py --configfile ../knots/build-repro/test/config.ini --tmpdir=/mnt/my_storage/tmp_knots_wallet_keypool_isactive_repro`
  (passes on unmodified Knots, confirming the local `wallet_keypool.py`
  failure was port-side test drift from losing Knots' explicit keypool setup)
- Original Knots cross-check:
  unmodified `../knots/build-repro/bin/bitcoind -regtest` accepted
  `bitcoin-cli -regtest -named dumptxoutset path=<tmp>/utxo.txt
  format='["txid","vout"]' show_header=false separator=':'` after one
  `generatetoaddress` block, and the output row used `:` between `txid` and
  `vout`. This confirms the port's missing `separator` CLI conversion was
  introduced by rebasing onto current Core's named-argument heuristic, not
  inherited from Knots.
- Original Knots cross-check:
  `python3 test/functional/feature_uacomment.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_feature_uacomment_knots --portseed=32921`
  passed on unmodified Knots with the added `-uaspoof=0` and `-uaspoof=1`
  assertions. The corresponding port run
  `python3 test/functional/feature_uacomment.py --configfile=build/test/config.ini --tmpdir=/mnt/my_storage/tmp_feature_uacomment_port --portseed=32920`
  also passed.
- Original Knots cross-check:
  `python3 test/functional/rpc_mempoolstats.py --configfile=../knots/build-repro/test/config.ini --cachedir=test/cache --tmpdir=/mnt/my_storage/tmp_rpc_mempoolstats_knots --portseed=32932`
  passed on unmodified Knots, confirming the default-disabled daemon path and
  enabled `getmempoolstats` sampling behavior are inherited from Knots. The
  corresponding port run
  `python3 test/functional/rpc_mempoolstats.py --configfile=build/test/config.ini --tmpdir=/mnt/my_storage/tmp_rpc_mempoolstats_port_2 --portseed=32931`
  passed, as did
  `build/bin/test_bitcoin --run_test=stats_tests --catch_system_error=no --log_level=error --report_level=short`.
  `../knots/build-repro/bin` only contains `bitcoind`, `bitcoin-cli`, and
  `bitcoin-wallet`, so there was no unmodified-Knots `test_bitcoin` binary for
  a C++ unit cross-run.
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
