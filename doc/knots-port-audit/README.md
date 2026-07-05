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

The following Knots commits are `git cherry` `+` candidates relative to
`origin/master`, meaning no equivalent patch-id was found in Core during this
pass. They are security-relevant, but not all are proven exploitable.

- External signer fingerprint hardening:
  `6d2c2259ee`, `12eefda89a`, `ee39394ad3`

  Validate fingerprint hex before shell-command use, require exactly eight hex
  characters, and avoid logging untrusted fingerprint text.

- RPC cookie replacement and permission hardening:
  `e49dfac324`, `7140e1f149`, `622b768945`, `50b7a50a61`, `198466d5d3`

  Set temporary cookie permissions before writing, delete stale temp files,
  delete before replace, and avoid deleting a cookie replaced by another process.

- Tor-control resource and isolation hardening:
  `4110843327`, `3eb50531a5`, `0d747bbc55`, `6df8bf7673`, `10397d85ca`

  Bound tor-control line handling to avoid OOM, reconnect after overlong-line
  disconnects, enable PoW defenses for created hidden services, and randomize
  Tor stream isolation credentials.

- Subprocess fd cleanup before exec:
  `214047ecd3`, `ed5a3b3604`

  Iterate `/proc/self/fd` and use `close_range`/fallback closing so child
  processes do not inherit unintended file descriptors.

- Local file/resource robustness:
  `163d3e5c13`, `d637873230`, `1953393f48`, `0f92fc907f`, `6c89453ca`,
  `126d6df18d`, `8caf0836a8`, `1e2eaebd79`, `98d9237d3e`

  These cover fee-estimator corrupt-file overflow avoidance, blockfile bounds
  underflow, fd-limit overflow/RLIMIT_INFINITY handling, a codex32 OOB read,
  service-score saturation, LevelDB uninitialized size, and terminal echo UB.

- RPC/HTTP/ZMQ race or failure containment:
  `c3dafb49ca`, `57becdf59e`, `fbe185ce7a`, `91c9e14639`,
  `268fb1e0e3`, `1c4d2d54d8`

  These cover lazy initialization to avoid static-order issues, stricter HTTP
  bind/listen cleanup, locking around `getblocklocations`, and ZMQ notifier
  failure handling.

- Wallet crash/null-deref fixes still Knots-only by patch-id:
  `7e125f8ed2`, `2a09a34129`

  These avoid null dereferences in bumpfee and `AvailableCoins` paths.

High-signal hardening already present in Core by patch-id and therefore not
counted as missing here: PSBT bounds assert (`b2f6128338`), v2-to-v1 reconnect
UAF (`f44b206a5e`), miner package overflow (`aff95a8a60`), feebumper combined
fee crash (`4b202bc91c`), BDB overflow data lengths (`885a34eceb`), miniscript
assert guards (`e0be30901c`), and most cpp-subprocess memory/Windows fixes.

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
