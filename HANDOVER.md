# Handover: audit session state (2026-07-19)
#
# NOTE (2026-07-19, resurrection session): newer work exists on `audit/resurrection`
# (3 commits on top of `audit/lifetimes`, below). See `HANDOVER-RESURRECTION.md` for
# the current state; this file still describes the audit/lifetimes + audit/supply-chain session.

Repo: /mnt/my_storage/bitcoin (bitcoin/bitcoin fork workflow; remotes: `origin`=bitcoin/bitcoin, `l0rinc`=fork)
Author of all audit commits: `Lőrinc <pap.lorinc@gmail.com>`

## Branch state

- `audit/lifetimes` @ `5d651df9a4` — CURRENT. Base = `2618715533` (tip of the 569-commit prior-work fuzz/oracle stack, `l0rinc/detached584`) + 7 continuation commits (below).
- `audit/supply-chain` @ `09a2c19d8e` — COMPLETE. Base = `a823249fbf` (txindex line) + 2 supply-chain commits.
- Prior-work stack: 569 commits (`b56b66fc64..2618715533`), mostly fuzz-oracle work. NOT rewritten.

## Build directories (existing — reuse, do not create new ones)

- `build-before/` — Release, gcc-13, `BUILD_TESTS=ON`, `BUILD_BENCH=ON` (added later), `ENABLE_IPC=ON`. Has bitcoind, bitcoin, bitcoin-cli, bitcoin-node, bitcoin-wallet, bitcoin-tx, bitcoin-util, test_bitcoin, bench_bitcoin.
- `build-after/` — clang-18, currently **Debug+ASan** (`-DSANITIZERS=address`, `ABORT_ON_FAILED_ASSUME` active). Was fuzz (`-DBUILD_FOR_FUZZING=ON -DSANITIZERS=fuzzer`) and plain Debug before; reconfigure as needed.
- Note: `Assume()` is compiled out in Release/RelWithDebInfo; active only in Debug/fuzz builds (`core_interface_debug` only). Release builds strip `-DNDEBUG` so plain `assert()` is always on.

## Delivered commits (strongest first)

### On `audit/supply-chain`
1. `48d6460315` ci: pin sha256 for shellcheck/mlc downloads in lint image. GitHub release assets are mutable; unverified binaries ran in every lint job (repo mounted, GPG keys on master pushes). Proof: sandboxed userns run — genuine assets pass, substituted valid tarball rejected (`sha256sum FAILED`), old script installed attacker binary rc=0. Note interplay: ci.yml has NO top-level `permissions:` block (token scope = repo default, unverifiable from tree).
2. `09a2c19d8e` ci: pin sha256 for cross-build SDK downloads (macOS/NetBSD/FreeBSD/OpenBSD). 200–530MB tarballs into compile sysroot with no integrity check; macOS hash was documented in contrib/macdeploy/README.md but never enforced. Cache-persistence angle: poisoned `sdk-sources` cache would persist into PR jobs. Proof: same sandbox pattern; hashes cross-checked against official sums (Xcode==repo README; FreeBSD==MANIFEST; OpenBSD==signify SHA256; NetBSD==SHA512).

### On `audit/lifetimes` (continuation of 569-commit stack, base `2618715533`)
3. `46e80718a2` index: drain notifications before reporting no best block. **Production regression in prior work `f344e8102c`**: pre-drain `if (!best_block_index) return false` raced with queued `BlockConnected` → spurious not-synced. Failed 3/3 at txindex_tests.cpp:58 → 10/10 after; post-drain false kept for genuine restart case.
4. `a7149fc813` test: check Assume-guarded rejections only where Assume aborts. Two validationinterface tests expected `NonFatalCheckError` from `Assume()`, compiled out in Release → deterministic Release failure. Gated with repo idiom `if constexpr (G_ABORT_ON_FAILED_ASSUME)`.
5. `2b03e8ba71` test: expect fail-closed rejection of unrepresentable RBF diagrams. Stack-internal inconsistency: `a5a8001c32` (accept expectation) vs `228c637014` (deliberate fail-closed diagram gate). Aligned test with later documented design. NOTE for review: the gate is a policy regression for miners using extreme `prioritisetransaction` — flag when publishing.
6. `77c5a11526` fuzz: crypter encrypt/decrypt round trip (oracle; mutation proof at crypter.cpp:65; empty-plaintext edge excluded — `Decrypt` returns len==0 as failure).
7. `ae2bf49412` fuzz: GetMinimumFeeRate floor contracts (oracle; mutation proof at fees.cpp:131; generator extended to cover fOverrideFeeRate).
8. `a6e490ab5b` bench: assert mempool stress receives full generated history (mutation: silent insertion drop → assert fires; baseline measured 6000/6000, 60000/60000).
9. `5d651df9a4` test: tolerate no-op tip updates in signals ordering check. Prior-work invariant violable by upstream `UpdatedBlockTip` suppression when `pindexFork==pindexNewTip` (failed-reorg→same-tip); 5/12 fail under ASan → 12/12 pass. KEY CONTRACT: `ActiveTipChange` is SYNCHRONOUS (direct Iterate) while `BlockConnected`/`UpdatedBlockTip` are QUEUED — no ordering between them.

## Verified gates (all green at `5d651df9a4`)

