# Codex Security Bitcoin round 2 — triage overview

Date: 2026-07-29

The source scan is at /data/my_storage/codex-security-results/bitcoin-full2.
This commit intentionally contains only this overview and the eight raw
candidate shards. The original candidate ledger, inventory, threat model, and
temporary review receipts remain at the absolute path above.

This was a discovery pass, not a completed canonical scan report. The output
has no final scan-manifest.json, findings.json, coverage.json, or report.md.
The 22 rows below are therefore not Codex-confirmed findings. They are triaged
against the current checkout and the first-round report in
doc/security/codex-security-bitcoin/investigation-report.md.

## Round inventory

There are 22 candidate records in 8 raw shards:

| Shard | Records |
| --- | ---: |
| shard_00.jsonl | 6 |
| shard_01.jsonl | 3 |
| shard_02.jsonl | 2 |
| shard_03.jsonl | 0 |
| shard_04.jsonl | 0 |
| shard_05.jsonl | 2 |
| shard_06.jsonl | 8 |
| shard_07.jsonl | 1 |

Fifteen records repeat candidates already reviewed in the first round. Seven
are new: two AJV JTD dependency behaviors, verify-commits ancestry handling,
two Model Context Protocol handler behaviors, Octokit URL redaction, and
watch-only wallet export destination handling.

## Disposition summary

| # | Candidate | Disposition |
| ---: | --- | --- |
| 1 | qt-addressbook-csv-formula-injection | Repeat of round-1 candidate 11. Source-supported, user/spreadsheet interaction required; central CSV fix remains appropriate. |
| 2 | qt-bip21-label-transaction-csv-formula-injection | Repeat of round-1 candidate 15. Same contextual CSV issue through the BIP21 label path. |
| 3 | AJV JTD __proto__ prototype mutation | Valid behavior in vendored AJV 8.20.0, but the JTD compileParser API is not used by Bitcoin Core or the Codex Security package path scanned here. Dependency-only; exclude from the Bitcoin finding set. |
| 4 | qt-bip70-merchant-name-out-of-bounds-read | Repeat of round-1 candidate 12. Source-established malformed legacy-wallet GUI read; add bounded parser checks. |
| 5 | psbt-global-unsigned-tx-value-boundary | Repeat of round-1 candidate 9. Source-established outer-stream boundary violation; quantify pre-rejection work with a bounded fixture. |
| 6 | psbt-input-non-witness-utxo-value-boundary | Repeat of round-1 candidate 10. Same generic deserialization defect for input maps. |
| 7 | qt-rpcconsole-private-descriptor-history | Repeat of round-1 candidate 14. Source-established local secret retention; extend the redaction list/tests. |
| 8 | macos-code-signing-and-notarization-passphrases | Repeat of round-1 candidate 2. Passphrase is directly in child argv. |
| 9 | windows-code-signing-passphrase | Repeat of round-1 candidate 3. Passphrase is directly in osslsigncode argv. |
| 10 | AJV JTD empty-properties exception | Valid dependency behavior for a valid JTD schema, but no Bitcoin/Codex consumer of JTD compileParser was found. Dependency-only; exclude from the Bitcoin finding set. |
| 11 | verify-commits-ancestry-error-success | **Confirmed by local reproduction.** A missing Git object returns verifier status 0. Fix the return-code distinction and add a regression. |
| 12 | external-signer-unsigned-transaction-substitution | Repeat of round-1 candidate 8. Intent-preservation defense-in-depth; compare unsigned transactions before assignment. |
| 13 | dns-rebinding:createMcpHandler | A documented, deliberately validation-free fetch API in a transitive dependency. Consumers must add Host/Origin checks; no Bitcoin consumer was found. Not a Bitcoin finding. |
| 14 | qt-psbt-file-size-check-read-toctou | Repeat of round-1 candidate 13. Open once and enforce the cap on the same descriptor. |
| 15 | http-chunked-quadratic-reparse | Repeat of round-1 candidate 6, with stronger reachability evidence: parsing happens before the client allowlist/auth worker check. Measure and bound raw framing/work. |
| 16 | http-pipeline-unbounded-request-queue | Repeat of round-1 candidate 7, with a concrete 250 ms invalid-authentication accumulation window. Bound per-client count and bytes. |
| 17 | remote-faucet-response-resource-bounds | Repeat of round-1 candidate 4. Add request timeout, bounded streaming read, and ImageMagick limits. |
| 18 | resource-exhaustion:createMcpHandler-body | No in-handler POST body cap in the transitive MCP API, but the API is a general building block and no Bitcoin/Codex production consumer was found. Dependency/design hardening, not a Bitcoin finding. |
| 19 | secret-redaction:RequestError-url | Plausible upstream Octokit redaction weakness for unusual query strings, but normal Codex GitHub authentication uses an Authorization header and no secret-bearing URL path was established. Dependency-only; do not file against Bitcoin. |
| 20 | watchonly-export-destination-symlink-race | **Source-established contextual issue.** Authenticated export into a directory writable by a lower-trust local user can follow a dangling symlink or a replacement after the placeholder is closed. Harden destination creation/publication. |
| 21 | ci-container-environment-file | Repeat of round-1 candidate 5. Use exclusive, no-follow creation and cleanup. |
| 22 | published-release-predictable-workdir | Repeat of round-1 candidate 1. Use an owned private temporary directory and safe output creation. |

