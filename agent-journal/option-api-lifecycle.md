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

## Cycle 224 - dynamic settings path stability

### Identity and gate

- Exact selector: shuf -i 0-98 -n 1 -> 43 (option-api-lifecycle); no reroll.
  Branch: uber-cycle-224-option-api-lifecycle-20260731. Start HEAD was
  e0d6264be2ef80c188c294ecf5d56bc1ef0de724; origin/master was
  67efced1fc83a0b7215cc1513e7c4754fee0f12f; merge-base was
  a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b; start divergence was 42 1234.
- Catalog, prompt, TSV, and uber-protocol hashes were unchanged. Tracked state
  was clean at the gate apart from the known untracked agent artifacts.
  Protected PIDs 777094, 956381, 1138182, and 1157959 remained alive.
- The prior -txsendrate help/RPC wording cell was excluded. This cycle chose
  the separate read/write settings-file lifecycle and restart-authority cell.

### Hypothesis and lifecycle trace

ArgsManager::ReadSettingsFile() resolves the settings path before loading
rw_settings, but WriteSettingsFile() resolves it again afterward. Because
GetSettingsPath() previously called the ordinary GetPathArg("-settings"), a
persisted "settings": "redirected.json" value could replace the command-line
or configuration-selected path after the file had been read. Startup then
wrote the loaded values, including the path-setting value itself, to a different
file. Subsequent restarts continued reading the original file, so runtime
settings updates written through interfaces::Node::updateRwSetting() could
appear successful while being ignored on the next restart.

The trust boundary is local configuration/settings-file state and the GUI/node
settings API. This is not a remote or consensus issue, but it can silently
discard persistent node policy changes. History confirms that commit
9c69cfe4c5 introduced the persistent settings file and that the current
startup path still performs read-then-write resolution without a source
boundary. No existing test exercised a settings file containing the settings
key.

### Independent reproduction

Using the old release-like binary
/data/my_storage/tmp/cycle122-clang19-release/bin/bitcoind, a scratch
regtest/settings.json containing:

    {"settings":"redirected.json","uacomment":"self-reference"}

was started without a command-line -settings. The old process left
regtest/settings.json as the two-key input and created
regtest/redirected.json containing the generated _warning_ plus both keys.
The RPC getnetworkinfo result still showed self-reference, proving the input
was read, while the file authority had moved. The old binary also failed the
added functional assertion with the exact result:

    AssertionError: not({'settings': 'redirected.json', 'uacomment': 'self-reference'} == {'_warning_': 'This file is automatically generated and updated by Bitcoin Core. Please do not edit this file while the node is running, as any changes might be ignored or overwritten.', 'settings': 'redirected.json', 'uacomment': 'self-reference'})

This control is independent of the implementation reasoning: it observes the
actual file writes across the process boundary.

### Fix

common::GetSetting() now has an explicit optional ignore_rw_settings source
filter. ArgsManager::GetSettingsPath() uses it only for resolving the
settings-file path, so forced, command-line, and read-only configuration values
still control the path while a value read from that path cannot redirect the
current process. Boolean false still disables dynamic settings; empty values
still fall back to settings.json; and path normalization/trailing-slash
handling remains unchanged.

The functional regression in test/functional/feature_settings.py writes the
self-referential value, restarts the node, asserts that the original settings
file receives the generated warning and retains both values, and asserts that
the redirected file is not created. The source/test commit message is
args: keep settings path stable across rw settings, authored as
Lőrinc <pap.lorinc@gmail.com>; its exact hash is recorded in the uber state
after commit.

### Verification and verdict

- Incremental build command:
  TMPDIR=/data/my_storage/tmp CCACHE_DIR=/data/my_storage/tmp/cycle224-ccache cmake --build /data/my_storage/tmp/cycle214-build --target bitcoind test_bitcoin -j2
  completed 221/221 and linked both targets. The first attempt failed only
  because the default /root/.cache/ccache path was unavailable on the full
  root filesystem; the redirected retry succeeded. Existing unrelated warning
  diagnostics appeared in httpserver_tests.cpp and util_tests.cpp.
