# Reviews index

Every review/finding produced in this workspace gets committed here. Chat output is ephemeral; this directory is the record.

| file | subject | verdict |
|---|---|---|
| 2026-07-22-fork-pr-scan.md | all 124 open fork PRs | 10+ more-serious-than-advertised items; fix/simplification/close lists |
| 2026-07-23-pr-247-gettxspendingprevout.md | fork PR 247 (O(n^2) RPC fix) | 🟢 correct, merge |
| 2026-07-24-pr-32575-script-check-unification.md | upstream PR 32575 (single-thread script checks via queue) | not dangerous, no leftover bugs; overlaps your 34875 |
| 2026-07-24-pr-237-psbt-zero-input.md | fork PR 237 (zero-input PSBT) | 🟢 correct; taproot case added; functional test still open |
| 2026-07-24-pr-245-headers-sync-limit.md | fork PR 245 (IBD presync cap) | 🟢 correct; slot-release test suggested (added) |
| 2026-07-24-pr-35793-bip54.md | upstream PR 35793 (BIP 54, no mainnet activation) | 🟢 no merge-blocker; gating correct |
| 2026-07-24-secp256k1-pr25-opaque-sig-overflow.md | secp256k1 PR 25 (opaque sig overflow) | 🟢 correct hardening, low severity |
| 2026-07-24-buzz-mirror-assessment.md | Buzz mirror of bitcoin/bitcoin | git mirror ~1 day; PR/issue import ~1 week; no public repos in v1 |

Older committed records: `../PR-REVIEW-TOP200.md` (200 most active open PRs), `../MERGED-2000-REVIEW.md` (last 2000 merged commits), `../COMMIT-REVIEW-PR240.md` (Knots rebase 1907 commits).
