# Campaign #40 — multi-agent-adjudication

## Cycle 1 (2026-07-29): L4 CheckBlock dup-check equivalence — two independent agents converge on EQUIVALENT; adjudicator spot-verified

### Draw
Random draw over the 12-goal eligible pool (11 pending + 1 CYCLE-1,
#60 excluded as just-cycled): raw=8140190821735971651, index 3 ->
#40 (first cycle). Branch: audit/multi-agent-adjudication from
0982e98f95 (#60 c2 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #40's
campaign-focus block contains bug-archaeology text — same offset
artifact class as #49/#58/#80; title+slug multi-agent-adjudication
authoritative.

### Subject selection
The open L4 item (URGENT.md, contributor-branch-radar c2): the fork
author's branch l0rinc/optimize-CheckBlock-input-duplicate-check
(f3cc8fd27d lineage: 532176cd27, 24cdff5396, c379975b5a, f3cc8fd27d)
restructures the CVE-2018-17144 duplicate-prevout check in
CheckTransaction (skip 1-input, direct-compare 2-input, sorted-vector
3+; CheckBlockBench 335.9 -> 181.9 us claimed). Equivalence was
"PLAUSIBLE, not proven" — the 1-input null-check arm unverified.
Ideal adjudication subject: unresolved, consensus-check, bounded.

### Protocol
Two independent subagents, identical inputs (HEAD tx_check.cpp,
branch diff, IsCoinBase/IsNull/operator< definitions, caller paths),
no shared context, opposite stances:
- Agent A (prover): enumerate the case space, argue equality.
- Agent B (breaker): find ANY accept/reject divergence reachable
  from blocks or mempool.
Adjudicator (me): spot-verify the decisive code facts directly;
no verdict adopted on agent authority alone.

### Agent results (independent, convergent)
Both returned EQUIVALENT with the same proof skeleton:
1. 1-input arm vacuously safe: IsCoinBase() is DEFINED as
   (vin.size()==1 && vin[0].prevout.IsNull())
   (primitives/transaction.h:342-345), so HEAD's non-coinbase null
   loop is vacuous for 1-input txs — the branch skips a check that
   could never fire. A 1-input null-prevout tx IS a coinbase in
   both versions.
2. 3+ arm sorted null-scan covers every null: operator< is
   lexicographic (hash, n); all-zero hash is the total-order
   minimum, so every hash-0 prevout forms a strict prefix; the scan
   full-checks IsNull() on each before the hash!=0 break.
3. Multi-fault precedence identical (dup before null) in both.
4. Caller inventory (validation.cpp:813 mempool, :4125-4131
   CheckBlock loop gated by :4116-4121 first/rest coinbase rules,
   kernel/bitcoinkernel.cpp:1633, fuzz targets) — none can
   distinguish the versions; punishment paths key on the enum only,
   which is TX_CONSENSUS in both.
Agent B additionally confirmed vin.size()==0 handled identically at
the shared top gate, and no UB in the empty-range else arm.

### Adjudicator spot-verification (direct reads)
- IsCoinBase definition: primitives/transaction.h:342-345 — exact
  two-conjunct form confirmed.
- CheckBlock ordering: validation.cpp:4116-4121 (first tx coinbase,
  rest must not) precedes the per-tx CheckTransaction loop
  (:4125-4131, which I read independently in #49 c1 context).
- Branch diff re-read (f3cc8fd27d): the 1-input arm, 2-input arm,
  and sorted-scan break/check sequence match the agents' description.

### Verdict
CONFIRMED (as "equivalence PROVEN", bounded proof by complete case
partition with two independent derivations + adjudicator
verification): the branch is accept/reject AND diagnosis identical
to HEAD in every arm. L4's open question is RESOLVED. The 1.85x
perf claim remains the branch author's bench data (335.9 -> 181.9
us, AppleClang); adoption is the fork author's decision (his own
upstream-PR-shaped work; this rotation records, never adopts
unmerged consensus changes). No local code change: the in-tree
std::set check is correct, merely slower — adopting or not is a
perf decision with an now-verified-equivalence premise.

### Method notes (for future cycles)
- The two-agent prover/breaker pattern converged in one round;
  total cost ~2 subagent runs + 2 adjudicator greps. Suitable for
  other "PLAUSIBLE" items (L1 bloom ctor is the next candidate).
- Both agents cited identical file:line evidence independently;
  disagreement handling was not needed this cycle — the protocol
  for a split verdict (adjudicator runs the deciding experiment)
  is queued but untested.

### Exact commands
- git show f3cc8fd27d -- src/consensus/tx_check.cpp (branch diff);
  git log --oneline f3cc8fd27d -5
- Agent prompts: full case-space proof vs adversarial break (both
  transcripts in session history)
- Adjudicator: sed reads primitives/transaction.h:340-346,
  validation.cpp:4113-4122

### Limitations / queue for cycle 2
- Equivalence is static-analysis-proven, not differentially fuzzed
  (a tx-level differential fuzz of both CheckTransaction versions
  would be the executable confirmation; the proof covers the
  partition, fuzz would sample it — value bounded, queued).
- The bench claim (1.85x) was NOT re-measured on this host
  (AppleClang numbers; aarch64/gcc may differ — a bench reproduction
  is the natural next cell if adoption becomes live).
- L1 bloom-ctor latent item is the next adjudication candidate.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): A11 kernel input_index assert — split verdict; breaker wins on policy + author's own fix branch

