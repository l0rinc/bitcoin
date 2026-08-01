# Cycle 253: wallet database newline injection in load warnings

## Selection and fresh gate

- The exact selector for this cycle was `shuf -i 0-98 -n 1` -> `30`, selecting
  `security-logging`. This is a distinct cell from Cycle 200's repeated P2P
  rejection-warning analysis, Cycle 196's fixed RPC whitelist method warning,
  and Cycle 164's persistent-setting redaction fix. The selected queue item
  was the remaining RPC/REST and wallet-data error/log surface.
- The dedicated branch is `uber-cycle-253-security-logging-20260801`. The
  fresh start HEAD was `7d683075db171a7f8efee2657d23cc642f3561a6`, with
  `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and start divergence `42 1293`
  from `git rev-list --left-right --count origin/master...HEAD`.
- The initial current build at `/data/my_storage/tmp/cycle243-build` had
  `ENABLE_WALLET=OFF`, so its attempted `walletload_tests` run exited 201 with
  `no test cases matching filter`. A separate existing wallet-enabled build at
  `/data/my_storage/tmp/cycle246-wallet` was used instead; no protected binary
  was rebuilt or stopped. Known unrelated untracked artifacts were preserved.
- Catalog, prompt, goals TSV, and protocol hashes remained
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

## Scope and hypothesis

Audit wallet load warnings for private metadata, malformed database values,
newline/control injection, misleading diagnostics, and repeated-output
amplification. The working hypothesis was that a malformed or tampered wallet
database can place arbitrary `purpose` and address strings in the
`LoadAddressBookRecords` path, and that the warning at
`src/wallet/walletdb.cpp:960` passes those strings directly to
`WalletLogPrintf`. The logger intentionally preserves newline characters for
legitimate multiline messages, so this sink could turn local wallet-file data
into additional physical log records.

The trust boundary is a local wallet database, including a malformed,
restored, or tampered file; this is not an unauthenticated remote compromise
and does not by itself expose a secret. The concrete impact is log-integrity
confusion for line-oriented collection, alerting, and operator review. The
existing journals and history were searched first; the prior security-logging
cycles contain no test or fix for this wallet-load sink.

## Evidence and independent verification

`WalletBatch::WritePurpose` accepts arbitrary test/database strings and
`CWallet::PopulateWalletFromDB` reaches the purpose-record loader. A focused
regression was added to `src/wallet/test/walletload_tests.cpp`: it writes
`not-an-address\nADDR` and `not-standard\nINJECT`, captures the warning with
`DebugLogHelper`, and requires the raw values to be absent while their
sanitized `not-an-addressADDR` and `not-standardINJECT` forms remain.

On the unmodified implementation, after rebuilding the wallet-enabled
`test_bitcoin`, this command:

    TMPDIR=/data/my_storage/tmp/cycle253-wallet-log-before /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=walletload_tests/wallet_load_sanitizes_invalid_purpose_log --random=253001 --catch_system_error=no --log_level=all

exited 201 with four assertion failures: both raw strings were present and
both sanitized strings were absent. This is a failing-before proof at the
actual wallet-load/logging boundary, not a pattern-only match.

The smallest fix adds the explicit `<util/strencodings.h>` include and passes
`SanitizeString(purpose_str)` and `SanitizeString(strAddress)` to the warning.
The first post-fix run with a nonexistent `TMPDIR` was discarded as setup-only
after a `temp_directory_path` exception. After creating the isolated scratch
directory, the focused rerun with seed `253003` exited 0 with `*** No errors
detected`. The broader `walletload_tests` run with seed `253004` passed both
cases, and the combined `walletdb_tests,walletload_tests` run with seed
`253005` passed all four cases. `cmake --build
/data/my_storage/tmp/cycle246-wallet --target test_bitcoin -j2` and
`git diff --check` also passed.

The sanitization is limited to the diagnostic fields; the database key is
still decoded normally, and wallet state semantics are unchanged. The test
does not claim that arbitrary wallet-file corruption is remotely reachable,
that every log backend has the same parser, or that sanitization replaces
wallet-file integrity checks.

## Verdict and handoff

Verdict: **confirmed and fixed**. A malformed local wallet database could
inject newline-bearing address/purpose values into a wallet-load warning,
creating misleading physical log records. The production fix, regression, and
this journal belong in one independent commit authored as
`Lőrinc <pap.lorinc@gmail.com>`. No other logging sink was changed.

The next distinct queue is to audit RPC/REST error text and request URIs for
sensitive query data or false diagnostics, then revisit wallet destination-data
and migration warnings only if their sinks or contracts differ. Do not reopen
the fixed P2P warning, RPC whitelist, or persistent-setting cells without new
callers, a changed logger, or new evidence.

# Cycle 200: repeated P2P rejection-warning amplification

## Selection and fresh gate

- The exact selector for this cycle was `shuf -i 0-98 -n 1` -> `30`, selecting
  `security-logging`. This is a distinct cell from Cycle 196's fixed RPC
  whitelist method newline injection and Cycle 164's fixed persistent-setting
  redaction. The current cell is repeated P2P rejection warnings, attacker-
  controlled diagnostic text, and per-peer/per-source log amplification.
- The dedicated branch is `uber-cycle-200-security-logging-20260731`. The
  fresh gate timestamp is `2026-07-31T09:10:35Z`; start HEAD is
  `4cf7b4e36af1fc14a5c75fb2640f49d48116c8be`, with `origin/master`
  `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and exact divergence `42 1190`
  from `git rev-list --left-right --count origin/master...HEAD`.
