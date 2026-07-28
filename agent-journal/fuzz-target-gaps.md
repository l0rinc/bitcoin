# Campaign #10 — fuzz-target-gaps

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/fuzz-target-gaps. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): wallet rescan harness — mock-chain wallet_rescan fuzz target delivered

### Draw
Random draw over the 58-goal eligible pool: raw=1450462071720032170,
index 6 -> #10.

### Gap inventory (measured)
195 fuzz targets in-tree. Wallet targets (7, src/wallet/test/fuzz):
coincontrol, coinselection, crypter, fees, scriptpubkeyman, spend,
wallet_bdb_parser. NO target reaches CWallet::ScanForWalletTransactions
— verified by grep over src/test/fuzz and src/wallet/test/fuzz, and by
git ls-tree origin/master (upstream master has the identical 7; the gap
is upstream-wide, not fork-local). Session context made this the
highest-value gap: the rescan FAILURE branches were hit twice this
session (#30 c4 exit-logging defect on the read-failure path; #41 c1
noted the mid-scan-reorg branch "racy to drive deterministically").

### Harness design (smallest production-like construction)
src/wallet/test/fuzz/rescan.cpp — FuzzRescanChain, an in-memory
interfaces::Chain implementing all 55 virtuals, with the block list,
per-block active/has_data flags, one mid-scan deactivation (flip
mechanism: blocks[flip_idx] goes inactive on first findBlock for its
hash — the deterministic stand-in for a reorg between the nextBlock
check and the block's own iteration), shutdown flag, and an optional
mempool tx, all driven by FuzzedDataProvider. Real CWallet on
CreateMockableWalletDatabase with a combo(key) descriptor; txs pay the
wallet's script (AddToWallet path is real). Deterministic time via
reserver.setNow + SetMockTime.
Crash-independent oracles:
- SUCCESS => !abort && !shutdown && every scanned block active+readable
  && no triggered flip inside the scanned range.
- FAILURE => a reason exists: unreadable scanned block, inactive start
  block, or triggered flip; USER_ABORT => abort || shutdown.
- last_scanned_height set => that block was active with data.
- every wallet tx in a fully scanned block is in mapWallet.

### Harness bugs found during bring-up (harness-side, not production)
1. Synthetic blocks with nBits==0 read as failed reads:
   CBlock::IsNull() checks CBlockHeader::IsNull() (nBits==0), which is
   exactly how ScanForWalletTransactions detects a failed disk read.
   Fixed by setting nBits=0x207fffff.
2. Determinism watchdog (check_globals.cpp) aborts on real-time access:
   CWallet construction uses GetTime; must SetMockTime before it.
3. The empty-input zero-hash block exposed an oracle false-alarm:
   last_failed_block can legitimately BE the null hash; reason-checks
   must not rely on IsNull() of a hash. (Oracles adjusted accordingly.)

### Verification
- Built in build_fuzz (BUILD_FOR_FUZZING=ON): `make -j4 fuzz` clean.
- Branch-coverage run (temp env-gated counters, reverted after):
  2000 runs -> success=1666 failure=38 abort=296 flip=55, zero oracle
  violations; all four status classes + the mid-scan deactivation
  mechanic exercised.
- Final validation (instrumentation removed): FUZZ=wallet_rescan
  ./bin/fuzz /tmp/r10_corpus -runs=5000 -max_len=4096 -> exit 0,
  "Done 5000 runs in 319 second(s)" (~64ms/iteration). No crash, no
  oracle failure, no sanitizer report.

### Verdict
- Harness DELIVERED (src/wallet/test/fuzz/rescan.cpp + CMakeLists
  registration). Oracles hold across 5k fuzzed scans covering
  SUCCESS, both FAILURE shapes (read failure, mid-scan deactivation),
  USER_ABORT (abort + shutdown), save_progress, and the mempool phase.
- No production defect found this cycle; the rescan FAILURE branches
  are now fuzz-reachable deterministically for the first time
  (upstream-wide gap closed).

### Limitations
- The flip mechanic models one deactivation at a deterministic point;
  multi-reorg sequences and tip extensions mid-scan are not modeled.
- guessVerificationProgress is constant (1.0); progress-interval
  branches are reached via setNow but progress VALUES are not fuzzed.
- Mempool phase injects at most one tx.
- Full 30k-run campaign not done (each iteration builds a full wallet;
  ~25ms/run on Cortex-A76). Corpus at /tmp/r10_corpus for continuation.

### Next queue for this campaign
- LoadWallet record-application seam (SQLite records -> wallet state):
  no target covers it (wallet_bdb_parser is container-level only).
- coinbase maturity / abandoned-tx rescan interplay once the rescan
  harness has corpus depth.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-28): load_wallet record-application harness delivered; first crash = harness-oracle bug (SetWalletFlag clobbers seeded FLAGS), not production

