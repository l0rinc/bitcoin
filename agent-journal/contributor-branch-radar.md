# Contributor branch and work-in-progress radar

## Cycle 273 selection and gate

- Selected goal: `65`, `contributor-branch-radar`.
- Selector: `shuf -i 0-98 -n 1` -> `65`.
- Dedicated branch: `uber-cycle-273-contributor-branch-radar-20260802`.
- HEAD at gate: `5aa1343ee6a752214fbe4448337829718aa3db12`.
- `origin/master` at gate: `556988790a7f961693a8fd93f73725baea66476a`.
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence: `1335` commits ahead and `45` commits behind `origin/master`.
- The gate had no tracked modifications. Existing untracked agent artifacts and scratch files were preserved.
- The catalog, prompt, TSV, and protocol hashes were unchanged from the previous gate:
  - goals `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
  - prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
  - TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
  - protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- The seven protected test processes were alive at the gate and after verification. None was stopped or modified. Root storage was full, so all new scratch state used `/data/my_storage/tmp`.

## Radar scope

The current open-PR list was queried on 2026-08-02 through the GitHub pull-request API. Recent active heads included PRs `35863`, `35860`, `35859`, `35819`, `35704`, `35713`, `34707`, `35861`, `35729`, `35744`, and `35754`. Public heads were fetched only into named remote-tracking namespaces; no upstream branch was rewritten and no contributor work was copied as an oracle.

The most useful current evidence was:

| PR/head | ancestry from current `origin/master` | scope and radar result |
| --- | --- | --- |
| `benthecarman/fix-wallet-kdf-rounds-cap` (#35859) | merge base `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; `3` behind, `2` ahead | Wallet KDF signed-count boundary and unchecked calibration result. Selected as the source of a confirmed local finding. |
| `azuchi/descriptor-parse-error-tests` (#35819) | merge base `e75b76b12c5dcaf1c3b9f02d8739b1f551dcf421`; `47` behind, `1` ahead | Test-only descriptor error vectors. Review mutation and corpus replay evidence covered the added branches; no production defect was found in this cell. |
| `jeanpablojp/test-p2sh-sigop-assert` (#35863) | merge base current `origin/master`; `0` behind, `1` ahead | One assertion used the previous transaction instead of the no-`scriptSig` transaction. This is a valid test correction, but no additional production or cross-file leftover was found. |
| `origin/pr-35860` / `maflcko/2608-fuzz-new-rpc` (#35860) | merge base `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; `3` behind, `1` ahead | RPC fuzz harness changes: recursive JSON arguments and command safety inventory. Source review found no independent production issue during this radar cycle; retain as a Goal 10 follow-up seed. |
| `l0rinc/current-coins-cache-invariants` (#34864) | merge base `2063f02bd5edebe1b5c9635db8850220811ebc90`; `356` behind, `9` ahead | Large historical cache-invariant migration. Too stale to treat as current WIP; inspected for provenance and overlap only. |
| `l0rinc/current-coins-cursor-resize-lifetime` (#35744) | merge base `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; `43` behind, `8` ahead | Historical database cursor lifetime work. Its older base and broad sync changes make it a separate Goal 95/54 queue item. |
| `l0rinc/current-wallet-encryption-write-failures` (#35752) | merge base `afa5e46bbc6dd750bd71920b659162a945abf0ae`; `85` behind, `9` ahead | Earlier wallet write-rollback stack. Prior cycles already closed the overlapping failure-state cells; it was not reopened. |

Review evidence for #35859 was especially actionable. The public review by `151henry151` recorded that `SetKeyFromPassphrase(UINT_MAX)` hangs on the base implementation and that the calibration calls ignore the fallible return. The PR's two public commits (`ec97238f45`, `7b18a0c88c`) were treated as a hypothesis and patch seed, then checked against the current tree with an independent pre-fix test.

## Candidate and evidence ledger

### Confirmed: oversized wallet KDF iteration counts

`CMasterKey::nDeriveIterations` is an unsigned value loaded from wallet state. `CCrypter::SetKeyFromPassphrase()` accepted it as `unsigned int` and passed it implicitly to `BytesToKeySHA512AES(int count)`. The latter uses `for (int i = 0; i != count - 1; i++)`. On the current GCC build, a crafted value above `INT_MAX` narrows to a negative count and the loop does not terminate within the bounded reproduction budget. This is a local-input denial of service for a corrupt or crafted wallet record and also leaves a signed-domain contract undocumented.

Independent pre-fix reproduction, using the narrow regression added in this cycle:

```text
cmake --build /data/my_storage/tmp/cycle246-wallet --target test_bitcoin -j2
TMPDIR=/data/my_storage/tmp/cycle273-pre-fix-tmp timeout --signal=TERM --kill-after=1s 3s /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=wallet_crypto_tests/passphrase_rounds_limit --log_level=message --report_level=short --color_output=false --random=273001
```

The old binary printed `Running 1 test case...` and exited `124` after the three-second isolation budget without returning. This is recorded as the reproduction stop condition, not as a hidden test pass.

### Confirmed source contract gap: calibration ignored a fallible derivation

`EncryptMasterKey()` performs two calibration calls before its final derivation. Before this cycle it ignored both calibration return values. The public KDF-boundary fix makes an oversized count return false instead of entering the signed loop, so continuing calibration after that failure can mutate the count and hide the first invalid operation. The source trace has two callers of the helper (`EncryptWallet` and `ChangeWalletPassphrase`) and both rely on its boolean result. The repair checks each calibration result and returns before using the failed sample.

The exact calibration failure is difficult to force without a clock fault or a test-only crypter hook: normal calibration starts from a valid count and `NodeClock` is process-global. The direct KDF reproduction independently proves the invalid-input return path, while source dataflow proves the ignored-return path. The public review is provenance for the hypothesis, not the verifier.

### Dismissed or deferred branch leads

- The descriptor parser branch added 17 exact error vectors, including the 128/129 taptree boundary. Its reviewers independently mutated messages and replayed descriptor corpora; no missing production guard was established here.
- The P2SH assertion branch corrected only a test's transaction argument. The neighboring implementation and test contract were already correct; no source change was justified.
- The RPC fuzz branch's recursive JSON construction and safe-command inventory were reviewed as harness realism evidence. No production behavior was changed by the branch and no separate defect was reproducible in this cycle. Keep its static/dynamic coverage comparison for the next fuzz-target campaign.
- The older l0rinc branches were fetched to inspect base, divergence, touched contracts, and overlap. Their stale bases and prior-cycle overlap make them historical seeds, not current work to cherry-pick.

## Repair

The source/test change is intentionally one self-contained finding commit:

- `src/wallet/crypter.cpp` rejects zero and values outside the `int` KDF loop domain before conversion, then uses an explicit checked conversion.
- `src/wallet/crypter.h` marks the fallible crypter methods and private derivation helper `[[nodiscard]]`.
- `src/wallet/wallet.cpp` checks both calibration derivation results before using elapsed-time samples.
- The fuzz caller explicitly discards its intentionally ignored result.
- `wallet_crypto_tests/passphrase_rounds_limit` checks zero, just over `INT_MAX`, and the normal default count. Existing encrypt/decrypt helpers now assert their fallible calls rather than silently ignoring failures.

No maximum below `INT_MAX` was invented. `INT_MAX` remains a potentially very expensive but representable count; choosing a policy cap requires a separate compatibility and wallet-migration decision. The floating-point-to-unsigned calibration overflow concern under an artificially tiny positive elapsed time remains a queued follow-up rather than an unproven change in this commit.

## Verification

- GCC 12 RelWithDebInfo build: `cmake --build /data/my_storage/tmp/cycle246-wallet --target test_bitcoin -j2` passed.
- GCC focused boundary test, seed `273002`: `1` case, `3/3` assertions, immediate return.
- GCC `wallet_crypto_tests`, seed `273004`: `4` cases, `10076/10076` assertions.
- GCC `wallet_tests`, seed `273005`: `26` cases, `220/220` assertions.
- Clang 19 UBSan build was configured in `/data/my_storage/tmp/cycle273-clang19-ubsan` with IPC disabled because the installed Cap'n Proto 0.9.2 is incompatible with Clang 19 C++20. After redirecting ccache and temporary files out of the full root filesystem, `test_bitcoin` built successfully.
- Clang 19 UBSan `wallet_crypto_tests`, seed `273006`: `4` cases, `10076/10076` assertions, no sanitizer report.
- Clang 19 UBSan `wallet_tests`, seed `273007`: `26` cases, `220/220` assertions, no sanitizer report.
- `git diff --check` passed, and the wallet source search found no unhandled `CCrypter` result after the `[[nodiscard]]` annotation. The full repository test suite was not run because protected long-running tests were preserved and the root filesystem is full.

## Handoff

The current source/test/journal changes are ready for the per-finding commit, authored as `Lőrinc <pap.lorinc@gmail.com>`. After that commit, record the exact hash in `agent-journal/uber-goal-state.md`, create the separate state close commit, and perform a fresh gate and exact selector for Cycle 274. Do not reroll this draw: Goal 65 was not previously closed at the selected evidence cell, and this cycle produced a confirmed finding.
