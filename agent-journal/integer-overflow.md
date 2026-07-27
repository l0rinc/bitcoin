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
