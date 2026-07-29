# Campaign #41 — history-seed-archaeology

Base: a49a2ebe92 (journal commit for #108 cycle-1 on
audit/adversarial-artifacts; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/history-archaeology. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): fee_estimates.dat version gate — archaeology of the migration contract, empirically fails closed both directions

### Draw
Random draw over the 33-goal pool (20 pending + 13 CYCLE-1; #108
excluded as just-cycled): raw=13402773429125480578, seed masked to 63
bits (4179401392270704770), index 5 -> #41. Seed: persistence-format
evolution of fee_estimates.dat (this session's estimator work: #63
fix, fork guards 62f15c4ab9/44fcabc565/0cf655f1d6).

### Archaeology (contract as it stands)
- CURRENT_FEES_FILE_VERSION = 309900 (estimator .cpp:37-39, with the
  bump instruction on breaking change). Write serializes version,
  best-seen height, historical range, buckets, 3 TxConfirmStats.
- Read gate (Read at :1103): version > CURRENT -> throw "too high to
  be read" (caught non-fatally); version < CURRENT -> LogWarning
  "Incompatible old fee estimation data (non-fatal)" and read on
  (backward-compatible); == CURRENT -> full read with snapshot
  variables (44fcabc565's atomicity: partial reads never corrupt
  live state).
- Fork's read-side guards (62f15c4ab9 IsSaneEstimatorVector,
  0cf655f1d6) are additive; the file format is unchanged by them and
  by #63's m_all_zero (not persisted).

### Empirical differential (current -> v28.2)
Current node on regtest writes fee_estimates.dat (309269 B, version
bytes 8cba0400 = 309900 LE) after real txs; start v28.2 against a
copy: log reads
  "CBlockPolicyEstimator::Read(): unable to read policy estimator
   data (non-fatal): up-version (309900) fee estimate file"
  "Failed to read fee estimates ... Continue anyway."
v28.2 starts fresh and stays healthy (getblockcount OK).

### Verdict
- DISMISSED: the persistence-migration contract fails CLOSED in both
  directions — older files read with a non-fatal warning, newer files
  rejected non-fatally; no silent misinterpretation path exists.
- The fork's guard series and #63's change preserve the format
  (verified by the 309900 version byte and v28.2's rejection message
  naming it) — archaeology consistent with the code.

### Exact commands
- greps/seds: block_policy_estimator.cpp:37-39,464-510,1103-1160
- node run to produce the file + v28.2 read against a copy
  (/tmp/btc41_d, /tmp/btc41_old, removed after)

### Limitations / queue
- Downgrade WITH data (v28.2 file -> current) exercised by the code
  path (non-fatal old read) but not run this cycle — the warning log
  line is the same class; noted, not re-run.
- Other persistence artifacts (mempool.dat, anchors.dat, xor.dat)
  have their own version stories — queued as separate archaeology
  cells.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
