# Release-to-release behavioral and consensus differential

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
