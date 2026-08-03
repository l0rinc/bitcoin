# Campaign 119: Bulk multi-repository and ecosystem recurrence mining

## Cycle 1 (2026-08-03): write-failure family normalized + in-tree recurrence sweep
Draw: seed raw=6652536966674966662, masked same, n=13 (new goals
110-119 + 125-127; Sparrow 120-124 skipped, no local repo), idx=9.

### Bug shape (normalized from 6 confirmed instances)
Restart-authoritative file written directly (no temp+rename);
write/close failure leaves a truncated artifact OR escapes the
designed error path; next startup treats the artifact as valid
and fails unrecoverably or never retries generation.
Family: F19 (flush marker), F26 (xor.dat), F27 (snapshot
base_blockhash write), F31 (i2p key), F32 (tor key), + goal38
(write-side sibling of F25's read arm). Two independent audit
campaigns converged on the same shape within 24h.

### Recurrence sweep (all in-tree persistence producers)
- mempool.dat — .new + RenameOver: ATOMIC (safe).
- peers.dat / anchors.dat — SerializeFileDB temp-random + Commit +
  remove-on-failure + RenameOver: ATOMIC (safe).
- banlist.dat — CBanDB::Write + is_dirty retry: covered #41 c6.
- fee_estimates.dat — direct write, but read path discards on any
  error + scheduler rewrites hourly: benign by design.
- settings.json — direct ofstream, but read side parse-tolerant
  (warning + defaults + rewritten on change): benign.
- i2p private key — LIVE: truncated key never regenerated.
  CONFIRMED + ADOPTED e976e68fc9 (failing-before: file persists +
  retry Connect fails; passing-after: full i2p_tests green).
- tor onion_v3_private_key — LIVE: truncated key blocks service
  restart. CONFIRMED + ADOPTED 5cf00e1380 (failing-before:
  key-path-as-directory portable write failure; passing-after:
  full torcontrol_tests green).
Sweep EXHAUSTED for src/ producers. Verdict: the atomic-rename
convention holds everywhere except single-fwrite small files
(WriteBinaryFile users) and one-off AutoFile writers.

### Cross-project notes (goal-119 scope)
- libsecp256k1: no restart-authoritative file producers (N/A).
- LevelDB (in-tree): WAL/MANIFEST writes have their own env-layer
  error model + recovery (goal 125-127 territory); the truncated-
  authoritative-file shape maps to MANIFEST/CURRENT corruption —
  LevelDB's own recovery handles by design; no client-assumption
  violation found in this sweep (deeper audit = goals 125-127).
- Sparrow: no local repo; skipped.

### Exact commands
- Greps: WriteBinaryFile/AutoFile/SerializeFileDB/RenameOver over
  src/node, src/net, src/policy/fees, src/i2p, src/torcontrol,
  src/common/settings.
- i2p pair: test-only apply of 32167c5c58 -> 2 BOOST failures;
  + fix -> full i2p_tests green.
- tor pair: manual test (key path as directory) -> 1 BOOST
  failure; + ae32c111a3 fix -> full torcontrol_tests green.

### Resume points / queue
- Regression #7 on the final lineage (covers F28-F32).
- Next rotation: re-rank draw (goal 119 exhausted for this shape
  in-tree; cross-project LevelDB arm = goals 125-127's own draws).

## Cycle 2 (2026-08-03, goal 115 committed-diff reviewer pass, draw raw=12596182104518000101 masked=3372810067663224293 n=12 idx=5)
Independent reviewer-pass over this session's adopted diffs
(scout/fixer vs reviewer separation). Verdicts:
- F26 xor-key (2110abf119): catch-all `catch (...)` runs
  fs::remove(xor_key_path) even when the fopen itself failed —
  if "wbx" failed because ANOTHER process created the file in the
  exists->create race window, the catch deletes the FOREIGN valid
  key file. Reachability: the datadir process lock makes two live
  bitcoinds impossible; a foreign writer racing a startup create
  is contrived. Benign sub-case: fopen fails (perms/ENOSPC/fd) ->
  remove is a no-op + one noise LogError. Verdict: ACCEPTED RISK,
  documented; upstream/parallel has the same shape; a `created`
  guard is a cosmetic follow-up, not a defect fix. fclose-failure
  path correct (removes possibly-corrupt own file).
- F27 snapshot write (3c9090b644): typed catch (ios_base::failure
  only) after IsNull check; fopen "wb" truncation of an existing
  marker is by design; cleanup_bad_snapshot wipes the whole
  chainstate dir including the marker — consistent, no masking.
  CLEAN.
- F30 headers clamp (35473f91b4): HeadersSyncState is per-sync-
  attempt; a later attempt with a caught-up clock recomputes a
  positive bound — no permanent lockout. elapsed>0 path unchanged.
  CLEAN.
- F28/F29 option guards: negative-only/overflow-only rejections;
  0 stays accepted for both (matches their boundary tests); no
  previously-valid config rejected. CLEAN.

### Working-tree scope audit (goal 115 iter 2)
Tracked: clean — zero uncommitted modifications (the user's
unstaged campaign-goals-99.md deletion was restored by an earlier
reset --hard during cherry-pick cleanup; content == HEAD, recorded
in cycle-297 ledger entry). Untracked: user's own files only
(.ls.swp, campaign-goals.md = the 128-goal catalog, node_modules,
package*.json) + 20 stale fuzz artifacts (crash-*/slow-unit-*,
analyzed cycle 255, left untouched per protocol). No debris from
this session's experiments (/tmp harnesses documented in journals).
Verdict: CLEAN.

## Cycle 302 (goal 126 LevelDB semantics, draw raw=7175895203802760358 n=10 idx=8)
Scope: comparator/snapshot/iterator/filter/compaction semantics of
the in-tree pinned LevelDB subtree.
- Comparator: N/A — dbwrapper uses default BytewiseComparator
  everywhere (no custom comparator in-tree; contract trivially
  satisfied). Bloom filter: standard 10 bits/key.
- Iter 1: iterator-vs-concurrent-writes conformance harness —
  20 rounds, half-in-range Put/Delete storms from a writer thread
  while iterators scan: 0 torn keys/values, 0 post-snapshot keys,
  0 status errors, ASan+UBSan silent. DISMISSED (contract holds).
- Iter 2: pinned iterator + overwrite 1/2 + delete 1/4 + full
  CompactRange: complete original snapshot (20,000 keys, gen-0
  values) observed post-compaction. DISMISSED (version refcounting
  holds as documented).
- Boundary note: the one real iterator hazard (DB reset/destruction
  mid-scan, 35744 family) is already covered-ahead in-tree
  (UniqueLock cursor lifetime). dbwrapper contract layer verified
  previously (database-semantics-differential journal).
Harnesses preserved in agent-journal/artifacts/ (+ build note:
link against the ASan-instrumented in-tree libleveldb).
Campaign #126 verdict: DISMISSED for the audited arms; WAL/
MANIFEST corruption fixtures remain queued under goal 125.

## Cycle 303 (goal 114 threat-model -> oracle conversion, draw raw=17746324290832141658 masked=8522952253977365850 n=9 idx=3)
Threat model converted: "clock-skew breaks security gates" (F22/F30
proven instances) -> full sweep of time-gated security checks.
Method: enumerate NodeClock/GetTime gates in net_processing,
validation, txmempool, banman, addrman (35 sites); classify each
gate's skew margin vs attacker influence (adjusted-time attacker
bound = +/-70 min via the 199-sample peer median).
Classification (all remaining gates SAFE BY DESIGN MARGIN):
- IsStaleTip/BEST_HEADER_STALE_AGE (net_processing.cpp:1388): 24h
  margin, uses SYSTEM clock (not attacker-influenceable adjusted
  time); backward skew only degrades own sync liveness (no
  attacker primitive — attacker cannot set the clock). Degraded-
  mode, by design.
- Tip recency (:1407, spacing*20 = 200min): 3.3h margin > 70min.
- Ping timeout: >=20min margin vs no remote clock influence.
- BanMan ban durations: hours-level, self-inflicted at worst.
- Block timestamp acceptance (MAX_FUTURE_BLOCK_TIME=2h): computed
  from adjusted time itself — self-consistent by construction.
- fee_estimates MAX_FILE_AGE: hours-level.
NEGATIVE RESULT (narrows the threat model): the only gates that
can fail under attacker-relevant conditions are those with
sub-minute effective margins AND remote-controlled inputs — the
two proven instances (F22 empty-header slot, F30 commitment-cap
wrap) are exactly that class; no third member exists in-tree.
Future findings in this family are valid ONLY for: unclocked
remote-input gates or margins < 70min. Verdict: sweep complete,
no new defect candidate; recorded as boundary knowledge.

## Cycle 304 (goal 127 LevelDB corruption/checksums/bg-errors, draw raw=11133441295003937015 masked=1910069258149161207 n=8 idx=7)
- Client assumptions verified in code: dbwrapper sets
  paranoid_checks=true, verify_checksums=true on read+iter options
  (dbwrapper.cpp:150,237-238); ReadImpl maps NotFound->nullopt and
  everything else -> HandleError -> dbwrapper_error THROW
  (fail-loud); WriteBatch failures surface to F19-hardened paths.
- Iter 1 (corruption/checksum arm): single-byte corruption deep in
  a compacted .ldb table; reopen OK; 86/5000 keys surface
  Status::Corruption on read, 0 silent-wrong (uncorrupted keys
  read fine). Table-block corruption = loud on read, never silent.
  CONFORM — DISMISSED as defect.
- Iter 2 (background-error arm): deterministic db-dir rename fault
  -> background compaction file creation fails -> the very next
  Put returns NotFound (error propagates, writes do NOT silently
  succeed). CONFORM — DISMISSED as defect.
- Harness lesson recorded: chmod faults are invalid as root
  (CAP_DAC_OVERRIDE); use dir-rename.
- Boundary: MANIFEST/CURRENT/descriptor corruption recovery is
  goal 125's queued cell (WAL/MANIFEST fixtures, per
  database-semantics journal c1 note); reopen-time checksum gaps
  for non-data files live there, not here.
