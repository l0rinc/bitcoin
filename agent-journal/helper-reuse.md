# Campaign #58 — helper-reuse

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/helper-reuse. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): mempool hex-tx-array decode loop — exact duplicate deduplicated (4f97fbfe1e)

### Draw
First draw of the turn hit #19 (seed raw=13757765497114791125, idx 9) —
journal on audit/benchmark-integrity shows c3 closed units/batching +
timed-region with "queue exhausted" (c1 prevector fix 138ef3c044, c2
mutation-sweep, c3 composition; only new-PR benches remain). Marked
exhausted per its own evidence; redrew: raw=14344813522794569601,
pool 65, idx 38 -> #58.

### Method
Exact-clone scan (normalized 7-line runs, sha1-windowed) over src/rpc,
src/wallet, src/node, net_processing, net, init, validation, rest,
httprpc, httpserver — bounded per rotation. Candidates triaged:
- RPCArg/RPCResult help-text duplicates: data, not logic — rejected
  (abstraction would hide case-specific meaning).
- UTXO-snapshot function pairs (blockchain.cpp:86/3384, 96/3208):
  declarations vs definitions — false positives.
- mempool.cpp:343 vs 1404: the only exact code-logic duplicate.

### Finding
testmempoolaccept and submitpackage open-coded the identical
decode-a-hex-tx-array loop (same reserve, same DecodeHexTx call, same
error text echoing rawtx). submitpackage interleaves a per-tx burn
check (MAX_BURN_EXCEEDED). No existing helper covers the loop; other
"TX decode failed" sites (rawtransaction.cpp, wallet/rpc) use different
messages — not exact duplicates, left alone.

### Change (4f97fbfe1e)
File-local DecodeHexTransactions + one minimal per_tx_check overload
(the campaign-sanctioned hook) preserving the decode/check interleave.
Equivalence: identical parse order and exception type/message/position
for single-defect inputs; identical error precedence for doubly-invalid
inputs (burn-in-tx1 vs undecodable-tx2 still throws burn first). No
state/lock/ownership/ordering change; no dead code created.

### Verification
- cmake --build build-before -j4 --target bitcoind — clean.
- test/functional/mempool_accept.py + rpc_packages.py green; both
  assert the exact diagnostics (mempool_accept.py:101,
  rpc_packages.py:394) and rpc_packages covers the burn path
  (test_maxburn_submitpackage). Behavior preservation is test-proven.

### Verdict
- CONFIRMED exact duplicate; FIXED by minimal dedup (no defect claimed —
  code-shape finding per campaign charter).

### Limitations
- Scan window 7 lines exact-normalized; semantic near-duplicates
  (different variable names/messages) not scored — e.g.
  rawtransaction.cpp:609's indexed-message variant is deliberately
  untouched (case-specific diagnostics).
- qt/ excluded from scope this cycle.

### Next queue for this campaign
- Near-duplicate families needing a semantic (not exact) pass:
  PSBT-decode error sites in rawtransaction.cpp (6 occurrences of the
  same strprintf(ErrorString) pattern) — check whether a shared
  DecodePSBTOrThrow helper already exists before touching.
- wallet/rpc fixture/builder duplicates (spend.cpp 1236/1732 vout_index
  blocks are help text — verify, likely reject).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
