# Journal: TODO, FIXME, stub, and deferred-work challenge audit (campaign 96)

Uber-goal rotation. Branch: audit/todo-deferred-work from
audit/resurrection @ 71d5917801. Method: enumerate production TODOs
(66 hits in src/{validation,txmempool,wallet,net_processing}+siblings),
link to origin, turn top items into falsifiable current questions.

## Verdicts

### wallet.cpp:1869 TODO ("take into account failure by ScanResult::USER_ABORT") — FALSIFIED: masked, no defect

Question: does an aborted rescan get reported as successful?
Trace: RescanFromTime (wallet.cpp:1859-1877) returns startTime on
USER_ABORT — conflating abort with full scan, as the TODO warns. But the
SOLE caller (importdescriptors, backup.cpp:430-436) checks
IsAbortingRescan() immediately after and throws "Rescan aborted by
user." The rescanwallet RPC also handles USER_ABORT explicitly
(transactions.cpp:920-921, throws "Rescan aborted."). No user-visible
wrong outcome: the conflated return is masked at both call sites.
Verdict: safe masked debt — journal only, no commit (a doc-note would be
churn).

### validation.cpp:2819 TODO ("Handle return error...") — ALREADY SOLVED ELSEWHERE

The FlushStateToDisk warn-only block-file flush TODO. Fixed by own open
upstream PR 35714 "validation: stop writes after flush failure"
(e1a337ee96 + characterization test) — identified in goal 86 C2.
No new work; tracked by crons.

### Remaining top items (quick verdicts)

- wallet.cpp:1116 "Store all versions of the transaction" — documented
  conflict-storage design limitation, not a falsifiable current defect.
- validation.cpp:2500 "Remove BIP30 checking from height 1,983,702 on" —
  optimization debt (checkpoint-based skip), not a bug.
- validation.cpp:6291 "cache the UTXO set" — assumeutxo belt-and-suspenders
  design note, not a bug.
- validation.cpp:2117 CuckooCache lock requirement — design debt.
- txmempool/net_processing TODOs: reviewed in-line during campaigns
  87/89 — all design notes, none falsifiable as current defects.

## Next queue
(#96 cycle complete: 66 TODOs enumerated, top 5 falsified — 1 masked debt,
1 own-PR'd, rest design debt. Rotate per ledger: #20 micro-optimization.)