- test_bitcoin --run_test=argsman_tests/util_ReadWriteSettings passed with no
  errors. test_bitcoin --run_test=settings_tests/* passed all four cases.
- The fixed daemon passed feature_settings.py with the isolated tmpdir
  /data/my_storage/tmp/cycle224-feature-settings and the cycle214 config file.
  The run covered invalid JSON, duplicate keys, -nosettings from CLI/config,
  alternate settings path, wallet settings, and the new restart case.
- git diff --check passed. The full unit suite was not rerun; the focused
  unit and functional tests exercise the changed utility and process lifecycle.

Verdict: confirmed persistent-settings lifecycle defect and fixed. The
original setting file no longer loses authority after startup, and command-line
or configuration-selected alternate paths remain supported. Remaining Goal 43
cells include includeconf chain-selection transitions, multi-file loadblock
ordering/restart behavior, and option removal/deprecation contracts. A future
cycle must choose one of those distinct cells rather than reopen this
self-referential settings-path case.

## Cycle 233 - failed dynamic-settings persistence rollback

### Identity and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `43` (`option-api-lifecycle`); no
  reroll. Branch: `uber-cycle-233-option-api-lifecycle-20260731`. Start HEAD
  was `c8d69d12f17a8ed7263b4f2d6987449da5408671`; `origin/master` was
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base was
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `1249 42`.
- Catalog, prompt, TSV, and uber-protocol hashes were unchanged. Tracked state
  was clean at the gate apart from known untracked agent artifacts. Protected
  PIDs `777094`, `956381`, `1138182`, and `1157959` remained alive.
- The prior `-txsendrate` metadata and self-referential settings-path cells
  were excluded. This cycle selected the distinct failure lifecycle of the
  public read/write settings APIs: what remains visible in memory when the
  required settings-file write fails.

### Lifecycle map and hypotheses

`interfaces::Chain::updateRwSetting()` and its `overwriteRwSetting()` helper
mutate `ArgsManager::Settings::rw_settings` under the settings lock, then call
`ArgsManager::WriteSettingsFile()` for `SettingsAction::WRITE`. Before this
cycle, a false return from the file write was returned to the caller while the
new value remained in memory. `interfaces::Node::updateRwSetting()` had the
same mutation ordering but ignored the write result entirely. A caller could
therefore observe a setting as updated during the current process even though
the next restart reverted it. The trust boundary is local GUI/wallet/IPC code
using these interfaces and the persistent settings file; no remote or
consensus behavior is involved.

The relevant source paths are `src/node/interfaces.cpp:179-215` for the Node
API and `src/node/interfaces.cpp:864-910` for the Chain API. The existing
`Chain::updateRwSetting()` contract says `WRITE` writes the updated value to
disk, while `SKIP_WRITE` intentionally changes memory only. The failed-write
path violated the first contract by exposing an update that was not durable.
Historical search found commit `1b41d45d46` fixing atomicity of concurrent
settings callbacks, but no existing rollback for a failed persistence step;
the older `argsman_tests/util_ReadWriteSettings` only checked the write error
log.

### Independent controls and negative probes

- The existing functional `feature_includeconf.py` could not start because its
  shared 199-block cache was absent; this was classified as setup-only. A
  scratch regtest daemon with `[regtest] includeconf=network.conf` did include
  the network file, and `getnetworkinfo` reported both `base` and `network`
  user-agent comments. No includeconf defect was reproduced.
- A scratch `regtest/settings.json` containing a manually inserted
  `loadblock` path caused startup to log `Setting file arg: loadblock = ...`,
  start the init-load thread, report the missing file, and exit that thread.
  The settings documentation limits this file to GUI/RPC-managed values and
  no supported API writes `loadblock`, so this was classified as unsupported
  manual-file behavior rather than a current finding. Multi-file loadblock
  ordering and restart behavior remain queued.
- A scratch `settings.json` containing `{"datadir":"...","uacomment":"persisted"}`
  did not redirect the settings file: the path cache is established before
  `settings.json` is read. The original settings file was updated and no
  redirected settings file appeared. This hypothesis was dismissed.
- Registered/deprecated options including `-walletrbf`, removed hidden options,
  and `-prevoutfetchthreads` had coherent registration, consumer, warning, and
  test paths. No independent option-removal defect was found.

### Reproduction and fix

The regression test in `src/test/interfaces_tests.cpp:173-203` first persists
`settings_lifecycle=before`, replaces `settings.json` with a directory so the
atomic rename fails, and attempts to write `after` through both APIs. The
unpatched binary was run with:

```text
env TMPDIR=/data/my_storage/tmp/cycle233-settings-focused-tmp \
  /data/my_storage/tmp/cycle214-build/bin/test_bitcoin \
  --run_test=interfaces_tests/settings_update_failure_preserves_previous_value \
  --log_level=message --report_level=short --color_output=false --random=23301
```

It exited `201` with both assertions failing: Chain returned failure but
`getRwSetting("settings_lifecycle")` was `after`, and Node's
`getPersistentSetting("node_settings_lifecycle")` was `after`.

The fix snapshots the prior presence/value and the post-callback value, then
restores the prior state when the write returns false or throws. Restoration is
conditional on the map still containing the failed update, so a different
concurrent update is not overwritten. A callback returning no action is also
rolled back. `SKIP_WRITE` retains its intentional in-memory behavior. The Node
API now restores state after a failed or throwing write even though its public
return type is void.

### Verification and verdict

- Build: `env TMPDIR=/data/my_storage/tmp/cycle233-settings-build-tmp
  CCACHE_DIR=/data/my_storage/tmp/cycle233-ccache ninja -C
  /data/my_storage/tmp/cycle214-build test_bitcoin -j2` completed and linked
  `test_bitcoin`.
- The fixed focused regression passed: 1 case, 10 assertions. The complete
  `interfaces_tests` suite passed 7 cases and 75 assertions. The existing
  `argsman_tests/util_ReadWriteSettings` passed 1 case and 1 assertion.
- `git diff --check` passed. The full test suite, GUI client, and a concurrent
  stress schedule were not rerun. The conditional restore protects against a
  changed value but cannot distinguish a concurrent operation that produces
  exactly the same serialized value; no evidence indicates that limitation is
  reachable as a separate contract violation here.

Verdict: **confirmed and fixed**. A failed dynamic-settings persistence step no
longer leaves a misleading unpersisted value visible in the current process.
The source/test/journal commit for this cycle is recorded in the uber-goal
state. Remaining Goal 43 cells are includeconf chain-selection transitions,
multi-file loadblock ordering/restart behavior, and option removal/deprecation
contracts; do not reopen the failed-write rollback cell without new evidence.
