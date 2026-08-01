# CI, Coverage-Bot, and Review-Bot Follow-up Audit

## Cycle 244: macOS GUI follow-up verification

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Branch: `uber-cycle-244-ci-bot-followup-20260731`
- Gate HEAD before cycle work: `f11066832929628c9110d243af7266da417ec830`
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at the gate: local `42`, upstream `1271`
- The tracked/index tree was clean. The six protected long-running test processes remained alive. The catalog, prompt, TSV, and uber-goal protocol hashes passed the pre-cycle gate.

### Hypotheses

1. The recent `qa: Enable interface_gui.py on macOS` change might be present in source but omitted, skipped, or misconfigured in the macOS CI matrix.
2. The change might have left an unresolved CI failure or review-bot follow-up after its first macOS failure.
3. CoreCheck might report a relevant coverage loss or benchmark regression requiring a test or implementation follow-up.

### CI wiring and source evidence

The upstream change was inspected at `45f5609f2ed9e35ae2a109c1e8a7c085e0a78006` and its merge `67efced1fc83a0b7215cc1513e7c4754fee0f12f`. The standard macOS matrix entry uses `ci/test/00_setup_env_mac_native.sh`, whose `--preset=dev-mode` configuration leaves GUI enabled, and the common setup defaults `RUN_FUNCTIONAL_TESTS=true`. The fuzz matrix entry intentionally disables functional tests. The common script invokes the build-directory `test/functional/test_runner.py` when that gate is true.

`interface_gui.py` is explicitly present in `BASE_SCRIPTS` in `test/functional/test_runner.py`, not only in the extended list. Its `skip_if_no_gui()` check is satisfied by the standard macOS GUI build; only Windows remains an explicit platform skip. The test framework selects the GUI wrapper (`bitcoin -m gui` when `BITCOIN_CMD="bitcoin -m"`) and, for GUI nodes on Darwin, sets `QT_QPA_PLATFORM=minimal` and defaults `QT_STYLE_OVERRIDE=fusion`. The latter is the relevant fix for the Cocoa/minimal-platform crash.

The earlier PR discussion supplied a concrete failure, not a speculative lead: `fanquake` reported `interface_gui.py` exiting with status `-11` on macOS when built using depends. The follow-up added the Fusion style workaround, and `maflcko` ACKed the resulting commit. This history explains why the change is a CI repair rather than an unresolved omission.

### Live checks and bot evidence

The GitHub check-runs endpoint for merge `67efced1fc83a0b7215cc1513e7c4754fee0f12f` was queried on 2026-07-31:

```text
total 29
28 executed checks: success
1 check: test ancestor commits, skipped
macOS native: completed success
macOS native, fuzz: completed success
```

The macOS native job `90858907736` completed every reported step successfully, including `CI script`. Its unauthenticated log-download endpoint returned HTTP 403, so raw job-log text was not treated as available evidence; the job-step API, source wiring, and test-list inspection were used instead. The commit-status endpoint reported zero status contexts, so check-runs were the applicable GitHub result source.

CoreCheck report 3393 for PR #35838 (`https://corecheck.dev/bitcoin/bitcoin/pulls/35838`) reported `status=success` and `benchmark_status=success` for commit `45f5609f2ed9e35ae2a109c1e8a7c085e0a78006`, with base commit `9611a356035be531d62bfc40879f388d5dc359c4`. It had no coverage data for new code and no actionable coverage annotation. The report’s listed lost/gained baseline files were unrelated to the two-file PR, and no benchmark table entry was present. An earlier report failed because an essential container exited, but the replacement report was successful and the PR merged only after the fix.

The PR API showed the final state as closed/merged with no unresolved review-bot objection. Human review explicitly documented the initial crash, requested the Fusion-style workaround, and then ACKed the fixed commit. The merge check matrix independently passed afterward.

### Verdict

**Dismissed as a current CI, coverage-bot, or review-bot omission; no confirmed finding.** The only concrete failure found was the macOS GUI crash that motivated the follow-up and was fixed before merge. The current matrix builds the GUI and runs the functional suite on standard macOS, the test is in that suite, the Darwin environment fix is present, and the live job/check/CoreCheck evidence is successful. No repository change is justified by this cycle.

### Limitations and next queue

- The full macOS CI job was not run locally because this Linux environment cannot reproduce the hosted macOS runner and its Cocoa/Qt stack.
- GitHub denied unauthenticated raw job-log downloads with HTTP 403, so the exact individual `interface_gui.py` pass line was not extracted from the hosted log. Source-level inclusion plus the successful standard macOS job and the resolved failure/review trail are the available independent evidence.
- CoreCheck reported no line-level coverage data for this two-file change; this is a monitoring limitation, not evidence of a coverage defect.
- Reopen Goal 42 only with a new workflow or test-list change, a failed/non-skipped check, a new review-bot comment, raw coverage evidence tied to the changed path, or a reproducible macOS GUI failure.
- Continue the uber loop by drawing another eligible catalog goal and recording its fresh gate.