- `git fetch origin master`, tracked/index cleanliness, and `git diff --check`
  passed at entry. Catalog, prompt, corrected goals TSV, protocol, and
  pre-cycle state SHA-256 values are `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`,
  `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`,
  `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, and
  `dc68bc5ec918487efde16dd9d5dd3edc35beb6f8df61d45dc15a286c8e097479`.
- Protected unrelated processes `777094`, `956381`, `1138182`, and `1157959`
  were observed alive and are excluded from this cycle. Scratch data, logs,
  sockets, and daemon state must stay under `/data/my_storage/tmp/cycle200-security-logging`;
  never use a default datadir, wallet, key, or production database.

## Scope and plan

Inventory P2P `LogPrint`, `LogWarning`, `LogError`, and disconnect/ban messages
that include peer-controlled or protocol-derived values: command names,
`subVer`, addresses, inventory counts, reject reasons, message sizes, hashes,
and malformed payload summaries. Trace each value from the network boundary to
the sink and classify it as public, private, attacker-controlled, or secret.
For repeated events, calculate the maximum log work and bytes per peer,
connection, address, and unit time, including reconnect and multi-peer cases.

Prioritize a falsifiable amplification or misleading-diagnostic hypothesis.
Use deterministic socket/message transcripts or an isolated functional test;
do not infer a defect from a large log alone. Search history, existing
discouragement/ban/rate-limit policy, and prior journals before fixing. A
confirmed finding needs captured pre-fix output plus a bounded resource or
truthfulness failure, then an independent replay after the smallest fix.

## Cycle 200 evidence and verdict

### Rejection and punishment paths

- The cached-invalid header warning at `src/net_processing.cpp:3429` is reached
  only for an invalid header received from an outbound peer whose result is
  `BLOCK_CACHED_INVALID`. The current validation sources searched for this
  result use fixed reasons such as `duplicate-invalid`; no peer-supplied
  payload is copied into the warning. The only variable fields are the bounded
  validation message and numeric peer id.
- The warning is followed immediately by
  `MaybePunishNodeForBlock(..., "invalid header received")`. For an ordinary
  outbound peer this sets `m_should_discourage`, and `SendMessages` calls
  `MaybeDiscourageAndDisconnect` before doing more work. That function clears
  the flag, discourages the address, and calls `DisconnectNode`, so the normal
  remote path cannot repeatedly emit the warning on one live connection.
- `NoBan` and manually connected peers deliberately bypass punishment and can
  reach the two unconditional `Not punishing ...` warnings repeatedly. These
  are explicit operator-trusted connection classes rather than unauthenticated
  public peers. Both call sites are still protected by the default unconditional
  logging quota. No correction to punishment or warning severity is justified
  by this cycle's evidence.

### Opt-in transaction rejection logging

`ProcessInvalidTx` logs one line per rejected transaction through
`LogDebug(BCLog::MEMPOOLREJ, ...)`. The category is disabled unless the
operator enables `-debug=mempoolrej`, and the release documentation describes
the same opt-in contract. The line contains fixed-size transaction and witness
hashes plus validation reasons. A source search over all transaction
`ValidationState::Invalid` call sites found enum names, bounded indexes and
numeric values, script-error names, hashes, and policy diagnostics, but no raw
transaction byte string or unbounded peer text. This remains an intentionally
verbose debug facility, not an unconditional security warning; its lack of
the unconditional quota is expected after explicit debug opt-in.

### Input-to-log contract checks

- P2P message types are sanitized at the normal processing entry point, and
  transport validation limits message types to printable 12-byte headers.
- Version user agents are read with `MAX_SUBVERSION_LENGTH` and sanitized
  before they are logged. `CNode::LogPeer()` formats only the numeric peer id
  and, when enabled, a structured address/port.
- The only unconditional P2P warning carrying validation text is therefore
  bounded diagnostic state, not a newline-capable or arbitrary remote string.

### Independent verification

The following source/history checks were run on the clean cycle base:

```text
git grep -n -E 'm_should_discourage|Discourage|MaybePunishNodeForBlock|BLOCK_CACHED_INVALID|If this happens with all peers' -- src/net.cpp src/net_processing.cpp src/net.h src/net_processing.h
git show --format=fuller --no-ext-diff 2f51951d03 -- src/net_processing.cpp
git grep -n -E 'LogWarning|MEMPOOLREJ|GetDebugMessage|MAX_SUBVERSION_LENGTH|LogPeer' -- src/net_processing.cpp src/net.cpp src/validation.cpp src/consensus/validation.h
/data/my_storage/tmp/cycle105-clang19-release/bin/test_bitcoin --run_test=logging_tests --log_level=test_suite
```

The logging run exited 0 with 9 cases and `*** No errors detected`, including
the per-source suppression and reset tests. The default implementation is
`RATELIMIT_MAX_BYTES = 1 MiB` per source location per `RATELIMIT_WINDOW = 1h`;
the limiter suppresses disk output while leaving console output available, and
`-nologratelimit` is an explicit operator override. Those documented limits
and exceptions are recorded rather than treated as defects.

### Cycle verdict and handoff

Verdict: **dismissed**. No confirmed security, correctness, privacy, or
misleading-diagnostic defect was found in this P2P logging cell, so no
production patch or regression test is warranted. The remaining useful queue
is to sample unconditionally logged P2P timeout/old-chain messages across
reconnect and multi-peer schedules, then separately audit whether any
operator-controlled connection class can bypass both disconnection and the
logging quota. The next cycle must choose from the full catalog again and
must not reopen the fixed RPC whitelist newline or persistent-settings
redaction findings.

# Cycle 196: security-sensitive and misleading logging audit

## Selection and gate

- The exact selector for this cycle was `shuf -i 0-98 -n 1` -> `30`, selecting `security-logging`. The prior Goal 30 cell (Cycle 164) fixed plaintext logging of recognized sensitive settings; this cycle is a distinct follow-up from that journal's queue and does not reopen the fixed `rw_settings` sensitivity path.
- The dedicated branch is `uber-cycle-196-security-logging-20260731`. The fresh start HEAD is `b64c39420374fce85bc25f8a75b99248780047cb`, with `origin/master` `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and exact divergence `42 1182` from `git rev-list --left-right --count origin/master...HEAD`. The gate timestamp is `2026-07-31T07:49:02Z`; tracked/index cleanliness and `git diff --check` passed.
- Catalog, prompt, corrected goals TSV, protocol, and pre-cycle state SHA-256 values are `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`, and `36db284731bb2ac0de3634e2b0a7bb6243626591bb1acee5ee5a7c3bd2742052`.
- Protected unrelated processes `777094`, `956381`, and `1138182` were observed alive and are excluded from this cycle. All scratch data, logs, sockets, and daemon state must stay below `/data/my_storage/tmp/cycle196-security-logging`; no default datadir, wallet, key, or production database may be used.

