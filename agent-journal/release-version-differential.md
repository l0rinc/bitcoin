## Cycle 191: historical mainnet block differential

### Selection and fresh gate

- Exact selector: `shuf -i 0-98 -n 1` -> `67` (`release-version-differential`). This is a distinct queued cell after the prior fixed-vector, coinbase-only reorg/restart, and prune/persistence cells; the remaining target is historical mainnet block and transaction behavior.
- Selected goal: `release-version-differential` (release-to-release behavioral and consensus differential).
- Branch: `uber-cycle-191-release-version-differential-20260731`.
- Cycle start HEAD: `166cbc30ae92feb85e8022b870428487d318dda0`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1172 42` (`HEAD...origin/master`).
- Fresh gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; known unrelated untracked artifacts were preserved. PIDs `777094` and `956381` were alive and untouched.
- Gate hashes: catalog `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, prompt `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, corrected TSV `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, protocol `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, state at gate `d2025699e746ff459811b7251d4912e13349fac345b39df0cdd32fbad856e8c2`.

### Distinct scope and hypothesis

The prior release-differential cells compared current versus v31.1 over fixed script and transaction vectors, deterministic coinbase-only regtest reorg/restart, and identical regtest prune/recovery fixtures. Those cells are closed. This cycle targets historical mainnet block and transaction validation that can exercise activation-era serialization, script, witness, and consensus boundaries not represented by the prior synthetic regtest corpus.

The trust boundary is the exact historical block/transaction bytes, release-tagged `bitcoind` binaries, current `bitcoind`, consensus validation results, and normalized chainstate observations. The hypothesis is that one supported release/current pair may accept or reject an identical historical object differently, or produce a state transition difference not explained by an intentional consensus activation, policy, or known bug fix. Expected version strings, log wording, performance, and storage layout are not findings.

### Investigation plan

1. Inventory locally available release binaries/tags and historical block or transaction fixtures without downloading unbounded chain data.
2. Select a bounded, provenance-preserving historical corpus spanning pre-SegWit, SegWit, Taproot, and activation-boundary blocks where available; record exact hashes and heights.
3. Replay the same bytes through release and current validation paths in isolated scratch datadirs, normalize only run-specific fields, and independently compare acceptance, reject reasons, chain tips, UTXO summaries, and restart results.
4. Classify every difference from release notes and source ancestry before considering a fix. A finding requires a minimal reproducible historical object or a rigorous unexplained compatibility proof.

### Corpus, replay, and controls

The available bounded corpus was `test/functional/data/mainnet_alt.json`. Its README identifies it as an alternate mainnet-parameter chain with 2,016 coinbase-only blocks, deterministic timestamps/nonces, and a deliberately low first retarget. The JSON fixture SHA-256 is `b85576c28ac5b0e5f26a3d4c089fc6693a921530df734f2cd134a4e557dc0681`. This is a provenance-preserving mainnet-style fixture, not a claim to have replayed live historical mainnet data.

The compared daemons were v28.2 (`/data/my_storage/tmp/cycle132-releases/v28.2/bin/bitcoind`), v31.0 (`/data/my_storage/tmp/cycle109-release-differential/v31.0-build/bin/bitcoind`), v31.1 (`/data/my_storage/tmp/cycle109-release-differential/v31.1-build/bin/bitcoind`), and current (`/data/my_storage/tmp/cycle105-clang19-release/bin/bitcoind`). The current functional harness requires the current `test/config.ini`; its v31.0, v31.1, and current runs used separate scratch datadirs, runtime roots, cache, ports, and port seeds. Each run accepted 2,016 blocks, passed the difficulty-adjustment and historical-target checks, and exited successfully.

The framework's v28.2 attempt stopped before replay because the current test runner passes the unsupported `-nologratelimit` option. To keep the release comparison on identical bytes, the v31.0 functional log supplied the extracted block stream to a direct v28.2 RPC replay using an isolated datadir, no peers, DNS, or listener. v28.2 accepted all 2,016 submissions; its older `submitblock` RPC emits an empty response for accepted blocks, so acceptance was counted from successful RPC exit status and the resulting chain height. The v28.2 direct run and the three current-framework runs therefore used the same raw blocks.

The concatenated raw block bytes from each v31.0, v31.1, and current framework log were byte-identical: 2,016 records, first hash `00000000c4b1855527033cd5147128c91538618801fba6b3d08696b6fc732c26`, height-2015 hash `00000000cf987b41e708b347f70db7f154022fdaf02f72bdeaa9ed5edfd59416`, height-2016 hash `000000000c806553811fd3e6c40bc9c279db53643da7d3b401a9882c074c24b2`, and concatenated-byte SHA-256 `71123c11e8b6fc90be12bd72c7035f63d20f19bde641709553ab95c01a804871`. The extracted hex manifest is `/data/my_storage/tmp/cycle191-alt-blocks.hex`, with SHA-256 `ab698116a465b79ee919bfacbad04b42607a3032c889c489d6d15a310a773156`.

### Results

All four releases reached height 2,016 with the same tip `000000000c806553811fd3e6c40bc9c279db53643da7d3b401a9882c074c24b2`, difficulty `4`, median time `1231247828`, and chainwork `00000000000000000000000000000000000000000000000000000000007e407e407e4`. The v31.0/v31.1/current RPC observations also agreed on bits `1c3fffc0`, target `000000003fffc000000000000000000000000000000000000000000000000000`, and block time `1231248621`. Release-specific omission of newer RPC fields (`headers`, `bits`, `target`, and progress fields in v28.2) was treated as API schema evolution, not validation drift.

Each node was stopped and restarted from its preserved scratch datadir. The post-restart height and tip remained 2,016. The v28.2 sample hashes at heights 0, 1, 2, 2,015, and 2,016 matched the common chain, including genesis `000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f` and the common tip.

The independent chainstate check also matched across all four releases: `gettxoutsetinfo` reported height 2,016, 2,016 transactions and outputs, `bogosize=151200`, `total_amount=100800.00000000`, and serialized UTXO hash `fa152d1792f014fd3537236637d83f032c1f5cfa44b116dd55d937b5efa49296`. `disk_size` was 136,085 on v28.2 and 136,120 on v31.0, v31.1, and current; this is a database-layout representation difference with no state digest difference.

Focused unit controls passed independently: current `test_bitcoin --run_test=pow_tests,blockchain_tests --random=19131` passed 25 cases and 1,098 assertions; v28.2 with `--random=19132` passed 22 cases and 1,057 assertions. The source/history sweep found the current target-contract commit `bb3429a9ca` in the comparison ancestry; it adds an explicit postcondition after valid target derivation and does not alter the normal target used by this fixture. No unexplained acceptance, rejection, chainstate, restart, or UTXO divergence was reproduced.

### Verdict and limitations

Verdict: dismissed for this cycle. The mainnet-parameter fixture is a useful additional release cell, but it contains only coinbase-only synthetic blocks. It does not cover live historical mainnet transactions, wallet/database migration, P2P transcripts, external indexes, or release-branch backports. No production or permanent test change is justified. The next distinct queue remains wallet/database migration, P2P transcripts, release-branch backports, and a real historical transaction/block corpus if bounded provenance-preserving inputs become available.

# Release-to-release behavioral and consensus differential

## Cycle 127: prune and persistence differential (complete)

### Selection and gate

- First selector: `shuf -i 0-98 -n 1` -> `36` (`sanitizer-analysis-matrix`), rejected because its MSan/instrumented-dependency, direct TokenPipe, and analyzer-warning cells were already closed in Cycles 26 and 78 with no new evidence.
- Accepted selector: `shuf -i 0-98 -n 1` -> `67` (`release-version-differential`).
- Branch: `uber-cycle-127-release-version-differential-20260730`.
- Cycle start HEAD: `171cdb9f32e6ca3b144cc1d62563876e4f6b6007`.
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `40 1043` (`origin/master...HEAD`).
- Fresh gate: `git fetch origin master --quiet` passed; tracked/index state was clean; `git diff --check` passed; catalog/protocol/TSV hashes matched; persistent PID `777094` and its parent `725042` were preserved.

### Distinct scope and hypothesis

Cycle 125 covered fixed script/transaction vectors, and Cycle 109 already covered a coinbase-only reorg/restart with unpruned chainstate. This cycle selects the remaining prune/persistence cell: run the same isolated regtest prune workflow on v31.1 and current, explicitly invoke block pruning, restart after pruning, exercise a short post-prune fork/reorg boundary, and compare chain identity, prune height, block availability/error behavior, chainstate queries, and durable files. Do not repeat the completed unpruned coinbase-only matrix or infer a defect from expected block-file layout differences.

The hypothesis is that pruning or restart could leave an inconsistent active tip, stale prune height, unusable retained block/undo boundary, or divergent reorg behavior between the release and current implementation. Any difference must be classified from source/history and reproduced with the same deterministic command sequence before a fix is considered.

### Execution and controls

The v31.1 release daemon is `/data/my_storage/tmp/cycle109-release-differential/v31.1-build/bin/bitcoind` (`v31.1.0`); the current daemon is `/data/my_storage/tmp/cycle105-clang19-release/bin/bitcoind` (`v31.99.0-a51e47bb0c90-dirty`). The persistent wallet unit-test process was not touched. An initial launch using unsupported legacy option `-upnp=0` failed before initialization in both versions; it created no chain data and is retained only as a configuration artifact. The corrected run used isolated datadirs, `-regtest -server -listen=0 -discover=0 -dnsseed=0 -connect=0 -prune=1 -fastprune=1`, separate RPC/P2P ports, and `setmocktime 1700000000`.

The first corrected run mined 800 coinbase-only blocks independently in each version, called `pruneblockchain 400`, invalidated the tip, and restarted. It returned `pruneblockchain=253` and `getblockchaininfo.pruneheight=254` on v31.1, versus `254` and `255` on current. This was an expected fixture difference, not yet a verdict: v31.1's generated blocks had `weight=892`/`size=250` at height 800, while current had `weight=888`/`size=249`.

To remove that confounder, a non-pruned current node generated one common 800-block stream and a two-block alternate fork using the same fixed mock times. The raw block corpus is `/data/my_storage/tmp/cycle127-common-blocks.Go5GHi/blocks.jsonl`, SHA-256 `d3120a34bf109b5adbcb7c642f8b520011d524fbb7f33f7e3dc18117e1f7d584`; the common fork corpus is `fork-blocks.jsonl`, SHA-256 `d0f088603dd99d5536460e45c4cb5c023feb610b668de294aaabe646fa1164c1`. Both v31.1 and current accepted all 800 common blocks with zero failures. Each then pruned at 400, invalidated common height 800, accepted both common fork blocks, and was stopped/restarted.

### Results

- With the identical block bytes, both versions returned `pruneblockchain=254` and reported `pruneheight=255`. Blocks at heights 100, 249, 253, and 254 returned `Block not available (pruned data)`; heights 255, 256, 400, 799, and 800 were readable. The submit-result manifests are byte-for-byte identical (`c26a2a1441afd18362f83202b33849496a54ccfc4e47a1a911062012ee033fdc`).
- Both versions accepted the identical alternate fork (`submitblock` returned `null` twice), ended at height 801, and reported the old height-800 block as an invalid one-block branch. After restart, both retained the same active/fork tip hashes, `pruneheight=255`, 801 transactions/txouts, total amount `14562.5`, and the same serialized UTXO state. Their version-specific `disk_size`, warning, verification-progress, and log capitalization differences are non-semantic.
- `verifychain 4 0` returned `false` on both pruned nodes, with both logs explicitly stopping verification at height 254 because block data was unavailable and reporting no coin-database inconsistencies for the retained range. This is the documented limitation of full verification on pruned data, not a cross-version failure.
- The current and v31.1 `blockchain_tests` binaries each passed 8 cases and 13 assertions, including the `GetPruneHeight` contract and invalidation case, with exit code 0. The current source's `feature_index_prune.py` and `rpc_getblockfrompeer.py` expected values are already updated from 248 to 249; the v31.1 versions retain 248. That test-vector change matches the observed one-byte block-size effect.

### Classification and verdict

The independent source/history explanation is commit `58eeab790d9825a777f907e3e912a2da78cbc76d` (`mining: only pad with OP_0 at heights <= 16`). It removes the dummy coinbase `OP_0` above height 16, making each newly mined block one byte shorter. Its commit message explicitly records that this shifts block-file wrapping and pruning boundaries, and it updates the pruning and block-fetch functional expectations. The first-run 250-versus-249-byte height-800 blocks and one-block boundary shift are therefore intentional release drift, not a persistence or pruning defect.

Verdict: dismissed for this cycle; no unexplained release-to-release prune, reorg, restart, block-availability, chainstate, or UTXO divergence was found. No production or permanent test change is justified. The earlier fixed-vector, coinbase-only unpruned, and current prune cells are now covered; remaining release-differential cells are historical mainnet blocks, wallet/database migration, P2P transcripts, release-branch backports, and any new version-specific evidence.

### Exact validation and limitations

The focused test commands were:

```text
TMPDIR=/data/my_storage/tmp/cycle127-tests-v311 /data/my_storage/tmp/cycle125-v311-test/bin/test_bitcoin --run_test=blockchain_tests --log_level=test_suite --report_level=short
TMPDIR=/data/my_storage/tmp/cycle127-tests-current /data/my_storage/tmp/cycle105-clang19-release/bin/test_bitcoin --run_test=blockchain_tests --log_level=test_suite --report_level=short
```

Both passed. The controlled daemon run has no wallet, external indexes, historical mainnet blocks, network peers, or wallet/database migration. It does not claim compatibility for other release branches or non-regtest block compositions. No relevant process remains running; scratch raw blocks, datadirs, logs, and manifests remain under `/data/my_storage/tmp/cycle127-common-blocks.Go5GHi/`.

## Cycle 125: v31.1/current consensus-vector differential

### Selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `67` (`release-version-differential`); this was a distinct reopen because Cycle 109's coinbase-only block/reorg/restart cell explicitly queued a fixed transaction/script-vector comparison.
- Branch: `uber-cycle-125-release-version-differential-20260730`.
- Cycle start HEAD: `d8bc102d77fc449f77d76573b6e7cc179ac3a5f2`.
- `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `40 1039` (`origin/master...HEAD`).
- The fresh gate fetched `origin master`, found no tracked or index changes, passed `git diff --check`, matched the catalog/prompt/TSV hashes, and preserved PID `777094` with its Codex parent `725042`.

