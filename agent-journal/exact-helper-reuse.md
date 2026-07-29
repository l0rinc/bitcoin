# Exact helper reuse audit

## Cycle 23: transaction disk-position size calculation

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `58`
- Selected slug: `exact-helper-reuse`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD before the cycle: `62714ee2fe1bf7e31c00fc4efa1af06e49a9e456`
- Catalog: 99 goal headings and 99 slug markers; catalog SHA256 `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`.
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- `git diff --check`: passed at the gate and after the fix.
- Source was clean at the gate. Existing agent artifacts and `test/cache/` were preserved.
- No daemon, test, fuzz, sanitizer, or profiling process was running at the gate.

### Scope and prior evidence

This cycle searched for repeated parser, serializer, validator, transaction-size, and index-position logic. The highest-value exact duplicate was the transaction disk-offset walk in `TxIndex::CustomAppend` and `TxoSpenderIndex::BuildSpenderPositions`:

```text
pos.nTxOffset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
```

Both call sites operate on `CTransactionRef` objects and need the total serialized transaction size including witness data. `CTransaction::ComputeTotalSize()` is the existing named method whose implementation is exactly that expression:

```text
unsigned int CTransaction::ComputeTotalSize() const
{
    return ::GetSerializeSize(TX_WITH_WITNESS(*this));
}
```

The duplicate was introduced when the tx output spender index was added in `3d82ec5bdd` and has remained behaviorally identical to the older txindex loop. It is a real maintenance divergence risk: a future transaction-size representation change can update the named primitive while leaving one index's disk offsets on an old formula.

Other repeated size expressions were intentionally left alone. Block-size callers operate on `CBlock`, consensus weight helpers have distinct BIP141 semantics, and the transaction-input helper has a different witness-weight contract. The two index traversals also have different output semantics: `TxIndex` records every transaction while `TxoSpenderIndex` records only non-coinbase inputs. A broad shared block iterator would couple unrelated index policies without removing a correctness defect.

### Candidate ledger

| Candidate | Classification | Verdict |
|---|---|---|
| Two index loops use the exact `CTransaction` total-size formula | Existing helper contract is available and type-compatible; duplicate can drift | Confirmed and fixed |
| Replace all direct `GetSerializeSize(TX_WITH_WITNESS(...))` calls | Block, input, consensus, and RPC callers do not share the `CTransaction::ComputeTotalSize` contract | Dismissed; intentional direct use |
| Extract one shared iterator for both index loops | Traversal outputs differ and the abstraction would add coupling with no proven behavior change | Dismissed; no helper extension justified |

### Verification

Baseline before the source change:

```text
build_unit_clang19/bin/test_bitcoin --run_test=txindex_tests,txospenderindex_tests --catch_system_error=no --log_level=test_suite
Running 6 test cases...
*** No errors detected
```

The two exact duplicate expressions were replaced with `tx->ComputeTotalSize()` in `src/index/txindex.cpp` and `src/index/txospenderindex.cpp`. No new abstraction, include, or test fixture was added.

After the source change:

- `cmake --build build_unit_clang19 --target test_bitcoin -j4`: passed; rebuilt both index translation units and linked `test_bitcoin`.
- `build_unit_clang19/bin/test_bitcoin --run_test=txindex_tests,txospenderindex_tests --catch_system_error=no --log_level=test_suite`: all 6 cases passed with `*** No errors detected`.
- `build_unit_clang19/bin/test_bitcoin --run_test=transaction_tests,txindex_tests,txospenderindex_tests --catch_system_error=no --log_level=message`: all 24 cases passed with `*** No errors detected`.
- `git diff --check`: passed.

The proof is semantic rather than a failing regression: the helper body and the replaced expression are identical on the same `CTransaction` object, while existing txindex and txospender index tests exercise serialized positions, multibyte transaction-count offsets, lookups, and reinitialization. The source change only centralizes the already-established contract.

### Commit and handoff

One independent source commit will contain only the two helper substitutions and this journal. It is authored as `Lőrinc <pap.lorinc@gmail.com>`. No production bug, consensus change, persistence-format change, or new runtime behavior was found. The next cycle must run a fresh gate and draw another distinct goal; do not reopen this exact offset expression unless the transaction-size contract or a new index backend changes.

## Cycle 91: validation and failure-contract helper equivalence

### Selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- Draw: `58`
- Selected slug: `exact-helper-reuse`
- Branch: `uber-cycle-91-exact-helper-reuse-20260729`
- HEAD at gate: `10e39ef493a79dae58c7839b891805710986e169`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; divergence: `2 969`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The tracked worktree passed `git diff --check`; known unrelated untracked artifacts remain preserved and excluded from cycle commits. No relevant Bitcoin Core, test, fuzz, sanitizer, or benchmark process was running at the gate.

This cycle excludes the prior `CTransaction::ComputeTotalSize()` reuse in `TxIndex` and `TxoSpenderIndex`, including commit `8ff6f9dd27`. The new queue searches status-returning validators, serialization/canonicalization, wallet/database error cleanup, networking limits, crypto wrappers, and test/fuzz helper pairs. A candidate must have identical input/output contract and a proof that reuse preserves behavior; constant-time, consensus, ownership, and performance boundaries are explicit rejection criteria.

### Candidate: witness-inclusive block size

The current tree had the same witness-inclusive block serialization-size contract in four production paths:

| Path | Existing expression | Contract |
|---|---|---|
| `BlockManager::UpdateBlockInfo` | `GetSerializeSize(TX_WITH_WITNESS(block))` | Recover block-file metadata from a block already written to disk |
| `BlockManager::WriteBlock` | `GetSerializeSize(TX_WITH_WITNESS(block))` | Reserve and write the exact serialized block payload |
| `blockToJSON` | `GetSerializeSize(TX_WITH_WITNESS(block))` | Report the RPC `size` field |
| `GetBlockWeight` | `GetSerializeSize(TX_WITH_WITNESS(block))` | Supply the total-size term of BIP141 block weight |

The expressions were introduced and maintained independently (`064859bbad6`, `fa39f27a0f8`, `fa2bbc9e4cf`, and the original `3babbcb4878` validation helper). `CTransaction::ComputeTotalSize()` already establishes the project naming and contract for the same witness-inclusive serialization operation. `CBlock` had no equivalent named primitive, leaving future block serialization changes to update several call sites manually.

The following alternatives were rejected:

| Candidate | Verdict |
|---|---|
| Replace the four production expressions with a `CBlock::ComputeTotalSize()` helper | Confirmed; the input object, serialization mode, and unsigned-byte result are identical at every site |
| Replace independent test/fuzz serializer expressions | Rejected; `src/test/block_tests.cpp`, `src/test/fuzz/block.cpp`, and validation tests should retain direct serialization oracles rather than test the helper through itself |
| Replace no-witness size, transaction-input size, compact-block size, or block-weight arithmetic | Rejected; each has a distinct stripped/witness, input, compact-message, or BIP141 contract |
| Add a shared block traversal or cache the result | Rejected; this cycle requires exact helper reuse only and does not justify new ownership, invalidation, or cache state |

### Verification

The helper body is exactly the former production expression:

```cpp
unsigned int CBlock::ComputeTotalSize() const
{
    return ::GetSerializeSize(TX_WITH_WITNESS(*this));
}
```

The focused unit test compares the helper against an independent direct serialization calculation. It uses a non-null block and therefore also exercises the existing transaction-reference assertion contract. The existing fuzz and validation tests retain their direct serializer calculations as independent oracles.

Validation on the modified tree:

- `CCACHE_DISABLE=1 cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4`: passed; all changed production and test translation units compiled and `test_bitcoin` linked. The initial cached build attempt was blocked only by a broken `/root/.cache/ccache` symlink; no compiler failure was hidden.
- `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=block_tests --catch_system_error=no --log_level=test_suite`: 4 cases passed, including `block_total_size_matches_serialization`.
- `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=validation_tests,validation_block_tests --catch_system_error=no --log_level=message`: 16 cases passed. One earlier parallel run exposed the existing timing-sensitive signal-ordering test; that case passed in isolation before the serial suite.
- `/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=blockmanager_tests,blockchain_tests --catch_system_error=no --log_level=message`: 20 cases passed.
- `git diff --check`: passed.

### Verdict and handoff

Confirmed as a maintenance-contract finding and fixed in the working tree. The change does not alter serialized bytes, block weight, RPC values, or file positions; it gives one named implementation point to all four production consumers. The test-only direct expressions remain intentionally duplicated for oracle independence. One source/test/journal commit is pending, authored as `Lőrinc <pap.lorinc@gmail.com>`; after it passes the final gate, the next selector must draw a distinct catalog goal.
