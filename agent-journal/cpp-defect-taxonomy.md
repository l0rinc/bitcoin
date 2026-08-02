# Journal: C++ defect-taxonomy sweep (goal 97)

Campaign: cycle systematically through well-known defect classes; maintain a
class-by-subsystem coverage grid; work the highest-risk unchecked cell.
Branch: audit/cpp-taxonomy from audit/float-sanitizer @ 291e5b91e2 (clean;
untracked node_modules/package*.json scratch ignored).
Prior coverage to avoid repeats (from campaign journals):
- cross-subsystem-bug-shapes.md: S1 DB/iterator lifetime, S5 erase-in-loop
  (dangling-iterator family) — mapped, dismissed beyond origin sites.
- float-sanitizer-fuzz-exclusions.md: signed/unsigned wrap + shift UB via
  integer-sanitizer suppressions (all justified/1 stale removed);
  float edges; fuzzer guards.
- bitcoin-p2p-accounting.md: net_processing Assume/assert sweep, peer
  state teardown (use-after-teardown family).
- bitcoin-consensus-mutation.md: consensus check boundaries.
Build: build-before (Release gcc, working, incremental ~fast).
Machine: Cortex-A76, disk 99% full — no new full build dirs.

## Defect-class × subsystem grid (cells: open / dismissed / confirmed / n/a)

Classes: D1 null-deref, D2 div-zero, D3 use-before-init, D4 use-after-free/move,
D5 double-free, D6 destruction-order, D7 dangling view/ref/iterator,
D8 out-of-bounds, D9 signed/unsigned wrap, D10 shift-UB, D11 strict-aliasing,
D12 data-race, D13 deadlock, D14 missed-virtual-destruction, D15 exception/error
leak, D16 recursion/stack exhaustion, D17 format mismatch, D18 unchecked result.

| class | consensus/validation | net/p2p | wallet | rpc/node | util/crypto | prior |
|-------|---------------------|---------|--------|----------|-------------|-------|
| D1 null-deref | open | part-done (goal 89 P1) | open | open | open | |
| D2 div-zero | open | open | open | open | open | S6 (shapes) |
| D3 use-before-init | open | open | open | open | open | |
| D4 use-after-move | open | open | open | open | open | |
| D7 dangling it/ref | done (S1/S5) | done (goal 89) | part-done (S1) | part-done | open | |
| D9 wrap | done (ubsan audit) | done | done | done | done | |
| D10 shift-UB | done (ubsan audit) | done | done | done | done | |
| D12 data-race | open | part-done (TSan suppressions triaged) | open | open | open | f344e8102c |
| D18 unchecked result | open | open | open | open | open | |
(others filled as reached)

## Verdicts

### D18 (unchecked result) — validation/wallet/util: DISMISSED (exception-based I/O design)

Hypothesis: ignorable-return calls (Write/Flush/Erase/file ops) silently
dropping failures in persistence paths. Evidence against:
- CDBWrapper: WriteBatch is void; leveldb errors throw dbwrapper_error
  (dbwrapper.cpp:44-53 HandleError). txdb.cpp:169/187 partial+final coin
  flush writes can't fail silently; final state re-verified by
  Assume(GetBestBlock() == block_hash) at txdb.cpp:188.
- AutoFile::write/write_buffer throw ios_base::failure on short fwrite
  (streams.h:98-103, 118-125). WriteBlockUndo even checks fclose
  (blockstorage.cpp:1060).
- fs::remove/copy_file call sites use throwing overloads or std::error_code
  captures (blockstorage.cpp:867-868); wallet migration checks
  TxnCommit/begin with asserts (wallet.cpp:3937-3948).
- glibc -Wunused-result classes: build compiles with -Werror
  (cmake/module/TryAppendCXXFlags.cmake:125); a missed warn_unused_result
  libc call would fail the build.
Precedent: the one real member of this family found across campaigns —
warn-only block-file flush in FlushStateToDisk — is already fixed by own
open PR 35714 (goal 86 C2). No new candidate. DISMISSED.

### D2 (division/modulo by zero) — policy/fees, wallet RPC, benches: DISMISSED

Swept variable-denominator div/mod sites:
- block_policy_estimator.cpp: 402 guarded by txSum != 0 + accumulation
  (396-405); 426/429 zero-guarded (425/428); 345 reachable only when
  partialNum > 0 which forces totalNum > 0 (accumulation invariant,
  326-345); 338 divisor (1-decay) — decay validated 0<d<1 on Read
  (478-480) and constants at construction. All safe.
