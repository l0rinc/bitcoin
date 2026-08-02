# Integer overflow, narrowing, signedness, and division audit

## Cycle 10

- Selector: `shuf -i 0-98 -n 1`
- Draw: `52`
- Selected goal: `integer-overflow`
- Date: 2026-07-27 UTC
- Repository HEAD at gate: `1dcc2da988ee625fbc5d7d55eb6f894c1103ec52`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Working tree at gate: no tracked changes; only agent-owned `agent-goals/` and
  `agent-journal/` directories were untracked

## Scope and contract

The selected surface was startup parsing and construction of the P2P connection
buffer limits. The public help contract says `-maxsendbuffer=<n>` and
`-maxreceivebuffer=<n>` are measured as `<n>*1000 bytes`
(`src/init.cpp:592-593`). `CConnman::Options` stores both resulting byte
counts in 32-bit `unsigned int` fields (`src/net.h:1109-1110`). The argument
reader returns `int64_t` (`src/common/args.h:330-331`), and the options were
registered with `ALLOW_ANY`, so the startup path had no range contract between
the user value, the multiplication, and the destination fields.

The default values are already byte counts (`5*1000` and `1*1000`) in
`src/net.h:100-101`; the old code intentionally multiplied the user-facing
values by 1000 when constructing `CConnman::Options`. The safe argument domain
is therefore `0 <= n <= floor(UINT_MAX / 1000)`, or `0..4294967` on the tested
platform. Negative values are also invalid because the destination fields are
unsigned.

## Hypothesis

The old assignments at `src/init.cpp:2144-2145` could silently narrow a valid
`int64_t` product into `unsigned int`, changing a user-specified buffer limit.
For `n=4294968`, the product is `4,294,968,000`, which becomes `704` after
32-bit modulo conversion. A negative argument similarly becomes a large
unsigned value. This is a local startup configuration correctness and resource
availability defect, not a consensus, cryptographic, or remotely reachable
defect.

Prior-finding checks found no `maxsendbuffer` or `maxreceivebuffer` entry in
`doc/fuzzing-findings.md`. The existing arithmetic findings and the current
PR #35773 implicit-truncation change concern different code paths. History
shows the fields and arguments were introduced together, but no later commit
added a range check.

## Discovery evidence

The clean-HEAD source inspection showed:

```text
src/init.cpp:2144 connOptions.nSendBufferMaxSize = 1000 * args.GetIntArg("-maxsendbuffer", DEFAULT_MAXSENDBUFFER);
src/init.cpp:2145 connOptions.nReceiveFloodSize = 1000 * args.GetIntArg("-maxreceivebuffer", DEFAULT_MAXRECEIVEBUFFER);
```

The exact Clang 19 sanitizer configuration was created in
`/data/my_storage/tmp/integer-overflow-implicit-build` with
`-DSANITIZERS=implicit-conversion`, Debug mode, wallet/IPC/ZMQ/GUI/tools/tests
disabled, and `clang-19`/`clang++-19`. The first configuration attempt exposed
an unrelated Cap'n Proto 0.9.2 versus Clang 19 C++20 incompatibility; the
reproducible configuration was then rerun with `-DENABLE_IPC=OFF` and built
`bitcoind` in 290/290 steps.

Before the fix, this scratch invocation used isolated state under
`/data/my_storage/tmp/integer-overflow-implicit-run-continue2/`:

```text
UBSAN_OPTIONS='halt_on_error=0:print_stacktrace=1:report_error_type=1' \
  .../bitcoind -regtest -datadir=... -listen=0 -connect=0 -dnsseed=0 \
  -discover=0 -natpmp=0 -maxsendbuffer=4294968 -maxreceivebuffer=4294968 \
  -printtoconsole=1 -daemon=0
```

The node continued startup and the log contained:

