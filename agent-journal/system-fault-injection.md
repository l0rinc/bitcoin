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