- Release full unit suite: 1119 cases (build-before).
- Debug full unit suite (Assume-active): 1121 cases (build-after, pre-ASan config).
- ASan+LSan full unit suite: 1121 cases, zero leak reports ×2 (build-after current config; suppressions=test/sanitizer_suppressions/lsan).
- Full functional suite: 475 runs (build-before); failures resolved: feature_rbf (→2b03e8ba71), tool_bitcoin+interface_ipc_cli (environmental: needed `bitcoin`/`bitcoin-node` binaries, now built).
- Previous-releases compat (8 verified releases downloaded to ./releases): wallet_migration, wallet_backwards_compatibility, mempool_compatibility all pass. tool_bitcoin_chainstate properly skipped (BUILD_UTIL_CHAINSTATE=OFF).
- Debug P2P subset (Assume-active binaries): 25 tests pass (addr-relay, compactblocks ×3, private broadcast ×3, handshake, invalid msgs/block/tx, headers presync, feature disconnect, tx download, orphan handling, eviction, 1p1c, feature_rbf).
- bench_bitcoin mempool stress with new asserts: pass.

## Leak-audit verdict (this task)

No definite leaks/cycles/dangling escapes found. Static: NodeContext all-unique_ptr; no `enable_shared_from_this` anywhere; WalletContext↔CWallet acyclic; HTTP work items value-capture shared_ptr + pool drains on Stop; IPC unique_ptr/kj::Own; PeerRef one-directional; validation registrations paired (peerman 1922/342, fee_estimator 1686/373, zmq 1845/423); AutoFile/SQLiteBatch RAII sound (write semaphore released on all paths); TorController libevent-free; init.cpp teardown explicitly ordered.

## Unproven candidates (report only — do NOT commit without proof)

- `EncryptWallet` (wallet.cpp ~858): discards `WalletBatch::WriteMasterKey` bool → silent-failure path on DB error (correctness, not leak).
- `BaseIndex::Init()` failure path returns after `RegisterValidationInterface(this)`; covered by destructor Stop(); benign.
- ci.yml missing top-level `permissions:` (unverifiable from tree).
- Mutable CI image tags (ubuntu:26.04, debian:trixie): deliberate upstream CI freshness.
- `verify.py` counts EXPKEYSIG as good toward threshold: upstream design, would break historical verification if changed.
- `HasValidOps()` vs OP_CHECKSIGADD: `MAX_OPCODE=OP_NOP10` → decodescript/testmempoolaccept misreport raw tapscripts (RPC-reporting only, LOW).
- `ChainstateManager::DeleteChainstate` asserts `m_mempool->size()` unconditionally: null-mempool deref only in kernel-API/test contexts (bitcoind always has mempool; LOW-MEDIUM).
- Headers-sync stall (audit-branch lead, not locally verifiable): timeout+grace escape hatches exist; MEDIUM at most.

## Working rules accumulated this session (user fragments)

1. Proofs: temp mutations in production OR in the generator itself when the generator is part of the test; NEVER commit mutations; verify per commit, not per batch.
2. Build/run from existing CMake build dirs only.
3. Keep #include deps accurate in touched files (IWYU; e.g. add `<cassert>` when using assert directly).
4. Sanitizers/scanners = discovery only; keep flags/suppressions out of commits unless project adopts them.
5. Separate loop/index fixes, public API type changes, signed delta changes, style modernization into separate commits unless coupling is required to compile.
6. Assert setup invariants a test/benchmark depends on (history size, spent/unspent ratios), not just the end result.
7. New benchmark subjects get short category prefixes (`mmap:`, `files:`, `coindb:`, `l0:`) so result files categorize without extra mapping; no retroactive renames.
8. Benchmark comparison artifacts: `=== before`/`=== after` sections, comma-split rows, sanitized benchmark-name field, `TryParse` second column with invariant culture.
9. PowerShell/native commands (link.exe, wsl.exe): explicit `$LASTEXITCODE` checks; ordinary PowerShell lacks `cl` on PATH (use VS dev shell); print WSL steps before running, timeout around wsl.exe (hang looks like frozen benchmark).
10. Environment-specific checks/benchmarks (WSL/MSVC, heavy perf) stay outside normal CI.
11. External DB ideas (RocksDB etc.) are leads for local benchmarks, not proof; map to closest Core/LevelDB knob; small commits; include conservative+aggressive values when unknown; `write_buffer_size` only with independent evidence, else leave default unset and remove obsolete budget plumbing; no LevelDB subtree edits unless the PR is about LevelDB (mirror logic inline with source-pointer comments; no complex formulas/APIs).
12. Scheduled chainstate maintenance: preserve reviewer-facing target behavior over old literal knob; check average cadence + no-event tail probability before choosing round probability/interval.
13. No `StopFetching()`-style guards around inherited ops (e.g. BatchWrite) for API symmetry when they only mutate `cacheCoins`; guard only when workers read same state or a test proves race/lifetime issue.
14. Warn only on countable evidence; skip warning when counting can't be done.
15. Mechanical/assisted rewrites as `scripted-diff:` commits with replayable script (test/lint/commit-script-check.sh).
16. Project-specific references beat general rules (repo idioms first).
17. Skill write-back mappings: CI/arch+benchmark pipeline → `references/ci-and-arch.md`; AI review+handovers → `references/agentic-reimplementation-review.md`; time helpers → `references/time-helpers.md`. (Skill dir absent in this environment; stage locally.)

## Suggested next steps

1. Publish preparation: this stack's most debatable behavior change is `228c637014`'s fail-closed RBF diagram gate (miner-facing policy regression vs upstream) — surface it prominently.
2. `audit/supply-chain` (2 commits) is standalone-ready for upstream CI-hardening PR.
3. If more fuzz time: remaining untouched-by-stack wallet fuzz targets (coincontrol, coinselection, scriptpubkeyman, wallet_bdb_parser) already have reasonable oracles — weak ROI.
4. Full functional suite on Debug (Assume-active) binaries is the last unrun gate (~1–2h); P2P subset already green.
5. LevelDB/chainstate benchmark pipeline (rules 7–12) not yet started; txindex-size.log at repo root shows prior 26G→25G measurement context.
