# Option and API Lifecycle Audit

## Cycle 38 selection and gate

- Selector command: `shuf -i 0-98 -n 1`
- First draw: `48` (`property-oracle-expansion`), excluded because its property-oracle cell was closed in cycle 28.
- Rerun: `shuf -i 0-98 -n 1` -> `43` (`option-api-lifecycle`)
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Cycle gate HEAD: `3005aa4f092d1e8234f104a869fe35c452c8b42d`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence at gate: `origin/master...HEAD = 2 844`
- Tracked and staged state was clean at the gate. Existing untracked catalog, journal, probe, and `test/cache/` artifacts were preserved.
- No relevant daemon, test, build, fuzz, sanitizer, or profiling process was running at the gate.

The previously audited debug-only option cells from cycle 32 were excluded. This cycle selected a distinct public option/API lifecycle: `-txsendrate` and its `getnetworkinfo` observation and help contracts.

## Hypothesis and trust boundary

The `-txsendrate` help and RPC schema descriptions still said the configured rate applied to “inbound peers”, although the current implementation constructs both inbound and outbound global transaction-inventory buckets. A user or automation reading either public description could therefore configure or interpret the rate with the wrong peer scope. The trust boundary is node configuration and RPC/help consumers; the mismatch is public contract metadata, not an untrusted-input memory-safety issue.

## Lifecycle map

1. `src/init.cpp:SetupServerArgs` registers `-txsendrate=<n>` with default `DEFAULT_TX_SEND_RATE` and marks it debug-only.
2. `src/node/peerman_args.cpp:ApplyArgsManOptions` reads the integer and clamps it to `1..1000` before putting it in `PeerManager::Options::tx_send_rate`.
3. `src/net_processing.h` defines the option as the maximum per-second rate for sending transaction inventory to peers.
4. `src/net_processing.cpp:PeerManagerImpl` initializes both `m_inbound_inv_bucket` and `m_outbound_inv_bucket` from the configured rate; outbound uses its documented multiplier.
5. `PeerManagerImpl::GetInfo` exposes the configured value, and `src/rpc/net.cpp:getnetworkinfo` exposes `tx_send_rate` plus inbound/outbound bucket state.
6. The option is startup-only in this path: no reload or persistence lifecycle exists. The relevant observable clients are startup help, RPC help/schema, `getnetworkinfo`, and the transaction-relay functional tests.

The implementation and release evidence agree on peer-wide scope. `test/functional/p2p_tx_relay_rate_limit.py` exercises inbound relay, while `test/functional/p2p_tx_relay_rate_limit_outbound.py` configures the option on the sender and exercises the outbound bucket. `doc/release-notes-34628.md` describes the global backlog and outbound multiplier.

## Historical evidence

- `74a47a5207` (`init: add -txsendrate configuration parameter`) introduced the option and its initial inbound wording.
- `4842903ac1` (`rpc: report -txsendrate and bucket info via getnetworkinfo`) introduced `tx_send_rate`, separate inbound/outbound buckets, and the stale RPC description.
- `5cde66341a` added the initial functional relay-rate coverage.
- `ce7258e1c9` added live outbound transaction-relay bucket coverage without changing production behavior.
- `12b0dc33c4` documented the global/outbound behavior in the release note.

The historical sequence shows that the implementation contract expanded to global peer scope, but the two user-facing descriptions were not updated with it. This is a cross-layer lifecycle leftover, not a reason to change the bucket algorithm.

## Reproduction and verdict

The regression assertion was added to `test/functional/rpc_net.py` and checks that `getnetworkinfo` help contains the peer-wide description and does not contain the old inbound-only wording.

Before control, using the unpatched parent `ebe09a67153fe09ba67b03b0cc49f01fa84e2381` in detached worktree `/data/my_storage/tmp/option-api-lifecycle-cycle38-before-src`, with its separately built daemon at `/data/my_storage/tmp/option-api-lifecycle-cycle38-before-build/bin/bitcoind`:

```text
BITCOIND=/data/my_storage/tmp/option-api-lifecycle-cycle38-before-build/bin/bitcoind python3 build_func_clang19/test/functional/rpc_net.py --configfile=/data/my_storage/bitcoin/build_func_clang19/test/config.ini --v1transport --tmpdir=/data/my_storage/tmp/option-api-lifecycle-cycle38-before-real-v1
```

The test reached `Test getnetworkinfo` and exited `1` at `rpc_net.py:246` because `configured target for maximum number of transactions per second to send to peers` was absent. The old inbound-only description remained. Earlier parallel and typo-based attempts were discarded and are not evidence.

The fix changes only the two descriptions:

- `src/init.cpp`: `sending transactions to (inbound) peers` -> `sending transactions to peers`
- `src/rpc/net.cpp`: `...to send to inbound peers` -> `...to send to peers`

After control, `cmake --build build_func_clang19 --target bitcoind -j2` rebuilt the current daemon successfully. The corrected functional test passed serially for both transport variants:

```text
python3 build_func_clang19/test/functional/rpc_net.py --configfile=/data/my_storage/bitcoin/build_func_clang19/test/config.ini --v1transport --tmpdir=/data/my_storage/tmp/option-api-lifecycle-cycle38-after-rpc-v1-final
python3 build_func_clang19/test/functional/rpc_net.py --configfile=/data/my_storage/bitcoin/build_func_clang19/test/config.ini --v2transport --tmpdir=/data/my_storage/tmp/option-api-lifecycle-cycle38-after-rpc-v2-final
```

Both exited `0` after covering `getnetworkinfo` and the remaining `rpc_net` cases. The outbound behavior control also passed:

```text
python3 build_func_clang19/test/functional/test_runner.py p2p_tx_relay_rate_limit_outbound.py --tmpdirprefix=/data/my_storage/tmp/option-api-lifecycle-cycle38-after-outbound
```

The current daemon's startup help and a standalone scratch RPC probe both exposed the peer-wide wording. `python3 -m py_compile test/functional/rpc_net.py` and `git diff --check` passed. No relevant process remains running.

## Verdict and handoff

Verdict: **confirmed and fixed**. The old help/RPC metadata narrowed a peer-wide option's documented scope. The patch preserves all runtime behavior and adds a behavioral help regression without introducing a reload or persistence contract.

The range clamp (`1..1000`) remains unchanged; no independent evidence required changing its debug-option help wording. No direct GUI or settings-reload path is applicable because this option is registered and consumed during startup. The next cycle must exclude this `-txsendrate` metadata cell and choose another distinct option/API lifecycle surface.
