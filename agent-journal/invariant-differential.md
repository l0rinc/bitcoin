# Invariant, Differential, and Metamorphic Audit

## Cycle 12

- Date: 2026-07-27 UTC
- Goal index: 51
- Slug: `invariant-differential`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `673e2ca63f270d83d53b0bd9f0ec793e0e62bcb4`
- Selector command/result: `shuf -i 0-98 -n 1` -> `51`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`

### Scope and hypothesis

The campaign focus was to define state/output invariants, compare alternative implementations, and use metamorphic transformations to find inconsistent behavior. The selected surface was the newly introduced `bitcoin-util getchainparams` JSON output, with the trust boundary at scripts and operators consuming its public chain parameters.

The hypothesis was that the `genesis` field used a raw internal byte representation rather than the conventional public hash representation used by Bitcoin Core RPCs and `uint256` APIs. The existing fixture tests were considered a weak oracle because they were generated from the same representation.

### Discovery and prior-finding search

The source, journals, history, tests, and current open-PR context were searched for this exact `getchainparams` genesis byte-order issue; no prior finding or documented raw-byte contract was found. The relevant introducing history was commit `7298281ba8dfb58e07121c74e64f07861ec21f5c`, which added `getchainparams` after the netmagic utility. PR #35610 review and follow-up discussion requested self-documenting chain constants but did not define a raw-byte hash format or mention byte order. `doc/release-notes-35610.md` likewise describes the utility without specifying a raw representation.

The implementation at `src/bitcoin-util.cpp:168` was:

```cpp
result.pushKV("genesis", HexStr(consensus.hashGenesisBlock));
```

`HexStr` serializes the bytes in their internal order. `src/uint256.h` documents that `GetHex()`, `ToString()`, and `FromHex()` use the public reverse-byte representation, and the daemon's user-facing hash fields consistently call `GetHex()`.

### Independent reproduction before the fix

The fresh utility was built from cycle-12 HEAD in `/data/my_storage/tmp/invariant-differential-cycle12/build` with the existing Clang 19 Debug configuration:

```text
cmake --build /data/my_storage/tmp/invariant-differential-cycle12/build --target bitcoin-util -j2
```

The pre-fix utility returned for regtest:

```text
genesis: 06226e46111a0b59caaf126043eb5bbf28c34f3a5e332a1fc7b2b73cf188910f
```

A scratch regtest daemon, started with an isolated datadir and no network peers, returned from `getblockchaininfo`:

```text
bestblockhash: 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206
```

This is the same genesis block, but the two public interfaces exposed different byte orders. The standard value also matches the chainparams assertion at `src/kernel/chainparams.cpp:609`.

The analogous pre-fix fixture values were raw-byte values for mainnet, testnet, testnet4, signet, and custom signet as well, confirming that the issue was systematic rather than regtest-specific.

### Fix and verification

The smallest fix changes `getchainparams` to call `consensus.hashGenesisBlock.GetHex()` and updates all six chain fixtures to conventional public hashes. A short source comment records why the representation is intentional.

The utility was rebuilt with the same command. The complete focused functional test passed:

```text
BITCOINUTIL=/data/my_storage/tmp/invariant-differential-cycle12/build/bin/bitcoin-util BITCOINTX=/data/my_storage/tmp/invariant-differential-cycle12/build/bin/bitcoin-tx BITCOIND=/data/my_storage/bitcoin/build_func_clang19/bin/bitcoind python3 test/functional/tool_utils.py --configfile /data/my_storage/tmp/invariant-differential-cycle12/build/test/config.ini --tmpdir /data/my_storage/tmp/invariant-differential-cycle12/tool-test-20260727T235500Z --loglevel INFO
Tests successful
```

The final direct differential probe used a new scratch regtest daemon and compared the parsed JSON fields:

```text
utility genesis: 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206
rpc bestblockhash: 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206
genesis representations match
```

Alias metamorphic probes from the same cycle also remained equal: default/main, `-regtest`/`-chain=regtest`/`-regtest=1`, testnet aliases, testnet4, signet, and equivalent custom-signet option forms.

### Verdict

Confirmed and fixed. The defect was a public-output formatting mismatch, not a consensus or chainstate error. It could cause scripts or operators to compare `bitcoin-util` output incorrectly with RPC or standard hash values. The fixture-only test passed before the fix because its expected values encoded the same bug; correcting the fixture oracle makes that test sensitive to the intended contract.

Changed files:

- `src/bitcoin-util.cpp`
- `test/functional/data/util/getchainparams-mainnet.json`
- `test/functional/data/util/getchainparams-regtest.json`
- `test/functional/data/util/getchainparams-signet-custom.json`
- `test/functional/data/util/getchainparams-signet.json`
- `test/functional/data/util/getchainparams-testnet.json`
- `test/functional/data/util/getchainparams-testnet4.json`
- this journal

`git diff --check` passed. The scratch daemon was stopped after the probe. Remaining limitation: the direct live differential was run on regtest; the six-chain functional fixture run covers the other configured chain outputs. Next work must draw a distinct catalog hypothesis after the uber-goal state is updated.
