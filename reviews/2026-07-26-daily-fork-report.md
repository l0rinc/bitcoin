# Daily fork/upstream report (2026-07-26)

## Sweep findings being fixed

- ✅ **asmap digit-leading path ignored** (sweep finding #5, 6244212a5): fork branch `l0rinc/asmap-file-argument` (`4063d8d582` "init: load digit-leading asmap paths" + characterization test `8e72845a90`) — fixes the `-asmap=1755187200_asmap.dat` silent-ignore (bool parse) issue, hitting the canonical asmap-data filename format.
- Two sweep findings now have fix branches: torcontrol backoff (#2, yesterday) and asmap (#5, today). Still open: df44afdc9 kernel get_ancestor (#1), d23641564 gettxspendingprevout O(n²) (#3 — fixed via fork PR 247), AES256 latent copy (#4), e0463b4e8 coinbaseTxToJSON CHECK_NONFATAL (#6).

## New branches

- detached617–620 (series continues, no flagged content yet).
- `l0rinc/asmap-file-argument` (above).

## Upstream

- 🟢 no new l0rinc-authored commits on origin/master since 2026-07-23; no new l0rinc PRs; no PR head updates since 2026-07-25.