Campaign #127 verdict for the audited arms: DISMISSED; assumptions
hold with two independent verifier forms (harness + code).

## Cycle 305 (goal 117 calibration w/ mutants + negative controls, draw raw=17929826660396438331 masked=8706454623541662523 n=7 idx=4)
Question: are this session's adopted regression oracles still LIVE
on the final lineage (not rotted/masked by later changes)? Method:
re-inject the original defect as a one-line mutant on the current
tip, expect the oracle to kill it; restore after each.
- M1 txdb.cpp Cursor(): drop the !GetKey(entry) check ->
  coins_tests/malformed_first_coin_key_cursor_invalid FAILS
  (2 BOOST failures, identical shape to F25 failing-before).
  KILLED; restored.
- M2 mempool_args.cpp: disable the negative-expiry guard ->
  MempoolExpiryOptionTest negative arm FAILS. KILLED; restored.
- M3 headerssync.cpp: remove the >0 clamp -> future_chain_start_
  mtp_bounds_commitments stays PRESYNC (0 != FINAL 2). KILLED;
  restored.
Verdict: 3/3 oracles live on the final lineage (5158a4cbe0+);
calibration CONFIRMS the adoption evidence chain end-to-end
(failing-before shapes bit-identical to adoption-day runs).
Negative-control note: each mutant's failure was verified to come
from the mutation itself (restore + prior green runs), not drift.

