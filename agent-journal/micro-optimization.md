# Journal: simple micro-optimization discovery and proof (campaign 20)

Uber-goal rotation. Branch: audit/micro-optimization from audit/resurrection
@ a44b389142. Method: measured hot site via existing benchmarks + perf,
one hypothesis, interleaved before/after proof.

## Finding 1: IsRoutable dedup in GetNetClass — CONFIRMED, committed 769822b5a6

Hot site: AddrManAdd bench showed 58µs/add; perf attribution: AddSingle →
GetNewBucket (47%) → GetGroup (33%) → classification helpers (~18%) with
IsRoutable/GetNetClass/HasLinkedIPv4 recomputed multiple times per insert.

Hypothesis: GetNetClass evaluates IsRoutable() then HasLinkedIPv4()
re-evaluates it — redundant on the branch where routable is already true.

Fix (6-line diff): cache IsRoutable() once; inline the HasLinkedIPv4()
predicate list with an equivalence comment (identical by construction).

Proof (build-before Release gcc aarch64, 5 interleaved runs each, median):
- before: 58.998M ns/op (358.0M ins, 61.1M bra)
- after: 53.79M ns/op (314.5M ins, 52.7M bra)
= -8.8% wall, -12.2% instructions, -13.8% branches; err ≤ 0.8%.
Correctness: net_tests + netbase_tests + addrman_tests green.
Commit: 769822b5a6 with full table in message.

## Campaign 20 cycle complete

One definitive measured win. Next per ledger: re-rank — #0 continuous
bug-mining, with #56's queued TransactionCanBeBumped fuzz target as its
first task.