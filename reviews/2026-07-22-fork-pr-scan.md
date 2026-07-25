# Fork PR scan (2026-07-22) — l0rinc/bitcoin open PRs

All 124 open fork PRs scanned (5 review batches over ~55 active PRs; rest stale experiments).

## More serious than advertised

- **PR 237** (zero-input PSBT) — under-advertised: live release-build abort via walletprocesspsbt/descriptorprocesspsbt on a legal BIP370 zero-input state (Core keeps asserts in release; `UpdatePSBTOutput` always signs `input_idx=0`). Upstream 7e19ce200b is a different bug. MERGE FIRST. (Follow-up: fully reviewed 2026-07-24, see pr-237 file.)
- **PR 225** (flush every dirty undo file) — fixes still-open upstream #25539: crash after WriteBlockIndexDB persists BLOCK_HAVE_UNDO for un-fsynced undo data → restart fails VerifyDB → forced reindex. Silent data loss on master today. Small, best-tested.
- **PR 236** (release sync slot after empty headers) — remote, unauthenticated liveness: inbound-only node (m_num_preferred_download_peers==0) held in IBD indefinitely by one empty-headers reply (timeout needs preferred_download>=1). Tiny correct fix.
- **PR 190** (pruneassumevalid) — bundles DoS-resistance weakening: swaps SaltedTxidHasher/SaltedWtxidHasher (mempool, attacker-influenced keys) to SipHash-1-3-jumbo — the trade the fork's own perf log rejected for the coins cache. Decide the hasher question first; split from feature.
- **PR 178** (verify assumeutxo hashes in IBD) — imposes a multi-minute cs_main-holding full-UTXO hash + forced flush at height 840000 on EVERY IBD node (from-genesis chainstates are Assumeutxo::VALIDATED), advisory-only check. Gate behind debug option or reject.
- **PR 218** (pruned block index bookkeeping) — consensus-adjacent: nSequenceId mutated while inside setBlockIndexCandidates → std::set ordering UB; pruned-chain liveness hole. USE the newer BLOCK_FAILED_VALID variant on audit branch, not the stale PR head.
- **PR 223/224/232** — silent-wrong-data family: indexes serve disconnected-chain state as "synced" (persisted); crash+reorg leaves gettxspendingprevout answering spends from a dead block; m_chainstate UAF after snapshot completion.
- **PR 245** (limit low-work headers syncs) — remote unauthenticated weak memory-DoS during IBD (~100KB+ per inbound peer's HeadersSyncState). (Fully reviewed 2026-07-24, correct.)
- **PR 204/109** — live HTTP bugs: HEAD gets full body (keep-alive desync), handler-set `Connection: close` ignored (204), partial sends O(n^2) memmove (~160GB for 100MiB reply, 109). Stack 204 first.

## Fix-the-fix / simplification

- PR 217: local SaturatingSub returns MAX for MIN,MIN (true 0) — use shared SaturatingSubtract from 228. Stack cluster 238→228→217→209.
- PR 232: drop BlockUntilSyncedToCurrentChain hunk (let 211 own it). PR 230: split txindex format overhaul from fetch-proxy. PR 203: needs IBD benchmarks (write_buffer_size silently drops to 4MB default).
- PR 222: split 46 commits into 5 behavioral vs 41 oracle. PR 123: shrink 2519-line reorg proxy (use existing P2PInterface). PR 127: drop 3 unrelated refactors + compressor hunk (take 188).
- PR 212: regenerate scripted diffs on fresh master, don't rebase. PR 137: doc rejection stale; correct but marginal — belongs upstream.
- Coverage gaps: functional zero-input walletprocesspsbt (237) ✅ later added; two-page BDB cycle (239); upgrade→downgrade roundtrip (244); golden hash_serialized_3 vector (241); post-IBD cap-off test (245); explicit -dbcache non-shrink test (126); in-flight count agreement test (190).

## Close (stale/superseded/benchmarked rejections)

199, 200, 152, 138 (perf-branch adjudicated), 229 (upstream 6aa5d8d948), 21+65+140 (measured rejections), 197 (knob sweep), 116 (mostly adjudicated; CRITICAL-overshoot raises OOM risk), 33 (rejected + assert-only regression), 122 (perf doc rejected sibling; IBD lead only).

## Knots rebase (PR 240) — critical-for-Core

~30 covert fixes to cherry-pick (top: cf436bed1f fixes open Core #30210; dfb593e8e9 Windows truncate; 5e3de447c9 GUI hang; a637d80c34 torcontrol; 48669516c6 warnings regression; f83772ae5e+24e81ee1ce first-run disk check). blockslop late-upgrade chainstate gap REAL (BIP110 validation provenance unmarked; dormant until mandatory signaling ~961,632). Feb-2026 cache-poisoning vector closed by insert guard. 6 rebase-introduced bugs in the PR itself (Qt build break, silent Core #32149 revert, guix allow-list corruption, stale cherry-picks, orphan fuzz file, rebase_assessment/ junk).