## Scope and plan

The prior cell covered startup argument redaction. This cycle audits attacker-controlled peer/message/error strings and their operational consequences: raw network payloads, peer addresses and identifiers, hostnames, RPC error parameters, filesystem/config paths, wallet labels, malformed object summaries, exception text, and repeated warnings. For each value, classify secret/private/public/intentionally disclosed; for each message, classify user-actionable, operational, debug, or unreachable. Check exact escaping, truncation, severity, rate/duplicate behavior, and whether the text describes the actual state.

The working queue is: inventory logging call sites and formatting helpers; trace untrusted values from P2P/RPC/file/wallet inputs; mine history and review precedent for disclosure or log-amplification fixes; build deterministic malformed-input and repeated-event probes; then independently verify any candidate with captured output and a resource or privacy bound. Do not treat a noisy or ugly message as a defect without a concrete leak, injection/confusion, amplification, or false-diagnostic contract violation. A source change requires a failing-before/passing-after regression and a separate focused validation run; otherwise leave a precise journal-only handoff.

## Prior-cycle exclusions

- Do not reopen Cycle 164's `ArgsManager::SENSITIVE` handling for `rw_settings`, unless this cycle finds a new source-precedence or metadata propagation failure.
- Do not count intentional diagnostic obfuscation keys, explicitly public peer-address policy, I2P private-key avoidance, or secret-memory lifetime as new logging findings without a changed contract or a new sink.
- Do not use log volume alone as evidence: reproduce the triggering path, identify the per-event or per-input cost, and establish whether existing debug-level or operational rate policy is violated.

