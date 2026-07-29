# Campaign #105 — project-bug-autopsy-recurrence

Base: 5bcde44b1e (journal commit for #74 cycle-2 on
audit/memory-pressure-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/autopsy-recurrence. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): txgraph saturation family autopsy — recurrence mapped; all siblings covered by existing guards + oracles

### Draw
Random draw over the 41-goal pool (24 pending + 17 CYCLE-1; #74
excluded as just-cycled): raw=3066498048559476725, index 21 -> #105.

### Autopsy subject
The fork's txgraph fee-saturation fix series (all pap.lorinc):
7abd26bb02 (fuzz: exercise full-range saturation), 4e89080615
(handle saturated chunk sum checks), 853a708a73 + f1d3c0f450
(canonicalize saturated chunk fee aggregation), 3ae78dbd25 (collapse
saturated disconnected chunks).

### Autopsy reconstruction
- Introducing change: upstream cluster-mempool txgraph — FeeFrac
  (int64 fee, int32 size) sums accumulated over clusters/chunks with
  NO saturation semantics; local priority deltas (RBF/priority
  overrides) can push sums to int64 limits.
- Failed assumption: "fees stay in MoneyRange so sums can't
  overflow" — true for individual consensus txs, false for
  local-priority-adjusted cluster aggregates.
- Exposure: FUZZ=mini_miner reached a connected accepted cluster
  exposing a disconnected main chunk and aborted assertion builds
  (3ae78dbd25 commit message).
- Why tests missed it: upstream tests exercise ordinary fee ranges;
  the overflow needs deliberate full-range fee injection — exactly
  what 7abd26bb02's fuzz target adds.
- Survival window: upstream feature lifetime until the fork's series
  (2026-07).

### Recurrence sweep (present-day siblings)
Grep of all fee arithmetic in src/txgraph.cpp:
- Sum paths: CheckedFeePerWeightSum (CheckedAdd -> SaturatingAdd with
  overflow flags, txgraph.cpp:35-59) covers chunk aggregation;
  AssumeMatches postcondition pinned in fuzz builds.
- Per-entry assignments (m_feerate.fee = fee at :2878) are
  consensus-MoneyRange-bounded upstream of entry.
- Builder chunking (:1147) — saturated-sum over-merge is the exact
  case 3ae78dbd25's collapse handles.
- Cross-check: #65 c2 radar entry 2 (txgraph-equal-feerate-prefix-
  overflow branch) — prefix tracking already present locally; the
  overflow guard belongs to this same covered class.

### Verdict
- DISMISSED (new sibling): the recurrence family is fully mapped and
  every discovered member has a guard plus a fuzz oracle; no
  uncovered fee-accumulation site found in present HEAD.
- Autopsy artifact (this section) recorded as the family's durable
  prior: integer-accumulation-at-limits is the author's recurring
  shape, and the fork's response pattern (fuzz full-range exercise ->
  saturating arithmetic + Assume postconditions) is validated by the
  zero-finding sweep.

### Exact commands
- git log --author='pap.lorinc' -- src/txgraph.cpp (series inventory)
- git show 3ae78dbd25 --format=%B (autopsy narrative)
- grep fee-sum/assignment/comparison sites across src/txgraph.cpp

### Limitations / queue
- One family autopsied. Next candidates: the wallet-encryption
  write-failure family (l0rinc/wallet-encryption-write-failures
  branch vs local b8fcf9ed17 lineage — the #65 radar's conflicting-
  assumptions flag) and the BDB overflow-chain family (71cf0ba593).
- Author-recurrence stats: 1 family, 0 new siblings — recorded for
  the evidence-based ranking table.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