## Cycle 143: live bot and coverage follow-up

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Branch: `uber-cycle-143-ci-bot-followup-20260730`
- Gate HEAD before branch creation: `2118a502f1daa581190ecbad17824d03366638b9`
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at the gate: local `40`, upstream `1070`
- The tracked/staged tree and `git diff --check` were clean. The unrelated long-running wallet test process with PID `777094` remained alive and was not touched.

The previous Goal 59 cycle already independently covered the local commit `85083424af` (`ci: pin external action references`). Its action-reference pinning was excluded from this cycle to avoid duplicating that finding.

### Hypotheses

1. A current workflow, matrix entry, or runner gate could silently omit the check named by the job.
2. A live coverage or review bot could have reported a follow-up that the local journal did not contain.
3. A current CoreCheck performance or coverage anomaly could be attributable to the merged LineReader PR and require a follow-up.

### CI and live bot evidence

The current tree was checked with the workflow and runner inventory. The scan found 8 workflow jobs, 20 external action references, 33 shell entrypoints, and 6 CI Python entrypoints. Every external action reference was a full 40-hex SHA. `bash -n` passed for all shell entrypoints, `python3 -m py_compile` passed for all CI Python files, and the workflow parsed with PyYAML. Every workflow-referenced setup environment existed, and no `FILE_ENV` reference pointed at a missing file.

The latest upstream merge commit `9611a356035be531d62bfc40879f388d5dc359c4` was queried through the GitHub API on 2026-07-30:

- Check-run endpoint: `https://api.github.com/repos/bitcoin/bitcoin/commits/9611a356035be531d62bfc40879f388d5dc359c4/check-runs`
- 29 check runs were returned. The 28 executed CI, sanitizer, fuzz, analyzer, cross-build, platform, lint, and release checks were successful.
- The only non-success conclusion was the expected `test ancestor commits` skip, which is conditional on a multi-commit pull request.
- The commit-status endpoint had no separate status contexts (`total=0`), so check-runs are the relevant result source.

The corresponding PR was merged as bitcoin/bitcoin#35828:

- PR: `https://github.com/bitcoin/bitcoin/pull/35828`
- `pinheadmz` approved; `furszy` left an ACK comment; there were no review-code comments.
- The PR's issue comments were bot metadata and ACKs only. No unresolved review-bot objection or failed check was found.

### CoreCheck verification

CoreCheck report 3332 for PR #35828 was successful: `https://corecheck.dev/bitcoin/bitcoin/pulls/35828`. It compared PR commit `dff44e4c8f` with base report commit `b33a7fcd`.

The report listed no coverage data for uncovered new code, listed covered new code in `src/httpserver.cpp`, `src/torcontrol.cpp`, and `src/util/string.cpp`, and listed lost baseline coverage in `src/addrman.cpp`, `src/torcontrol.cpp`, `src/util/sock.cpp`, `src/util/string.h`, `src/validation.cpp`, and `src/wallet/spend.cpp`. Four of those six files were untouched by the PR. The two touched files do not, by themselves, establish a behavior or test omission; the report had no failing coverage gate or actionable annotation.

The benchmark section showed `OrphanageEraseForPeer` at `+22.70% ns/op`, `+21.78% cycles`, and `-18.50% op/s`. This is not attributable to the PR: `git show b33a7fcd:src/bench/txorphanage.cpp` and `git show dff44e4c8f:src/bench/txorphanage.cpp` produced identical hashes, and the PR diff contained no benchmark change. The result is therefore a bot measurement signal to monitor, not a source regression or justified optimization commit.

### Shell/path follow-up

The current path-space hardening remains wired through `BASE_BUILD_DIR` and the runner quotes its filesystem uses. The remaining unquoted `source $FILE_ENV` and GitHub environment-file redirects use the repository's fixed matrix filenames and runner-provided temporary path; no current input allows an attacker-controlled space-bearing `FILE_ENV` or `GITHUB_ENV` path. Treating these lines as a defect would be speculative and would duplicate the existing path-space work.

### Verdict

**Dismissed as a current CI, coverage-bot, or review-bot omission; no confirmed finding.** The live upstream checks were green, the current orchestration inventory was internally consistent, and the only reported CoreCheck benchmark regression was on unchanged benchmark source. No production, test, or CI change is justified by this cycle.

### Limitations and next queue

