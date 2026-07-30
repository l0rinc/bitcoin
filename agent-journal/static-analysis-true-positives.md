# Static-analysis true-positive campaign

## Cycle 172 start: newly reached core analyzer paths

### Selection and fresh gate

- The exact post-Cycle-171 selector `shuf -i 0-98 -n 1` returned `12`
  (`static-analysis-true-positives`). It was not explicitly closed in the
  authoritative ledger, so no reroll was made.
- Branch: `uber-cycle-172-static-analysis-true-positives-20260730`.
- Start HEAD: `fbb264b16d48875df068f2f2b6bc68d8e009a72c`; origin/master:
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base:
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1125 42`.
- `git fetch origin master`, `git diff --check`, and the cached check passed.
  Known untracked agent/user artifacts remain outside scope. PIDs `777094`
  and `956381` are persistent unrelated unit tests and must not be stopped.
- Catalog, prompt, corrected goals TSV, and protocol hashes are unchanged at
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

### Prior evidence and exclusions

Cycle 26 already ran Clang `scan-build` on the core `bitcoin_crypto` and
`bitcoin_util` targets and GCC 12 `-fanalyzer` on the same 55-step scope. Its
warnings were classified as false positives or documented inline-assembly,
filesystem, standard-library, enum, logging, and template modeling artifacts.
Cycle 78 separately ran the new MSan/direct-TokenPipe analyzer cell and fixed
the EPIPE status contract; that cell is closed. This cycle must not relabel
those reports or repeat the same crypto/util translation units. Clang-tidy,
CodeQL, Semgrep, cppcheck, and IWYU binaries are not installed in this
environment. LLVM 19 `scan-build` is available, while the default PATH Clang
is 14 and GCC is 12.2.

### Scope and hypothesis

Inventory a fresh Clang 19 compilation database for wallet-off, IPC-off core
targets, then analyze newly selected node, validation, chainstate, mempool,
RPC, and persistence translation units with the LLVM analyzer checkers. The
initial hypothesis is that a source/configuration path not covered by the old
crypto/util scan contains a real nullability, lifetime, unchecked-result,
dead-store, or error-path defect that the compiler/runtime tests do not expose.
Prioritize files changed by recent merges and code with untrusted input,
state mutation, persistence, or concurrency boundaries. Use the analyzer only
to route candidates: every warning needs a source/dataflow proof, caller and
history trace, a valid-domain reachability argument, and an independent
runtime, mutation, or focused compile/test control before it can become a
finding.

### Required cycle protocol

1. Build a scratch Clang 19 compilation database under `/data`, recording
   feature flags and exact compiler/tool versions. Select a bounded set of
   new core translation units; exclude vendored secp256k1, LevelDB, minisketch,
   and the already classified crypto/util files.
2. Run direct Clang analyzer/`scan-build` diagnostics with raw output and
   stable checker options. Classify each report as source, test, dependency,
   unsupported instrumentation, or analyzer artifact. Search history and prior
   journals before retaining a candidate.
3. For each retained candidate, trace the first invalid operation and trust
   boundary, reproduce it on clean HEAD, and independently verify it with a
   focused test, a temporary mutation, a concrete caller/input, or a rigorous
   proof. Do not change code for a pattern-only warning.
4. If a defect is confirmed, add the narrowest regression oracle and one
   self-contained source/test/journal commit. Otherwise close with the raw
   report paths, classifications, exact commands, tool limitations, and the
   next untouched analyzer cell. Keep all scratch builds on `/data` because
   the root filesystem is nearly full.

## Cycle 172 evidence: Clang 19 and GCC 12 core scan

### Build and analyzer setup

The scratch compilation database was configured with:

```text
cmake -S . -B /data/my_storage/tmp/cycle172-static-scan1 -G Ninja \
  -DCMAKE_C_COMPILER=/usr/bin/clang-19 \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++-19 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_WALLET=OFF -DENABLE_IPC=OFF -DBUILD_TESTS=ON \
  -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF \
  -DWITH_CCACHE=OFF