### Scope and hypothesis

Cycle 109 covered deterministic coinbase-only blocks, a short reorg, restart, and persisted chainstate. Cycle 11 covered the genesis/RPC matrix. This cycle therefore selected the next queued cell: fixed script and transaction-validation vectors between the local `v31.1` tag (`9be056a8a72b624dae9623b2f7bded92c2a21c91`) and current HEAD. The hypothesis was that a shared valid or invalid script/transaction vector would change acceptance, script error, or validation-cache behavior unexpectedly. Release-specific test-count growth and implementation-contract checks were treated as expected differences until a common vector or source-backed behavior contradicted them.

### Builds and commands

- The v31.1 source was the existing `git archive v31.1` scratch tree at `/data/my_storage/tmp/cycle109-release-differential/v31.1-src`. It was configured as a Clang 19 Release, wallet-off, IPC/ZMQ/GUI-off test build in `/data/my_storage/tmp/cycle125-v311-test`; `env CCACHE_DIR=/data/my_storage/tmp/ccache-cycle124 ninja -C /data/my_storage/tmp/cycle125-v311-test test_bitcoin` completed successfully.
- Current HEAD was rebuilt in `/data/my_storage/tmp/cycle105-clang19-release`; `ninja -C /data/my_storage/tmp/cycle105-clang19-release test_bitcoin` completed successfully after the source tree was stable. The persistent wallet test process was not touched.
- Each side ran the same commands with separate existing `TMPDIR` roots and logs under `/data/my_storage/tmp/cycle125-release-differential-clean/`:
  - `--run_test=script_tests`
  - `--run_test=transaction_tests`
  - `--run_test=txvalidation_tests`
  - `--run_test=txvalidationcache_tests`