- The full containerized CI job was not run locally; its script intentionally requires the CI container contract.
- CoreCheck did not provide raw coverage data for independent line-level replay, so the coverage movement was classified as non-actionable rather than proved absent.
- The live evidence covered the latest upstream merge, not a newly submitted local branch. The local action pinning was already handled by Goal 59.
- Reopen Goal 42 only with a new workflow change, failed/non-skipped check, raw coverage artifact showing a relevant regression, review-bot comment, or reproducible job failure.
- Continue the uber loop by drawing another eligible catalog goal and recording its fresh gate.

## Cycle Identity

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `42`
- Selected goal: `ci-bot-followup` (CI, coverage-bot, and review-bot follow-up audit)
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`

## Scope and Hypothesis

The selected goal asks for CI failures, coverage comments, analyzer reports, review-bot output, stale suppressions, and follow-up omissions. This bounded cycle focused on a concrete static hypothesis: a current CI matrix entry may reference a missing setup script, or a runner gate may be silently unconfigured and skip the checks that its job name implies.

The trust boundary is the repository's CI orchestration. A confirmed defect would be a dangling workflow reference, a syntax-invalid entrypoint, or a matrix job whose configured gate is not consumed by `ci/test/03_test_script.sh`. Review comments and external check-run state were not treated as available evidence because this cycle had no online status snapshot.

## Evidence

The workflow references 22 setup environments. The following command checked every distinct `ci/test/00_setup_env_*.sh` path found under `.github` and `ci`:

```text
grep -Rho 'ci/test/00_setup_env_[A-Za-z0-9_]*\.sh' .github ci | sort -u | while read -r f; do test -f "$f" && printf 'OK %s\n' "$f" || printf 'MISSING %s\n' "$f"; done
```

All 22 paths were present: ARM, macOS native and cross, Windows, i686, the native ASan/MSan/TSan/fuzz/tidy/IWYU/previous-release/no-wallet jobs, Alpine, the BSD cross jobs, and bare-metal RISC-V. No `MISSING` line was produced.

The entrypoints passed syntax checks:

```text
bash -n ci/test/03_test_script.sh ci/test_run_all.sh
python3 -m py_compile .github/ci-test-each-commit-exec.py .github/ci-windows.py .github/ci-windows-cross.py
```

Result: `syntax=ok` and exit status 0.

The common setup file defaults `RUN_UNIT_TESTS=true`, `RUN_FUNCTIONAL_TESTS=true`, `RUN_TIDY=false`, `RUN_FUZZ_TESTS=false`, and `GOAL=install`. Specialized jobs explicitly override the relevant gates: fuzz jobs disable unit and functional tests and enable fuzzing; IWYU and tidy jobs disable unrelated tests and enable their analyzer; cross and deploy jobs disable runtime tests; and the bare-metal RISC-V job selects its three supported build targets and invokes `link-riscv.sh`. The runner consumes these gates in the expected build, unit-test, functional-test, tidy, IWYU, dependency-check, Valgrind, fuzz-input, and bare-metal branches.

Recent history contains the CI path-space follow-up (`a31c30290`, merged as part of the current history), and the current runner still carries the intentional dummy default-datadir guard and the path-with-space environment. No stale reference or accidental omission was found in this matrix slice.

## Review and History Search

The local history search covered recent commits mentioning CI, coverage, review bots, and follow-up. It found current CI maintenance merges and the path-space hardening follow-up, but no local unresolved bot finding that contradicted the present matrix. GitHub check runs, coverage-bot comments, and review-bot discussions were not fetched in this cycle, so their absence here is not evidence that no remote follow-up exists.

## Verdict

**Dismissed as a current CI matrix or entrypoint omission; no confirmed finding.** Every workflow-referenced environment script exists, both shell and Python entrypoints parse, and the inspected feature gates map to explicit job settings or common defaults. No production or CI change is justified by this cycle.

## Limitations and Rejected Leads

- The full containerized CI job was not run locally; `ci/test/03_test_script.sh` intentionally refuses host execution without its container contract and can make unsafe global changes.
- No live GitHub check-run, coverage-bot, review-bot, or issue state was available in the local checkout.
- This did not prove analyzer findings, sanitizer output, or coverage quality; it only checked orchestration references and gate wiring.
- The remaining high-value follow-up is to fetch current check-run/review data or inspect a specific failing job, then compare the reported status with the exact matrix environment and logs.

## Next Queue

1. Draw another eligible catalog goal and record the exact command and draw before work.
2. Reopen this goal only with new CI configuration, a live bot/check-run report, a changed runner contract, or a reproduced job failure.
3. Keep the matrix-reference check as a reusable preflight for future CI edits.