```text
src/init.cpp:2144:38: runtime error: implicit conversion from type 'int64_t' ... value 4294968000 ... to type 'unsigned int' changed the value to 704
SUMMARY: UndefinedBehaviorSanitizer: implicit-signed-integer-truncation src/init.cpp:2144:38
src/init.cpp:2145:37: runtime error: implicit conversion from type 'int64_t' ... value 4294968000 ... to type 'unsigned int' changed the value to 704
SUMMARY: UndefinedBehaviorSanitizer: implicit-signed-integer-truncation src/init.cpp:2145:37
init message: Done loading
```

The broad implicit-conversion build also reports unrelated pre-existing
secp256k1 conversion diagnostics. They do not overlap the two `init.cpp`
stacks above and are retained as a limitation rather than suppressed.

## Fix and verification

`AppInitParameterInteraction` now rejects negative values and values above
`floor(UINT_MAX / 1000)` before startup side effects. The multiplication is
performed in `uint64_t` and explicitly narrowed only after validation. The
error identifies the offending option and the maximum accepted user value.
The node-init test exercises both send and receive boundary failures through
the parameter-interaction contract. The test uses one case because the
node-init suite intentionally permits only one process-wide `InitContext`.

Evidence after the fix:

```text
cmake --build build_unit_clang19 --target test_bitcoin -j2
# completed; focused binary relinked
build_unit_clang19/bin/test_bitcoin --run_test=node_init_tests --log_level=test_suite --report_level=short
# 2 test cases passed, 4 assertions passed

cmake --build build_func_clang19 --target bitcoind -j2
# completed; bitcoind relinked
```

Independent normal-build startup checks, each using a fresh scratch datadir,
returned exit status 1 before networking:

```text
-maxsendbuffer=4294968 -maxreceivebuffer=4294968
Error: -maxsendbuffer must be between 0 and 4294967.

-maxsendbuffer=1000 -maxreceivebuffer=4294968
Error: -maxreceivebuffer must be between 0 and 4294967.
```

The repaired implicit-conversion build was rebuilt with:

```text
cmake --build /data/my_storage/tmp/integer-overflow-implicit-build --target bitcoind -j2
```

The same oversized-send startup command exited with status 1 and the new
`-maxsendbuffer` error. Its log contained unrelated pre-existing secp256k1
implicit-conversion reports but no `init.cpp:2144`, `init.cpp:2145`, or new
runtime conversion report. `git diff --check` passed, and the process gate was
empty after the runs.

## Verdict

The hypothesis is **confirmed and fixed**. The old code accepted values whose
documented multiplication did not fit its 32-bit destination, silently
changing the configured P2P buffer limit. The fix establishes an explicit
argument domain and makes the conversion auditable. The patch is limited to
startup validation, safe conversion, and its focused test; no unrelated
arithmetic or networking behavior was changed.

## Limitations and next evidence

- The sanitizer configuration is broad and exposes unrelated existing
  secp256k1 conversion diagnostics; the target-specific before/after evidence
  is the exact `init.cpp` stack and the normal-build startup behavior.
- The tested host has 32-bit `unsigned int`; a cross-architecture compile was
  not required because the destination type is explicitly bounded by the
  portable `numeric_limits<unsigned int>` expression.
- The next arithmetic cycle must select a different unchecked surface rather
  than reopening these two startup arguments.

## Artifacts

- Implicit-conversion build: `/data/my_storage/tmp/integer-overflow-implicit-build/`
- Before-fix log: `/data/my_storage/tmp/integer-overflow-implicit-run-continue2/run.log`
- After-fix logs: `/data/my_storage/tmp/integer-overflow-repaired-run/run.log` and
  `/data/my_storage/tmp/integer-overflow-repaired-receive/run.log`

## Cycle 29

- Selector: `shuf -i 0-98 -n 1`
- Draw: `52` (reopened on a distinct unchecked arithmetic surface)
- Selected goal: `integer-overflow`
- Date: 2026-07-28 UTC
- Repository HEAD at gate: `97cb20252003ac2aff08969368f3302b7824c2af`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Ahead/behind at gate: `2 829`
- Working tree at gate: no tracked or staged changes; only agent-owned journals,
  catalog artifacts, and pre-existing `test/cache/` were untracked