- v31.1 passed `20/683` script cases with `503623` assertions, `13/683` transaction cases with `23717` assertions, `3/683` transaction-validation cases with `118` assertions, and `2/683` cache cases with `210032` assertions.
- Current passed `27/1129` script cases with `505510` assertions, `18/1129` transaction cases with `23817` assertions, `5/1129` transaction-validation cases with `132` assertions, and `2/1129` cache cases with `210032` assertions. The differing registered-case counts are test-suite growth, not a failed common vector.

### Current-vector cross-check

- `git diff v31.1..HEAD -- src/test/data/script_tests.json` contains 18 added script-vector entries: CHECKSIGVERIFY/CHECKMULTISIGVERIFY failures, CHECKLOCKTIMEVERIFY boundaries, and negative-zero CSV behavior. No transaction, `tx_valid`, or `tx_invalid` vector file changed in this range.
- To evaluate those additions against the old implementation, the current generated `script_tests.json.h` was copied only into the v31.1 scratch build, `script_tests.cpp` was relinked, and the old binary reran with a fresh temp root. It passed `20/683` cases and `505474` assertions. Thus the entire current expanded script vector set passed under the v31.1 evaluator; no vector exposed an unexplained release drift.
- The current source does contain broad post-v31.1 script/validation refactors and contract checks, but the common executable suites plus the old-engine/current-vector cross-check produced no failing-before or cross-version discrepancy that justifies a source change.