## Cycle 196 result: whitelist warning log injection

The inventory found one request-controlled value that bypassed the existing RPC logging policy. `src/rpc/request.cpp:252-258` already calls `SanitizeString(strMethod)` for the normal `ThreadRPCServer` trace, while `src/httprpc.cpp:118` and `:147` passed `jreq.strMethod` and the batch `strMethod` directly to `LogWarning`. `JSONRPCRequest::parse()` accepts a JSON string as the method name before whitelist enforcement, so an authenticated RPC client can supply control characters even though the method is rejected. The logger's deliberate `LogEscapeMessage` policy escapes non-newline controls but preserves `\n` for legitimate multiline messages, making caller-side sanitization necessary for one-line structured warnings. The policy and its history are documented by `0b18ea6f57` (`util: Filter control characters out of log messages`) and its `test_LogEscapeMessage` coverage.

The clean-base reproduction used `/data/my_storage/tmp/cycle196-rpc-whitelist-2`, a scratch regtest daemon from `/data/my_storage/tmp/cycle84-build/bin/bitcoind`, with `-rpcuser=rpc-test`, `-rpcpassword=pass-test`, `-rpcwhitelist=rpc-test:getblockcount`, and `-rpcwhitelistdefault=1`. A valid authenticated POST with JSON method `bad\\nINJECT` (the JSON escape decodes to an embedded newline) returned HTTP 403 as expected, but the pre-fix debug log contained:

    2026-07-31T07:53:06Z [warning] RPC User rpc-test not allowed to call method bad
    INJECT

