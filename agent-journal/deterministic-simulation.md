# Campaign #71 — deterministic-simulation

Base: audit/fuzz-target-gaps @ faee3fe5b2 (journal commit for #10 cycle-1).
Branch: audit/fuzz-target-gaps (existing harness owner; base recorded).
Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): mid-scan tip-extension fault class in the rescan simulation (b427b59b54)

### Draw
Random draw over the 47-goal eligible pool: raw=9033587112740688746,
index 35 -> #71.

### Rationale / continuity
The #10 c1 wallet_rescan harness IS this fork's deterministic
simulation of the rescan subsystem (seeded block outcomes, flip/deact
events, shutdown, mempool injection, deterministic time). Campaign #71
adds a new workload/fault class per its own protocol ("each cycle adds
a new workload, fault class, or invariant") on the harness-owning
branch rather than building a parallel fake elsewhere.

### New fault class (mechanism)
When scheduled (~25% of inputs), the first findBlock() for a
fuzz-chosen height appends 1-6 pre-built active+readable blocks and
calls wallet->SetLastBlockProcessed(new tip) — the deterministic
stand-in for a blockConnected notification processed mid-scan, the
interleaving wallet.cpp:2030-2033 explicitly designs for ("additional
blocks that were added during the rescan will be re-processed if the
notification was processed and the last block height was updated").
Safe to call inside findBlock: the scan's chain lookups happen outside
cs_wallet (the LOCK(cs_wallet) region starts only at SyncTransaction).

### New invariants
- SUCCESS + triggered extension + no max_height => scanned_upto ==
  grown tip (n_blocks + n_ext - 1) exactly — the scan must not stop at
  the stale tip.
- Extension-block wallet transactions must be in mapWallet once
  scanned past the original tip.
- All prior oracles compose unchanged (extension blocks are
  active+has_data by construction).

### Evidence
- Coverage (temp env-gated counters, reverted before commit):
  2000 runs -> success=1584 failure=52 abort=364 flip=82 extend=75,
  zero oracle violations. extend < scheduling rate by design: the
  extension fires only if the scan REACHES the scheduled height
  (early flip/gap/abort prevents it) — exactly the schedule-dependence
  a simulation should expose.
- Final validation (instrumentation removed):
  FUZZ=wallet_rescan ./bin/fuzz /tmp/r10_corpus -runs=5000 -max_len=4096
  -> exit 0, "Done 5000 runs in 319 second(s)".

### Verdict
- Fault class delivered and oracle-clean; no production defect found.
  The scan's tip-extension handling matches its documented contract
  under aggressive reproducible schedules.

### Limitations
- One extension event per run (no repeated tip growth); reorg of
  extension blocks not modeled (flip only targets the original range).
- The progress_end recompute branch (prev_tip_hash != tip_hash) is
  reached structurally but progress VALUES remain unfuzzed.
- No parallel-fake risk here: all logic runs production CWallet code;
  only the chain interface is simulated.

### Exact commands
- `cd build_fuzz && make -j4 fuzz`
- `R71_COUNTS=1 FUZZ=wallet_rescan ./bin/fuzz /tmp/r10_corpus -runs=2000`
- `FUZZ=wallet_rescan ./bin/fuzz /tmp/r10_corpus -runs=5000 -max_len=4096`

### Next queue for this campaign
- Crash-resume durability invariant: save_progress bestblock records ->
  resume scan from the recorded locator, assert no missed/duplicated
  wallet txs (needs a DB that survives "process" boundaries within the
  iteration — MockableSQLite record inspection per walletload_tests).
- Repeated tip growth + progress-value fuzzing.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): crash-resume durability invariant delivered (6c6e7d9f87) — 3000 runs clean

Base: df2a799285 (journal commit for #90 cycle-2 on
audit/knowledge-recipes-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/deterministic-sim-c2. Start state: clean
(untracked scratch only).

### Draw
Random draw over the 47-goal pool (30 pending + 17 CYCLE-1; #90
excluded as just-cycled): raw=12477597616785016971, seed masked to 63
bits (3254225579930241163), index 42 -> #71. Queued cell from c1:
"Crash-resume durability invariant: save_progress bestblock records ->
resume scan from the recorded locator, assert no missed/duplicated
wallet txs".

### Fault class (src/wallet/test/fuzz/rescan.cpp, +34 lines)
After the primary scan's oracles, when save_progress was on and the
scan made progress (last_scanned_height set):
1. Read the recorded position (GetLastBlockHash under cs_wallet).
2. Locate it in the mock chain; v1 resumes only when it is an
   ORIGINAL block with successors (extension-block resume queued).
3. Resume ScanForWalletTransactions(recorded, recorded_idx, nullopt,
   reserver, save_progress=false) — the "later process".
4. Assert: mapWallet size never shrinks (no lost txs), and on resume
   SUCCESS every wallet tx in blocks after the recorded position is
   present (no missed txs).
Crash-independence: the oracle references only the recorded hash and
chain flags, never the primary scan's volatile state, so it holds
across all primary statuses.

### Verification
- make -C build_fuzz -j4 fuzz clean (one signature fix during
  bring-up: start_height argument).
- FUZZ=wallet_rescan build_fuzz/bin/fuzz -runs=3000 -max_len=4096
  /tmp/r71_corpus -> exit 0, "Done 3000 runs in 287 second(s)".
  No crash, no oracle failure, no sanitizer report.

### Verdict
- Fault class DELIVERED and oracle-clean: crash-resume from the
  persisted position preserves and completes the wallet's tx set
  across 3000 fuzzed scan+resume schedules.
- No production defect found; the save_progress -> resume contract
  holds under aggressive reproducible schedules (deactivations,
  read failures, aborts, tip extensions upstream of the record).

### Exact commands
- make -C build_fuzz -j4 fuzz
- FUZZ=wallet_rescan build_fuzz/bin/fuzz -runs=3000 -max_len=4096
  /tmp/r71_corpus

### Limitations / queue
- Extension-block resume (recorded position inside the extension)
  not covered in v1 — queued.
- Resume with a REORGED recorded position (record no longer active)
  is the next fault class — the mock needs a second-generation chain
  view — queued.
- Progress-value fuzzing (guessVerificationProgress) still open from
  c1's queue.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-30): reorged-record resume oracle delivered (+ mock duplicate-hash fidelity fix) — 3000 runs clean

### Draw
Re-harvested-queue draw (seed_raw=5478373626480380153, masked
same, n=3, idx=2) -> resume-reorged-position -> #71 (third cycle;
c2 queue cell "REORGED recorded position ... the mock needs a
second-generation chain view"). Branch: audit/deterministic-sim-c3
from 83d10f09c9 (#73 c3 journal tip).

### Mechanism analysis first
The mock's flip can only deactivate mid-scan, so at the c2
oracle's point the recorded position is always ACTIVE — the
reorged-start classes need deterministic forcing (exactly what
the queue note predicted). wallet.cpp semantics for the classes:
deactivated start -> FAILURE + break with last_failed_block =
record (wallet.cpp:1977-1982, 'prevent marking transactions as
coming from the wrong block'); unknown start (second-generation)
-> FAILURE at :1996, then !next_block break; a pending
abort/shutdown -> USER_ABORT with nothing scanned (legitimate).

### Oracle delivered (src/wallet/test/fuzz/rescan.cpp, +~30 lines)
After the c2 resume block, when the recorded position is in the
mock chain, force BOTH sub-classes:
(a) deactivate the recorded block in place;
(b) replace its hash with a guaranteed-distinct fresh one.
Assert: resume status is never SUCCESS; FAILURE implies
last_failed_block == recorded; mapWallet never shrinks.

### The artifact and the mock-fidelity root cause
First campaign crashed on seed BwoANwKKAAA= (Base64): the mode-1
resume returned SUCCESS + last_scanned_h=7 over a replaced
genesis. Instrumented root cause (M-DEBUG/C3-DEBUG, all restored
after): the harness's null-hash correction (hash=0 -> uint256::ONE)
made MULTIPLE mock blocks share hash=ONE; the resume's IndexOf
found a same-hash SUCCESSOR after blocks[0] was replaced — a
degenerate unrepresentable chain, not a production defect. Fixed
at the source: block hashes are now made unique at construction
(low-byte bump until distinct); the mode-1 replacement is
guaranteed distinct likewise.

### Verification
- make -C build_fuzz -j4 fuzz (clean).
- Artifact repro post-fix: runs clean (no assert).
- FUZZ=wallet_rescan build_fuzz/bin/fuzz -runs=3000 -max_len=4096
  /tmp/r71_corpus (artifact seeded as
  /tmp/r71_corpus/seed_reorg_resume_dup): 'Done 3000 runs in
  191 second(s)', exit 0, no crash/oracle/sanitizer report.

### Verdict
- Fault class DELIVERED and oracle-clean: resuming from a reorged
  recorded position fails cleanly (or aborts with nothing
  scanned), never scans wrong-chain data, never loses wallet txs.
- Mock-fidelity flaw CONFIRMED+FIXED (duplicate block hashes from
  the null->ONE correction) — would have weakened every oracle in
  the harness that locates blocks by hash.
- No production defect found: the wallet.cpp FAILURE paths are
  exactly as designed.

### Exact commands
- as above; provenance: seed /tmp/r71_corpus/seed_reorg_resume_dup,
  original artifact Base64 BwoANwKKAAA=.

### Limitations / queue
- Extension-block resume (recorded position inside the extension)
  still open (c2 queue).
- The reorg classes are forced post-scan deterministically; a
  mock that reorgs DURING a resume (second flip) is a deeper
  schedule — nicety, not queued.

## Rotation note
Three cycles; tip-extension, crash-resume durability, and
reorged-record resume all delivered with oracles.

## Cycle 4 (2026-07-31): extension-block resume oracle delivered (+ extension-hash uniqueness fix, + c1 oracle correction) — 3000 runs clean

### Draw
RE-RANK draw 136 over the 6-cell queue: raw=18304798480260019009,
masked 9081426443405243201 -> idx 5 -> #71 extension-block resume
(c2/c3 queue). Branch: audit/deterministic-sim-c4 from 7e88645b92.

### Delivered: forced extension-resume oracle (rescan.cpp)
Natural-schedule analysis (recorded-index histogram probe, n=200):
the BESTBLOCK record after a save_progress scan is essentially the
stop/tip — abort is requested pre-scan, extension blocks are always
active/readable (no mid-extension failure), and max_height stops in
the original range — so a recorded position INSIDE the extension
never occurs naturally. Forced deterministically (same pattern as
c3's reorg classes): when the extension triggered with n_ext >= 2,
resume from the FIRST extension block and assert SUCCESS plus every
later extension wallet tx present, wallet size never shrinks.
Fire-proofed with a temporary counter (fires at the expected few-%
rate), counter removed for the final campaign.

### Bug 2 found+fixed IN THE HARNESS: extension-block hash duplicates
Artifact (Base64 FuwnJycncRknJycnJycnJ///lZWVAAAAAJUCAAR1bg==...
actually the mode-1 seed from the campaign) crashed c3's reorg
oracle: mode=1 (hash-replaced genesis) resume returned SUCCESS.
Root cause (findBlock trace): c3's uniqueness fix uniquified only
ORIGINAL blocks; an EXTENSION block sharing the recorded hash let
the "unknown start" resume find a same-hash successor at height 15 —
an unrepresentable chain. Fix: extension hashes are bumped unique
against chain.blocks and each other at construction. (A real chain
can never repeat a hash; mock-fidelity class identical to c3's.)

### Bug 3 found+fixed IN THE HARNESS: c1 extension oracle overclaimed
Second artifact crashed c1's `scanned_upto == n_blocks+n_ext-1`
assert. Trace: a PRE-SCAN findBlock(tip) caller fired the flip
before the scan started (deactivating blocks[2]); the scan then
legitimately stopped at flip_idx-1 via wallet.cpp's "previous block
no longer on the chain" break (:2014) with SUCCESS, while the
extension (extend_at=0) had already grown the chain. Corrected
oracle: under SUCCESS + flip_triggered the scan must stop exactly at
flip_idx-1; otherwise at the grown tip. (Under SUCCESS the flip can
only fire pre-reach — firing at the flip iteration yields FAILURE.)

### Verification
- make -C build_fuzz -j4 fuzz clean (one pre-existing unused-Find
  warning, not from this change).
- All three preserved artifacts pass: /tmp/c4_crash_seed (dup-hash),
  /tmp/c4_crash2 (oracle-overclaim), /tmp/r71_corpus/
  seed_reorg_resume_dup (c3).
- FUZZ=wallet_rescan build_fuzz/bin/fuzz -runs=3000 /tmp/r71_corpus:
  'Done 3000 runs in 192 second(s)', rc=0, zero assert/sanitizer
  reports (log /tmp/c4_final2.log).

### Verdict
- Extension-resume fault class DELIVERED, oracle-clean: resuming
  from inside a tip extension completes the grown chain with no tx
  loss. No production defect — wallet.cpp's extension/reorg behavior
  matched its documented contract in every probed class.
- Two harness-fidelity defects CONFIRMED+FIXED with crashing
  artifacts preserved (would have weakened/misfired every
  hash-locating and extension oracle in the harness).

### Limitations / queue
- Pre-scan findBlock caller identity not pinned down (reserve/start
  path; irrelevant to the corrected oracle).
- #71 queue: progress-value fuzzing (guessVerificationProgress) from
  c1 remains the only open cell.

## Rotation note
Four cycles; tip-extension, crash-resume, reorged-record, and
extension-resume all delivered with oracles. Near exhaustion.
