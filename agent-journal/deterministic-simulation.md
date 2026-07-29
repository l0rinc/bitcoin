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