The random draw repeated goal 52, but cycle 10 already exhausted the P2P
buffer-argument cell. This cycle therefore selected distinct cache-size and
mempool-cluster-size cells from the same arithmetic campaign, with the earlier
finding excluded from the hypothesis queue.

### Cache-size conversion

#### Scope and hypothesis

`-maxsigcachesize` is a debug/test option whose input is documented in MiB.
`ApplyArgsManOptions` read it as `int64_t`, multiplied the non-negative value
by the `uint64_t` literal `1_MiB`, divided by two, and assigned the result to
the `size_t` byte fields in `ChainstateManager::Options`. The hypothesis was
that an accepted input at the `uint64_t` product boundary silently wrapped,
rather than being rejected before cache construction.

The exact boundary `17592186044416` (`2^44`) MiB makes the old product equal to
`2^64` and therefore zero in unsigned arithmetic. The old code consequently
configured both caches as zero bytes, which `CuckooCache::setup_bytes` treats as
the minimum two-element cache. `GetIntArg` accepts this value because its
return type is `int64_t`; no parser or option registration limit prevented the
path.

#### Before-fix evidence

A temporary assertion in `validation_chainstatemanager_tests/chainstatemanager_args`
passed on the old source with both cache byte fields equal to zero for
`-maxsigcachesize=17592186044416`. A rebuilt normal daemon then reached
`Done loading` and logged:

```text
Using 0 MiB out of 0 MiB requested for signature cache, able to store 2 elements
Using 0 MiB out of 0 MiB requested for script execution cache, able to store 2 elements
```

The command used only a scratch regtest datadir:

```text
build_func_clang19/bin/bitcoind -regtest -datadir=/data/my_storage/tmp/integer-overflow-maxsigcache-before-cycle29b \
  -listen=0 -server=0 -connect=0 -dnsseed=0 -discover=0 -natpmp=0 -daemon=0 \
  -printtoconsole=1 -maxsigcachesize=17592186044416
```

#### Fix and verification

The conversion now uses `CheckedMul<uint64_t>`, rejects products that do not
fit, rejects a per-cache result that cannot fit `size_t`, and narrows only
after those checks. Negative inputs retain the existing zero-floor behavior.
The chainstate-manager test now covers zero, odd MiB splitting (`33_MiB / 2`),
and the `2^44` overflow rejection.

Evidence after the fix:

```text
cmake --build build_unit_clang19 --target test_bitcoin -j2
build_unit_clang19/bin/test_bitcoin --run_test=validation_chainstatemanager_tests/chainstatemanager_args --report_level=short
# 1 case, 40 assertions passed

cmake --build build_func_clang19 --target bitcoind -j2
build_func_clang19/bin/bitcoind ... -maxsigcachesize=17592186044416
# status 1
# Error: -maxsigcachesize is too large (got 17592186044416 MiB)

build_func_clang19/bin/bitcoind ... -maxsigcachesize=33
# reached Done loading; each cache logged 16 MiB and stored 540672 elements
```

`git diff --check` passed. The hypothesis is **confirmed and fixed**. The
source change is limited to checked arithmetic, representability validation,
and its option-contract test.

### Next distinct cell

The same draw then exposed `-limitclustersize`: its `kB * 1'000` assignment
was unchecked, and the resulting value feeds signed `*40` and `*4` operations
in `CTxMemPool`. A separate journal update and source commit records that
finding after its verification.

### Mempool cluster-size conversion

#### Scope and hypothesis

`-limitclustersize` is documented in kilobytes and is registered as a
debug/test option. The local `ApplyArgsManOptions` helper assigned
`*vkb * 1'000` directly to the signed `cluster_size_vbytes` field. The
resulting field is then multiplied by 40 in `CTxMemPool::Flatten` and by the
witness scale factor in the graph constructor. The hypothesis was that a
negative or saturated positive option could reach those downstream arithmetic
operations and trigger an invalid limit or a failed `Assume`, instead of
producing a parameter error.

