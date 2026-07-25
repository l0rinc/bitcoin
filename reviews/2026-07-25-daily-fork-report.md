# Daily fork/upstream report (2026-07-25)

## Sweep findings being fixed

- ✅ **torcontrol busy reconnect loop** (sweep finding #2, eae193e75): fork branch `torcontrol-backoff-after-disconnect` (`bc0ebcc9b8`) applies exponential backoff after disconnect, not just connect failure — exactly the prescribed fix, plus characterization test `e1b6f893d1`.
- **kernel C-API robustness** (sweep finding #1 area): new branch `kernel-handle-invalid-c-api-arguments` (4 commits: `80257396b6` invalid chain types, `d2f17ee891` invalid logging settings, `6f23568be8` script verification args, `9a1cdef1e1` invalid spent output arrays). The df44afdc9 get_ancestor null-wrap is NOT among them — still open.

## New fork fix branches (UB-class, worth review when upstreamed)

- `p2p-handle-zero-orphanage-latency-share` (`379c85c2d1`): TxOrphanage floor-shares floored at 1 (avoids div-by-zero in ByRatio) and DoS threshold `>1`→`>=1` (a peer at exactly floor-share could otherwise trip `Assume(score>1)` during trimming); test flipped from expecting abort to expecting success.
- `txgraph-equal-feerate-prefix-overflow` (`b24c47255c`): equal-feerate fee overflow in txgraph chunk prefixes + extraction refactor + tests.
- `net-refresh-peer-tx-activity` (`0b6abc43e0`): refresh peer activity timestamp after accepted tx (+ fuzz coverage, orphan activity characterization).
- `pr35164-mixed-sigop-coverage`: FIXME-revert exploration on mixed witness/legacy sigop scaling and witness sigops in P2SH txs — consensus-counting territory, watch when it firms up.
- `test-kernel-sigaltstack` (`b2236a5195`): undersized Boost.Test signal stacks.

## Force-pushes (content already reviewed)

- `coins-cursor-resize-lifetime` → the reviewed shared_mutex 3-commit series rebased (`b446775e8e`/`ea101a2d8a`/`1154f96c5f` ≡ 791ab17e8c/f9b96fe758/2f125947d4).
- `psbt-zero-input-output-update`, `reobfuscate-blocks`, `share-dbcache-defaults`: restructured with more test commits, same content.

## Upstream

- 🆕 **PR 35797 "psbt: support output metadata updates before inputs are added"** — the zero-input PSBT fix upstreamed; head `0fa3581395` identical to the reviewed fork branch (all three guards correct, taproot zero-input coverage included). Ready for upstream review; see reviews/2026-07-24-pr-237-psbt-zero-input.md.
- 35744 updated = same shared_mutex content (already reviewed).
- No new l0rinc-authored commits on origin/master since 2026-07-22.
