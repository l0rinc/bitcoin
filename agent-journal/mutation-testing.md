# Mutation-testing campaign

## Cycle 107: kernel wrapper bridge and recent behavioral oracles

Status: in progress.

- Exact goal selector: `shuf -i 0-98 -n 1` returned `35`.
- Catalog row: `35 mutation-testing Mutation-testing campaign`.
- Branch: `uber-cycle-107-mutation-testing-20260729`.
- Cycle start HEAD: `8bf20a40f0b6f52c92856d845af544727d2fb040`.
- `origin/master` at start: `9611a356035be531d62bfc40879f388d5dc359c4`.
- Merge-base at start: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence (`HEAD...origin/master`): `40 1003`.
- Catalog SHA-256 gates:
  - `agent-goals/REUSABLE_AGENT_GOALS.md`: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
  - `agent-goals/RANDOM_GOAL_PROMPT.md`: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
  - `agent-goals/goals.tsv`: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Initial tracked/index state was clean and `git diff --check` passed. Existing untracked catalogs, journals, probes, `node_modules/`, package files, and `test/cache/` remain outside this campaign.
- PID `777094` (`test_bitcoin --run_test=wallet_tests`) is an unrelated persistent process and was preserved.

### Scope and exclusions

The mutation engine binaries `mull` and `mull-runner` are unavailable. This cycle therefore uses manually applied, immediately reverted source mutations with exact build/test commands and captures the killed/surviving result. LLVM coverage tools and Clang 19 are available for supporting evidence. The campaign is limited to mutations with an observable production-facing oracle.

Prior mutation/oracle journals already cover consensus checks, generic weak oracles, script/property expansion, TokenPipe EOF handling, libsecp constant-time boundaries and MuSig state, wallet rollback, REST resource bounds, database constructor cleanup, floating-point canonical-NaN handling, and other cells. Those surfaces are excluded from this cycle. The current candidate is the recent `PrecomputedTransactionData` wrapper bridge in `src/kernel/bitcoinkernel_wrapper.h`, together with its existing Taproot verification tests.

### Hypotheses

1. Dropping the spent-output pointer vector would be detected by the one-input Taproot verification test and the two-input Taproot verification test.
2. Passing only the first spent output for a two-input Taproot transaction would be detected by the input-index-one verification oracle.
3. Reverting the bridge to the old pointer reinterpretation would be behaviorally indistinguishable in the current test suite; if so, record it as an ABI/aliasing oracle gap rather than manufacture a runtime finding.

### Planned evidence

- Clean Clang 19 kernel-library/test build in `/data/my_storage/tmp/cycle107-kernel-clang19`.
- Baseline `test_kernel` result, including the focused wrapper and script-verification cases.
- For each mutation: exact source hunk, build result, test result, first failing assertion or surviving status, and restoration check.
- A finding commit only if a mutation exposes a real missing behavioral oracle or the baseline source itself has a confirmed defect. Otherwise commit a journal-only handoff snapshot with the mutation score, surviving mutants, limitations, and next distinct cell.

### Cycle 107 results

Build configuration:

```text
cmake -S . -B /data/my_storage/tmp/cycle107-kernel-clang19 -G Ninja \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_KERNEL_LIB=ON -DBUILD_KERNEL_TEST=ON \
  -DENABLE_IPC=OFF -DENABLE_WALLET=OFF -DBUILD_GUI=OFF \
  -DBUILD_TESTS=ON -DBUILD_BENCH=OFF -DWITH_CCACHE=OFF
```

The clean baseline built `test_kernel` successfully. The focused command
`/data/my_storage/tmp/cycle107-kernel-clang19/bin/test_kernel --run_test=btck_precomputed_txdata,btck_script_verify_tests --log_level=test_suite` passed both cases. The restored source then passed the complete `test_kernel --log_level=test_suite`: 19 test cases, including chain manager, mainnet, in-memory, regtest, transaction-check, and wrapper verification tests, with `*** No errors detected`.

Mutation matrix for `src/kernel/bitcoinkernel_wrapper.h`, lines 723-739 in the restored source:

| ID | Mutation | Result | Evidence |
| --- | --- | --- | --- |
| M1 | Replace the explicit pointer vector with `btck_precomputed_transaction_data_create(tx_to.get(), nullptr, 0)` | Killed | Focused script verification failed six assertions across the legacy/SegWit/Taproot checks; exit 201. |
| M2 | Keep the vector but always pass length `1` | Killed | The kernel C API assertion `spent_outputs_len == tx.vin.size()` aborted the focused script verification test; exit 201. |
| M3 | Restore the historical `reinterpret_cast<const btck_TransactionOutput**>(const_cast<TransactionOutput*>(spent_outputs.data()))` bridge | Survived | The focused suite passed with no errors on x86_64 Clang 19. This mutation changes the aliasing/ABI contract but not the observed values on this platform. It is not a new runtime finding; the prior bridge fix and its ABI evidence are already recorded elsewhere. |
| M4 | Push `spent_outputs.front().get()` for every element instead of `spent_output.get()` | Killed | The two-input Taproot verification case failed at `test_kernel.cpp:226`; exit 201. The compiler also emitted the expected unused-variable warning for the deliberately mutated loop variable. |

The focused score was 3/4 killed (75%). M3 is an explicit survivor and limits the conclusion: this test suite proves pointer cardinality and per-input value behavior, but it cannot detect the historical type-punning representation on this architecture. No production or test change is justified by that survivor in this cycle because the explicit bridge is already the correct implementation and the missing distinction is undefined-behavior/ABI evidence, not an observable contract failure.

All mutations were reverted immediately after their runs. The final full build/test passed, and `src/kernel/bitcoinkernel_wrapper.h` has no cycle mutation remaining. No source finding was confirmed; the cycle will close with a journal-only commit and the next selector will choose a new goal.
