# Journal: hit-frequency and suspicious-branch coverage audit (campaign 9)

Uber-goal rotation. Branch: audit/hit-frequency-coverage from
audit/resurrection @ 50302ae602. Tool: libFuzzer sancov via
`-print_coverage=1` (the deprecated -dump_coverage produces nothing on
current libFuzzer; print_coverage gives UNCOVERED_FUNC/UNCOVERED_PC with
edge counts). Host runs ~10 exec/s for coins_view.

## Cycle 1: coins.cpp / txdb.cpp via coins_view target (2000 runs)

### Coverage result: every substantive production function COVERED

No non-destructor function in coins.cpp is entirely uncovered. Uncovered
PCs classify into 4 benign buckets:

1. ASSERT-ONLY lines: uncache base_cache-present assertions
   (coins.cpp:484-488, G_ABORT_ON_FAILED_ASSUME-gated), uncache TrySub
   bookkeeping (473).
2. TERNARY ARM: PeekCoin on a spent cache entry (coins.cpp:36 true
   branch → nullopt) — mechanically asserted at 37; harness only peeks
   unspent/missing. Coverage nicety, not a defect.
3. Reset() internals (543-544): memory-resource/map reconstruction —
   the harness doesn't drive Reset in these runs.
4. Sibling-target entries + dtors (noise floor: each target's entry is
   uncovered when running another; resize-cursor target has its own
   290-edge entry, not run here).

### Verdict: DISMISSED — no suspicious uncovered logic branch

The fuzz harness hits all substantive logic; the rare branches are the
defensive ones, exactly where they belong. No missing-scenario branch,
no dead code found in the audited subsystem.

## Next queue
(same method on net_processing — larger, ~10 exec/s here so needs
longer/parallel runs or targeted entry points; then rotate per ledger)
## Cycle 2 (2026-07-28): net_processing via process_messages — sancov + dictionary + focus-function escalation

Base: cd20010714 (journal commit for #76 cycle-1 on
audit/reproducible-builds; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/hit-freq-c2 (c1 journal carried in the carry
commit). Start state: clean (untracked scratch only).

### Draw
Random draw over the 63-goal repaired pool (40 pending + 23 CYCLE-1;
#76 excluded as just-cycled): raw=13504447271606250885, seed masked to
63 bits (4281075234751475077), index 43 -> #9. Queued cell from c1:
"same method on net_processing".

### Setup
build_fuzz fuzz binary rebuilt to HEAD cd20010714 (make -j4 fuzz, 19
min, exit 0). Target: process_messages (drives ProcessMessage(s)
directly with injected CSerializedNetMsg; empty starting corpus,
/tmp/btc9_corpus).

### Run 1: 20000 runs, no dictionary (137s, ~146 exec/s)
cov 3025 -> 3742 edges. UNCOVERED analysis for net_processing.cpp:
- 99 fully-uncovered functions, classifying as harness scope, NOT
  defects: validation-signal callbacks needing a driven chain
  (BlockChecked 210e, NewPoWValidBlock 159e, BlockConnected,
  UpdatedBlockTip, ActiveTipChange, BlockDisconnected), scheduler/
  timer family (StartScheduledTasks, SendPings,
  CheckForStaleTipAndEvictPeers 163e, InitiateTxBroadcast* 107e),
  RPC/stats getters (GetNodeStateStats 190e), unit-test-only entries
  (UnitTestMisbehaving, SetBestBlock).
- Top uncovered LINE clusters inside covered functions: tip-relative
  arms (4679/4707: headers-vs-active-tip chainwork comparisons —
  harness chain stays at genesis, so "we know something better" arms
  are precondition-blocked), LogDebug-only lines (4491/4348/3763),
  fork Assume verification sites (3921: tx-inventory-empty-at-verack
  privacy check — fires when reached).
- Fork-added production code check: all pap.lorinc commits on
  net_processing.cpp are Assume/check additions except 1a3cbf1bd2
  (compact-block extra-tx optimization); fork check-sites are covered
  EXCEPT one class below.

### The one sharp edge: IsExpectedPeerMessageDeserializationFailure 0/23
The fork's parse-failure classifier (d6aae17a9f, net_processing.cpp:
208-225; two call-site Assumes at 4063 and 5290 in the ProcessMessage
exception catches) had ZERO hits in run 1. The commit's stated purpose
is to turn the catch boundary into an executable contract during
fuzzing — if the harness never throws there, the contract is vacuous
from this target. Escalation:
- Hypothesis H1: dictionary-free discovery of 12-byte wire message
  types is the blocker (random 12-byte type ~2^-80).
- Test: run 2 with /tmp/btc9.dict (28 padded NetMsgType strings).

### Run 2: 30000 runs + dictionary (158s, ~190 exec/s)
cov 3742 -> 5311 edges (+42%), corpus 114 -> 231; corpus now contains
typed messages (tx: 44 files, headers: 13, block: 49, version: 4).
RESULT: helper STILL 0/23; catch LogDebug (5291) still uncovered.
H1 refuted as the full explanation: typed messages were processed but
no exception reached the production catch in 50k guided runs.

### Run 3 + probe: focus-function steering and forced-message construction
- Run 3: 30000 runs with -focus_function="(anonymous
  namespace)::IsExpectedPeerMessageDeserializationFailure(std::exception
  const&)" (focus accepted after qualifying the name): cov 5342,
  helper STILL reported UNCOVERED_FUNC 0/23.
