# Closed-unmerged PRs worth reviving? (2026-07-25)

18 closed-unmerged PRs since 2026-07-10 evaluated.

## YES, revive

**PR 33916 — fuzz: wallet: add target for `TransactionCanBeBumped` (brunoerg)**
- Wallet bump/RBF logic is fund-loss-adjacent and exactly where fuzzing pays. brunoerg is a core fuzz contributor; the PR died on open harness questions, not design rejection.
- Open questions to settle first: mock-vs-real mempool (janb84's data shows no speed benefit from the mock — drop it), and a UBSan "downcast" error hit at `src/test/fuzz/package_eval.cpp:213` during testing — likely harness-mock type confusion, but verify it isn't master-live before reviving.
- Revive with: non-mocked mempool, the downcast fixed or proven harness-only, and the descendant-state setup via direct TryAddTx-style helpers.

## MAYBE (low priority, politeness fix)

**PR 35740 — http: linger-close after parse errors so clients can read the reply (b-l-u-e)**
- Died with zero review. The change (httpserver.cpp/h + sock helpers + tests) makes the server send the 400 response before closing instead of an abrupt EOF — a real protocol-robustness improvement for REST/RPC consumers in the freshly rewritten HTTP server (same area as the 35735 chunked wedge).
- Author is credible (merged descriptorprocesspsbt fix). Not a crash/UB fix — revive only if someone wants to own it; the diff is small and self-contained.

## NO

- 35784 — fourth duplicate of the first-cursor-key fix; 35654 is canonical.
- 35712 (mining getInfo IPC) — superseded by the block-template-manager line (35581/35675).
- 35587 (remove boost test runner) — superseded by still-open 35713.
- 35707 (m_file_path optional), 35789 (LIBEXECDIR), 35791 (doc tweak), 35801/35804/35806/35799 (Rajkaran-122 drive-by refactors), 35758/35757/35748/35706 (junk/spam), 35739 (bench PR, served its purpose), 35711 (doc nit).