The normal RPC trace at the preceding line contained `method=badINJECT` because it already sanitized the same request field. The second physical line is an attacker-controlled log record with no timestamp/severity prefix. This is a local logging-integrity defect at the authenticated RPC trust boundary: it can confuse line-oriented log collectors, alert matching, and operator review. It is not an unauthenticated daemon compromise; the client must first authenticate as a user subject to a whitelist, and the direct probe did not expose any secret value.

The minimal fix passes `SanitizeString(jreq.strMethod)` to the singleton whitelist warning and `SanitizeString(strMethod)` to the batch warning. The functional regression adds `rpccall_batch()` and sends `bad\\nINJECT` through both branches, requiring the single-line `badINJECT` warning and rejecting the raw `method bad\\nINJECT` sequence. Post-fix validation used `BITCOIND=/data/my_storage/tmp/cycle84-build/bin/bitcoind BITCOINCLI=/data/my_storage/tmp/cycle84-build/bin/bitcoin-cli python3 test/functional/rpc_whitelist.py --configfile=/data/my_storage/tmp/cycle84-build/test/config.ini --cachedir=/data/my_storage/tmp/cycle196-rpc-whitelist-cache --tmpdir=/data/my_storage/tmp/cycle196-rpc-whitelist-functional --portseed=19601 --randomseed=19601 --loglevel=INFO`, which reported `Tests successful`. The rebuilt `test_bitcoin` then passed `rpc_tests` with seed `19603` (23 cases, 316 assertions) and `logging_tests` with seed `19604` (9 cases, 164 assertions). The first `rpc_tests` attempt with a nonexistent `TMPDIR` was discarded as setup-only; the clean rerun created `/data/my_storage/tmp/cycle196-rpc-tests` first.

Static follow-up found no remaining raw `jreq.strMethod` or batch `strMethod` arguments in whitelist warning sinks. The username remains a configured/authenticated identity in these paths, and the normal request trace's existing treatment was not broadened without a separate contract. No P2P user-agent or HTTP framing issue was confirmed in this cycle: P2P `subVer` is sanitized before logging, HTTP control data rejects NUL and line framing prevents an embedded LF in the URI, and the logger escapes other control bytes.

Verdict: confirmed and fixed one authenticated RPC log-injection/confusion defect. The source, functional regression, and this journal belong in one independent commit. Remaining queue: audit RPC/REST error text and request URIs for sensitive query data or false diagnostics, then inspect repeated P2P rejection warnings for event-rate and per-source bounds. Do not reopen the fixed whitelist method sinks without a new logging backend or API contract.

## Cycle 164 historical record

### Selection and gate

