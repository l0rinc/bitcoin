# Invariant, Differential, and Metamorphic Audit

## Cycle 315

- Date: 2026-08-03 UTC
- Goal index: 51
- Slug: `invariant-differential`
- Branch: `uber-cycle-315-invariant-differential-20260802`
- Base: `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`
- Cycle-start HEAD: `fdd1720c84961aaf40eb16f3bf460856b0f84f22`
- Selector command/result: `shuf -i 0-114 -n 1` -> `51`
- Catalog SHA256: `51e561f5a1ea1c1db165416a0ff29b698a9e380279179a3878745517ca7ace7c`

### Scope and prior-finding exclusions

Exclude the previous Goal 51 chain-parameter hash representation fix, the
transaction-request tracker and layered UTXO-cache model campaigns, and the
recent formatter, BIP32, and Taproot descriptor cells. This cycle targets
state/output invariants that cross a real storage or serialization boundary,
using an independent model or transformation rather than implementation-copy
tests.

Initial queue:

1. Database batch/cursor failure schedules: compare logical state, iterator
   visibility, and retry/restart behavior when writes, sync, or cursor reads
   fail at each operation.
2. Transaction, block, undo, and coin serialization round trips: compare
   canonical bytes, hashes, decoded state, and failure-state outputs for
   truncated, noncanonical, and boundary encodings.
3. `CBlockUndo` and chainstate transformations: compare connect/disconnect,
   serialize/deserialize, and replay under empty, duplicate, and maximum-size
   entries.
4. A distinct stateful relation selected from any suspicious code found while
   tracing the first three cells.

Current hypothesis: a storage or serialization helper may report failure
after partially changing caller-visible state, causing a retry or restart to
observe a state that is not equivalent to the successful operation's prefix.

### Evidence ledger

| ID | Surface | Status | Next evidence |
| --- | --- | --- | --- |
| C315-1 | Database batch/cursor failure symmetry | queued | Trace real callers and inject one deterministic failure at a time. |
| C315-2 | Transaction/block/undo serialization metamorphisms | queued | Inventory paired serializers and existing negative tests. |
| C315-3 | Chainstate connect/disconnect and replay invariants | queued | Use scratch fixtures only after the lower-level contracts are known. |
| C315-4 | Oversized compressed-script replacement invariant | confirmed | Commit the minimal clear-before-replace fix and extend the catalog with a focused recurrence campaign. |

### C315-4: oversized compressed scripts depend on destination state

Hypothesis: `ScriptCompression::Unser` may violate the deserialize invariant
that identical bytes produce identical output regardless of the destination's
prior contents. The oversized-script branch at `src/compressor.h` subtracts
`nSpecialScripts`, detects a decoded length above `MAX_SCRIPT_SIZE`, and the
comment says it will "replace" the script with `OP_RETURN`. Before this cycle,
it appended the opcode instead. `DecompressScript` already clears its output
on its own invalid-key failure paths, but that prior cleanup does not cover
this successful oversized-input branch.

The historical rationale is commit `5d0434d13d` (2016-04-24), which changed
an unbounded resize/read into the bounded `s.ignore(nSize)` path to prevent an
OOM from an invalid UTXO script length. The replacement wording and the
bounded-read behavior support treating the destination as an output, not an
accumulator.

The deterministic fixture encodes amount 1, then
`MAX_SCRIPT_SIZE + nSpecialScripts + 1` (10007) as the compressed script size,
followed by exactly 10001 ignored bytes. On the unchanged binary,
`compress_script_oversize_deserialize_replaces_output` failed when decoding
into `CTxOut{1, CScript{} << OP_TRUE}`: the result retained `OP_TRUE` before
`OP_RETURN`. The same bytes decoded into a fresh output produced only
`OP_RETURN`. The failing run is recorded in
`/data/my_storage/tmp/cycle315-compress-before.log` with status 201.

The production path is reachable through `Coin::Unserialize` in
`src/coins.h:83-90` and `TxInUndoFormatter::Unser` in `src/undo.h:36-49`.
More importantly, `CDBIterator::GetValue` first constructs a decoded value
from the caller's existing `Coin` (`src/dbwrapper.h:208-217`) and
`CCoinsViewDBCursor::GetValue` passes its caller's object through that path
(`src/txdb.cpp:283-288`). `WriteUTXOSnapshot` reuses one `Coin` across its
cursor loop (`src/rpc/blockchain.cpp:3420-3449`), so malformed persisted UTXO
entries can make one decoded result depend on the preceding result. Common
point lookups start from a fresh local `Coin`, which limits the normal impact,
but it does not make the formatter's public output contract state-dependent.

Fix: clear `script` immediately before appending the replacement `OP_RETURN`.
The regression decodes the identical byte stream into both a pre-populated and
a fresh `CTxOut`, then compares both scripts with the expected one-opcode
script. With the fix, the targeted case and all 8 cases in `compress_tests`
passed in `/data/my_storage/tmp/cycle315-after-test_bitcoin`; the after run is
recorded in `/data/my_storage/tmp/cycle315-compress-after.log`.

Independent checks: the unchanged binary failed the pre-populated-output
oracle, while the fixed binary passed both destination states; the full
compression suite retained all existing amount, special-script, and invalid
pubkey checks. The change is limited to malformed oversized serialized input;
it does not alter serialization of valid scripts or consensus validation.

