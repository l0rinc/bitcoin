# Fuzz-target gap and harness-realism audit

## Cycle 147

- Selected index: `10`
- Selected goal: `fuzz-target-gaps`
- Selector: `shuf -i 0-98 -n 1` -> `10`
- Branch: `uber-cycle-147-fuzz-target-gaps-20260730`
- Gate HEAD: `7e1b355db86329667fdfa4f5071c346dfbc19938`
- Gate `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Gate merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Gate divergence (`HEAD...origin/master`): `1078 42`
- Gate state: tracked/staged files clean; unrelated untracked agent artifacts were preserved. The pre-existing wallet test process remained alive and was not touched.
- Catalog, protocol, and goals TSV hashes were checked and unchanged.

## Scope and prior-finding search

The existing fuzz-target-gap journal did not exist. I searched the target inventory, CMake registration, `test/fuzz/test_runner.py`, the current coverage-runner change, recent fuzz history, `fuzz-introspector.md`, sanitizer notes, and related private-broadcast journals before choosing a cell. The earlier `process_messages` empty-corpus coverage-script finding and the `coins_view_db_resize_cursor` candidate were excluded as repeats. The selected distinct hypothesis was the production-backed P2P private-broadcast harness and its external corpus coverage.

## Hypothesis and static evidence

`src/test/fuzz/p2p_private_broadcast.cpp` has a target-specific reachability gap when its QA-assets directory is absent. The trust boundary is the fuzz input/corpus that drives `PeerManager`, `PrivateBroadcast`, connection handshakes, and inbound/outbound message processing.

The empty-input path is structurally limited:

- `ConsumeBool()` at line 60 returns false, so the test remains in IBD.
- `ConsumeIntegralInRange(0, 3)` at line 80 returns its lower bound when no bytes remain, so no transaction is seeded and `InitiateTxBroadcastPrivate` at lines 82-85 is skipped.
- `ConsumeBool()` at line 111 makes the private-broadcast peer non-relaying, intentionally exercising only the connected-in-vain disconnect assertion at lines 169-171.
- `ConsumeIntegralInRange(0, 2)` at line 175 creates no extra peers.
- `LIMITED_WHILE` at line 188 does not execute, so none of the guided inbound message, transaction broadcast, GETDATA, PONG, or non-private-peer TX paths at lines 199-237 run.

The production paths missed by this empty execution include `PeerManagerImpl::PushPrivateBroadcastTx` and the nontrivial `PrivateBroadcast::Add`, `PickTxForSend`, `GetTxForNode`, and `NodeConfirmedReception` state transitions. The target is registered in `src/test/fuzz/CMakeLists.txt`, but the latest QA-assets tree does not contain `fuzz_corpora/p2p_private_broadcast`.

## Corpus inventory and dynamic verification

The QA-assets repository was fetched into the disposable worktree `/data/my_storage/qa-assets-sparse`; its inspected `origin/main` is `6ea645d13b09bb5be644f154e50a9aa84f929ecb`. It contains `fuzz_corpora/private_broadcast` but no `fuzz_corpora/p2p_private_broadcast`. The sibling model target corpus was used only as an independent control, not treated as a target-specific corpus.

The existing libFuzzer build `/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz` was run with fixed seeds. The target binary hash was `a94e84a143d6e465004d5292ee752f458b8ebd34e38912a797f194b62ab4b7b1` and the current P2P harness source hash was `8c1d9cecd27c77a83794690daac93ab8c90a8f840fd203758e563d75f5f4d999`.

Empty target corpus, one replay:

```text
FUZZ=p2p_private_broadcast .../bin/fuzz -runs=1 -seed=14711 -print_final_stats=1 <empty-dir>
status=0; executed=2; cov=15344; feature_count=15293; peak_rss=323 MB
```

The full coverage report for that run showed `PushPrivateBroadcastTx` at `0/103` edges, `PrivateBroadcast::Add` at `0/28`, `PickTxForSend` at `0/76`, `GetTxForNode` at `0/22`, `NodeConfirmedReception` at `0/52`, and `GetStale` at `0/55`. All six guided operation lambdas at lines 199-237 were uncovered.

Independent sibling-seed control, using `fuzz_corpora/private_broadcast/0018d62465abdbd362eaf01c9c256527d8baa2e6` (15,337 bytes):

```text
FUZZ=p2p_private_broadcast .../bin/fuzz -runs=1 -seed=14710 -print_final_stats=1 <sibling-seed-dir>
status=0; executed=3; cov=21357; feature_count=21311; peak_rss=329 MB
```

Its coverage report reached `PushPrivateBroadcastTx` with `20/103` edges, `PrivateBroadcast::Add` with `13/28`, `PickTxForSend` with `30/76`, `GetSendStatusByNode`, and `GetTxForNode`. The same seed passed the `private_broadcast` target as a control (`status=0; executed=2; cov=5690; peak_rss=242 MB`). A sanitizer-configured replay of the P2P target passed (`status=0; executed=3; peak_rss=336 MB`) with no ASan, UBSan, or leak diagnostic.

The runner's bounded empty-corpus mode can eventually discover additional paths, but that is not equivalent to deterministic replay. A fresh empty directory with `-max_total_time=2` reached `cov=17726` after 284 executions and peaked at 554 MB; the generated 19-file mutation corpus replay reached `cov=17730` after 25 executions. This confirms that the missing seed is a coverage and resource-efficiency gap, not an assertion that the harness is unreachable.

## Verdict

**Confirmed external fuzz-corpus gap; no local source fix.** The P2P harness intentionally preserves a meaningful zero-transaction/connected-in-vain case, so forcing a transaction or loop iteration into every input would weaken the test's contract. The correct remediation is a target-specific, minimized `p2p_private_broadcast` corpus in `bitcoin-core/qa-assets`, preserving at least one relay-enabled handshake, seeded transaction, peer-message sequence, and confirmation/disconnect path. The locally generated mutation files and the sibling corpus seed are evidence and candidate material only; they were not added to Bitcoin Core or the external repository.

## Limitations and handoff

The latest QA-assets commit was inspected through its Git tree; no external PR was opened. The reused fuzzer build was not rebuilt in this cycle, although the P2P source had no later change in the selected target path after the build lineage. Coverage counters are comparative evidence, not a complete reachability proof. Future work should submit/minimize a dedicated corpus externally, rerun the target under the current sanitized and high-throughput builds, and verify that the corpus remains useful after future private-broadcast protocol changes.
