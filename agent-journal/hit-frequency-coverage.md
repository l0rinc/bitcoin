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

## Cycle 3 (2026-07-29): qa-assets psbt corpus import — covers everything EXCEPT the key-requiring sign-complete arm; correlated seeds remain necessary

### Draw
Re-rank singleton (last queue cell; findings-index resume point
"qa-assets selective import per target"): #9 (third cycle).
Branch: audit/hit-frequency-c3 from 3e3f49d25b (#24 c3 journal
tip). Import: git clone --depth=1 --filter=blob:none --sparse +
sparse-checkout fuzz_corpora/psbt (7773 seeds, 55 MB, removed
after; recipe recorded).

### Hypothesis
qa-assets' psbt seed corpus (upstream-fuzzer-built) leaves the
SignPSBTInput ProduceSignature-complete arm for KEY-REQUIRING
scripts uncovered — the provider-key-correlation gap the #50
family was built to close.

### Experiment (temporary instrumentation, reverted after)
Per-input counters over the full corpus
(FUZZ=psbt .../fuzz -runs=7773, three instrumented passes):
- 923/8776 seed runs reach the signing section; 1956 input-attempts.
- ok=77, verified=71 — DECOMPOSED:
  * pre-signed in the seed (PSBTInputSigned): 18 OK / 12 verified
    (fuzz-built final scripts; some P2SH with redeem data).
  * NOT pre-signed but early-OK: 59/59 — ALL with push-only /
    anyone-can-spend / P2WSH-of-empty-script spks (prefix histogram
    has ZERO 76a914/0014/a914/0020/5120 key-requiring templates).
- Mechanism of the 59: PSBTInputSignedAndVerified (psbt.cpp:558-590)
  does NOT require final data — it runs VerifyScript on whatever
  final fields exist (possibly empty); a truthy anyone-can-spend
  script verifies with an EMPTY scriptSig, so the early-OK arm
  (psbt.cpp:662) fires with no signing at all.