ninja -C /data/my_storage/tmp/cycle172-static-scan1 src/node/data/ip_asn.dat.h
```

The generated database contains 426 KiB of Clang 19 C++ commands. The
generated `src/node/data/ip_asn.dat.h` prerequisite was built explicitly after
the first `init.cpp` attempt exposed the normal generated-file dependency.
Each selected command was converted from compilation to direct Clang static
analysis with `--analyze -Xanalyzer -analyzer-output=text -c`, preserving its
include paths, defines, C++20 mode, and debug flags. Raw output is retained
under `/data/my_storage/tmp/cycle172-static-scan1/direct*`.

The scanned core set was:

```text
node/blockstorage.cpp node/chainstate.cpp node/mempool_persist.cpp
net_processing.cpp validation.cpp coins.cpp dbwrapper.cpp flatfile.cpp
rpc/rawtransaction_util.cpp kernel/disconnected_transactions.cpp
node/eviction.cpp node/mempool_args.cpp node/peerman_args.cpp
rpc/blockchain.cpp rpc/mempool.cpp init.cpp net.cpp rpc/server.cpp
rpc/request.cpp rpc/util.cpp index/base.cpp index/txindex.cpp rpc/mining.cpp
node/miner.cpp node/utxo_snapshot.cpp kernel/cs_main.cpp kernel/context.cpp
policy/packages.cpp node/interfaces.cpp policy/rbf.cpp
```

Clang emitted ten warnings in four translation units and no warnings in the
other listed units. As an independent checker, each of `validation.cpp`,
`coins.cpp`, `node/interfaces.cpp`, and `policy/rbf.cpp` was also run through
GCC 12.2 with `-fsyntax-only -fanalyzer` using the same compilation database
defines and include paths. All four GCC runs exited 0 without diagnostics.

### Candidate classification

The six `validation.cpp` warnings are all explicit-contract or ownership
artifacts:

- `validation.h:219` reports a moved-from `m_replaced_transactions` while the
  constructor initializes that member from its moved parameter. No later use
  of the source object is present; the warning is the expected move checker
  limitation.
- `validation.cpp:1703` and `:1713` report null dereferences after
  `m_pool.exists(wtxid)` or `m_pool.exists(txid)`. Both calls hold the mempool
  lock. `exists` and `GetEntry` query the same `mapTx` index, and the second
  branch additionally asserts the txid relation at `:1714`. The analyzer does
  not retain that container-index relation.
- `validation.cpp:4690` reports `tip->phashBlock` after
  `CBlockIndex* tip{Assert(chainstate.m_chain.Tip())}`. `TestBlockValidity`
  requires an active chain tip; the assertion is the release/debug contract,
  not untrusted-input validation.
- `validation.cpp:5060` reports `GetAncestor(nHeight)` in a loop bounded by
  `pindexNew->nHeight`, with the fork height established from the same block
  index chain. The explicit `Assert` expresses the valid-domain invariant.
- `validation.cpp:5967` reports a leak from `snapshot_chainstate`, but
  `AddChainstate(std::unique_ptr<Chainstate>)` immediately moves ownership into
  `m_chainstates` and asserts that the returned reference is its last element
  (`validation.cpp:6417-6426`). This is a false leak path caused by interprocedural
  unique-pointer modeling.

The two `coins.cpp` warnings are also dismissed:

- `support/allocators/pool.h:190` flags the placement construction of a
  `std::byte[m_chunk_size_bytes]` array in exactly allocated aligned storage.
  A minimal standalone reproduction of the same placement-new expression
  reproduced the warning with Clang 19. The probe ran clean under Clang 19
  ASan+UBSan with leak detection, and the existing pool and overlay tests
  passed 16 cases and 23,962 assertions in the normal GCC test binary. The
  warning alone does not establish an object-lifetime or allocation defect,
  so no allocator rewrite is justified.
- `coins.h:750` reports `base->PeekCoin` through a null pointer. The base is
  initialized by `CCoinsViewBacked(CCoinsView* in_view) : base{Assert(in_view)}`;
  production and test `CoinsViewOverlay` constructors all pass a live backend.
  The public `SetBackend` contract also takes a reference. This is another
  analyzer failure to propagate the assertion-backed invariant.

The final two warnings are explicit API/index contracts:

- `node/interfaces.cpp:155` reports `*Assert(m_context)` in `NodeImpl::startShutdown`.
  `MakeNode` accepts `NodeContext&`, and the constructor immediately stores its
  address. The nullable `setContext` hook exists for tests and all repository
  callers provide a live context before invoking node operations. The warning
  identifies a possible misuse of that testing hook, not a reachable null path
  from the node factory. The broader public-pointer API question belongs to a
  separate misuse-resistance campaign and has no independently demonstrated
  production failure here.
- `policy/rbf.cpp:57` reports `*Assert(pool.GetEntry(tx.GetHash()))` after
  `pool.exists(tx.GetHash())`. `IsRBFOptIn` requires `pool.cs`; both methods
  access the same `mapTx` index under that lock. The current code came from
  commit `14804699e597` (`[refactor] remove access to mapTx from policy/rbf.cpp`),
  which deliberately replaced a direct `mapTx.find` with the checked helper.
  GCC independently accepted the path and the focused RBF tests passed.

### Independent runtime controls

The existing normal test binary was run with:

```text
CCACHE_DIR=/data/my_storage/tmp/cycle170-ccache \
  /data/my_storage/tmp/cycle170-mempool-build/bin/test_bitcoin \
  --run_test=rbf_tests,interfaces_tests,validation_chainstate_tests \
  --log_level=test_suite --report_level=short --color_output=false