- The post-Cycle-163 gate had catalog SHA-256
  `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, prompt
  SHA-256 `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`,
  and goals TSV SHA-256
  `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`.
  Tracked state was clean at `98f0c7b4eac40756eb6c2441818b2d013516e324`,
  with origin/master `67efced1fc83a0b7215cc1513e7c4754fee0f12f`, merge base
  `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`, and divergence `42 1109`.
- The exact selector for Cycle 164 first returned `35`, which is the closed
  mutation campaign from Cycle 107. The required reroll returned `30`,
  `security-logging`; no previous security-logging journal or closed ledger
  entry was found. The branch is
  `uber-cycle-164-security-logging-20260730`.
- PIDs `777094` and `956381` are unrelated long-running test processes. They
  were not started or terminated by this cycle. Existing untracked probes,
  agent-goals files, and other artifacts remain outside the cycle scope.

## Scope and exclusions

The inventory covered `LogInfo`, `LogWarning`, `LogError`, `LogDebug`, startup
argument logging, RPC authentication diagnostics, network peer/message logs,
wallet and database paths, raw payload representations, and error strings.
The following were checked as context rather than reopened: the I2P code
explicitly avoids logging its private key; RPC cookie messages log paths rather
than cookie contents; `LogPeer` and `fLogIPs` handle the existing peer-address
privacy contract; and wallet secret-lifetime campaigns cover in-memory copies,
not log output. Database/block obfuscation keys are intentionally diagnostic
obfuscation keys, not encryption secrets, and were not treated as this finding.

The expected contract is that `ArgsManager::SENSITIVE` applies to every source
of an argument that reaches startup logging. `logArgsPrefix()` already applies
that contract to config and command-line values. `LogArgs()` had a separate
`m_settings.rw_settings` loop that wrote `setting.second.write()` directly.

## Candidate: persistent sensitive setting logged in plaintext

`src/common/init.cpp` reads `settings.json` through `ArgsManager::ReadSettingsFile`
before application startup. `src/init.cpp` registers `-rpcpassword`, `-rpcauth`,
`-rpcuser`, and `-torpassword` with `ArgsManager::SENSITIVE`. After logging is
started, `src/init/common.cpp` calls `args.LogArgs()`. The `rw_settings` loop
therefore had a reachable path from a persistent settings file to `debug.log`
that bypassed the sensitivity metadata.

History supports this as an omission rather than a deliberate disclosure:
`b951b0973cfd4e0db4607a00d434a04afb0d6199` introduced startup argument logging
and its redaction test for command-line values. The settings-file persistence
path was added later by `9d0e9fa8898f002f89dc455d9a30ec75fef1951e`, while the
`rw_settings` logging line remained a raw `write()`.

The independent regression extends the existing `getarg_tests/logargs` case
with a recognized SENSITIVE argument, `rw_settings["dontlog"] = "private42"`.
On the pre-fix source, after rebuilding the Clang 19 UBSan/alignment/object-size
test binary with:

`cmake --build /data/my_storage/tmp/cycle106-clang19-ubsan --target test_bitcoin -j2`

this command:

`env TMPDIR=/data/my_storage/tmp/cycle164-getarg-before /data/my_storage/tmp/cycle106-clang19-ubsan/bin/test_bitcoin --run_test=getarg_tests/logargs --random=164001 --log_level=message --report_level=short --color_output=false`

exited 201. Five of seven assertions passed; the expected masked setting line
was absent and `private42` was present in the captured log. This is direct
before/after output evidence, not a pattern-only review finding.

The fix resolves the same `InterpretKey(setting.first).name` used by
`ReadSettingsFile`, obtains `GetArgFlags_`, and emits `****` when the setting's
argument is SENSITIVE. Unknown settings retain their existing diagnostic
behavior and non-sensitive values retain their serialized representation.
The after-fix command with seed `164002` passed all seven assertions. The
focused `getarg_tests,argsman_tests` run with seed `164003` passed 25 cases and
220787 assertions.

An independent production-path check built `bitcoind` from the same UBSan
build and created only the scratch file
`/data/my_storage/tmp/cycle164-settings-runtime/regtest/settings.json`:

`{"rpcpassword":"cycle164-secret"}`

The bounded command used `-regtest`, that scratch `-datadir`, console logging,
no listener/discovery/seeding connections, zero automatic connections, and an
8-second planned TERM stop. It reached normal initialization and emitted:

`Setting file arg: rpcpassword = ****`

It never emitted `cycle164-secret`, and the captured shutdown sequence ended
with `Shutdown done`. Exit status 124 is the expected `timeout` status for the
planned long-running daemon stop, not a product assertion or hidden failure.

## Verdict and handoff

Confirmed local security-sensitive logging defect: a password stored in the
recognized persistent settings map was written verbatim to startup logs even
though the argument metadata marked it sensitive. The minimal source/test
change and this journal belong in one independent commit. Remaining queue:
audit attacker-controlled peer/message/error strings for truthfulness,
escaping, and bounded repetition, then separately review diagnostic severity
and rate-limit choices. Do not reopen the fixed `rw_settings` sensitivity cell
unless argument-source precedence or metadata handling changes.