### Draw
Re-rank draw over the rebuilt 3-cell queue:
raw=6403733973450834911, index 2 -> #40 (second cycle; c1 queue
"L1 bloom ctor is the next adjudication candidate" — superseded by
#102 c1's L1 replay; A11 kernel input_index assert is next).
Branch: audit/multi-agent-adjudication-c2 from 63b4ed5862
(#106 c2 bookkeeping).

### Protocol (same as c1)
Two independent subagents, identical inputs, no shared context,
opposite stances: defender (assert acceptable for WIP API) vs
breaker (assert is a defect). Adjudicator verifies decisive claims
directly.

### Agent positions
- DEFENDER: acceptable — 7+ sibling index asserts are house style;
  API is explicitly unstable/unreleased (bitcoinkernel.h:75-76);
  NDEBUG is stripped project-wide (ProcessConfigurations.cmake:
  121-125) so no silent UB, and an un-ignorable abort beats a
  nullable status; precondition is cheaply caller-checkable; no
  in-tree consumer. Flip triggers: stabilization, release
  inclusion, data-derived-index consumer.
- BREAKER: defect — developer-notes.md:369-371 ("must never be
  used to validate user, network or any other input"; an API
  caller IS the user); intra-function inconsistency (every other
  caller error uses the status channel); precondition undocumented;
  abort from a shared lib = host DoS for embedders; fix tiny and
  API-permitted; upstream master still carries it; the fork
  author's own branch already implements the fix (6f23568be8).

### Adjudicator verification (direct)
- developer-notes.md:369-371 — quote verified verbatim.
- remotes/l0rinc/l0rinc/kernel-handle-invalid-c-api-arguments:
  6f23568be8 "kernel: report invalid script verification
  arguments" — "Invalid script verification flags and input indexes
  currently trigger public C-API assertions. Report both through
  btck_ScriptVerifyStatus." Verified: the exact fix exists on the
  fork author's own branch (plus d2f17ee891, 80257396b6 in the
  same invalid-arguments series).

### Verdict (adjudicated)
DEFECT-CLASSIFIED, parked with the author: the breaker wins — the
assert violates the project's stated assertion policy at a public
API boundary, and the fork author demonstrably agrees (his branch
replaces it with a status report). The defender's sibling-
convention argument holds only for the pointer-getter sites, which
lack an error channel; this function HAS one and uses it for every
analogous input error. Severity for THIS tree: none today
(unreleased WIP API, no in-tree caller, unreleased). No local fix:
the fix belongs to the author's upstreaming path (his branch), not
to this rotation (rotation records, never adopts the fork author's
own work). A11 in suspicion-index.md updated accordingly.

### Method note
First split verdict of the pattern; the adjudication protocol
(decisive-document + branch evidence over agent authority) worked
as designed.

### Exact commands
- two explore subagents (defender/breaker prompts; transcripts in
  session history)
- sed doc/developer-notes.md:367-378
- git log/show remotes/l0rinc/l0rinc/kernel-handle-invalid-c-api-
  arguments

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-08-01): L4 executable confirmation — tx-level differential fuzz of HEAD vs branch dup-check: 300k structured cases, ZERO divergences (verdict + reason code)

### Draw
RE-RANK draw 166 over the 7-cell pool: raw=14119870030197581011,
masked 4896497993342805203 -> idx 4 -> #40 CheckTransaction
differential fuzz (c1 queue: "executable confirmation; proof covers
the partition, fuzz would sample it"). Branch:
audit/multi-agent-adjudication-c3 from 40102e35cb.

### Differential (/tmp/btc40c4_probe.cpp, branch f3cc8fd27d objects local)
Both dup-check excerpts compiled verbatim against real CTransaction
objects (HEAD set-based tx_check.cpp:41-45 + cb/null branches vs
branch skip-1 / direct-compare-2 / sorted-3+), driven by a seeded
mt19937_64 corpus (300,000 txs, 1-8 inputs, ~1/3 planted
duplicates, 1/5 planted null prevouts, random n-values).
- RESULT: cases=300000 planted_dups=99760 divergences=0.
- Compared at the Verdict level (OK / bad-txns-inputs-duplicate /
  bad-txns-prevout-null / bad-cb-length) — accept/reject AND
  diagnosis identical in every case, exactly matching the c1 proof
  (1-input null-prevout IS coinbase by definition, so both routes
  hit bad-cb-length only).

### Verdict
CONFIRMED (executable second form): the static partition proof
(c1) now has a differential-fuzz witness with a seeded corpus
(seed 0xC0FFEE, reproducible). L4 is closed by proof AND by
sampling. The branch author's perf claim remains unmeasured here
(AppleClang numbers; adoption is his decision).

### Limitations / queue
- The corpus covers the check region only (no MoneyRange/size
  arms — those are HEAD-identical text in the branch).
- #40 queue: empty (L1 bloom-ctor candidate was c2-covered). 
  Campaign COMPLETE on current surface.

## Rotation note
Three cycles; adjudication surface closed.

## Cycle 4 (2026-08-02, draw 231, raw=16378166984725965126, masked 7154794947871189318, idx 4/6): corpus-extension tail assessed — VACUOUS (the extension region is HEAD-identical text in the branch; zero possible divergence); campaign stays COMPLETE

### Assessment (the c3 Limitations tail)
Extending the differential corpus to MoneyRange/size arms was
assessed: those arms are HEAD-IDENTICAL text in the branch
(the c1 diff showed the branch touches only the check region),
so an extended differential can only return 0 by construction —
it tests nothing the branch changes. The tail is not a cell;
it is the null experiment. Recorded and closed.

### Verdict
Cell VACUOUS (provably null); campaign remains COMPLETE on the
current surface (c1 static proof + c3 300k-case differential).

### Exact commands
- c1 branch diff scope re-check (check-region-only).

### Limitations
- None new.
