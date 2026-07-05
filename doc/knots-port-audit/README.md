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
  vulnerability.

- External or Knots-only surfaces:
  `d637873230` fixes `GetBlockFileInfo` bounds handling, but the obvious
  RPC-facing caller is Knots' `getblockfileinfo`. Current Core's corresponding
  callers are internal/tests. The BDB cleanup/data-loss fixes matter for this
  port because BDB support is retained, but current Core master no longer has
  the same BDB write-environment files.

High-signal hardening already present in Core under the same or different
commits and therefore not counted as missing here: secp256k1 ellswift overflow
key handling, `LocalServiceInfo::nScore` saturation, miner `addPackageTxs`
overflow, compact-block witness mutation checks, `LoadChainTip` UB,
`SetStdinEcho` UB, fd-limit overflow/RLIMIT_INFINITY handling, RPC credentials
hashed in memory, PSBT bounds asserts, v2-to-v1 reconnect UAF, feebumper
combined-fee crash, BDB overflow data lengths, miniscript assert guards, and
most cpp-subprocess memory/Windows fixes.

## Open Risks

- The `ignore_rejects` policy surface still deserves focused review. Knots
  removes individual bypass names for unknown witness/taproot/op-success policy
  flags, but the broad `non-mandatory-script-verify-flag` bypass may still be
  able to mask policy checks outside the consensus-enforced RDTS path. The
  current RDTS functional test verifies consensus checks are not bypassed.
- BIP-110/RDTS consensus equivalence still needs dedicated review. This pass
  verified the replayed tests and fixed issues they exposed; it does not prove
  consensus equivalence to Knots or Core.

## Verification

Builds:

- `cmake -B build -DRDTS_CONSENT=RUNTIME_WARN`
- `cmake --build build --target bitcoind bitcoin-cli test_bitcoin -j4`
- `cmake --build build --target bitcoind -j4`
- Original Knots repro build:
  `cmake -S ../knots -B ../knots/build-repro -DRDTS_CONSENT=RUNTIME_WARN`
  and `cmake --build ../knots/build-repro --target bitcoind bitcoin-cli -j4`

Unit tests:

- `build/bin/test_bitcoin --run_test=versionbits_tests`
- `build/bin/test_bitcoin --run_test=script_tests`
- `build/bin/test_bitcoin --run_test=mempool_tests`
- `build/bin/test_bitcoin --run_test=transaction_tests`
- `build/bin/test_bitcoin --run_test=txvalidationcache_tests`
- `build/bin/test_bitcoin --run_test=txvalidation_tests`
- `build/bin/test_bitcoin --run_test=peerman_tests`
- `build/bin/test_bitcoin --run_test=net_tests`
- `build/bin/test_bitcoin --run_test=rest_tests`
- `build/bin/test_bitcoin --run_test=validation_tests`
- `build/bin/test_bitcoin --run_test=validation_block_tests`

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
- `python3 test/functional/interface_rest.py --configfile build/test/config.ini`
- `python3 test/functional/feature_init.py --configfile build/test/config.ini`