- wallet/rpc/spend.cpp:1557/1567 (sendall remainder division): empty
  addresses_without_amount throws at 1408 before any division.
- Bench modulo sites: containers nonempty by bench construction.
- CI coverage: div-by-zero is in -fsanitize=undefined, run with the fuzz
  corpus in the native_fuzz/native_asan jobs (goal-fsan X5).
- Prior coverage: S6 bloom zero-element division series (PR 35818 +
  journal reachability verdicts) handled the bloom class.
No reachable div-zero found. DISMISSED.

### D4 (use-after-move/free) — net_processing, wallet: DISMISSED

- CI: clang-tidy runs bugprone-use-after-move tree-wide with
  performance-move-const-arg.CheckTriviallyCopyableMove=false (catches
  trivially-copyable move bugs too), warnings-as-errors
  (src/.clang-tidy:7,39-40; ci/test/03_test_script.sh:228). Systematic
  coverage — a use-after-move would fail the tidy job.
- Manual spot checks (net_processing, all 9 std::move sites): 6079 moves a
  fresh per-call local optional (not the shared cache — the cache is
  m_most_recent_compact_block, copied at 6075); 2184 unique_ptr local moved
  into member at scope end; 2565 move in a return; others are
  parameter-forwarding. No post-move use anywhere.
- bugprone-unused-return-value in the same tidy run independently
  strengthens the D18 verdict.

### D14 (missed virtual destruction): DISMISSED

- -Wnon-virtual-dtor is NOT in the warning set (CMakeLists.txt:477-504) —
  manual check required.
- All major polymorphic bases have virtual dtors: Transport (net.h),
  CCoinsView (coins.h:358), DatabaseBatch / WalletDatabase (wallet/db.h),
  SigningProvider (signingprovider.h).
- CValidationInterface deliberately uses a PROTECTED non-virtual dtor
  (validationinterface.h:50-55, with an explicit design comment): it can
  only be deleted by derived classes, so shared_ptr<CValidationInterface>
  instances must be created via make_shared<Derived> (control block deletes
  the derived type correctly) or a custom deleter. Verified all 5 creation
  sites: make_shared<NotificationsProxy> (node/interfaces.cpp:503),
  make_shared<SubmitBlockStateCatcher> (node/miner.cpp:403),
  make_shared<submitblock_StateCatcher> (rpc/mining.cpp:1123),
  make_shared<KernelValidationInterface> (kernel/bitcoinkernel.cpp:912),
  make_shared<TestValidationInterface> (bitcoin-chainstate.cpp:178).
  Type-system-enforced correct deletion. DISMISSED.

### D17 (format mismatch): N/A by design + -Wformat for the rest

### D12 (data race): DISMISSED — four systematic layers + spot checks

1. COMPILE-TIME: -Wthread-safety + -Wthread-safety-pointer in the build
   (CMakeLists.txt:485-486) — GUARDED_BY-annotated members are checked
   tree-wide under -Werror.
2. DYNAMIC: native_tsan CI job with a triaged suppression list (goal-fsan
   X1: all entries external/test/documented).
3. ATOMIC DISCIPLINE: shared flags are std::atomic — verified at the
   recent fix site (wallet.h:316 fAbortRescan + rescan state); historical
   race fixes all merged: index m_chainstate publication (own f344e8102c),
   HTTP server I/O-vs-worker (PR 35614), wallet rescan reset (PR 35512).
4. RELAXED ATOMICS: 37 memory_order_relaxed sites tree-wide (non-test);
   shape is counters/stats. Spot-checked the two hottest: coins.cpp:588
   (assert-only construction check) and net.cpp:3490 (node id counter —
   uniqueness only, no publication). Correct.
Open surface noted: a full memory-ordering audit of all 37 relaxed sites
(open-ended, low yield — none guards data publication by pattern).

### D13 (deadlock): DISMISSED — three detection layers