History confirms that this option was intentionally left without a fixed
resource cap when the old 16,384 MiB cap was removed in commit `b370164b31`,
because it is DEBUG_ONLY and the project has a terminating `std::new_handler`.
That rationale does not cover arithmetic values that cannot be represented.

#### Before-fix evidence

The first direct daemon probe used a scratch regtest datadir and the saturated
positive value `9223372036854776` kB, which is just above
`INT64_MAX / 1'000`:

```text
build_func_clang19/bin/bitcoind -regtest -datadir=/data/my_storage/tmp/integer-overflow-limitclustersize-before-cycle29 \
  -listen=0 -server=0 -connect=0 -dnsseed=0 -discover=0 -natpmp=0 \
  -daemon=0 -printtoconsole=1 -maxmempool=5 \
  -limitclustersize=9223372036854776
# status 132, SIGILL, empty log
```

A negative `-limitclustersize=-1` was also accepted by the old parser and
stored a negative virtual-byte limit. A second boundary isolates the later
constructor calculation: `230584300921370` kB fits `kB * 1'000`, but exceeds
`floor(INT64_MAX / 40 / 1'000)`. With the largest representable mempool size,
the old daemon again terminated with status 132 and `SIGILL` before startup
completed.

#### Fix and verification

The argument helper now returns `util::Result<void>`, rejects negative values,
and rejects values above the largest kB input that keeps the constructor's
`*40` signed calculation representable. It also retains a `CheckedMul` guard
at the first conversion. The focused mempool test covers a valid `101` kB
value, `-1`, the initial product boundary, and the downstream `*40` boundary.

Evidence after the fix:

```text
cmake --build build_unit_clang19 --target test_bitcoin -j2
build_unit_clang19/bin/test_bitcoin --run_test=mempool_tests/MempoolLimitArgumentBounds --report_level=short
# 1 case, 5 assertions passed

build_unit_clang19/bin/test_bitcoin --run_test=mempool_tests --report_level=short
# 24 cases, 423 assertions passed

cmake --build build_func_clang19 --target bitcoind -j2
build_func_clang19/bin/bitcoind ... -limitclustersize=9223372036854776
# status 1; Error: -limitclustersize is too large (got 9223372036854776 kB)
build_func_clang19/bin/bitcoind ... -limitclustersize=230584300921370
# status 1; Error: -limitclustersize is too large (got 230584300921370 kB)
```

`git diff --check` passed and no daemon or test process remains. The hypothesis
is **confirmed and fixed**. The source change is limited to the argument
contract and its focused test; ordinary cluster-size behavior remains
unchanged.

### Cycle verdict and next queue

Cycle 29 produced two independent confirmed arithmetic findings and no
remaining tracked changes for the selected goal after the second commit. The
next arithmetic queue includes cache-size paths and amount/fee conversions not
covered by the P2P buffer, validation-cache, or cluster-size fixes. The next
uber cycle must draw again from the full catalog rather than reopening these
cells without new evidence.

## Cycle 304

- Selector: `shuf -i 0-103 -n 1`
- Draw: `52`
- Selected goal: `integer-overflow`
- Date: 2026-08-02 UTC
- Repository HEAD at gate: `0f220b92529f28210a09ad965f8c49eff29b9297`
- Branch: `uber-cycle-304-integer-overflow-20260802`
- Base: `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`
- Ahead/behind at gate: `0 1404`
- Working tree at gate: no tracked changes; unrelated agent artifacts were
  untracked

The prior P2P buffer, signature-cache, and cluster-size arithmetic cells were
excluded. This cycle inspected startup options and selected the
`-maxconnections` representability and file-descriptor sizing boundary.

### Scope and hypothesis