```

It passed 16 selected cases and all 1,315 assertions. This included the RBF
index path, all six node interface tests, and the selected chainstate
validation cases. The earlier allocator/overlay control passed 16 cases and
23,962 assertions. The minimal placement-new probe used:

```text
/usr/bin/clang++-19 -std=c++20 -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer pool-placement-probe.cpp
ASAN_OPTIONS=abort_on_error=1:halt_on_error=1:detect_leaks=1 \
  UBSAN_OPTIONS=halt_on_error=1 ./pool-placement-probe
```

It produced no sanitizer or leak diagnostic. A temporary mutation removing
the `Assert`-backed `GetEntry`/constructor assumptions was not promoted to a
source change because the mutation would test the analyzer's model rather than
an observable production contract; the independent GCC scans and focused
runtime controls are the stronger controls for this cycle.

### Cycle verdict and handoff

No new source, test, or build defect was confirmed. The ten Clang reports are
classified as six assertion/index/ownership/move artifacts in validation, one
placement-new checker artifact, one constructor-backed base invariant, and
two explicit context/index contracts. No semantic duplicate of the already
closed Cycle 26 crypto/util scan or Cycle 78 TokenPipe cell was reopened.

Limitations: clang-tidy, CodeQL, Semgrep, cppcheck, and IWYU are unavailable;
the scan used direct LLVM analyzer invocations rather than a full project-wide
`scan-build` build. GCC analyzer covered the four warning-bearing files only.
No full unit suite was attempted because the root filesystem remains at 99%
capacity. Scratch logs and the minimal probe are under `/data`; no owned test
or analyzer process remains, and persistent PIDs `777094` and `956381` were
preserved.

The next static-analysis queue is the conditional-compilation/configuration
matrix for node/RPC/persistence units, followed by a fresh tool-version scan
of public nullable hooks if a new checker or caller provides independent
evidence. The cycle remains closed as a dismissed/no-finding campaign cell;
future selection may revisit goal 12 only with a distinct evidence source.