Verdict: confirmed local parser/output-state correctness defect. Fix is
self-contained in `src/compressor.h` with a focused regression test; no broad
caller changes or suppressions are justified.

## Cycle 231

- Date: 2026-07-31 UTC
- Goal index: 51
- Slug: `invariant-differential`
- Branch: `uber-cycle-231-invariant-differential-20260731`
- Base: `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- HEAD at cycle start: `9d22f97873279e7cdd33362e91f5090352c907ba`
- Selector command/result: `shuf -i 0-98 -n 1` -> `51`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Prompt SHA256: `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`
- TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`

### Scope and prior-finding exclusions

The prior Goal 51 cell fixed the `bitcoin-util getchainparams` genesis-hash byte-order mismatch and remains closed. This cycle deliberately excluded that output contract and the already-covered transaction-download/orphanage request and duplicate-hash cells. The fresh scope was state and output equivalence in the lower-level transaction-request tracker and UTXO view/cache layers, with independent executable models rather than another fixture-only oracle.

The initial tracked-source gate was clean. A first transaction-request invocation failed only because two commands raced while creating the scratch `TMPDIR`; that setup result was discarded. Re-running with a pre-created isolated directory passed the full selected unit suite.

### Hypothesis A: transaction-request metamorphic state divergence

`txrequest_tests` passed 5 cases with 294,741 assertions; 1,243 assertions were skipped by the suite. The current Clang 19 Debug ASan/UBSan/libFuzzer binary was rebuilt from this checkout with:

```text
CCACHE_DIR=/data/my_storage/tmp/cycle231-ccache TMPDIR=/data/my_storage/tmp/cycle231-build-tmp ninja -C /data/my_storage/tmp/cycle131-build-libfuzzer fuzz -j2
```

All 109 build tasks completed. The fixed-seed smoke run for `FUZZ=txrequest` completed 40 executions with no diagnostic. The 30-second mutation run used seed `23152`, `-max_len=2048`, `-timeout=10`, and `-rss_limit_mb=3000`; it completed 1,454 executions at about 46 executions/sec, grew the corpus from 3 to 484 files, reached 7,860 coverage counters and 43,110 feature units, and peaked at 2,151 MiB RSS. No assertion, sanitizer report, timeout, crash, or artifact occurred.

The request tracker hypothesis was dismissed for the exercised domain. The unit model and fuzz state transitions covered duplicate announcements, timeout/expiry, peer removal, and request cleanup without an observable output or state mismatch. No source or permanent test change was justified.

### Hypothesis B: layered UTXO cache metamorphic state divergence

The independent `FUZZ=coinscache_sim` campaign compared a four-level real cache/overlay stack with a simulated cache and bottom database. It exercised read purity, failed-write no-ops, spent/unspent transitions, uncache idempotence, overlay creation/removal, flush/sync/reset, best-block inheritance, cache statistics, and prevout-fetch worker boundaries. The fixed-seed smoke run completed 7 executions. The 30-second run used seed `231511`, `-max_len=4096`, `-timeout=20`, and `-rss_limit_mb=3000`; it completed 1,287 executions at about 41 executions/sec, grew the corpus from 3 to 427 files (83 KiB), reached 14,463 coverage counters and 67,992 feature units, and peaked at 2,126 MiB RSS. No diagnostic or artifact occurred.

Because this simulator already has explicit relations for the obvious cache operations, a second independent persistence/backend campaign used `FUZZ=coins_view_db_resize_cursor` and `FUZZ=coins_view_stacked`. The resize-cursor smoke run completed 425 executions. Its 30-second run used seed `231513` and completed 4,398 executions at about 141 executions/sec, expanded its output corpus to 209 files (33 KiB), reached 19,579 coverage counters and 49,732 feature units, and peaked at 2,101 MiB RSS. It repeatedly resized a memory-backed LevelDB cache while a cursor iterated persisted coins, then compared the result with the expected map; no cursor, persistence, or resize mismatch occurred.

The stacked-view run used seed `231514` and completed 4,517 executions at about 145 executions/sec with 402 new corpus units and a 532 MiB peak RSS. It exercised a database-backed cache, an overlay with asynchronous input fetching, and the post-overlay backend cache through the shared `TestCoinsView` contract. No assertion, sanitizer report, timeout, crash, or artifact occurred.

### Verdict and limitations

Both hypotheses were dismissed as current defects. The campaigns provide independent model comparisons and broad mutation evidence, but a passing fuzz run is not a proof of exhaustive correctness. The fuzzer was rebuilt after branch checkout so the target binary matched the current source; the build used Clang 19 Debug with address, undefined-behavior, and libFuzzer instrumentation. Docker, AFL++, Honggfuzz, and a full sanitizer-engine matrix were not required for this narrow cell and were not run. The unit suite and fuzz campaigns used isolated scratch directories under `/data/my_storage/tmp`; no default datadir, wallet, key, or production database was used. All four unrelated long-running test processes remained alive.

No source or permanent test change is justified. The next Goal 51 run should select a distinct invariant relation, such as a database batch/cursor failure schedule or a consensus-independent serialization metamorphic pair, rather than repeat these cache and request cells.

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
