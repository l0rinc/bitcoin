# Campaign #93 — system-fault-injection

Base: c6c83e3e2a (journal commit for #68 cycle-2 on audit/arch-abi-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/system-fault. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-29): coins-DB mid-flush crash injection — recovery rolls forward identically, no corruption

### Draw
Random draw over the 29-goal pool (16 pending + 13 CYCLE-1; #68
excluded as just-cycled): raw=17371044710458590394, seed masked to 63
bits (8147672673603814586), index 14 -> #93.

### Hook (existing infrastructure, test-only)
txdb.cpp:171-174: -dbcrashratio=N triggers _Exit(0) inside
CCoinsViewDB::BatchWrite's PARTIAL batch path
(batch.ApproximateSize() > batch_write_bytes, default 32 MiB,
tunable via -dbbatchsize per node/coins_view_args.cpp:13).

### Schedule (scratch datadir /tmp/btc93_d)
1. Wallet-funded regtest node, -dbcrashratio=1 -dbbatchsize=1000
   (forces a crash on the first partial batch of >1 KB dirty coins).
2. 150 sends + 80 generate -> 3x "Simulating a crash. Goodbye." with
   "Rolled forward to 1e69ae8d..." recoveries between restarts.
3. Final restart WITHOUT the hook: clean load, rolled forward to the
   SAME point again (04:25:12 and 04:25:55 lines identical), resumed
   at 415 blocks, no inconsistency/corruption logs.

### Verdict
- CONFIRMED (recovery correctness): mid-flush crashes leave the
  chainstate recoverable via the HEAD_BLOCKS two-phase protocol
  (transition marker present -> roll forward to a consistent point,
  identical across repeats). No silent corruption, no wedged state.
- Bring-up notes (methodology, recorded): the crash fires only in the
  PARTIAL batch path; small flushes never trigger (needed
  -dbbatchsize=1000); wallet must be loadwallet'ed after each
  restart (no auto-load).

### Exact commands
- bitcoind -regtest -datadir=/tmp/btc93_d -dbcrashratio=1
  -dbbatchsize=1000 (workload loop), restarts, debug.log grep
  'Simulating a crash|Rolled forward|inconsistent'

### Limitations / queue
- Crash timing granularity: only the partial-batch point is
  injectable via the hook (per-write _Exit). Crash between batches of
  the same flush IS the tested point; crash between HEAD_BLOCKS and
  first coin batch is adjacent but not separately hooked — the same
  protocol covers it (rolled forward observed).
- Wallet side of the same crash (wallet DB mid-write) is the
  b8fcf9ed17-family's territory (already fixed in-tree).
- Allocator/clock/syscall hooks (the other campaign families) not
  exercised this cycle — queued (hook-per-family, each bounded).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-08-01): fs/syscall fault family (permission injection) — both arms fail LOUD and attributed, zero corruption on restore; DISMISSED

### Draw
RE-RANK draw 164 over the 9-cell pool: raw=10692565713273262,
masked 1469137528858497454 -> idx 1 -> #93 fault-hook families
(c1 queue). Branch: audit/system-fault-c2 from 83d10f09c9.

### Hook inventory first
In-tree crash hooks: ONLY -dbcrashratio (txdb partial-batch, c1's).
Clock family = MockTime (already covered #73 c4/c5 with its limits
recorded). Allocator family: no in-tree hook (would need
RLIMIT/container work; #74 c2/c5 covered LockedPool specifically).
So the syscall/fs family goes via permission injection on the block
IO path — distinct from #102's capture-path EISDIR and #41's
banlist write-fail.

### Arms (scratch 120-block datadir, setpriv nobody)
- ARM B (blk00000.dat chmod 000 at startup): LOUD attributed abort
  — "Unable to open file .../blk00000.dat" -> "Error loading block
  database. Please restart with -reindex..." -> clean shutdown.
  Restore perms -> full recovery at 120.
- ARM A (blocks dir chmod 0555 mid-run): generate -> "Unable to
  open file ... while writing block" -> "fatal internal error:
  Failed to write block" -> RPC "block not accepted" — loud,
  attributed, tip untouched at 120. Restore + chown -> node
  continues, 2 blocks generated (122), blk file intact.
- SETUP TRAPS (recorded): (1) root ignores chmod permissions
  (CAP_DAC_OVERRIDE) — arm B as root BOOTED NORMALLY; permission
  faults need an unprivileged user (setpriv nobody, same lesson as
  CAP_IPC_LOCK at #74 c5). (2) root-created files are not
  nobody-writable at 644 — my own chown omission reproduced a real
  flush fatal on restore until chown -R.

### Verdict
DISMISSED: fs-level failures on the block IO path fail loudly with
the correct path in the error, never silently, and the datadir
survives. Both pre-write (startup) and mid-write (generate) arms
verified with full recovery.

### Limitations / queue
- allocator family has no in-tree hook (noted; #74's LockedPool
  cells cover the secure allocator specifically).
- #93 queue: empty (c1 crash-mid-flush, c2 fs faults). Campaign
  COMPLETE on current surface.

## Rotation note
Two cycles; fault-injection surface closed.