- KEY-REQUIRING ProduceSignature completions: 0 (the fuzz-consumed
  provider keys never match a seed's spk hash — 2^-160 lottery).

### Control
The #50 correlated family (v0/v2/2-input/P2WPKH seeds) drives the
complete arm with the provider key on every run (in-target trace +
RPC verifier, #50 c4-c6) — the arm the qa-assets corpus cannot
reach.

### Verdict
CONFIRMED (hypothesis): qa-assets import is complementary, NOT a
substitute — it covers decode/merge/analyze/finalize and the
anyone-can-spend early-OK arm, but the key-requiring
sign-complete arm needs correlated seeds. Characterization
recorded (not a defect): PSBTInputSignedAndVerified returns true
for final-data-less anyone-can-spend inputs — name-vs-behavior
subtlety, upstream-identical, semantically defensible (the empty
scriptSig IS the valid final state there).

### Exact commands
- git clone --depth=1 --filter=blob:none --sparse
  https://github.com/bitcoin-core/qa-assets /tmp/qa-assets;
  git -C /tmp/qa-assets sparse-checkout set fuzz_corpora/psbt
- three instrumented builds (SIGCOUNT/SIGVER/SIGFRESH variants),
  FUZZ=psbt build_fuzz/bin/fuzz -runs=7773 <corpus>
- restore: git checkout -- src/test/fuzz/psbt.cpp; rebuild;
  -runs=100 /tmp/psbt_c5_corpus clean; git diff empty

### Limitations / queue
- Only the psbt target imported; other targets per the same recipe
  when a coverage question lands there.
- The 923/8776 reach rate includes the hybrid-consumption mode
  split; the corpus was built for upstream's single-mode target.

## Rotation note
Three cycles; qa-assets psbt cell closed with a mechanism-level
answer. Not exhausted (other targets).

## Cycle 4 (2026-07-29): qa-assets coins-target imports — compatible + clean; load_wallet has no upstream corpus

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=5635536915814007501, index 2 (of 3) -> #9 (fourth cycle; c3
queue cell "other targets per the same recipe"). Branch:
audit/hit-frequency-c4 from cea4a8c15f (#60 c7 journal tip).

### Hypothesis
The coins-family qa-assets corpora drive the fork's coins targets
cleanly (the overlay target is upstream-lineage); fork-local
targets (load_wallet, resize-cursor) have no importable corpus.

### Results
- fuzz_corpora/coins_view_overlay (3017 seeds, 18 MB): runs clean
  on the fork's coins_view_overlay target (the prevout-pool
  overlay target — same upstream lineage, #49 c5's subject).
  cov=4308 ft=14885, 3028 runs, no crash.
- fuzz_corpora/coins_view_db (4551 seeds, 66 MB): clean on
  coins_view_db. cov=10706 ft=52270, 4554 runs, no crash.
- fuzz_corpora has NO load_wallet corpus (wallet targets there are
  bdb_parser / create_transaction / fees only) — the fork's
  load_wallet record-application harness (#10 c2, widened at
  ed5cc9281a) has no importable seeds; its widened record classes
  (crypted keys, ACTIVE*SPK, BESTBLOCK) remain our own seeds' job.
- coins_view_db_resize_cursor (fork-local target from the
  UTXO-scan/resize race fix e049f064e1): no upstream corpus, as
  expected for a fork-only target.

### Verdict
CONFIRMED (import compatibility): the upstream-lineage coins
corpora run clean and drive the fork's targets; fork-only targets
stay on locally-grown seeds. The c3 pattern holds directionally:
qa-assets substitutes only where the target lineage is upstream's.

### Exact commands
- sparse-fetch recipe from c3 (clone --filter=blob:none --sparse,
  sparse-checkout set fuzz_corpora/{coins_view_overlay,coins_view_db})
- FUZZ=coins_view_overlay ... -runs=3017; FUZZ=coins_view_db ...
  -runs=4551 (outputs above)

### Limitations / queue
- Coverage deltas vs scratch corpora not re-measured (c1's
  baselines stand); the check here was compatibility + crash.
- process_messages corpus import would duplicate c2's dictionary
  work; skipped deliberately.

## Rotation note
Four cycles; coins-target import cell closed. Campaign #9's
remaining cells need fresh coverage signals.

## Cycle 5 (2026-07-29): qa-assets process_messages import — clean at upstream-grade coverage; no fork-added arms to miss

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=8574729547574712489, index 4 (of 5) -> #9 (fifth cycle; c3
recipe applied to process_messages). Branch:
audit/hit-frequency-c5 from 785fe50ced (#24 c4 journal tip).

### Result
- fuzz_corpora/process_messages (3783 seeds, 56 MB) on the fork's
  target: 4963 runs in 59 s, cov=18197 ft=58683, NO crash.
- Fork-arm check: the fork's process_messages target has NO
  PRIVATE_BROADCAST references and does not pick connection types
  — there are no fork-added arms for the corpus to miss (the
  c3/c4 pattern "fork section invisible to upstream seeds" does
  not apply here).

### Verdict
CONFIRMED (import compatibility + coverage): the corpus drives
the fork's process_messages at upstream-grade coverage with no
compatibility break. PRIVATE_BROADCAST message-path coverage
would need a dedicated harness (connection-type-driving); noted,
not a defect.

### Exact commands
- sparse-fetch recipe (c3); FUZZ=process_messages build_fuzz/bin/
  fuzz -runs=3783 <corpus> (numbers above)
- grep -in private src/test/fuzz/process_messages.cpp (empty)

### Limitations / queue
- Per-arm accounting of expected-failure branches (c2 queue)
  remains a nicety.
- PRIVATE_BROADCAST harness cell queued to whoever owns P2P
  harness work next.

## Rotation note
Five cycles; process_messages import closed. Campaign #9's cells
now need fresh coverage signals.

## Cycle 6 (2026-07-29): qa-assets psbt sibling-target imports — all three clean on the fork's targets

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=5487700768165387789, index 4 (of 5) -> #9 (sixth cycle; the
sibling-target import cell). Branch: audit/hit-frequency-c6 from
7304d57169 (#80 c9 journal tip).

### Results (sparse-fetch recipe from c3)
- psbt_base64_decode (2090 seeds): 2103 runs, cov=5556 ft=28360,
  clean.
- psbt_input_deserialize (1467 seeds): 1468 runs, cov=4387
  ft=23175, clean.
- psbt_output_deserialize (736 seeds): 737 runs, cov=2846
  ft=16086, clean.
All three targets exist in the fork's build with upstream-lineage
semantics; no fork-added arms to miss (the fork's PSBT work is in
the main `psbt` target, covered by the #50 family).

### Verdict
CONFIRMED (import compatibility): the sibling corpora run clean
and drive their targets; nothing fork-specific uncovered or
broken. The psbt target family (main + 3 siblings) is fully
covered between qa-assets (decode/deserialize paths) and the #50
family (signing arms).

### Exact commands
- sparse-fetch fuzz_corpora/{psbt_base64_decode,
  psbt_input_deserialize, psbt_output_deserialize}
- FUZZ=<t> build_fuzz/bin/fuzz -runs=<n> <corpus> (numbers above)

### Limitations / queue
- Union-merge coverage (family + upstream psbt) not re-measured
  (marginal; both directions already recorded separately).
- Campaign #9's remaining cells need fresh coverage signals
  (new targets or new fork arms).

## Rotation note
Six cycles; sibling imports closed. #9 quiet until new signals.
