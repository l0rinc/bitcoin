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
