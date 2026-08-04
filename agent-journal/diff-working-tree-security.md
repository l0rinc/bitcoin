# #115 — Committed-diff, working-tree, and pre-commit security regression audit

## Cycle 1 (2026-08-04, draw r163 raw=5739169061478724851) — first cycle: audit the rebase-resolution diff

Target: the committed diff MY rebase resolutions introduced (the
most security-relevant fresh diff): backup/...-pre-rebase-1ed14c
(2b4ee58377) -> HEAD across the hand-resolved files.

Method: tree diff per file, classify every hunk as upstream-change /
intended-adaptation / unexplained; contract-level review of the
adaptations; battery evidence cross-ref (cycle 332/334).

Findings by file:
- key.h/pubkey.h: vchFingerprint[4]+memcmp -> KeyFingerprint(std::array)
  with ==; our default-initializers carried. Semantics-preserving
  (4-byte array equality), covered by descriptor_tests/rpc_tests.
  Upstream's id_key_fingerprint() helper added. CLEAN.
- blockencodings.cpp: have_txn[]/have_extra_txn[] -> upstream TxSource
  enum; our unconditional extra_count-- replaced by upstream's precise
  extra_count -= (tx_source == EXTRA) (strictly more correct — the
  unconditional form could underflow on MEMPOOL-sourced collisions,
  which our have_extra_txn commit had patched separately; convergence
  verified). The ONLY contract removal is our own
  Assume(wtxid == tx->GetWitnessHash()) — over-strong relative to
  upstream's claim-based extra_txn contract (cycle-332 record);
  upstream's FillBlock merkle/mutation check remains the real guard.
  Null-slot skip (our genuine fix) intact. CLEAN.
- coins.cpp/torcontrol.cpp/util/string.cpp: Assume-contract
  re-expressions only (debug-only invariants; production behavior
  unchanged). CLEAN.
- net_processing.cpp (402-line delta): 100% upstream commits
  (delay-queue subsystem + #35832) — zero of my resolution hunks;
  reviewed separately in cycles 332/336. N/A for this cell.
- Test-only files (blockencodings_tests, httpserver_tests, torcontrol
  tests, ipc_tests, interface_http.py, fuzz targets): adaptations
  verified by the green battery + functional + smoke runs. CLEAN.

Verdict: NO security regression in the rebase-resolution diff.
Every contract change is upstream's or a documented restoration of
upstream semantics; all fork fixes' substances survived (null-slot
skip, member initializers, Assume contracts, shrink_to_fit, F35 test,
F33 lock). DISMISSED for this diff.

Next cells for #115: pre-commit review discipline for FUTURE fix
commits (diff audit before each exposure); working-tree hygiene
(untracked crash-*/slow-unit-* artifacts in repo root are user's —
leave alone per protocol).