### Verdict and limits

Dismissed for this cycle. The selected deterministic consensus/script/transaction cell showed no unexplained acceptance, rejection, error-code, or validation-cache drift. No production or permanent test change was justified.

This does not cover historical mainnet blocks, wallet/database migration, P2P transcripts, prune/reorg persistence, release-branch backports, architecture/compiler matrices, or vectors not represented by the selected suites. The next release-differential cycle should choose one of those cells rather than repeat this vector matrix. Raw logs and scratch build paths are preserved above for recurrence checks.

## Cycle 109: deterministic block/reorg/restart differential

### Selection and gate

- First selector: `shuf -i 0-98 -n 1` -> `84` (`secp-nonce-session`), which repeated the already-closed Cycle 95 campaign without new evidence.
- No-repeat reroll: `shuf -i 0-98 -n 1` -> `67`.
- Slug: `release-version-differential`
- Branch: `uber-cycle-109-release-version-differential-20260729`
- Cycle start HEAD: `cfe20c739a88eb28837c7faf61ee9885956e7be2`
- `origin/master` after the fresh gate: `9611a356035be531d62bfc40879f388d5dc359c4`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD` was `40 1007`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`.
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
- Fresh gate: `git fetch origin master` succeeded; tracked and index state were clean; `git diff --check` passed; all catalog hashes matched; PID `777094` and its Codex parent `725042` were present and preserved.

### Scope and exclusions

Cycle 11 already compared v31.1 with current HEAD over a fresh-regtest RPC/genesis matrix. It intentionally excluded historical block submission, reorg, persistence recovery, wallet migration, network transcripts, and release-pair comparisons. This cycle therefore compares v31.0, v31.1, and current HEAD on deterministic regtest chainstate behavior: the same submitted coinbase-only blocks, valid block outcomes, a short reorg, restart/reload, and persisted chainstate/index observations. RPC fields are used only to observe state transitions, not as the primary interface-drift oracle.

Expected differences must be classified from release notes, BIP/deployment history, and source ancestry before being treated as defects. The run uses isolated scratch datadirs, fixed ports, no peers, no wallet, no DNS, and deterministic block/transaction fixtures. It must preserve raw responses and block bytes, normalize only run-specific paths/times, and distinguish current-tree changes from accidental cross-version divergence.

### Hypotheses

1. A block or transaction accepted by one adjacent release is unexpectedly rejected by another under the same regtest rules, or the resulting chainstate differs after normalization.
2. Connect/disconnect, reorg, restart, or index persistence is asymmetric across versions for an otherwise shared fixture.
3. Any observed divergence is intentional activation/policy/interface drift, fixture construction error, or unsupported migration behavior rather than a consensus defect; no source change is made without a minimal cross-version reproducer and ancestry evidence.

### Required evidence

Record exact tag/commit, compiler and flags, binaries, datadirs, RPC commands, raw JSON/block/transaction fixtures, normalized state digests, chain heights/tips, mempool/index results, and failure classification. A source finding requires a failing-before/passing-after regression or an independently reproducible cross-version contract violation. Do not label undocumented output ordering, version strings, or release metadata as consensus drift without an authoritative contract.

### Execution

