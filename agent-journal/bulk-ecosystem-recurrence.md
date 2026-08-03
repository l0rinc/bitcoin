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
