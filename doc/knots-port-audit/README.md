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
OOM hardening, BDB overflow data lengths, miniscript assert guards, and most
cpp-subprocess memory/Windows fixes.

## Open Risks

- The `ignore_rejects` policy surface still deserves focused review. Knots
  removes individual bypass names for unknown witness/taproot/op-success policy
  flags, but the broad `non-mandatory-script-verify-flag` bypass may still be
  able to mask policy checks outside the consensus-enforced RDTS path. The
  current RDTS functional test verifies consensus checks are not bypassed.
- Legacy-wallet creation is a non-consensus divergence from Knots on this
  current-Core base. Core master no longer creates new legacy wallets, and this
  port now preserves Core's explicit RPC error instead of crashing. Ported
  legacy-only wallet tests are skipped by the framework when `--legacy-wallet`
  mode is selected.
- BIP-110/RDTS consensus equivalence still needs dedicated review. The current
  pass mapped the Knots RDTS subjects to replayed/adapted port commits and
  reran the focused RDTS, UTXO-height, temporary-deployment, peer-manager, net,
  and P2P handshake tests. That is focused regression evidence, not a full
  consensus-equivalence proof against Knots or Core.

## Verification

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
- `build/bin/test_bitcoin --run_test=peerman_tests`
- `build/bin/test_bitcoin --run_test=net_tests`
- `build/bin/test_bitcoin --run_test=net_peer_connection_tests`
- `build/bin/test_bitcoin --run_test=rest_tests`
- `build/bin/test_bitcoin --run_test=validation_tests`
- `build/bin/test_bitcoin --run_test=validation_block_tests`
- `build/bin/test_bitcoin --run_test=merkle_tests`
- `build/bin/test_bitcoin --run_test=policyestimator_tests`
- `build/bin/test_bitcoin --run_test=util_tests/test_sanitize_string_printable_chars`

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
- `python3 test/functional/rpc_rawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_packages.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_psbt.py --configfile build/test/config.ini`
- `python3 test/functional/mempool_fee_histogram.py --configfile build/test/config.ini`
- `python3 test/functional/rpc_getblockfrompeer.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_compactblocks_extratxs.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_dos_header_tree.py --configfile build/test/config.ini`
- `python3 test/functional/p2p_block_times.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_sweepprivkeys.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_importseed.py --configfile build/test/config.ini`
- `python3 test/functional/wallet_implicitsegwit.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_dump.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_import_rescan.py --configfile build/test/config.ini`
  (skipped: legacy wallets can no longer be created)
- `python3 test/functional/wallet_createwallet.py --configfile build/test/config.ini --legacy-wallet`
  (skipped: legacy wallets can no longer be created)