- Sources were extracted from the `v31.0` and `v31.1` tags with `git archive`. Both were configured as Clang 19 Release builds with wallet, IPC, ZMQ, tests, benches, and GUI disabled; `bitcoind` and `bitcoin-cli` built successfully. The current comparison binaries were `/data/my_storage/tmp/cycle105-clang19-release/bin/bitcoind` and `bitcoin-cli`, rebuilt from this HEAD after Cycle 108.
- The v31.0 source node used isolated regtest state and `mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn` as the output. It generated a 100-block common prefix. A main datadir then generated blocks 101-110 (branch A); a copy at height 100 generated blocks 101-112 (branch B). Raw block hex and height/hash manifests are under `/data/my_storage/tmp/cycle109-release-differential/run1/blocks/`.
- Fresh v31.0, v31.1, and current datadirs received the identical 110 A blocks followed by the identical 12 B blocks through `submitblock`. All 330 A submissions and all 36 B submissions returned exit status 0. The v31.1 and current nodes were each stopped, restarted, and queried again; the fresh v31.0 replay node was also restarted before its final snapshot.
- After A, all comparison nodes reported height 110 and A tip `786ff236a3c288fe1090f42d7755edaf7a8c0343edd916b94e5a2411033c26dd`. After B, all reported height 112 and B tip `2e80ee1d19f2bafac7d0fc3cb845f801590c50de8bd895d8ec684552a3202dcc`. `getchaintips` retained the same A height-110 `valid-fork` entry after the reorg and restart.
- `getblockhash` for every height 0-112 matched across all three versions. The raw tip block SHA-256 was `86b1f2ffee3155fdc35a5d6b55e6e4c8219ce83d09d6f94f44f45dfb481869f6` for each. Tip headers, `getblockstats`, and `verifychain 4 0` matched except for the classified metric below; all three verification calls returned `true`.
- Normalized `getblockchaininfo`, `getchaintips`, and block-header results were identical after restart. `gettxoutsetinfo` agreed on best block, height, 112 transactions/outputs, `bogosize` 8400, serialized UTXO hash `137bfac0c7c6157307aaed254da898401ec00b42bf1e0d06123a62a24a8fb4da`, and total amount 5600 BTC. The only storage observation was `disk_size` 4141 on v31.0 versus 4148 on v31.1/current, which is a release-specific LevelDB/chainstate layout metric, not a state or consensus difference. The v31.0-v31.1 history includes `78714f6d4f` (`Disable seek compaction`) and related chainstate compaction changes.

### Classified drift

Current `getblockstats` returned `utxo_size_inc=161` and `utxo_size_inc_actual=74`; v31.0 and v31.1 returned 163 and 75 for the identical tip block. This is intentional source-backed drift: current includes `5f36e0ff1e` (`rpc: fix getblockstats UTXO overhead accounting`), merged as `89e7c4274c`, which removes the extra `sizeof(bool)` because height and the coinbase bit are packed into one 32-bit value. The current values therefore reflect the corrected accounting, not a consensus or chainstate discrepancy. The current-only pre-release warning in `getblockchaininfo` is likewise expected for the local `31.99.0` development build.

### Verdict

Dismissed for this cycle. The identical raw block/reorg/restart fixture produced identical chain identity, active/stale tips, serialized UTXO state, headers, and verification results across v31.0, v31.1, and current HEAD. The only observed differences were the documented `getblockstats` accounting correction, LevelDB/chainstate disk-size representation, and development-build warning. No production source change is justified.

### Limitations and next queue

This cycle used coinbase-only regtest blocks and no wallet, transaction spends, P2P transcript, external index, prune mode, or release-branch backport. It did not test mainnet historical blocks or wallet/database migration. The next distinct release-differential cell is a fixed consensus transaction/script vector or a cherry-pick/backport comparison, not another empty-block RPC matrix. Preserve the raw fixtures and normalized snapshots for recurrence checks.

