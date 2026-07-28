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