Base: 2d5dace4db (journal commit for #79 cycle-1 on audit/fuzz-corpus;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/fuzz-gaps-c2 (c1 journal carried; rescan harness
cherry-picked in: 537e819eb0 + 04254c1da7 from audit/fuzz-target-gaps,
closing part of the unmerged-side-branch gap noted in #66).
Start state: clean (untracked scratch only).

### Draw
Random draw over the 61-goal pool (42 pending incl. new 99-109 + 19
CYCLE-1; #79 excluded as just-cycled): raw=4438550641115630626,
index 0 -> #10. Queued cell from c1: "LoadWallet record-application
seam (SQLite records -> wallet state): no target covers it".

### Harness (src/wallet/test/fuzz/load_wallet.cpp)
Pre-seeds an in-memory SQLite database (CreateMockableWalletDatabase,
real engine/cursors) with up to 24 fuzzed records across 6 classes:
FLAGS (uint64), VERSION/MINVERSION, NAME (address+label),
WALLETDESCRIPTOR (valid combo descriptor or mutated bytes), TX
(mutated bytes), and unknown-type records (prefix-cursor robustness).
Applies them via the production WalletBatch::LoadWallet on a real
CWallet over the test chain. Oracles:
- classification contract: every input yields a DBErrors status;
  an uncaught exception/abort is a defect (the trust boundary:
  torn/corrupt persisted records must be classified, not fatal).
- FLAGS round-trip: on LOAD_OK, every bit of the FLAGS record the
  loader saw is set in m_wallet_flags (expected value READ BACK from
  the DB after wallet setup, not the pre-setup seed — see crash note).
- NAME round-trip: on LOAD_OK, seeded NAME records with valid
  destinations are present with the seeded label.
Registered in src/wallet/test/fuzz/CMakeLists.txt (alphabetical).

### First crash (within 3000 runs) — diagnosed to root cause
Seed /tmp/lw_crash_flags_seed (preserved). Failing assert: FLAGS
round-trip. Debug trace: seeded FLAGS=0x308, result=LOAD_OK,
final m_wallet_flags=0x400000004 (DESCRIPTORS|LAST_HARDENED_XPUB_
CACHED). Mechanism: the harness itself called
wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS) during setup, which
PERSISTS to the DB immediately (WriteWalletFlag), clobbering the
seeded FLAGS record before the loader ran. The loader then correctly
read and assigned DESCRIPTORS (hence LOAD_OK despite the seeded
record missing it — the LEGACY_WALLET branch never triggers because
the clobbered record contains DESCRIPTORS). VERDICT: harness-oracle
bug; production load path behaved exactly per contract at every step.
Fix: harness no longer pre-sets flags, and the oracle reads back the
DB record the loader will see (expected_flags) instead of trusting
the pre-setup seed. The incident is recorded as evidence the oracle
has teeth: it detected a real pre-load state divergence within 3k
runs.

### Verification
- Build: make -C build_fuzz -j4 fuzz clean (3 compile fixes during
  bring-up: g_setup TU-local declaration, ConsumeRandomLengthByteVector
  free-function form, DecodeDestination returns CTxDestination
  variant not optional, CAddressBookData::label optional field).
- Crash seed re-run post-fix: PASS (no abort).
- FUZZ=load_wallet build_fuzz/bin/fuzz -runs=5000 -max_len=4096
  /tmp/lw_corpus -> exit 0, "Done 5000 runs in 98 second(s)" (~51/s).
  No crash, no sanitizer report.

### Verdict
- Harness DELIVERED (load_wallet.cpp + CMake registration). The
  record-application seam (FLAGS/VERSION/NAME/descriptor/TX/unknown
  records) is now fuzz-reachable with classification + round-trip
  oracles.
- No production defect found: the loader's exception classification
  and flag/label application held across 5k+3k fuzzed loads.

### Exact commands
- make -C build_fuzz -j4 fuzz
- FUZZ=load_wallet build_fuzz/bin/fuzz -runs=1 <seed> /
  -runs=5000 -max_len=4096 /tmp/lw_corpus
- diagnosis: temporary fprintf traces (reverted), GetWalletFlags,
  DatabaseBatch::Read(DBKeys::FLAGS) readback; wallet.cpp:1806-1819
  (SetWalletFlag persistence + LoadWalletFlags assign-verbatim)

### Limitations / queue
- Record classes 6/…: MASTERKEY/CKEY/WCRYPTO (crypted-wallet paths),
  WATCHER, ACTIVEEXTERNALSPK/ACTIVEINTERNALSPK, WALLETDESCRIPTORCKEY/
  KEY, BESTBLOCK_NOMERKLE (CWallet::LoadWallet level, above
  WalletBatch) not yet seeded — queued.
- Oracles are exact only for FLAGS/NAME; descriptor/TX records
  currently covered by the classification contract only (a corrupted
  CWalletTx apply-vs-reject semantic oracle queued).
- Crash seed kept at /tmp/lw_crash_flags_seed (provenance); corpus
  /tmp/lw_corpus for continuation.
- Scope note (2026-07-28 objective refresh): future draws weight
  core-scoped campaigns (consensus/coins/P2P/serialization/crypto/
  validation/indexes/storage); wallet campaigns only when they prove
  reachability of an in-scope core defect.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
