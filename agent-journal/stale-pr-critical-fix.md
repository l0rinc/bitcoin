# Stale PR Critical-Fix Resurrection

## Cycle 94 active investigation

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `56`
- Slug: `stale-pr-critical-fix`
- Branch: `uber-cycle-94-stale-pr-critical-fix-20260729`
- Start HEAD: `b72e2ade288714d66864e2b33048aa7297428bd2`
- `origin/master`: `9b38d077f8`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `29 977` (`origin/master...HEAD`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; no relevant Bitcoin Core process was running. Existing untracked artifacts, including the prompt catalog, were preserved and excluded from commits.

### Scope ledger

Mine abandoned or stale Bitcoin Core and libsecp256k1 pull requests for a technically reproducible security, correctness, testing, portability, or performance fix. Reconstruct original intent from the PR diff, review discussion, commits, linked issue, and current callers; then test the current tree rather than blindly rebasing old code.

Initial priority order:

1. Stale PRs with a regression test, sanitizer/fuzzer seed, or concrete reviewer-identified failure whose code path still exists.
2. PRs closed for inactivity or conflicts where the defect shape can be independently reproduced against current HEAD.
3. PRs whose proposed fix is obsolete but whose test, invariant, or negative case reveals an unprotected current boundary.
4. Performance and portability PRs only after correctness/security candidates are checked with stable measurements.

Exclude already mined review threads, already recorded goal journals, and proposals whose only evidence is an old style preference. A closed PR is a seed, not proof that its premise remains true. Do not resurrect a patch until current callers, history, and tests establish the contract.

### Working protocol

For every candidate, record PR number/title/status, source and base commits, linked issue, age and closure reason, touched contract, current-code applicability, review objections, exact reproducer or missing proof, and verdict. Search local history and journals for prior findings before opening a patch. Keep discovery and verification independent when practical. Preserve rejected candidates and their missing evidence so later cycles do not repeat them.

Potential finding classes are: stale security fix still reachable; incomplete follow-up to a merged fix; test-only regression oracle that masks a current defect; portability/build gap; or obsolete proposal correctly rejected by current design. The first experiment must be small and deterministic, with scratch data and a clean current build. No code change is justified until a current failing test, minimized seed, trace, matrix result, or rigorous contract proof exists.

Next actions: inventory closed PRs and local history by security/correctness keywords; select a distinct candidate with a preserved test or review objection; reproduce on current HEAD; then compare the old patch's intended contract with current implementation and coverage.

## Cycle 94 results

### Candidate ledger

| Candidate | Evidence and current applicability | Verdict |
| --- | --- | --- |
| [#35784](https://github.com/bitcoin/bitcoin/pull/35784), `txdb: Check GetKey() result when priming cursor` | Closed as a duplicate of [#35654](https://github.com/bitcoin/bitcoin/pull/35654). Its source commit was `beaded80bf53022304139ea98f064edf6e958dc7`, and its regression-test shape independently identified the live `CCoinsViewDB::Cursor()` initialization path. | Merged into the #35654 evidence; not a second patch. |
| [#35654](https://github.com/bitcoin/bitcoin/pull/35654), `txdb: reject undecodable first coin cursor key` | The current tree still called `GetKey()` after `Seek(DB_COIN)` without checking its result. The PR's reviewer discussion questioned whether local database corruption justified the change, then ACKed the fail-closed behavior after the consumer impact was traced. | **Confirmed and fixed below.** |
| [#35772](https://github.com/bitcoin/bitcoin/pull/35772), early RPC allowlist rejection | Closed as a partial/duplicate direction for [#35592](https://github.com/bitcoin/bitcoin/pull/35592); the proposed rework did not establish a distinct current failure. | Dismissed; retain #35592 as a future queue item. |
| [#35092](https://github.com/bitcoin/bitcoin/pull/35092), skipped descriptor address materialization | Maintainer review explained that materializing the addresses is intentional because it updates the address book and prevents lower indexes from being interpreted as change. | Dismissed as a contract violation, not a stale critical fix. |
| [#31682](https://github.com/bitcoin/bitcoin/pull/31682), specialized IBD block checks | The optimization had no independently demonstrated correctness defect; review accepted the concept but rejected the risk/benefit for consensus-adjacent code. | Dismissed; no current failing proof. |
| [#35755](https://github.com/bitcoin/bitcoin/pull/35755), automated `strlcpy`/`snprintf` fix in `bench_ecmult.c` | The flagged calls use fixed format strings and benchmark-only local buffers; no attacker-controlled input or current truncation failure was shown. | Dismissed as a pattern-only alert. |
| libsecp [#1838](https://github.com/bitcoin-core/secp256k1/pull/1838), uninitialized field element | Superseded by merged [#1839](https://github.com/bitcoin-core/secp256k1/pull/1839), whose range-check and `VERIFY_CHECK` changes are present in the subtree. | Dismissed as already resolved. |
| libsecp [#1850](https://github.com/bitcoin-core/secp256k1/pull/1850), clear invalid Schnorr pubnonce output | Maintainer review found the proposed output-clearing rule unsupported by the API contract and the random independent output not practically dangerous. | Dismissed as an obsolete contract proposal. |
| libsecp [#1797](https://github.com/bitcoin-core/secp256k1/pull/1797), shell-injection hardening in CI | Review found no meaningful attacker model because the relevant workflow context already permits arbitrary code execution; the added complexity was not justified. | Dismissed by established project threat model. |

The ledger distinguishes a stale patch with a live defect from a proposal that was correctly rejected. In particular, #35092, #1850, and #1797 were not resurrected merely because their old diffs were easy to apply. The old PRs were used as discovery seeds; current callers, history, and tests supplied the verdict.

### Confirmed finding: the first database cursor key must decode successfully

`CCoinsViewDB::Cursor()` seeks the `DB_COIN` prefix and primes `keyTmp` before returning the cursor. Before this cycle, it checked only `pcursor->Valid()` and ignored the boolean result of `pcursor->GetKey(entry)`. `CCoinsViewDBCursor::Valid()` is defined by `keyTmp.first == DB_COIN`, so a key that enters the prefix range but cannot decode as a complete `CoinEntry` could make the cursor appear valid. `GetKey()` could then expose the default cached outpoint, while consumers such as coinstats, UTXO snapshot creation, and RPC chainstate traversal treated the cursor as a real coin.

This is an independently reproduced local-corruption boundary, not a remotely reachable database-write path. The malformed fixture writes the one-byte key `C`, which is inside the coin prefix range but lacks a serialized `COutPoint`. The pre-fix focused test failed with exit status 201: both `!cursor->Valid()` and `!cursor->GetKey(outpoint)` were false. This directly proves the old initialization path exposed an undecodable record. The already-symmetric `CCoinsViewDBCursor::Next()` path checks both iterator validity and `GetKey()`, providing an independent implementation contract for the fix.

The minimal change makes initial cursor priming use the same fail-closed rule as `Next()`: set the cached type byte to zero if either the underlying iterator is invalid or key decoding fails. The regression test remains in `coins_tests`, uses only a scratch database under the fixture data directory, and checks both the cursor validity and output-key contract. The test was first compiled with an incorrect `BOOST_REQUIRE` around `CDBWrapper::Write()`; that harness mistake was corrected before the product test was evaluated and was not a product failure.

### Verification record

- Pre-fix focused command: `/data/my_storage/tmp/cycle93-build/bin/test_bitcoin --run_test=coins_tests/coins_db_rejects_undecodable_first_cursor_key --catch_system_error=no --log_level=all`; exit `201`, `Running 1 test case`, and the two expected checks failed.
- Source/test rebuild: `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle93-build --target test_bitcoin -j2`; passed, four build actions.
- Post-fix focused command: `/data/my_storage/tmp/cycle93-build/bin/test_bitcoin --run_test=coins_tests/coins_db_rejects_undecodable_first_cursor_key --catch_system_error=no --log_level=message`; passed one case with `*** No errors detected`.
- `coins_tests`: passed 37 cases.
- Related `dbwrapper_tests,txindex_tests,validation_chainstate_tests`: passed 20 cases.
- Full `/data/my_storage/tmp/cycle93-build/bin/test_bitcoin --catch_system_error=no --log_level=message`: passed all 1,209 cases with `*** No errors detected`.
- `git diff --check`: passed before commit preparation.

The fix is limited to `src/txdb.cpp` and its focused regression in `src/test/coins_tests.cpp`. It does not claim to repair arbitrary LevelDB corruption or change consensus behavior; it ensures a cursor fails closed when the first key cannot satisfy the existing decode contract. The GitHub API was rate-limited after the relevant PR metadata, files, and comments had been captured; no additional remote evidence was needed for this finding.

### Review recipe extracted

For stale critical-fix work, first establish whether the old PR is duplicate, superseded, rejected by contract, or still independently applicable. Then identify a current invariant from a symmetric path or consuming callers, create the smallest deterministic reproducer against current HEAD, and require failing-before/passing-after evidence. For local corruption or recovery findings, state the fault boundary explicitly and prefer fail-closed behavior only when it matches the existing consumer contract. Preserve the old review objection in the journal and do not broaden the patch to resolve unrelated stale proposals.

### Cycle verdict and next queue

Cycle 94 produced one confirmed, independently reproducible correctness finding. The self-contained source/test/journal commit is `af1995129681c5427e494afe2305ad731bb2689f` (`coins: reject undecodable first cursor key`), authored as `Lőrinc <pap.lorinc@gmail.com>`. No other candidate met the evidence threshold. Remaining queue: #35592 early RPC allowlist enforcement, historical P2P outbound relay-slot accounting (#28538), old libsecp wNAF portability proposals (#1770/#1772), and a fresh closed-PR inventory after the next gate. Recheck the queue only with new current-code or review evidence; do not repeat the dismissed candidates above.

## Cycle 252 results

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `56`
- Slug: `stale-pr-critical-fix`
- Branch: `uber-cycle-252-stale-pr-critical-fix-resurrection-20260801`
- Start HEAD: `666fa84636049bfbad614f2baa8c7ceae0eb1a0b`
- `origin/master`: `67efced1`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `42 1291` (`HEAD...origin/master`)
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- Gate: the branch was created from the recorded close HEAD. Existing untracked artifacts were preserved and excluded. The first test invocation was blocked by the full root filesystem; rerunning with `TMPDIR=/data/my_storage/tmp/...` succeeded in reaching the test and all later work used scratch storage there.

### Candidate ledger

| Candidate | Current evidence | Verdict |
| --- | --- | --- |
| [#35473](https://github.com/bitcoin/bitcoin/pull/35473), 64-bit package weight accumulator | `src/policy/packages.cpp` still initialized `std::accumulate` with literal `0`, so the accumulator type was `int`; a large package could narrow intermediate totals before the package-weight check. | **Confirmed and fixed below.** |
| [#35154](https://github.com/bitcoin/bitcoin/pull/35154), crafted MuSig2 PSBT inputs | A current non-hardened invalid derivation reproduced an `EOF` response and daemon death in a scratch regtest instance. Existing Lőrinc-authored fix `29453e530dc10a8f46797f56ab18111a0b43a58b` is already available on `refs/remotes/l0rinc/detached552`. | Confirmed elsewhere; do not duplicate. |
| [#35155](https://github.com/bitcoin/bitcoin/pull/35155), nonce-less MuSig2 PSBT retry | The current duplicate-session assertion remains, but existing fixes `543cb625e039e47833baabaf3e949792d590441c` and `8f97284e018e09bd2419339941bbb422aae32ecd` already cover the issue. | Confirmed elsewhere; do not duplicate. |
| [#35208](https://github.com/bitcoin/bitcoin/pull/35208), future-MTP header commitments | The current arithmetic still has the reviewed signed-to-unsigned risk, but fixes `3a70822391d3d240ef0b526d609940530bda0316` and `9274f925a7ca64d114e1172957a9f94e1ed1796b` already exist on the Lőrinc remote. | Confirmed elsewhere; do not duplicate. |
| [#32789](https://github.com/bitcoin/bitcoin/pull/32789), compact-block integer overflow | The current loop checks `lastprefilledindex` against `uint16_t::max()` before another increment can approach `int32_t` overflow; history also contains `5b80dfd676`. | Dismissed. |
| [#33061](https://github.com/bitcoin/bitcoin/pull/33061), always check `close()` | Review identified signal-safety, locking, and error-propagation risks in throwing from this path; the underlying concern was addressed through a different direction. | Dismissed by current design/review evidence. |
| [#32782](https://github.com/bitcoin/bitcoin/pull/32782), disable secp256 tests by default | The discussion was about test resource usage and iteration counts, not a proven correctness or security defect. | Dismissed. |
| [#34231](https://github.com/bitcoin/bitcoin/pull/34231), null dereference in sequence locks | The assertion remains, but the production caller contract and invalid `prevHeights` reachability were not independently proven in this cycle. | Inconclusive; retain for a future caller-proof cycle. |

The GitHub API became rate-limited after the relevant PR bodies, files, and review evidence were captured. The #35473 source and test evidence are local and independently reproducible; no unavailable remote metadata is needed for the finding.

### Confirmed finding: package-weight accumulation narrows before validation

`IsWellFormedPackage()` computes package weight with `std::accumulate`. Its `int64_t` lambda did not control the accumulator type because the initial value was the literal `0`; the standard algorithm therefore accumulated as `int` and converted each intermediate result back to `int64_t` only after narrowing. A package whose true combined weight exceeds `INT_MAX` can consequently avoid the intended `MAX_PACKAGE_WEIGHT` rejection and proceed to later package checks, consuming additional validation work and potentially returning a less relevant error.

The regression uses 25 references to one transaction with a 23,000,000-byte input script. The combined weight is greater than `INT32_MAX`. Before the fix, the focused test reached duplicate detection and returned `package-contains-duplicates` instead of the expected `package-too-large`; this is a direct failing-before proof of the wrong validation boundary. The issue is policy-only and does not change consensus validity, but it is reachable through package submission and defeats the intended package-size guard for oversized input.

The minimal fix changes the initial value to `int64_t{0}`, making the accumulator and every intermediate sum 64-bit. The test asserts the large package is rejected with `PCKG_POLICY` and `package-too-large`.

### Verification record

- Pre-fix build: `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin -j2`; passed.
- Pre-fix focused command with scratch `TMPDIR`: `.../cycle243-build/bin/test_bitcoin --run_test=txpackage_tests/package_sanitization_tests --catch_system_error=no --log_level=test_suite`; failed at the new assertion with `package-contains-duplicates != package-too-large`.
- Post-fix rebuild: `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle243-build --target test_bitcoin -j2`; passed.
- Post-fix focused test: the same test case with `--log_level=message`; passed with `*** No errors detected`.
- Full `txpackage_tests`: all 15 cases passed with `*** No errors detected`.
- `git diff --check`: passed.

The source change is one initializer token and the regression is confined to `txpackage_tests`. No broad refactor, timeout, input narrowing, or suppression was used. The first root-filesystem failure was an environment limitation, not a product result; the reproducible commands use the available scratch filesystem.

### Cycle verdict and next queue

Cycle 252 produced one confirmed, independently reproducible policy correctness finding from stale PR #35473. The source/test/journal commit is pending and will be authored as `Lőrinc <pap.lorinc@gmail.com>`. Existing fixes for #35154, #35155, and #35208 are deliberately not duplicated. Remaining queue: #35592 early RPC allowlist enforcement, historical P2P outbound relay-slot accounting (#28538), a caller-proof investigation of #34231, old libsecp wNAF portability proposals (#1770/#1772), and a fresh closed-PR inventory. Re-evaluate after the next cycle gate and do not reopen dismissed candidates without new evidence.
