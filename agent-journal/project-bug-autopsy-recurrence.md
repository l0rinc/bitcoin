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

## Cycle 2 (2026-08-01, draw 171, raw=3220867768142961582 (63-bit), idx 0/1): dbwrapper failed-construction leak family — autopsy + sibling sweep zero; upstream master STILL missing the destructor (fork fix 461c21cbfa offerable, live)

### Family selection
From `git log --author='pap.lorinc' --oneline HEAD` (non-journal):
chose 461c21cbfa (F14, raii-resource-leaks c2) — a CORE storage
family, over the two wallet-area queued candidates (wallet-
encryption write-failures, BDB overflow-chain; deprioritized per
uber-goal scope).

### Autopsy narrative (from the fix commit)
- Shape: throw-out-of-constructor with raw-owned resources held by
  a PLAIN STRUCT context (LevelDBContext had no destructor) ->
  partial construction runs no destructor -> option-owned
  allocations leak (block_cache NewLRUCache dbwrapper.cpp:142,
  filter_policy :144, info_log :146, penv memenv).
- Exposure: every failed CDBWrapper construction (corrupt/locked
  DB, TryCreateDirectories throw) leaked 79,800 B / 361 allocs per
  19 failed opens under ASan+LSan.
- Why tests missed it: constructions in tests succeed; the failure
  path needed deliberate failed-open injection (the LSan probe).
- Fix: LevelDBContext destructor deletes pdb then the options
  members then penv; Close() deletes+nulls first so success-path
  double-free is impossible (deleting nullptr safe). Verified in
  HEAD: dbwrapper.cpp:202-209 + Close() :295-307.
- Survival window: feature lifetime until 2026-07-30 fork fix.

### Recurrence sweep (present-day siblings) — ZERO new
- dbwrapper.cpp remaining raw owns: Logv heap buffer (:74) freed
  on both arms (:107-109); Close() nulls match the destructor's
  deletes exactly; no other plain-struct context in the file.
- Raw `new CDBIterator` return (dbwrapper.cpp:407, declared
  dbwrapper.h:280): EVERY call site wraps immediately in
  unique_ptr — dbwrapper.cpp:388, txdb.cpp:40, txdb.cpp:253 (into
  CCoinsViewDBCursor's unique_ptr member via make_unique),
  node/blockstorage.cpp:135, index/coinstatsindex.cpp:219,
  index/blockfilterindex.cpp:296,333, index/txospenderindex.cpp:166.
  No unwrapped temporary; the make_unique bad_alloc window is the
  generic C++ caveat, not this family.
- Wider same-shape scan: no other LevelDBContext-like plain struct
  holding owned pointers in core storage.

### Upstream cross-check (2026-08-01)
curl raw.githubusercontent.com/bitcoin/bitcoin/master/src/
dbwrapper.cpp -> LevelDBContext at :197 STILL has NO destructor
(grep -c '~LevelDBContext' = 0). The family is LIVE upstream; the
fork fix remains offerable (as recorded in F14).

### Verdict
DISMISSED (new sibling): family fully mapped; single site fixed
in HEAD with a before/after LSan oracle; all raw-own siblings
immediately RAII-wrapped; upstream exposure re-confirmed live
today (second verifier form: independent upstream source fetch).

### Author-recurrence stats
2 families autopsied (txgraph saturation c1, dbwrapper failed-
construction c2), 0 uncovered siblings; the author's fix pattern
(RAII-ize the context + null-after-delete + LSan before/after
probe) validated again.

### Exact commands
- git show 461c21cbfa; sed/grep line refs above; curl upstream
  dbwrapper.cpp + grep count above.

### Limitations / queue
- Wallet-area families (encryption write-failures, BDB overflow-
  chain) remain queued but descoped unless a core-reachability
  bridge appears.
- Next core candidate if re-drawn: serialize SizeComputer
  WriteVarInt overload family (f6e78b44c0).

## Cycle 3 (2026-08-02, draw 188, raw=12902246674943953776, masked 3678874638089177968, idx 43/45): serialize SizeComputer WriteVarInt overload family (f6e78b44c0) — autopsy + forced-instantiation sibling sweep; ZERO new siblings; contract guard proven both directions

### Autopsy narrative (from the fix commit + HEAD state)
- Shape: dead template code that CANNOT COMPILE when instantiated
  — the SizeComputer WriteVarInt overload called
  GetSizeOfVarInt<I>(n) missing the VarIntMode template argument;
  zero callers tree-wide meant it was never instantiated, so the
  defect survived upstream-inherited until #35 c6.
- Why tests missed it: C++ templates compile bodies only on
  instantiation; dead code has no compile or runtime oracle.
- Fix: one-line repair (VarIntMode::DEFAULT, serialize.h:1150
  today) + boundary battery + mutation sweep (3 mutants killed).
- Survival window: upstream feature lifetime until 2026-07-31.

### Recurrence sweep (present-day siblings) — ZERO new
Forced instantiation of the FULL SizeComputer overload surface
(/tmp/btc105c3/instantiate.cpp, preserved):
- WriteVarInt(SizeComputer&, {uint8..64}_t) + WriteCompactSize +
  generic operator<< + explicit-mode instantiations (DEFAULT/
  uint64, NONNEGATIVE_SIGNED/int64): ALL compile, link, run
  clean (rc=0) — no second uninstantiable overload exists.
- Negative control: WriteVarInt(SizeComputer&, int64_t) (signed,
  implicit DEFAULT) FAILS to compile with the CheckVarIntMode
  static_assert (serialize.h:406) — the mode/signedness contract
  is a compile-time guard, not a convention. Both directions
  proven.
- Grep census: no other GetSizeOfVarInt single-arg call sites
  (only the pinned test at serialize_tests.cpp:192-200).

### Verdict
DISMISSED (new sibling): family fully mapped; single member fixed
with a boundary battery; the overload set now has a forced-
instantiation proof of compilability, and the contract guard
rejects the misuse direction at compile time.

### Author-recurrence stats
3 families autopsied (txgraph saturation, dbwrapper failed-
construction, serialize dead-template), 0 uncovered siblings;
pattern record: the author's fixes pair the minimal repair with
a boundary/mutation oracle — validated third time.

### Exact commands
- git show f6e78b44c0; g++ instantiate.cpp (rc=0); negative
  control via stdin TU (static_assert trace above).

### Limitations / queue
- Sweep covers serialize.h's SizeComputer surface; other headers'
  dead-template members would need their own instantiation TUs
  (no signal for any).