## Cycle 11

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `67`
- Selected goal: `release-version-differential`
- Catalog: `agent-journal/reusable-continuous-agent-goals.md`, SHA-256 `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD: `ad2eb8be3afe4c0c0f1a254555dcda4262dda78b`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD/base relation at the gate: `0` behind, `801` ahead
- Dirty state at the gate: no tracked changes; only prior agent-owned files under `agent-goals/` and `agent-journal/`
- Running-process gate: empty

## Hypothesis and scope

An RPC, block-template, or consensus-facing behavior introduced after the local `v31.1` release tag might drift unexpectedly on identical fresh regtest state. The experiment compared the locally available `v31.1` commit `9be056a8a72b624dae9623b2f7bded92c2a21c91` with the current HEAD. The scope was deliberately bounded to deterministic daemon RPCs and the genesis block rather than treating the large development diff as evidence of a defect.

## Provenance and commands

The v31.1 source was extracted with:

```text
git archive v31.1 | tar -x -C /data/my_storage/tmp/release-version-differential/v31.1-src
```

It was configured and built in scratch with Clang 19:

```text
cmake -S /data/my_storage/tmp/release-version-differential/v31.1-src -B /data/my_storage/tmp/release-version-differential/v31.1-build -GNinja -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 -DCMAKE_BUILD_TYPE=Debug -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_GUI=OFF -DBUILD_BITCOIN_QT=OFF -DBUILD_BITCOIN_UTIL=OFF -DBUILD_BITCOIN_TX=OFF -DBUILD_BITCOIN_CLI=OFF -DBUILD_BITCOIND=ON
cmake --build /data/my_storage/tmp/release-version-differential/v31.1-build --target bitcoind bitcoin-cli -j2
```

The current daemon was `/data/my_storage/bitcoin/build_func_clang19/bin/bitcoind`; the v31.1 daemon was `/data/my_storage/tmp/release-version-differential/v31.1-build/bin/bitcoind`. Each ran with a fresh isolated regtest datadir, loopback RPC credentials, no listeners, no peers, DNS seeding disabled, and separate RPC/P2P ports. The controlled run is `/data/my_storage/tmp/release-version-differential/rpc-controlled-20260727T231851Z/`.

After requiring successful `getblockchaininfo` and `getnetworkinfo` responses from both nodes, the harness sent identical calls and stored sorted JSON responses. The matrix covered `getblockchaininfo`, `getmempoolinfo`, `getmininginfo`, `getrawmempool`, `getnetworkinfo`, `getnettotals`, `getrpcinfo`, `getdeploymentinfo`, `getdifficulty`, `getblockcount`, `getbestblockhash`, `getchaintips`, `getchaintxstats`, invalid `validateaddress`, four `decodescript` inputs, invalid `decoderawtransaction`, `getblockheader` at height zero, and `getblock` at verbosity 2. Run-specific warning, version, timing, path, connection, and active-command fields were removed only for the normalized comparison; raw JSON and daemon logs remain in the scratch directory.

## Results

The deterministic block and parser results matched:

- `getblockhash 0`: `0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206`
- `getblockheader` for that hash: identical
- `getblock` verbosity 2, including the genesis transaction and descriptor output: identical
- `decodescript` for empty, `51`, P2PKH, and witness-v1 scripts: identical
- invalid `validateaddress` and invalid `decoderawtransaction`: identical error/result shapes

The observed release drift was explainable:

1. Current `getdeploymentinfo` omits `taproot`, while v31.1 lists it as an active deployment. Commit `74f71c5054ff32a9ba922eed59fceeaf79ec1ed8` (`Remove Taproot activation height`, 2026-03-10) explicitly drops `DEPLOYMENT_TAPROOT` from `consensus.vDeployments` and the deployment RPC, documents that genesis-enforced rules are not listed, and adds a mining-template test. The independent template probe confirmed both versions still return `rules: ["csv", "!segwit", "taproot"]`.
2. Current `getnetworkinfo` reports protocol version `70017`, the pre-release warning, `tx_send_rate`, and `inv_buckets`; v31.1 reports protocol version `70016` without those fields. Commits `74a47a5207` and `4842903ac122c8683a810a1b5da4cc230580fe90` add and expose the configured transaction-send rate and bucket state. These are intentional current-branch interface additions, not unexplained behavior.
3. Current `getmempoolinfo` omits `fullrbf` by default. The current source emits it only when `IsDeprecatedRPCEnabled("fullrbf")` is enabled, and commit `f89d18c3b1c750150832eceb854d97c436d4c8cc` explicitly deprecates that response key. This is an intentional compatibility/deprecation change.
4. Current `getblockchaininfo` and `getmininginfo` include the expected pre-release warning from the locally built `31.99.0` development binary. `getrpcinfo` contains only run-specific log paths and active-command durations after normalization.

## Verdict

Dismissed for this cycle. No unexplained release-to-release behavioral or consensus divergence was found in the bounded matrix, and no production change is justified. No finding commit was made; this journal is the focused evidence snapshot for the cycle.

## Limitations and next queue

This was a fresh-regtest/RPC comparison, not a full historical mainnet replay, wallet migration, network transcript, or every release-branch pair. It did not exercise `submitblock` with a generated historical block, reorg/persistence recovery, or the RPC deprecation override. Future release-differential work should select one of those distinct cells, preferably a consensus block-validation vector or a release-branch backport, rather than repeating this genesis/RPC matrix. Reopen this goal if a new release, migration, protocol change, or unexplained cross-version fixture divergence appears.
## Cycle 215: v28.4/current wallet migration differential (complete)

### Selection and fresh gate

- Exact selector: `shuf -i 0-98 -n 1` -> `67` (`release-version-differential`). This is a distinct queued cell after the fixed-vector, coinbase-only reorg/restart, prune/persistence, and synthetic mainnet-parameter cells; the selected scope is wallet/database migration.
- Selected goal: `release-version-differential` (release-to-release behavioral and consensus differential).
- Branch: `uber-cycle-215-release-version-differential-20260731`.
- Cycle start HEAD: `d567fd49688e4753b26c9fcd672c329e447c3098`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Fresh gate: `git fetch origin master` passed; tracked worktree and index were clean; `git diff --check` passed; catalog/prompt/TSV/protocol hashes matched; protected processes `777094`, `956381`, `1138182`, and `1157959` were alive and untouched.

### Distinct scope and hypothesis

The prior release-differential cells did not compare a legacy BDB wallet migration. This cycle compares the same unloaded legacy wallet bytes through v28.4 and current v31.99.0 migration paths, including generated key material, labels, an imported raw script, P2SH-related watch paths, backup creation, SQLite conversion, reload, and restart. The trust boundary is the exact BDB fixture, migration result, descriptor records, address ownership, backup bytes/paths, and restart-visible wallet state. The hypothesis is that a release/current pair may differ in an undocumented way that loses wallet material, watch state, labels, backups, or reloadability.

### Fixtures and controls

The legacy fixture was created by `/data/my_storage/tmp/cycle167-release28-wallet/src/bitcoind` (`v28.4.0`) in an isolated regtest datadir with `-deprecatedrpc=create_bdb`, no peers/listener/DNS, and RPC port `28555`. Wallet `legacy` was created with `descriptors=false`; it received a labeled address `mr6FGskS6otRenxAwVrKobhzhxMzyiCFLR`, an internal change address, and a watch-only raw script `51`. After unloading and stopping the source daemon, the BDB directory was copied byte-for-byte to separate v28.4 and current scratch datadirs. The source `wallet.dat` SHA-256 was `5b4950d29e342a9d66a24ea797cf2f4c5a9c31a6323ee9b3204141124cb06dcc`.

The v28.4 migration daemon was `/data/my_storage/tmp/cycle167-release28-wallet/src/bitcoind`; current was built from the cycle HEAD at `/data/my_storage/tmp/cycle214-build/bin/bitcoind` with `ENABLE_WALLET=ON`. Both used isolated regtest roots, no peers, separate RPC ports, and `setmocktime 1785508000`. The migration calls were:

```text
/data/my_storage/tmp/cycle167-release28-wallet/src/bitcoin-cli -datadir=.../v284-copy -regtest -rpcport=28557 migratewallet legacy
/data/my_storage/tmp/cycle214-build/bin/bitcoin-cli -datadir=.../current-migrated -regtest -rpcport=28559 migratewallet legacy '' true
```

Both returned `wallet_name=legacy`, `watchonly_name=legacy_watchonly`, and a successful backup. The backup bytes were identical, with SHA-256 `5b4950d29e342a9d66a24ea797cf2f4c5a9c31a6323ee9b3204141124cb06dcc`. v28.4 returned `/data/my_storage/tmp/cycle215-release-wallet/v284-copy/regtest/wallets/legacy/legacy_1785508000.legacy.bak`; current returned `/data/my_storage/tmp/cycle215-release-wallet/current-migrated/regtest/wallets/legacy_1785508000.legacy.bak`.

For a second independent migration fixture, a donor key from a separate v28.4 wallet was used to build a P2PKH redeem script. The script was imported into the legacy wallet with `p2sh=true` and label `raw_p2sh`; the resulting BDB was copied to v28.4 and current scratch roots. Its v28.4 and current backup bytes were identical, with SHA-256 `098100569c57a5b119ea7ab177c128e14ba9a1009da713d1b854fb167f7b6828`.

### Results

- The primary migrated wallet had the same descriptor set, keypool sizes, label `receive`, derived address metadata, empty transaction/coin state, and regtest best block in both versions. The source public address returned the same `pkh` descriptor and key origin after migration.
- The watch-only wallet retained `raw(51)` in both versions. Current additionally retained `addr(2ND8PB9RrfCaAcjfjP1Y6nAgFd9zWHYX4DN)` for the P2SH wrapping of the raw `51` script. This was reproducible before and after restart; v28.4 reported that address as `ismine=false`, current reported `ismine=true` with the corresponding `addr(...)` descriptor.
- On the P2SH fixture, both versions retained the expected P2SH and redeem-script descriptors for the imported P2PKH script. Current additionally retained the `addr(2ND8PB9RrfCaAcjfjP1Y6nAgFd9zWHYX4DN)` descriptor from the pre-existing raw-script watch set. The valid imported P2SH address remained `ismine=true` with label `raw_p2sh` in both versions.
- Both versions converted the primary and auxiliary wallets to SQLite and returned the same migration wallet names. API schema drift was limited to expected fields: v28.4 returned legacy balance fields and no `flags`, while current returned `flags` and omitted deprecated balance fields in `getwalletinfo`; `listwalletdir` likewise gained current warning arrays.
- After stopping and restarting both daemons, each auto-loaded `legacy_watchonly`; manually loading `legacy` succeeded on both. The descriptor sets, address ownership, labels, best block, and zero balances remained valid. No backup or SQLite file was lost.
- Current unit controls passed independently: `env TMPDIR=/data/my_storage/tmp/cycle215-wallet-tests /data/my_storage/tmp/cycle214-build/bin/test_bitcoin --run_test=wallet_tests,walletdb_tests,walletload_tests --random=21501 --log_level=message --report_level=short --color_output=false` reported 29 cases and 231 assertions passed. An earlier attempt with a nonexistent `TMPDIR` was stopped after fixture temp-directory/assertion failures; it was an environment error, not used as product evidence, and the corrected rerun passed.

### Classification and verdict

The backup-path difference is documented release behavior. Commit `c5d9f75c4b` is included in v28.4 and explicitly moves the backup into the migrated wallet directory for file-to-directory migration; current retains the newer walletdir placement. The extra current P2SH-related descriptor is also source-backed: v28.4 predates `440ea1ab63` (`legacy spkm: use IsMine() to extract watched output scripts`, included from v29.0 onward), which constructs a superset of candidate scripts from imported scripts and filters it through legacy `IsMine()`. Its documented purpose is to preserve output scripts and edge cases that the older inverse logic missed. Current's `addr(2ND8...)` result matches that candidate-set behavior, while v28.4's omission is the known historical limitation. The current functional migration oracle in `test/functional/wallet_migration.py::test_migrate_raw_p2sh` independently requires the valid imported P2SH descriptor and rejects invalid nested forms.

Verdict: dismissed for this cycle. The only release differences were documented backup placement, API schema evolution, and the known v29 migration coverage improvement. No unexplained current defect, wallet-data loss, label loss, backup mismatch, reload failure, or restart inconsistency was reproduced. No production or permanent test change is justified.

### Limitations and next queue

The fixtures were bounded regtest wallets without funded transactions, encrypted passphrases, cross-chain wallet best-block state, external indexes, P2P transcripts, or release-branch cherry-pick comparison. This cycle does not claim complete wallet compatibility. The next distinct release-differential queue remains funded/encrypted wallet migration, P2P transcript behavior, release-branch backports, and a real historical transaction/block corpus when bounded provenance-preserving inputs are available. Scratch datadirs, backups, logs, and raw wallet copies remain under `/data/my_storage/tmp/cycle215-release-wallet/`.

## Cycle 227: v31.1/current P2P transcript differential

Status: complete.

### Selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `67` (`release-version-differential`); no reroll.
- Branch: `uber-cycle-227-release-version-differential-20260731`.
- Cycle start HEAD: `490e9d60fc62daa3885d873473e1903c41bc721b`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Gate relation: `git rev-list --left-right --count HEAD...origin/master` returned `1239 42`.
- Catalog, prompt, TSV, and uber-protocol hashes matched their recorded values. The four protected long-running test processes remained alive. No tracked worktree change was present before the cycle; pre-existing untracked probes/catalogs/artifacts were preserved.

The comparison release is the signed local `v31.1` tag at commit
`9be056a8a72b624dae9623b2f7bded92c2a21c91`. The release binary is
`/data/my_storage/tmp/cycle109-release-differential/v31.1-build/bin/bitcoind`
and the current binary is the incrementally rebuilt
`/data/my_storage/tmp/cycle214-build/bin/bitcoind` from the cycle HEAD. Both
were exercised with the v31.1 functional test framework so the test oracle,
message serialization, and timing constants were held constant.

### Hypothesis and scope

The hypothesis was that a P2P handshake, v1 framing, v2 transport transition,
or transaction-download state transition changed between v31.1 and current in
an undocumented way: a valid peer could be disconnected unexpectedly, an
invalid message could be accepted without the expected accounting/penalty, a
v2 fallback could leave asymmetric state, or an announced transaction could
be requested incorrectly. The trust boundary is the remote byte stream,
handshake state, peer service/type flags, connection lifecycle, message
limits, and transaction-request bookkeeping. The comparison deliberately
excluded current-only features until their source/history classification was
available.

### Common functional transcript results

All commands used separate explicit `--tmpdir` directories under
`/data/my_storage/tmp`, fixed random seeds, and distinct port seeds. Each
listed command exited 0 with `Tests successful` for both v31.1 and current.

1. `p2p_handshake.py`, `--v1transport`, random seed `22701`, port seeds
   `22701` and `22702`. The same 3x service/type matrix passed: services
   `0x00000000`, `0x00000001`, and `0x00000008` disconnected for
   `outbound-full-relay`, `block-relay-only`, and `addr-fetch`; `0x00000009`
   connected for all three; `0x00000408` disconnected when the mocktime tip
   was older than 24 hours and connected inside the 24-hour window. Redundant
   `verack`, feeler completion, and self-connection disconnect behavior also
   matched.
2. `p2p_invalid_messages.py`, `--v1transport`, random seed `22702`, port
   seeds `22703` and `22704`. Both releases passed fragmented-header input,
   duplicate version, wrong magic, bad checksum, oversized payload and invalid
   type handling, all ADDRv2 malformed cases, 50,001-entry `inv`/`getdata`
   limits, 2,001-header limits, invalid-PoW and non-continuous headers, and
   80 maximum-valid-size junk messages while keeping the node responsive.
3. `p2p_v2_transport.py`, random seed `22703`, port seeds `22705` and
   `22706`. Both passed v2-to-v2 synchronization, v1-to-v1 synchronization,
   v2/v1 fallback, session-ID equality and zero-session checks, advertised
   service checks, v1 prefix detection, wrong-network prefix rejection, and
   missing-garbage-terminator disconnect.
4. `p2p_tx_download.py`, `--v1transport`, random seed `22704`, port seeds
   `22707` and `22708`. Both passed the same expiry, disconnect, and notfound
   fallback paths; preferred-peer selection and tiebreaking; TXID/WTXID relay
   delay behavior; large inventory handling with and without relay permission;
   rejection-filter reset after a block; wtxidrelay mismatch filtering; and
   the multi-peer in-flight request cap.

The corrected runs completed without a daemon, fuzz, or profiling process
left behind. The first attempts omitted a parent for `TMPDIR`, so Python fell
back to the full `/tmp` filesystem and both daemons failed before networking
with a disk-space diagnostic. A full current `net_tests` attempt had the same
setup issue and was interrupted; after creating the scratch root it passed 36
test cases and 142,212 assertions. The v31.1 build did not contain a
`test_bitcoin` executable, so the unit-suite count was not compared there;
the old functional framework supplied the common release oracle.

### Intentional current-only drift

Current commit `cfcff2e6a00b088fad1387815cec618d4858003` changes V2 long
message-type validation from accepting bytes through `0x7f` to accepting
through `0x7e`, matching `CMessageHeader::IsMessageTypeValid()` and rejecting
the DEL byte `0x7f`. Its source commit adds
`net_tests/transport_v2_rejects_del_message_type`; the current focused control
passed 1 case and 1,742 assertions. The old v31.1 functional v2 matrix passed
because it does not send DEL as a message type. This is an intentional,
source-backed compatibility hardening change, not an unexplained release
divergence or a defect in the old node.

### Verdict and next queue

Dismissed for this cycle. No unexplained P2P handshake, framing, v2 fallback,
invalid-message, peer-accounting, or transaction-download divergence was
reproduced, so no production or permanent test change is justified. Remaining
Goal 67 cells are funded/encrypted wallet migration, historical mainnet
transaction/block replay, release-branch cherry-pick/backport equivalence,
and new protocol changes with a provenance-preserving common transcript. Do
not repeat these four v31.1 P2P matrices unless a new source change or fixture
provides a distinct hypothesis.