## New candidate details

### 3 and 10 — AJV JTD behaviors

The shard reports two behaviors in node_modules/ajv 8.20.0:

* JTD compileParser({values:{}}) assigns a JSON member named __proto__ to an
  ordinary object, changing the returned object's prototype.
* JTD compileParser({properties:{},additionalProperties:true}) calls the
  code-generator and throws reduce-of-empty-array for an otherwise valid schema.

Both behaviors are locally reproducible against the vendored dependency. They
are not reachable through the relevant Bitcoin source. The scanner's own
Codex Security package uses Ajv2020.compile() for its JSON Schema contracts,
not JTD compileParser; repository search found no JTD compileParser consumer.
The files are untracked scan-environment dependencies, not Bitcoin Core
release inputs. Keep these as upstream dependency watch items only. If the
Codex Security package later exposes JTD parsing, add package-level tests and
upgrade/patch AJV upstream; do not modify Bitcoin Core for these rows.

### 11 — verify-commits ancestry check

At contrib/verify-commits/verify-commits.py:114-117, every nonzero return from
git merge-base --is-ancestor is treated as the normal “candidate predates the
trusted root” case and exits with status 0. Git returns 1 for a valid
non-ancestor result, but returns 128 for a missing object or operational error.
The verifier therefore fails open on an invalid revision or incomplete
repository.

This was reproduced locally without changing the repository:

    python3 contrib/verify-commits/verify-commits.py this-object-does-not-exist

The command printed “predates the trusted root” and returned 0, while Git
reported “Not a valid object name”. This is source-established and locally
confirmed, but practical impact still depends on a CI/release workflow trusting
this script while its object database or candidate ref can be made incomplete.

Minimal fix: handle return code 0 as ancestor, 1 as non-ancestor, and every
value greater than 1 as an operational failure with a nonzero exit. Apply the
same distinction to the trusted SHA-512 root check. Add a test using mocked
subprocess results for 0, 1, and 128, plus the missing-object integration
fixture. No valid commit is accepted by the current test above; the defect is
the false-success status on an error.

### 13 and 18 — Model Context Protocol handler behaviors

The candidates are in the transitive node_modules/@modelcontextprotocol/server
dependency. createMcpHandler() returns a fetch function that intentionally
does not perform Host or Origin validation; its own documentation at
index.mjs:1167-1183 tells fetch-native consumers to put those checks in front.
The same entry reads an entire POST body with request.text() before JSON.parse
(index.mjs:1048-1064), with no body-size limit in the handler itself.