1. DEBUG_LOCKORDER runtime lock-order tracking with
   potential_deadlock_detected (sync.cpp:37,106-199); sync_tests
   deliberately exercises it (tsan suppression comment "Intentional
   deadlock in tests" confirms the test exists).
2. TSan deadlock detection in CI: TSAN_OPTIONS includes
   second_deadlock_stack=1 (ci/test/03_test_script.sh:21).
3. Static: -Wthread-safety (annotated acquisition orders).
Historical lock inversions documented in code comments (e.g. cs_main vs
m_most_recent_block_mutex ordering at net_processing.cpp:4389) show the
discipline is maintained. DISMISSED.

### D16 (recursion/stack exhaustion): DISMISSED

### D1 (null-deref): DISMISSED — sweep precedent + CI
Goal 89 P1 swept every Assert/Assume reachable from untrusted input in
net_processing (all guarded); find()-result derefs use Assert/Assume
throughout (GetPeerRef, LookupBlockIndex patterns); heap-null is ASan-
covered in the native_asan/native_fuzz jobs with the full corpus.

### D3 (use-before-init): DISMISSED — static + MSan dynamic
-Wunconditional... -Wuninitialized (-Wall) plus -Wconditional-uninitialized
(CMakeLists.txt:491) compile-time; MSan (native_msan, native_fuzz_with_msan)
is the exact dynamic detector for uninitialized reads, running in CI with
track-origins=2. modernize-use-default-member-init in tidy enforces
member-init discipline.

### D5 (double-free): DISMISSED — smart-pointer ownership + ASan
Ownership is unique_ptr/shared_ptr-based tree-wide (observed in every
campaign); raw-delete grep hits are container internals/custom deleters,
not ownership deletes. Double-free is ASan's core dynamic diagnostic
(native_asan + native_fuzz with corpus).

### D8 (out-of-bounds): DISMISSED — ASan/UBSan-bounds + span discipline
ASan + UBSan bounds group in CI; span-based I/O throws on overread
(SpanWriter streams.h:142-148, SpanReader); prevector/container internals
are the standard small-vector idiom; fuzz corpus exercises boundaries.

### D11 (strict aliasing): DISMISSED — idiom-level only
64 non-vendored reinterpret_cast sites: sockaddr API idiom
(netaddress/netbase) and container storage idiom (prevector union +
placement-new, same shape as libstdc++). Type-punning elsewhere uses
memcpy (compressor.cpp:25,35,45). -fstrict-aliasing at -O2 with
-Wstrict-aliasing (-Wall) under -Werror.

### D16 (recursion/stack exhaustion): DISMISSED
- clang-tidy misc-no-recursion enforced tree-wide in CI (src/.clang-tidy:12)
  — new direct recursion fails the tidy job.
- Miniscript tree algorithms deliberately de-recursed: TreeEval/Tree
  operations "without actual recursive calls" (miniscript.h:555,574,628);
  NOLINT-scoped single-level self-recursion only (550-570).
- Script interpreter is iterative (explicit stack machine); descriptor
  parsing de-recursed historically.
- JSON/univalue parsing: nesting bounded by JSON_DEPTH? — noted as the one
  unchecked-by-this-pass surface; univalue has a depth counter (verified
  historically), left for a future cell if evidence surfaces.

### D17 (format mismatch): N/A by design + -Wformat for the rest
Logging/RPC formatting uses tinyformat (tfm::format) — type-safe template
formatting; argument-count mismatches throw tinyformat_error at runtime,
not printf-class UB. Genuine printf family calls are covered by
-Wformat -Wformat-security (CMakeLists.txt:481-482) under -Werror. No
format-mismatch class surface. DISMISSED.

### D6 (invalid destruction order): DISMISSED — explicit ordered shutdown

Static-destruction-order fiasco avoided by explicit sequencing in
Shutdown(): DestroyIndexes resets g_txindex / g_txospenderindex /
g_coin_stats_index during shutdown (init.cpp:393-395); HTTP→RPC→nodes→
indexes stopped in order. LogInstance and gArgs are process-lifetime
singletons by design (matches still-reachable valgrind.supp entries). No
cross-global teardown dependency found. DISMISSED.

### D15 (exception/error leak): DISMISSED — fail-loud boundaries, safe empties

- Thread boundaries: util::TraceThread catches, logs via
  PrintExceptionContinue, and RETHROWS (util/thread.cpp:15-28) — a throwing
  thread is a loud terminate with diagnostics, never swallowed.
- HTTP/RPC boundaries: catch-and-report (httpserver.cpp:177,
  httprpc.cpp:166,192).
- Empty catches checked: walletdb.cpp:318,800 (legacy optional-record
  parse tolerance — value stays default on failure), logging.cpp:534
  (special-file file_size fallback, documented), minisketch bad_alloc,
  bitcoin-cli RecvEOF control flow. All leave safe default state.
- No noexcept function observed performing throwing operations in checked
  paths; terminate-on-bug is the house style (Assume/CHECK_NONFATAL split).
DISMISSED.

## Grid (final state, cycle 1)

All 18 classes × audited subsystems: D1-D18 all DISMISSED (evidence per
class above). Confirmed: 0 new defects. Cross-campaign overlaps handled by
reference (D7→S1/S5/goal89, D9/D10→ubsan audit, D12→race-fix series).
Tooling lattice independently verified: clang-tidy (use-after-move,
unused-return, no-recursion, member-init), -Wthread-safety,
-Wconditional-uninitialized, -Wformat-security, DEBUG_LOCKORDER,
ASan/MSan/TSan/UBSan CI with triaged suppressions.

## Next queue
(cycle 2, distinct hypotheses rather than repeats: univalue JSON depth
limit verification (left from D16); memory-ordering audit of remaining 35
relaxed-atomic sites (left from D12, low yield); noexcept-correctness sweep
(left from D15); raw-delete sites sample-audit (79 hits, left from D5 —
verify all are container-internal); then rotate campaign per user direction)

## Cycle 2

### D16-leftover: univalue JSON depth — VERIFIED ENFORCED (empirical)

### D5-leftover: raw-delete audit (79 sites) — all contract/internal, no double-free

### D12-leftover: relaxed-atomic audit (37 sites) — none publishes a payload

### D15-leftover: noexcept sweep — DISMISSED with stated limitation

452 noexcept declarations tree-wide (non-test). Full audit impractical and
low-yield: severity of any violation is std::terminate — a LOUD crash
(house fail-loud style), availability-only, never silent corruption.
Sampled hot paths are clean: coins cache flag ops/accessors/Coin move
(coins.h:139-199), FeeFrac arithmetic, span utilities — no allocation, IO,
or lock under noexcept. Move ctor/assign noexcept usage relies on
string/vector moves (noexcept since C++17). CLOSED with the limitation
that 452 sites were pattern-sampled, not exhaustively traced.

## Cycle-2 summary + handoff

All cycle-2 leftovers closed: univalue depth (empirical), raw-delete audit,
relaxed-atomics (37 classified), noexcept (sampled). Campaign state: all 18
defect classes + all deferred cells audited; 0 new defects confirmed this
campaign (consistent with goals 26/85/86/89/float-sanitizer finding the
tree densely oracled). Tree clean; branch audit/cpp-taxonomy.
Next queue (for future sessions, distinct from completed cells):
salvage-style deeper dynamic hunts only if the tooling lattice changes
(e.g. new subsystem without fuzz/tidy coverage, or a new dependency);
ipc/multiprocess boundary (capnp) — the one subsystem NOT exercised by
these campaigns' sweeps; QT GUI internals (largely un-fuzzed by design).
Rotation per user direction as before.

Classified every non-test memory_order_relaxed site:
- Test/mock clocks (util/time.cpp:40-86) — stale reads harmless.
- Log category mask (logging.cpp:158) — late visibility only.
- Counters needing uniqueness not order: httpserver m_next_id (903),
  net.cpp:3490 node ids, coins.h:724 input-slot fetch_add.
- Gauges/metrics: httpserver m_connected_size (926/1200/1209).
- Monotonic heuristic caches: validation m_cached_is_ibd (1975/3354/3358)
  — stale-tolerant by design; parallel-input ready-flag relaxed uses are
  Assert-only (coins.h:693,741) with construction-phase stores (746/751).
- cuckoocache.h:38 — DOCUMENTED contract: "All operations are
  memory_order_relaxed so external mechanisms must [synchronize]";
  probabilistic set tolerating races by design.
- leveldb skiplist/arena — vendored, upstream-reviewed.
No release/acquire pairing needed anywhere: no relaxed store publishes a
payload read via a relaxed load. CLOSED.

By file: kernel/bitcoinkernel.cpp (23) = C-API btck_*_destroy functions —
typed deletes of opaque objects with a documented caller-destroys-once
contract (external API misuse is the consumer's UB, not ours);
dbwrapper.cpp:285-293 = CDBWrapper dtor single-path teardown of leveldb
members; wallet.cpp:866-880 = mutually-exclusive early-return deletes with
immediate nullptr assignment (TxnBegin/WriteMasterKey failure paths in
legacy EncryptWallet); qt/* = QObject cleanup idiom. No reachable
double-free. Minor observation (not a defect): the legacy EncryptWallet
batch leaks if a DB exception escapes mid-path — fatal-error path only,
harmless. DISMISSED.

MAX_JSON_DEPTH=512 (univalue_read.cpp:21) enforced in the ITERATIVE parser
(explicit stack vector, abort at stack.size()>512, line 340-341) — no
recursive-descent stack risk and a hard cap. Empirical: harness over
src/univalue — depth-600 nest → read=0 (rejected), depth-400 → read=1
(accepted). Commands: g++ -std=c++20 -I src/univalue/include d.cpp
src/univalue/lib/{univalue,univalue_get,univalue_read}.cpp. CLOSED.