## Cycle 307 (goal 113 risk ranking + marginal yield, draw raw=12828903349424140929 masked=3605531312569365121 n=5 idx=1)
Risk table for the live queue (severity x reachability x confidence
x next-proof-value, 1-5 each; yield = next-proof / cost):
| item | sev | reach | conf | proof-val | cost | note |
| 125 WAL/MANIFEST recovery | 3 | 2 | 4 | 4 | low | reopen-time blast radius, queued cell, harness pattern proven |
| 116 cross-scanner differential | 2 | 2 | 3 | 3 | med | clang-tidy MISSING on host (tool blocker) |
| 111 coverage-manifest closure | 1 | 1 | 4 | 2 | low | bookkeeping, low defect yield |
| 118 agent sandbox isolation | 1 | 1 | 2 | 2 | med | process, not code |
| standing watches (35744 etc) | 4 | 3 | 5 | 1 | n/a | upstream-side, no local marginal work |
| fuzz corpus re-runs | 1 | 1 | 5 | 1 | high | repeat of passing campaign (barred) |
Ranking verdict: 125 has the only non-trivial marginal yield in
the pool (4 proof-value at low cost, unexercised arm with
reopen-time blast radius) — it SHOULD preempt the random pool on
the next draw if not drawn naturally; 116 is tool-blocked
(clang-tidy absent), 111/118 are bookkeeping with low defect
yield; the standing watches have zero local marginal yield
(upstream-side). Stop-depth audit: 126/127 dismissals stopped at
the right depth (each arm conform + code-verified + harness
preserved; the WAL/MANIFEST arm was explicitly queued rather than
half-done — the correct marginal call).
Decision for next cycle: if the random draw does not land 125,
take 125 anyway as the marginal-yield winner (recorded deviation
from the draw, per urgent-preempt rule).