These are API-contract/design hazards only when an application directly mounts
the low-level handler without the documented middleware or request-size limit.
No Bitcoin Core code consumes this API, and the Codex Security package has no
direct createMcpHandler consumer. Do not report these as Bitcoin vulnerabilities.
If the upstream package wants secure-by-default behavior, add middleware or a
bounded stream option upstream and test direct and wrapped mounting separately.

### 19 — Octokit RequestError URL redaction

The shard points to @octokit/request-error and @octokit/request. The redaction
patterns are narrow: URL query values containing URL encoding, punctuation, or
case-variant parameter names can leave suffixes in error.request. That
dependency behavior is plausible and should be raised upstream if a caller
puts credentials in query strings.

The Codex Security bulk-scan path uses Octokit authentication in an
Authorization header. No Bitcoin code uses Octokit, and no normal path in this
checkout was found that puts a secret in a GitHub request URL. This remains a
dependency hardening item, not a Bitcoin finding. A package-level regression
would need to construct an error through the public request API and verify that
the complete URL is either removed or fully redacted before logging.

### 20 — watch-only export destination race

The authenticated exportwatchonlywallet RPC passes the user-selected path to
ExportWatchOnlyWallet (src/wallet/rpc/wallet.cpp:947-963). The export helper
checks fs::exists(), creates and closes a placeholder with std::ofstream, does
substantial wallet-copy work, and later calls BackupWallet(destination)
(src/wallet/export.cpp:47-63, 205-207). The SQLite backup implementation opens
the destination pathname again with sqlite3_open (src/wallet/sqlite.cpp:356-380).

A lower-trust local user who can modify the destination directory can place a
dangling symlink, which may pass the existence check and be followed by the
placeholder/open, or replace the closed placeholder before the final open.
The RPC caller must be authenticated and must choose a shared writable
directory, so this is contextual rather than a remote wallet issue. The source
sequence is nevertheless a real pathname TOCTOU/symlink boundary violation.

Minimal fix: export to a securely created temporary file in the destination
directory, keep it private while SQLite writes, and publish it with an atomic
no-replace operation that rejects a pre-existing destination/symlink. Use
platform-safe exclusive/no-follow primitives. Add a wallet export test with a
dangling symlink and a replacement attempt between placeholder creation and
backup; assert that the outside target is never written.

## Updated notes on repeated HTTP candidates

The second shard has stronger evidence than the first review recorded. The
chunked parser performs all repeated reparsing before MaybeDispatchRequestToWorker
calls ClientAllowed. Therefore an externally bound RPC listener can spend CPU
on an ultimately unauthorized request before authentication/allowlisting. The
pipeline queue also grows before the separate worker queue limit applies, and
an invalid-authentication request deliberately occupies a worker for about
250 ms. These findings still require an operator-exposed HTTP listener and a
measurement of practical throughput, but the “authentication must pass first”
qualification should be removed from future triage.

The minimal fixes remain: retain incremental parser state or cap raw request
bytes/chunk count, and bound each client's queued request count and aggregate
body bytes while suppressing reads at the limit.

## Run the scan on the sibling secp256k1 library

The patched Codex Security package is installed in the Bitcoin checkout's
node_modules. Run npx from the Bitcoin checkout and pass the sibling library
as the repository argument; do not run npx from secp256k1, which has no local
Codex Security installation:

    cd /data/my_storage/bitcoin
    npx --no-install codex-security scan ../secp256k1 \
      --output-dir /data/my_storage/codex-security-results/secp256k1-full \
      --mode standard --max-cost 500

Validate inputs first:

    cd /data/my_storage/bitcoin
    npx --no-install codex-security scan ../secp256k1 \
      --output-dir /data/my_storage/codex-security-results/secp256k1-full \
      --mode standard --max-cost 500 --dry-run

The dry run was verified locally and resolves the target as
/data/my_storage/secp256k1. Use a new output directory for each independent
run, or add --archive-existing when deliberately resuming/replacing an
existing result. The scan uses the stored Codex credentials and can incur
model cost; keep --max-cost set and keep the library checkout unmodified.