`ArgsManager::GetIntArg` returns `int64_t`, but the startup path assigned
`-maxconnections` directly to `int`. The value then participated in the file
descriptor request passed to `RaiseFileDescriptorLimit(int)`. The hypothesis
was that a valid `int64_t` option outside the `int` domain could silently wrap
to a different connection limit, and that a large representable `int` could
make the descriptor request narrow to a negative value.

### Before-fix evidence

The source path was:

```text
int user_max_connection = args.GetIntArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS);
available_fds = RaiseFileDescriptorLimit(user_max_connection + max_private + min_required_fds);
```

On the pre-fix normal binary, a scratch regtest daemon accepted
`-maxconnections=4294967296` and reached `Done loading` while logging:

```text
Using at most 0 automatic connections (1048576 file descriptors available)
```

The matching `getnetworkinfo` response reported `"connections": 0`. The
argument was therefore neither rejected nor treated as a large request; its
conversion to `int` changed it to zero on the tested x86_64 build.

The exact probe used no wallet, public network, default datadir, or production
database:

```text
/data/my_storage/tmp/cycle246-wallet/bin/bitcoind -regtest \
  -datadir=/data/my_storage/tmp/cycle304-maxconnections-before \
  -server=1 -listen=0 -listenonion=0 -connect=0 -dnsseed=0 -discover=0 \
  -natpmp=0 -daemon=1 -daemonwait -printtoconsole=0 -rpcport=18499 \
  -maxconnections=4294967296 -disablewallet=1
```

An independent Clang 19 `implicit-conversion` build also reached startup with
the same value before the fix, but its daemonized stderr did not preserve a
conversion diagnostic. That run is recorded as behavior corroboration, not as
sanitizer proof.

### Fix and verification

`AppInitParameterInteraction` now rejects negative values and values above
`std::numeric_limits<int>::max()` before parameter interactions. The later
narrowing is explicit, and the descriptor request is computed in `size_t` and
capped before calling the `int`-typed `RaiseFileDescriptorLimit`. The help text
now states the accepted `0` through `INT_MAX` range. A focused node-init test
covers `INT_MAX + 1`.

Evidence after the fix:

```text
TMPDIR=/data/my_storage/tmp/cycle304-node-init \
/data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin \
  --run_test=node_init_tests --log_level=test_suite --report_level=short \
  --color_output=false
# 4 cases, 6 assertions passed
# Error: -maxconnections cannot exceed 2147483647.

CCACHE_DIR=/data/my_storage/tmp/cycle304-ccache-implicit-after \
cmake --build /data/my_storage/tmp/cycle304-implicit-build --target bitcoind -j2
# completed [6/6]

/data/my_storage/tmp/cycle304-implicit-build/bin/bitcoind ... \
  -daemon=0 -printtoconsole=1 -maxconnections=4294967296
# status 1
# Error: -maxconnections cannot exceed 2147483647.
# UBSan log contained no init.cpp diagnostic; its records were pre-existing
# libsecp256k1/crypto startup diagnostics from this broad configuration.

/data/my_storage/tmp/cycle304-implicit-build/bin/bitcoind ... \
  -daemon=1 -daemonwait -maxconnections=2147483647
# started successfully; warning reduced to 1048416 connections
# debug.log: Using at most 1048416 automatic connections (1048576 file descriptors available)
# debug.log: init message: Done loading
```

`git diff --check` passed. The hypothesis is **confirmed and fixed**. The
source change and regression test are limited to the startup option contract
and the descriptor-request narrowing boundary. The evidence is Linux x86_64
with `int` width 32 and does not establish behavior on every supported OS or
architecture.

### Learned queue

The next configuration-arithmetic campaign should inspect distinct startup
and resource paths exposed by this audit: `-par`'s `int64_t` to `int` thread
count, duration options such as `-mempoolexpiry` and `-rpcservertimeout`, the
wallet fee path's unsigned transaction-size to `int32_t` conversion, and block
filter index height differences. Keep the repaired `-maxconnections` cell and
the Cycle 10/29 cells out of the queue unless a new evidence source shows a
different reachable defect shape.
