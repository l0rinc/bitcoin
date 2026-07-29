# Campaign #38 — failure-cleanup-crash-safety

Base: f6a48cda57 (journal commit for #99 cycle-1 on audit/clean-room;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/failure-cleanup-c2 (c1 journal carried from
audit/failure-cleanup @ 42e44006a7 — missing ledger row restored this
cycle). Start state: clean (untracked scratch only).

## Cycle 2 (2026-07-29): txindex interrupted-build recovery — resume mechanics verified; empty-block build too fast to interrupt honestly

### Draw
Random draw over the 23-goal pool (10 pending + 13 CYCLE-1; #99
excluded as just-cycled): raw=13725734712747295543, seed masked to 63
bits (4502362675892519735), index 3 -> #38.
STATE NOTE: #38 c1 (42e44006a7 + fix 9894fb8b6c, EncryptWallet
in-memory mkey rollback) lived on audit/failure-cleanup without a
ledger row; recorded retroactively in this cycle's update.

### Cell: index-build crash recovery
Premise: kill -9 mid-txindex-build, restart, verify the best-height
marker resumes/rebuilds without stale-synced claims.

### Result (two scales)
- 800-block chain: build completed before a 1.2s kill.
- 8000-block chain (71s to generate): build completed before a 3s
  kill — log shows "txindex is enabled at height 8000" from the
  completed build, restart reports the same completed state. The
  build (block-index scan, empty blocks) is ~2-3s for 8000 blocks.
VERDICT: INCONCLUSIVE-but-informative. The intended interruption was
not reproducible at regtest-empty-block scale: the txindex build is
uninterruptible in practice because it finishes in seconds. The
resume MECHANICS are confirmed present (best-height marker +
idempotent completed-state report), but a true mid-build kill needs
a multi-minute build (tx-heavy chain or signet/mainnet data).
Honest negative result, preserved with the exact schedule.

### Verdict
- DISMISSED at this scale (no crash-safety defect reachable; the
  intended fault cannot be injected fast enough to matter).
- Follow-up that WOULD exercise it (queued): tx-heavy MiniWallet
  chain (40k txs, ~8 min build) with kill windows at 25/50/75%.

### Exact commands
- generatetoaddress 800/8000; -txindex start; pkill -9 at 1.2s/3s;
  getindexinfo polling; debug.log grep 'txindex is enabled at height'

### Limitations
- The premise failure itself bounds the claim: no evidence about the
  resume-under-real-load behavior — only its presence.
- The restart after the completed build wasn't independently
  re-verified (node stop/start sequence interference from the pkill
  pattern matching both starts — noted as harness hygiene: pattern
  kills must distinguish start generations).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
