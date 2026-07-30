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
