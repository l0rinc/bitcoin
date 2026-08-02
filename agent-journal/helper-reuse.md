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

## Cycle 2 (2026-07-29): c1 stack backport + PSBT decode-or-throw dedup (6 exact sites)

### Draw
Random draw over the 16-goal eligible pool (13 pending + 3 CYCLE-1,
#6 excluded as just-cycled): raw=2597777539898758520, index 8 ->
#58. Ledger had NO row and the pool treated #58 as pending, but
audit/helper-reuse holds a complete c1 (fix 4f97fbfe1e + journal
8dc79ee2a0, 2026-07-28) stranded off-lineage — the #66 problem
again. Branch: audit/helper-reuse-c2 from 3ea00a44b9 (#6 c3
bookkeeping). Catalog note: #58's campaign-focus block contains
supply-chain text (the focus blocks in this region are offset from
their title/slug pairs, same artifact class as #49); title+slug
helper-reuse is authoritative.

### Stranded-work recovery
Cherry-picked the c1 stack into this branch: fix as a7067512e8,
journal as 3e887dbbf7 (uber-rotation.md conflict resolved ours —
historical file; the c1 row is restored in uber-goal-state.md
instead). Verification at HEAD: cmake --build build-before --target
bitcoind clean; mempool_accept.py AND rpc_packages.py both
"Tests successful" (both assert the exact diagnostics the dedup
preserves).

### Finding (c1 queue cell 1)
rawtransaction.cpp open-coded the identical 3-line
decode-base64-PSBT-or-throw block SIX times (decodepsbt,
descriptorprocesspsbt, combinepsbt loop, finalizepsbt,
joinpsbts loop, analyzepsbt): same DecodeBase64PSBT call, same
if(!psbt_res) throw JSONRPCError(RPC_DESERIALIZATION_ERROR,
strprintf("TX decode failed %s", ...)). A 7th byte-identical copy
lives in wallet/rpc/spend.cpp:1637 — left untouched (cross-file
sharing would need a header move; wallet deprioritized per scope
note). Checked first: no existing OrThrow/ParsePSBT helper anywhere
(rpc/util.h, node/psbt.h, rawtransaction.cpp).

### Change
File-static DecodeBase64PSBTOrThrow(const std::string&) (mirroring
c1's file-local pattern) + six one-line replacements (+16/-30).
analyzepsbt's `const PartiallySignedTransaction& psbtx = *psbt_res`
became a const value (read-only use; safe). No error
type/code/message/position change for any input; loop sites keep
identical first-failure-wins semantics.

### Verification
- cmake --build build-before -j4 --target bitcoind — clean.
- test/functional/rpc_psbt.py — Tests successful; it exercises ALL
  six RPCs (decodepsbt, descriptorprocesspsbt :389-420, combinepsbt
  :301/346, finalizepsbt, joinpsbts :1058-1084, analyzepsbt :469)
  and asserts the "TX decode failed" family of diagnostics.

### Verdict
CONFIRMED exact duplicate (6 sites); FIXED by minimal file-local
dedup. No defect claimed — code-shape finding per campaign charter.

### Limitations / queue for cycle 3
- wallet/rpc/spend.cpp:1637 (7th copy) untouched — needs a shared
  home (rpc/util.h?) and the wallet-scope decision.
- c1 queue cell 2 (wallet/rpc spend.cpp 1236/1732 vout_index
  suspected help-text duplicates) still open.
- Exact-clone scan window was 7 lines in c1; a wider normalized
  window over consensus-critical helpers (merkle, sighash, varint)
  is the natural next cell.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): 7th copy deduplicated — DecodeBase64PSBTOrThrow moved to rpc/rawtransaction_util (shared home)

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=6707348440583223643, index 3 -> #58 (third cycle; c2 queue
cell "spend.cpp:1637 (7th copy) — needs a shared home and the
wallet-scope decision"). Branch: audit/helper-reuse-c3 from
410b1340fc (#60 c5 bookkeeping).

### Finding
The shared home already exists: rpc/rawtransaction_util.h/.cpp —
and BOTH files include it (rawtransaction.cpp:27, spend.cpp:12).
So the c2 "cross-file sharing needs a header move" reduces to a
function move with zero new dependencies.

### Change
- Moved DecodeBase64PSBTOrThrow from rawtransaction.cpp's
  file-static to rpc/rawtransaction_util.{h,cpp} (verbatim body;
  added psbt.h / node/psbt.h / util/result.h includes).
- spend.cpp: replaced its 3-line copy with the shared call
  (+1/-4 lines).
- rawtransaction.cpp: file-static deleted (call sites unchanged).

### Verification
- cmake --build build-before --target bitcoind: clean.
- rpc_psbt.py: Tests successful — covers the rawtransaction.cpp
  sites AND the spend.cpp walletprocesspsbt site (rpc_psbt.py:796
  asserts "TX decode failed" via walletprocesspsbt on rawtx).
  Behavior preserved on both.

### Verdict
CONFIRMED exact duplicate (7th copy); FIXED by moving the helper
to its existing shared header. No defect claimed — code-shape
finding per campaign charter; minimal-diff, no new dependencies.

### Exact commands
- reads: rpc/rawtransaction_util.h/.cpp, spend.cpp:1630-1645
- build + rpc_psbt.py run

### Limitations / queue
- qt/GUI PSBT decode sites (if any) unexamined (deprioritized).
- The campaign's helper-dedup cells are now all closed (mempool
  c1, PSBT c2+c3).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-08-02, draw 216, raw=6114129767934136255 (63-bit), idx 10/15): duplicate-family census — DecodeHexTx all-shared; the AmountFromValue/ParseHexUV near-twins are binary-separated by design (CLI vs RPC error envelopes); queue EMPTY

### Census (post c1-c3 closures)
- DecodeHexTx: 13 call sites across 8 files, ALL through the
  shared core_io helper — no local copies.
- AmountFromValue: TWO implementations with identical 6-line
  logic — bitcoin-tx.cpp:555-563 (static, runtime_error,
  decimals=8 inline) and rpc/util.cpp:98-108 (JSONRPCError,
  decimals param). NOT an exact duplicate: the exception types
  differ by binary contract (CLI tool errors vs RPC envelope),
  and bitcoin-tx does not link the RPC util TU — sharing would
  ADD a cross-binary dependency for 6 lines, against the
  minimal-helper-extension rule. Classified: near-twin by
  design, not a dedup candidate.
- ParseHexUV: same shape (per-binary copies, CLI/RPC error
  split) — same classification.
- ParseFixedPoint: single strencodings implementation, both
  wrappers call it (no logic duplication at the parse layer).

### Verdict
Queue EMPTY: every exact-duplicate family found by the census
is already closed (mempool c1, PSBT x7 c2+c3); the surviving
near-twins are contract-separated per binary. No commit
manufactured (journal-only per policy).

### Exact commands
- grep DecodeHexTx/ParseFixedPoint censuses above; sed reads
  bitcoin-tx.cpp:550-575, rpc/util.cpp:96-115.

### Limitations / queue
- qt/GUI helper copies remain deprioritized.
- A future shared CLI-RPC error-mapping helper would only pay
  off if a THIRD copy appears (standing rule from c3).