- Construction probe (TEMP, uncommitted, reverted): harness patched to
  force every injected message to type "tx" with payload {01,02,03};
  30 runs: no abort, helper STILL 0/23 — yet the TX parse line
  (net_processing.cpp:4528) WAS covered, i.e. forced messages reached
  the parser. (First probe attempt had a static-flag bug — fired once
  in 50 runs; second attempt forced every message.)

### Resolution: inlining artifact, not a coverage gap
Per-line UNCOVERED_PC check of the helper's source (208-225) across
ALL logs (batch 20k, batch 30k, both probes): EVERY sampled line
covered in EVERY run. The helper is called only from the two catch
sites (4063, 5290) and is INLINED at both; sancov attributes the
executed edges to the call sites, while the out-of-line symbol (nm:
0x414db5c, emitted but never called) reports 0/23 — the same
instrumentation/inlining family as #36 c2's clang -Wunneeded warnings.
CONCLUSION: the fork's expected-parse-failure contract (d6aae17a9f)
IS exercised by process_messages even without a dictionary; the
"vacuous contract" hypothesis is REFUTED. The catch-site LogDebug
(5291) keeps partial uncovered PCs because debug logging is compiled
in but unevaluated when the category is off — benign.
Tooling lesson recorded: UNCOVERED_FUNC with edges 0/N for a small
static function with few call sites is ambiguous — ALWAYS confirm with
per-line UNCOVERED_PC before classifying a branch as uncovered
(single-file -runs=1 print_coverage output is ALSO unreliable: the
helper printed NEITHER covered nor uncovered lines there; do not use
grep-fallback logic as evidence).

### Verdict
- DISMISSED: no suspicious uncovered logic branch in net_processing
  under process_messages. Fully-uncovered functions are harness-scope
  families (validation callbacks, scheduler/timers, stats getters,
  unit-test entries); tip-relative arms are genesis-locked-chain
  preconditions; the one alarming 0/23 was an inlining artifact.
- No new oracle added: the campaign's "add a test only when the branch
  matters" bar is not met — the existing harness already exercises the
  fork's contract.

### Exact commands
- make -C build_fuzz -j4 fuzz (rebuild to HEAD; and again after the
  reverted probe)
- FUZZ=process_messages build_fuzz/bin/fuzz -runs=20000
  -print_coverage=1 /tmp/btc9_corpus
- same with -runs=30000 -dict=/tmp/btc9.dict (28 padded NetMsgType
  strings from src/protocol.h)
- same with -runs=30000 -focus_function="(anonymous
  namespace)::IsExpectedPeerMessageDeserializationFailure(std::exception
  const&)" -print_coverage=1
- TEMP probe: forced net_msg.m_type="tx", data={01,02,03} in
  src/test/fuzz/process_messages.cpp (reverted; binary rebuilt clean)
- analysis: grep/sed per-line UNCOVERED_PC checks (208-225, 4528,
  5290-5291) across /tmp/btc9_cov{,2}.log and /tmp/btc9_forced{,2}.log

### Limitations / queue
- Which individual expected-failure arms (exception texts) fired was
  not enumerated — the classifier is reached; per-arm accounting is a
  queued nicety.
- The 99 harness-scope functions would need a chain-driving or
  scheduler-driving harness (process_messages' documented scope
  boundary); upstream faces the same boundary.
- Corpus/dict kept in /tmp/btc9_* (scratch, small); the -dict recipe
  (28 padded NetMsgType strings) is reusable for any P2P message
  target.
