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
