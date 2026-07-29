# Campaign #50 — fuzz-introspector-blockers

Base: 8ad188e4fe (journal commit for #95 cycle-2 on
audit/db-semantics-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/introspector-blockers. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): PSBT blocker map — magic is weak; the real blocker is harness truncation (ConsumeRandomLengthString starves long valid documents)

### Draw
Random draw over the 32-goal pool (19 pending + 13 CYCLE-1; #41
excluded as just-cycled): raw=7696717260708278889, index 9 -> #50.

### Blocker 1: magic prefix — MEASURED WEAK
Hypothesis: "psbt\xff" magic is a structural blocker. Refuted by
measurement: 3000 runs empty corpus -> cov 467; 3000 with
"psbt\xff" dictionary -> cov 464 (no delta; 5 bytes is discoverable).

### Blocker 2: decode-success gate — REAL, seed-solvable
The whole serialize/analysis half of the target sits behind
`if (!psbt_res) return;` (psbt.cpp:37-39). UNCOVERED_FUNC after
6000 runs (empty+dict): PartiallySignedTransaction::Serialize 0/82,
PSBTInput::Serialize 0/344, PSBTOutput::Serialize 0/152 — decode
almost never succeeds from random strings. A 19-byte minimal valid
PSBT (psbt\xff + GLOBAL_UNSIGNED_TX of an empty tx) opens the gate:
PST::Serialize<VectorWriter> and AnalyzePSBT become covered.

### Blocker 3 (the real one): harness input-structure truncation
psbt.cpp:37 feeds `fuzzed_data_provider.ConsumeRandomLengthString()`
into DecodeRawPSBT — the provider emits a RANDOM-LENGTH PREFIX of
the input, so long documents almost never arrive whole. Measured
with hand-built seeds (validated through decodepsbt RPC first):
- psbt_min (19 B, zero-input, valid): 1372 edges in isolation —
  decodes, round-trips, analyzes.
- psbt_1in (136 B, one input with non_witness_utxo, valid per
  decodepsbt RPC including the full input map): 528 edges in
  isolation — truncation makes whole-document decode rare.
Consequence: PSBTInput::Serialize (344 edges) and
PSBTOutput::Serialize (152 edges) remain at 0/344 and 0/152 even
with valid seeds — the harness's consumption model, not the seeds,
is the blocker. (Bring-up: my first 1-in seed had the txid endianness
backwards — decodepsbt caught it; corrected and RPC-verified.)

### Verdict
- CONFIRMED (blocker identified and mechanism-proven): the PSBT
  target's serialize side (~500 edges) is unreachable because
  ConsumeRandomLengthString truncates long valid documents. This is
  a HARNESS-REALISM blocker (campaign class: input structure), not a
  production defect and not a seed problem.
- Recommended minimal fix (next cycle, one line): consume the full
  buffer as the document (ConsumeRemainingBytes-style) or prefix
  documents with an explicit length contract, so valid long PSBTs
  survive whole; re-run this before/after table to close it.

### Exact commands
- FUZZ=psbt build_fuzz/bin/fuzz -runs=N [-dict] [-print_coverage=1]
  /tmp/btc50_{empty,dict,seed,i0,i1,iso}
- decodepsbt RPC verification of both hand-built seeds
- seed constructors recorded in the journal history (python struct +
  sha256d without reversal for the serialized txid)

### Limitations / queue
- The one-line harness fix is NOT applied this cycle (scout stage);
  queued with the exact before/after table above as its acceptance
  test.
- Key-origin/HD-keypath serializers (SerializeHDKeypath et al.)
  blocked behind the same gate — same fix covers them.
- Corpus seeds preserved: /tmp/btc50_seed/{psbt_min,psbt_1in}.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
