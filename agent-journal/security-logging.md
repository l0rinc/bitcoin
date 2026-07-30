# Cycle 164: security-sensitive and misleading logging audit

## Selection and gate

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
