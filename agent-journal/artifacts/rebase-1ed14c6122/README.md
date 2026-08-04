# Rebase onto 1ed14c6122 — repair artifact

## Provenance

- Source branch: `audit/transplant-index-fuzz`
- Repair commit: `5e0a9d153d` ("repair: post-rebase API-drift fixes onto 1ed14c6122")
- Rebase: 1245 commits replayed from base `18c05d9301` onto `origin/master` `1ed14c6122`
  (193 upstream commits brought under the lineage). Pre-rebase tip preserved at
  `backup/transplant-index-fuzz-pre-rebase-1ed14c` (`2b4ee58377`).
- New branch HEAD after repair: `5e0a9d153d` (parent `d42daa1e14`).

## Why a patch artifact instead of a cherry-pick

The repair commit is base-relative: every hunk adapts our auto-merged code to APIs
that only exist on the new base (string-typed `m_recv_buffer`, `string_view`
`LineReader`, the `TxSource` enum in blockencodings, `SaltedCoinsCacheHasher`).
Cherry-picking it onto this archive's pre-rebase tree conflicts
(`src/test/blockencodings_tests.cpp`) and resolving would falsify content —
the "before" and "after" of these hunks are both new-base concepts.
The full repair is preserved here as `repair-5e0a9d153d.patch` (format-patch,
applies cleanly to `d42daa1e14`).

## Failing-before / passing-after

Failing-before: incremental `test_bitcoin` build failed on 7 files after the
rebase replay; `blockencodings_tests` aborted under ASan Debug at
`blockencodings.cpp:174` (`Assume(wtxid == tx->GetWitnessHash())` in `InitData`,
reached by upstream's `ReceiveWithExtraTransactions`, which deliberately passes
mismatched `{claimed_wtxid, tx}` pairs — the API contract is "key is a claim";
production `vExtraTxnForCompact` always builds honest pairs).

Passing-after (ASan Debug clang-18, on `5e0a9d153d`):

- `cmake --build build-after --target test_bitcoin bitcoind` — 100%
- `test_bitcoin --run_test=blockencodings_tests,coins_tests,httpserver_tests,torcontrol_tests,util_tests,validation_chainstatemanager_tests,rpc_tests,descriptor_tests`
  — 200 cases, 1,255,425 assertions, all pass
- `test_bitcoin --run_test=ipc_tests` — 2 cases pass (raw-UniValue helper ported
  across upstream's ipc_test.cpp → ipc_tests.cpp moveonly combine)
- `cmake --build build_fuzz --target fuzz` — 100%
- `FUZZ={partially_downloaded_block,http_request,torcontrol,rpc,tx_pool,coins_view} -runs=2000`
  — all DONE, no artifacts
- `python3 test/functional/interface_http.py --timeout-factor=8` — Tests successful
- `python3 test/functional/feature_index_prune.py --timeout-factor=8` — Tests
  successful (reaches the F35 unclean-kill/restart section)

## Conflict-stop inventory (~20 stops during replay)

- Keep-both: `TestPartiallyDownloadedBlock` + our new test cases (2 stops),
  `WtxidsToRelay` + ancestry-check helpers in fuzz/tx_pool.cpp,
  tokenbucket/vecdeque includes in util_tests.cpp.
- Adapted to upstream rewrites: HTTP fuzz helpers and helpers' signatures to
  `std::string`/`string_view` (LineReader, HTTPRemoteClient::m_recv_buffer);
  torcontrol ProcessBuffer contracts to `reader.Consumed()`; LineReader
  `Assume` contracts re-expressed on char-iterator internals; CExtKey/CExtPubKey
  default initializers carried onto `KeyFingerprint` members.
- Dropped as superseded: cmpctblock `have_txn[]`/`have_extra_txn[]` accounting
  (subsumed by upstream's `TxSource` enum; our null-slot skip kept, accounting
  assert re-expressed in enum terms); rpc-fuzz `RPCConvertValues` arms (harness
  redesigned to UniValue params; deterministic rpc_tests coverage retained);
  `ci/lint/requirements.txt` deletion confirmed (uv.lock pins match upstream;
  lief comment carried into pyproject.toml).
- Ported: ipc_test.cpp tests into ipc_tests.cpp (empty-Data round-trip; raw
  UniValue JSON helper incl. the exception_ptr-race fix).
- Kept ours over upstream's minimal version:
  `chainstatemanager_delete_chainstate_no_mempool` (on-disk coins db +
  post-delete state assertions).
