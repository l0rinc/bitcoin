# Integer Option Narrowing and Resource-Boundary Audit

## Seed from Cycle 304

Cycle 304 Goal 52 confirmed and fixed an accepted `int64_t` to `int` narrowing
for `-maxconnections`. The pre-fix value `4294967296` reached a normal daemon,
became zero automatic connections, and was reported as successful. The same
path also had a second boundary where a representable `INT_MAX` connection
count plus reserved descriptors could narrow the `size_t` request to the
`int`-typed `RaiseFileDescriptorLimit` API. The fix rejects values above
`INT_MAX`, caps the descriptor request, and adds a node-init regression test.

That finding exposed a wider class of startup and resource option boundaries.
This goal is a distinct follow-up campaign, not a reason to reopen the repaired
`-maxconnections` cell or the earlier Cycle 10 and Cycle 29 arithmetic fixes.

## Initial scope

Inventory every option and API in startup, node, wallet, RPC, GUI, bindings,
and indexes where an `int64_t`, `uint64_t`, `size_t`, byte count, height, or
duration becomes `int`, `int32_t`, `unsigned`, `chrono`, or an OS resource type.
Prioritize the learned cells:

- `-par` and related script-thread counts, including negative values, values
  above `INT_MAX`, and additions to worker or queue counts;
- `-mempoolexpiry`, `-maxtipage`, `-rpcservertimeout`, and other duration
  options where negative or huge values change clock or scheduling behavior;
- wallet fee and weight paths that convert unsigned transaction sizes to
  signed `int32_t` values;
- block-filter and index height differences, `+1` operations, and loop bounds;
- any value that is parsed safely but later enters allocation, descriptor,
  timeout, thread, cache, or persistence calculations.

For each candidate, write the source and destination domains, boundary table,
negative-value contract, downstream arithmetic, and OS/API limit before
testing. Use clean startup probes, focused unit tests, implicit-conversion and
UBSan builds, static warnings, and faithful arithmetic models. Verify one
value below, at, and above each boundary on 32-bit-relevant domains. Require a
production-path reproducer or rigorous caller/dataflow proof before changing
behavior, and preserve exact rejected values and logs.

## Learned queue

Search `src/init.cpp`, `src/node`, `wallet`, `src/index`, RPC argument parsers,
and GUI option adapters for unchecked narrowing. Search prior journals and
history before each candidate. Keep option parsing, resource policy, and
consensus/serialization rules separate; do not impose a platform-dependent
limit where a checked cap or explicit rejection is the intended contract.
