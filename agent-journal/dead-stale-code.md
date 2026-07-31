# Goal 29: dead code, stale feature, and TODO archaeology

## Cycle 212 start

- Selected by the exact selector `shuf -i 0-98 -n 1`: goal 29, `dead-stale-code`.
- Branch: `uber-cycle-212-dead-stale-code-20260731`.
- Start HEAD: `c1c3d6d76a7089fa99ce30672224055ef277e12b`.
- Origin/master: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence convention: `1215` local commits, `42` remote-only commits.
- Catalog SHA: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Prompt SHA: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- TSV SHA: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Protocol SHA: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Start state SHA: `48b3085a1037592c1261da2b963864b82b513ad7de108bb24ddc35a8edaa4eec`.
- Tracked/index state was clean. Known unrelated untracked files were preserved.
- Protected long-running test processes were checked alive before work.

## Scope ledger

Inspect production TODO/FIXME markers, conditional or compatibility paths, option registration/use, registered tests, platform stubs, and historical follow-ups. A removal or cleanup is allowed only when call/build/history/test evidence proves the code is no longer reachable or the documented contract is obsolete. Intentional scaffolding, supported compatibility, and harmless platform stubs are journaled as dismissed rather than changed.

## Initial hypotheses and evidence

1. `src/node/txreconciliation.cpp` keeps fields that have no reads. The fields are assigned but the protocol is explicitly incomplete and opt-in (`-txreconciliation`), while the header documents future sketch/reconciliation work. This is intentional scaffolding, not safe dead-code removal.
2. `test/functional/rpc_deprecated.py` is registered but its `run_test()` only logs. History commit `331a5279d2` made it a deliberate placeholder after removing the currently deprecated RPCs; its comment requires future deprecated RPC checks to retain independent functionality coverage. Candidate for cleanup, but the explicit placeholder contract currently weighs against deletion.
3. `src/index/coinstatsindex.cpp` retains the old `indexes/coinstats` path. The code, release notes, and `feature_coinstatsindex_compatibility.py` document downgrade compatibility through v29 and test the warning. Not dead until the compatibility policy changes. An earlier CoinStats audit is recorded in `agent-journal/rejected-finding-resurrection.md`; do not reopen that cell without new evidence.
4. `src/qt/macdockiconhandler.mm` says its activation workaround is for Qt 5.5.1, while the build requires Qt 6.2. Current `ForceActivation()` calls remain live through `guiutil.cpp`, and commit `c8b2aeb226` recently preserved the behavior. The wording is stale, but no behavior defect is established.
5. `AddArg` options with low textual occurrence counts were checked. `-loglevelalways`, `-logratelimit`, `-output-csv`, and `-shrinkdebugfile` each have real consumers; occurrence count alone did not identify a dead option.
6. `GetTime()` and related deprecated timing helpers remain used by production and benchmark code. No removal is justified from the deprecation marker alone.

## Commands and outputs

- `git status --short`: only the known unrelated untracked artifacts were present.
- `git rev-parse HEAD`: `c1c3d6d76a7089fa99ce30672224055ef277e12b`.
- `git grep -n -E 'run_test\\(self\\)|def run_test|TODO|FIXME|placeholder|not implemented' -- test/functional src/qt src/index src/node/txreconciliation.cpp src/node/txreconciliation.h`: found the registered deprecated-RPC placeholder, the TxReconciliation TODO fields, the CoinStats compatibility TODO, and the Qt activation TODO among the reviewed candidates.
- `git fetch origin master`: succeeded; origin/master remained `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.

## Verification queue

- Enumerate functional-test registrations and identify `run_test()` implementations with no behavioral oracle; compare each against history and neighboring coverage.
- Search conditional compilation and generated/build manifests for files that are registered but omitted from every supported configuration.
- Re-check remaining TODOs only when their premise can be falsified with current callers, history, and tests.

## Limitations and handoff

The registered no-op scan found `create_cache.py` (the cache-builder helper), `example_test.py` (the documented example), and `rpc_deprecated.py`. The latter is the only candidate with a production-facing test name, but its history shows that commit `331a5279d2` intentionally converted it to a placeholder after the last deprecated RPC was removed. Its retained comment explicitly instructs contributors not to delete or modify it and requires functionality coverage in other tests when a future deprecation is added. It is therefore a dormant hook, not an accidentally unregistered test.

The registered test list in `test/functional/test_runner.py` contains `rpc_deprecated.py` at line 231. A direct run using the release build and scratch datadir `/data/my_storage/tmp/cycle212-rpc-deprecated-run1` exited 0 and logged only the two expected placeholder messages. The first attempt used a pre-created tmpdir and failed at setup; it was discarded as setup-only and rerun with a fresh path.

The RPC formatter comment in `src/rpc/util.cpp:1262` says object-valued `RPCArg::ToStringObj()` cases are currently unused. Current metadata uses `OBJ_NAMED_PARAMS` and `OBJ_USER_KEYS`, but only as top-level arguments or as array/object elements handled by `Sections::Push`; no current production registration nests these types in the `ToStringObj()` branch. The `rpc_tests` suite passed 23 cases, and the OpenRPC functional test passed with the cached regtest chain at `/data/my_storage/tmp/cycle89-build/test/cache`, covering `getopenrpcinfo`, `rpc.discover`, hidden arguments, named options, and object schemas. This comment remains an explicit invariant about the current metadata shape, not evidence of dead production code.

Recent deletion/reference scanning found no current build or test reference to removed source files. The reviewed TODOs remain supported compatibility (`CoinStats` old path and NetBSD 10 linker behavior), deliberate incomplete protocol scaffolding (`TxReconciliation`), live platform behavior with stale wording (macOS activation), or active work items. No source or test change is justified. Close with a journal-only snapshot; the next cycle must re-check the gate and draw a distinct eligible goal.
