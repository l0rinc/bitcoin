# Journal: public API, CLI, RPC, config, and help contract audit (campaign 4)

Uber-goal rotation. Branch: audit/public-interface-contracts from
audit/resurrection @ a33d66dc1a. Method: end-to-end per-interface checks —
registration, parser, validation, application, lifecycle paths, help text.

## Cycle 1: config args surface

### Parsed-but-ignored sweep — CLEAN
140 args registered in init.cpp; naive no-reader sweep found 19 candidates,
all false positives (read via GetArgs plural/other spellings — addnode,
uacomment, shutdownnotify all wired). No parsed-but-ignored option.

### FINDING: -maxmempool 32-bit error path asserts instead of erroring — FIXED (36156ad934)

InitAndLoadChainstate asserted ApplyArgsManOptions(mempool_opts) claiming
AppInitParameterInteraction pre-validated everything. The 32-bit
>500MB rejection lives INSIDE ApplyArgsManOptions (mempool_args.cpp:49-53),
so on 32-bit with an oversized -maxmempool the Assert aborts with an
internal-bug report instead of the designed graceful error.
Fix: return FAILURE_FATAL with the error (matches the mempool_error path
directly below). 64-bit no-op.
Evidence: code trace both paths; build + init_tests green; Flatten
rejection verified behaviorally (regtest -maxmempool=1 → "must be at
least 5 MB", exit 1). Limitation: 32-bit assert path verified by trace
only (no 32-bit build here).

### -maxmempool contract otherwise consistent

### RETRACTION (campaign 3): the "32-bit assert" finding was a FALSE POSITIVE

The 36156ad934 fix was based on a flawed premise: I grepped init.cpp for
-maxmempool-specific checks, found none, and concluded the 32-bit
>500MB rejection wasn't pre-validated. But AppInitParameterInteraction
calls ApplyArgsManOptions(mempool_opts) at init.cpp:1168-1170 and
SURFACES the error via InitError — the same function containing the
32-bit check (mempool_args.cpp:49-53). The oversized value errors
gracefully there, before LoadChainstate's assert can ever fire. The
assert's comment was accurate all along.
Corrective action: reverted on-branch (5a16d316af). Lesson recorded:
"no -foo-specific check in init.cpp" does not imply "no validation" —
the generic ApplyArgsManOptions sweep at 1145-1171 covers the options.
(My GetNetClass fix and TopUpWithDB leftover check are unaffected and
stand.)

## Cycle 2: RPC help bounds

### estimatesmartfee conf_target "1 - 1008": CONSISTENT
ParseConfirmTarget enforces 1..max_target with max_target =
HighestTargetTracked(LONG_HALFLIFE) = 1008 (rpc/util.cpp:374-383).
Help matches enforcement exactly. getInt<int> rejects fractional/overflow
inputs.

### prioritisetransaction dummy "Must be zero or null": CONSISTENT
Code rejects *dummy != 0 (mining.cpp:559-561); fee_delta documented as
satoshis and parsed as int64. Help matches behavior.

### getblock verbosity 0/1/2/3: tolerant-by-design, DISMISSED
Mapping (blockchain.cpp:875-890): ≤0 → hex; 1 → SHOW_TXID; 2 →
SHOW_DETAILS; ELSE (3, 4, 99, ...) → SHOW_DETAILS_AND_PREVOUT. Any
verbosity ≥ 3 coerces to the richest view. Historical tolerance (v3
inherited the pre-existing else branch); coercion direction is the safe
one (richest, no silent narrowing). Help enumerates 0/1/2/3 without
claiming others are rejected — consistent with tolerant parsing.
Strict-mode rejection would be a compat break + churn. DISMISSED.

## Next queue
(rotate per uber-ledger — next: #6 serialization untrusted-input,
severity-first)
Help ("below n megabytes, default 300") matches code (mb × 1e6 bytes,
decimal MB); cluster-era minimum (≥ cluster_size×40) enforced loudly in
Flatten with clear message; 32-bit cap now graceful (fix above).

## Next queue
(cycle 2: RPC arg help vs behavior bounds (estimatesmartfee conf_target
range, getblock verbosity enums); REST/CLI parity; then rotate per ledger)
## Cycle 3 (2026-08-02, draw 189, raw=16823264503710230643, masked 7599892466855454835, idx 3/44): LINEAGE REPAIR (this journal was missing from agent/all-findings — pre-rotation gap, #66-c1 class) + re-verification of the c1 retraction at today's HEAD

### Lineage repair
This journal lived only on audit/public-interface-contracts
(pre-rotation era) and was never copied to agent/all-findings.
Detected on the draw-189 journal read (file absent from the
archive worktree). Repaired by content copy from the branch tip
(this commit); branch remains authoritative for c1/c2.

### Re-verification of the c1 retraction at HEAD (2026-08-02)
- The 32-bit -maxmempool cap returns util::Error inside
  ApplyArgsManOptions (node/mempool_args.cpp:49-53).
- AppInitParameterInteraction checks the result at init.cpp:1168
  (graceful startup error), making the InitAndLoadChainstate
  Assert at init.cpp:1338 unreachable-by-construction — its own
  comment says exactly this ('already checked in
  AppInitParameterInteraction').
- The c1 finding (36156ad934, later reverted 5a16d316af) stays
  RETRACTED; the retraction's premise holds at current line refs.

### Verdict
Bookkeeping CONFIRMED + repaired: archive lineage complete again;
no defect (retraction verified sound).

### Exact commands
- git show audit/public-interface-contracts:agent-journal/
  public-interface-contracts.md (source of this file);
  grep init.cpp:1150-1338, mempool_args.cpp:49-53.

### Limitations / queue
- The "Next queue" sections above (c1/c2 era) stand; both were
  superseded by the retraction and later campaigns' arg sweeps.
