# Codex Security candidate validation — Bitcoin Core

Date: 2026-07-29
Checkout: /data/my_storage/bitcoin
Scan output: /data/my_storage/codex-security-results/bitcoin-full

## Executive status

The Codex run was a partial discovery run, not a completed security report. It
ran for roughly 30 minutes, reached an estimated cost of $69.707493, and
stopped when a model response was flagged by the security policy. It reviewed
6,571 inventory entries and retained 6,365 review receipts (about 96.9%). The
receipt gap covers inventory lines 4933–5138, primarily Qt GUI,
localization, and asset files. About 3,250 untracked node_modules files were
also included in the scan.

There is no final manifest, finding set, coverage aggregate, or report in the
partial output. artifacts/04_reconciliation and artifacts/05_findings are
empty. Consequently the scan itself established **zero confirmed findings**.
The eighteen records below are discovery candidates. This report is a second,
local source review: it does not turn a candidate into a confirmed vulnerability
unless the code is a rigorous proof of the stated behavior. No live network,
wallet, faucet, signing service, or hostile multi-user operation was used.

Status terms used below:

* **Source-established** — the relevant behavior follows directly from the
  current source and is not dependent on a fragile timing or external service
  assumption. A focused regression should still be added before changing code.
* **Reproduction pending** — the source indicates a plausible issue, but a
  bounded local benchmark, fixture, or fuzz seed is needed to establish impact.
* **Contextual** — the behavior is real or plausible but requires a special
  deployment, a trusted component becoming malicious, or a user interaction;
  severity depends on the supported threat model.

The recommended fixes are deliberately minimal. They are remediation plans,
not changes made by this commit.

## Reproduction rules

Run focused tests against a debug/ASan build and keep all listeners bound to
loopback. For network parser tests, use the existing socket-pair or HTTP test
harness; do not expose RPC/REST to an untrusted interface. For wallet and BDB
tests, use a temporary copy or an in-memory/generated fixture. For release
scripts, replace the signing/download tool with a stub and never use production
keys. A timeout is part of every resource-exhaustion test so a malformed input
cannot consume the test host indefinitely.

## Candidate review

### 1. contrib/verify-binaries/verify.py — predictable shared verifier directory

**Boundary and prerequisites.** verify_published_handler() derives
/tmp/bitcoin_verify_binaries.<version> from attacker-predictable release
metadata (verify.py:436-462), creates it with exist_ok=True, and changes
into it. The verifier then downloads files by name. A local user who can create
the directory (or entries in it) before the verifier runs can pre-position a
symlink or file. This is a release-host/multi-user issue, not a Bitcoin runtime
or remote-peer issue.

**Deterministic local test.** In contrib/verify-binaries/test.py, create a
temporary replacement for tempfile.gettempdir(), pre-create the predictable
directory, and make a filename used by the parsed checksum list a symlink to a
sentinel outside that directory. Monkeypatch download_with_wget() (or replace
wget with a stub) so it opens the requested output exactly as the production
path does. Invoke the handler with a fixed version and assert that the sentinel
is not modified. The current implementation follows the symlink or writes
into the pre-positioned directory; a hardened implementation must reject it.

**Expected behavior and confidence.** The wget -O output path is resolved by
the operating system and no ownership, exclusivity, or O_NOFOLLOW check is
made. A pre-existing directory may instead make the run fail, which is a
denial-of-service false-positive for installations where /tmp is protected;
that does not remove the symlink-integrity concern when the attacker can create
writable entries. **Reproduction pending; medium confidence.**

**Minimal fix and regression.** Use tempfile.mkdtemp(prefix="bitcoin_verify_binaries.")
and retain the returned private path; create each output with exclusive,
symlink-safe flags (or verify a newly created regular file before writing).
Clean up only that owned directory in a finally path. Keep the unit test in
contrib/verify-binaries/test.py with a symlink sentinel.

### 2. contrib/macdeploy/detached-sig-create.sh — passphrases in signapple argv

**Boundary and prerequisites.** The script reads the code-signing and
notarization passphrases without echo, then interpolates them into child
process arguments (detached-sig-create.sh:32-58). Any same-user process able
to inspect process arguments while signapple runs (for example /proc, ps, or an
accounting tool) can read the secrets. This is a release workstation/CI
boundary; it does not expose a node wallet.

**Deterministic local test.** Point SIGNAPPLE at a stub that records its
arguments and sleeps. Feed two test passphrases to the script through a
controlled stdin, locate the stub PID, and inspect its cmdline (or the stub's
captured argv). The passphrases are present. The test needs no Apple account,
certificate, notarization, or network access.

**Expected behavior and confidence.** Environment variables are not an
equivalent fix on Linux because /proc/<pid>/environ can also be observable; the
secret must be supplied through a protected descriptor, stdin, or a tool API
designed for secret input. hidepid, container isolation, and a very short child
lifetime can reduce exposure but are deployment assumptions. **Source-
established; high confidence for same-user process-argument disclosure.**

**Minimal fix and regression.** Use a signapple secret-input option that reads
from a protected file descriptor/stdin, or add such an option to the signing
wrapper; never place a passphrase in argv. Add a shell integration test with
the stub and assert that ps/captured argv contains no secret. Also add a trap
to restore terminal echo if the script exits during the prompt.

### 3. contrib/windeploy/detached-sig-create.sh — passphrase in osslsigncode argv

**Boundary and prerequisites.** The Windows signing script passes the
passphrase through osslsigncode ... -pass <passphrase>
(detached-sig-create.sh:25-40). The same-user process-argument boundary and
release-only threat model apply.

**Deterministic local test.** Set OSSLSIGNCODE to a stub that logs argv and
waits, run the script over a small fake unsigned/*.exe tree, and inspect the
stub process. The passphrase is directly visible in argv; no certificate or
timestamp server is needed.

**Expected behavior and confidence.** This is a direct information-flow proof,
although hidepid or isolated CI can make it unreachable in a particular
deployment. **Source-established; high confidence in the release-host
scenario.**

**Minimal fix and regression.** Use an osslsigncode password file/descriptor
facility if available, or a wrapper that hands the secret to the signer through
protected stdin/FD. Add the argv-capture shell test and ensure the passphrase
is absent from process listings and logs.

### 4. contrib/signet/getcoins.py — unbounded/untimed CAPTCHA download

**Boundary and prerequisites.** With --captcha, the helper calls
session.get(args.captcha) without a timeout, retains the entire response in
res.content, parses it as XML, and passes the same bytes to ImageMagick with
captured output (getcoins.py:116-136). The URL is user/configuration input and
the helper is normally run interactively; this is not a node's inbound network
path.

**Deterministic local test.** Use a loopback HTTP server in a Python test. One
endpoint should accept the connection and drip bytes without finishing; the
test must return within its timeout and assert a request-timeout error. A
second endpoint should return an SVG with the expected 150x50 attributes but
more than a chosen byte limit, while a fake ImageMagick binary records whether
it was invoked. The current code waits indefinitely or buffers/processes the
oversized body. Do not contact the public signet faucet.

**Expected behavior and confidence.** XML parsing and ImageMagick may also have
their own limits, but they occur after the unbounded HTTP buffer. The CAPTCHA
server is normally trusted by the operator, so this is an availability issue
for a user who selects a malicious/compromised URL rather than a remote attack
on Bitcoin Core. **Reproduction pending; medium confidence.**

**Minimal fix and regression.** Use stream=True with connect/read timeouts,
reject an excessive Content-Length, read at most a documented maximum, and put
a timeout/output limit on ImageMagick. Add a mocked loopback HTTP test for
slow and oversized responses.

### 5. ci/test/02_run_container.py — predictable /tmp environment file

**Boundary and prerequisites.** The CI wrapper constructs
/tmp/env-USER-CONTAINER_NAME and opens it with plain "w"
(02_run_container.py:38-48). open("w") follows symlinks and is not exclusive.
A local user able to pre-create that predictable name can cause the CI process
to overwrite a chosen file or read attacker-controlled environment data
through the Docker --env-file path. The file is also not unlinked on the
normal path shown here.

**Deterministic local test.** In a Python unit test, set fixed USER and
CONTAINER_NAME, pre-create the expected path as a symlink to a sentinel, and
mock run() so Docker is never started. Invoke the environment-file setup and
assert that a hardened implementation refuses the symlink and removes the
temporary file in finally. The current implementation truncates/follows it.

**Expected behavior and confidence.** Sticky-bit /tmp and a single-user CI host
reduce the attacker set. The path is still unsafe under the stated multi-user
threat model. **Reproduction pending; medium confidence.**

**Minimal fix and regression.** Create a private temporary directory or use
os.open(O_CREAT|O_EXCL|O_NOFOLLOW, 0600)/NamedTemporaryFile, pass its path to
Docker, and unlink it in finally. Keep the test beside the CI helper.

### 6. src/httpserver.cpp — incremental chunked-body reparse/copy

**Boundary and prerequisites.** HTTPRequest::LoadBody() creates a fresh
std::string body and parses from the beginning of a LineReader
(httpserver.cpp:459-530). HTTPRemoteClient::ReadRequest() recreates that reader
over the complete retained m_recv_buffer on each socket-read event, and
SocketHandlerConnected() appends new bytes before trying again. An incomplete
chunked request therefore causes all previous chunk lines and body bytes to be
parsed and copied again. The decoded body is capped at 32 MiB, but raw chunk
framing/extensions can remain in the buffer and the request has no
chunk-count/raw-size budget.

**Deterministic local test.** Extend src/test/httpserver_tests.cpp (or the HTTP
request fuzz harness) with a socket-pair/request fixture that sends
Transfer-Encoding: chunked one byte or one framing line per write, never
sending the terminating zero chunk. Instrument LoadBody() calls and compare
CPU/time and bytes copied for N versus 2N chunks. The call count grows with
every readiness event and the retained buffer grows until a separate limit is
introduced. A loopback functional test in test/functional/interface_http.py
can validate the same behavior without exposing a listener.

**Expected behavior and confidence.** The source proves repeated work; the
security impact is an algorithmic CPU/heap denial of service. The default RPC
listener is loopback-only, so a remote attack requires an operator-exposed
-rpcbind/-rpcallowip endpoint and still has to pass HTTP access controls.
**Reproduction pending; medium-high confidence.**

**Minimal fix and regression.** Preserve parser state/cursor across receives, or
impose a strict raw request-buffer and chunk/extension budget and stop reading
a client whose request is incomplete. Add the drip-feed benchmark and assert
bounded work/bytes in src/test/httpserver_tests.cpp; retain a small fuzz seed
for incomplete chunked bodies.

### 7. src/httpserver.cpp — uncapped per-client pipelined request queue

**Boundary and prerequisites.** MaybeDispatchRequestsFromClient() parses all
complete requests in m_recv_buffer and unconditionally appends them to
m_req_queue. The dispatcher handles one request and returns while m_req_busy is
true; socket readiness continues to read because the wait-set does not account
for queue depth. There is no per-client request-count or byte cap. Each body
can be up to 32 MiB, so many complete pipelined requests can retain substantial
memory while the first handler is slow.

**Deterministic local test.** Add a test in src/test/httpserver_tests.cpp with
a dispatcher whose first request blocks on a condition variable. Send N
complete small requests on one socket before releasing it, then inspect the
client queue size/RSS. Repeat with increasing N and assert that a fixed queue
limit causes back-pressure or connection closure. Existing busy/queue tests
near the request-dispatch code are the natural fixture location.

**Expected behavior and confidence.** The unbounded deque is a direct source
fact. Practical exploitation needs a permitted client and a slow authenticated
RPC or enabled REST handler, so this is not automatically a public
unauthenticated DoS. **Source-established queue growth; reproduction pending
for impact.**

**Minimal fix and regression.** Bound queued request count and total serialized
bytes per client, stop registering receive readiness when the limit is reached,
and return a bounded error/close. Ensure the worker-queue limit and the
per-client limit are both enforced. Test queue saturation and recovery in the
HTTP unit suite.

### 8. src/external_signer.cpp / src/qt/sendcoinsdialog.cpp — signer PSBT substitution

**Boundary and prerequisites.** ExternalSigner::SignTransaction() serializes
the requested PSBT, invokes the configured signer, decodes the returned PSBT,
and assigns it wholesale (external_signer.cpp:80-126). It checks only that a
master fingerprint appears in an input. The Qt send flow shows confirmation for
m_current_transaction, then signs and finalizes the returned PSBT and may
broadcast it (sendcoinsdialog.cpp:462-532). A compromised or malicious
external signer with access to the relevant key can return a fully signed PSBT
for a different transaction.

**Deterministic local test.** In src/wallet/test/psbt_wallet_tests.cpp, use the
existing mock external-signer process. Supply a requested PSBT and make the
mock return a valid, signed PSBT with the same fingerprint but different
outputs/amounts. Call the signer path and assert that it rejects the response
or leaves the original unsigned transaction intact. A Qt integration fixture
can then assert that the transaction passed to sendCoins() is the confirmed
one. No real hardware or funds are needed.

**Expected behavior and confidence.** The assignment is unconditional, so the
host does not enforce the GUI confirmation's transaction identity. A hardware
device that displays and requires confirmation of outputs may mitigate the
attack, and the signer is normally trusted; those are contextual false-positive
conditions. **Source-established defense-in-depth gap; medium confidence as a
security finding.**

**Minimal fix and regression.** Before assignment, compare the returned PSBT's
unsigned transaction (version, locktime, inputs, outputs, sequences and
amounts) with the requested transaction; reject any mismatch. Permit only
signatures and allowed PSBT metadata to change. Put the negative test in
psbt_wallet_tests.cpp and cover the GUI broadcast path separately.

### 9. src/psbt.h — global unsigned transaction value boundary

**Boundary and prerequisites.** The generic UnserializeFromVector() helper
reads a compact declared value length, deserializes directly from the outer
PSBT stream, and checks the number of consumed bytes only after nested parsing
(serialize.h:667-697). The PSBT_GLOBAL_UNSIGNED_TX map entry uses that helper
(psbt.h:1402-1410). A malformed PSBT can make the transaction parser consume
bytes belonging to later map entries, or perform vector parsing and allocation,
before the declared-length mismatch is thrown.

**Deterministic local test.** In src/test/psbt_tests.cpp or src/test/fuzz/psbt.cpp,
construct a minimal PSBT map with a global unsigned transaction key whose
declared value length is one byte, append a transaction serialization
containing deliberately chosen input/output vector counts, and append a
valid-looking next map key. Decode it with DecodeRawPSBT() and assert a clean
rejection. Instrument the reader to prove that the nested transaction consumed
bytes beyond the one-byte field; run the same fixture under ASan with a bounded
allocation/timeout.

**Expected behavior and confidence.** The issue is pre-rejection cross-field
parsing/resource work; this review does not claim that an attacker can make a
semantically valid PSBT. The Taproot BIP32 path in the same file already copies
the declared bytes into a bounded SpanReader, which is the relevant contrast.
**Source-established boundary violation; reproduction pending for measurable
amplification.**

**Minimal fix and regression.** Read exactly the declared value into a bounded
byte vector (with a maximum field size), deserialize from a SpanReader, and
require the subreader to be exhausted. Add malformed global-field fixtures and
keep a fuzz seed in src/test/fuzz/psbt.cpp.

### 10. src/psbt.h — input non-witness UTXO value boundary

**Boundary and prerequisites.** PSBT_IN_NON_WITNESS_UTXO uses the same
outer-stream UnserializeFromVector() path (psbt.h:618-626), so the input map
value can consume later fields before its declared length is checked. This is
separate from the global transaction instance and is reachable for each input
map in an imported/received PSBT.

**Deterministic local test.** Build a PSBT with one input, declare the
non-witness UTXO value as a short field, place transaction vector counts and a
following map key after it, and decode through DecodeRawPSBT(). Assert
rejection and instrument the DataStream/SpanReader to show cross-field
consumption. Keep counts small for the ordinary unit test and put larger
bounded cases in the fuzz corpus.

**Expected behavior and confidence.** The same pre-rejection CPU/allocation and
parser-differential concern applies; semantic acceptance is not asserted.
**Source-established boundary violation; reproduction pending for impact.**

**Minimal fix and regression.** Apply the bounded subreader fix from candidate
9 to this map value and add a dedicated malformed-input test in psbt_tests.cpp
plus a fuzz seed.

### 11. src/qt/addresstablemodel.cpp / csvmodelwriter.cpp — address CSV formula

**Boundary and prerequisites.** Wallet address names are copied into the Qt
model (addresstablemodel.cpp:77-90) and exported as the Label column by
AddressBookPage (addressbookpage.cpp:263-281). CSVModelWriter::writeValue()
only doubles quotes and surrounds the field with quotes
(csvmodelwriter.cpp:32-37). A label beginning with =, +, -, @, tab, or
carriage return can therefore be interpreted as a spreadsheet formula.

**Deterministic local test.** Add a label such as =1+1 through the address book
model, export a CSV to a temporary file, and assert that the first data cell is
still "=1+1" with the current code. Open the same file in a test spreadsheet
only if policy permits; formula evaluation is a user-side effect, not needed
to prove the unsafe serialization. Also test a quoted label and leading
whitespace.

**Expected behavior and confidence.** The data flow and unsafe CSV encoding are
source-established. Exploitation requires an attacker-influenced label and the
user to open the export in a formula-evaluating spreadsheet. **Source-
established behavior; contextual/lower severity.**

**Minimal fix and regression.** Centralize formula neutralization in
CSVModelWriter::writeValue() (for example, prefix a documented apostrophe or
another non-formula character when the first effective character is one of the
spreadsheet trigger set). Add writer unit tests and address-book integration
coverage; document the chosen display/round-trip trade-off.

### 12. src/qt/transactiondesc.cpp — malformed BIP70 merchant parser

**Boundary and prerequisites.** GetPaymentRequestMerchant() searches a legacy
BIP70 payment-request blob for certificate OID bytes, then reads a type byte,
one length byte, and that many UTF-8 bytes without checking that the offsets are
inside pr (transactiondesc.cpp:69-95). Payment-request strings are wallet
transaction metadata and the function is called while rendering transaction
details (transactiondesc.cpp:97-105 and the payment-request rendering below).

**Deterministic local test.** Add a direct unit test for the helper (or extract
it into a small testable parser) with a byte string containing the supported PKI
marker, two OID occurrences, a UTF-8 type 0x13/0x0c, and a length such as 0x7f
but no following 127 bytes. Under ASan/UBSan, call the helper while rendering a
synthetic wallet transaction and assert a clean false result; the current code
forms a QString from beyond the string object. A shorter fixture that truncates
before the type/length byte covers both unchecked reads.

**Expected behavior and confidence.** This is a malformed-input out-of-bounds
read in a GUI metadata parser. It requires a crafted legacy payment request to
be stored in a wallet and the user to open transaction details; it is not a
direct P2P path. **Source-established memory-safety defect; high confidence.**

**Minimal fix and regression.** Check cn_pos + 2 <= pr.size() before reading
type/length, treat the length as unsigned, require
str_len <= pr.size() - cn_pos, and return false for truncated DER. A proper
bounded DER/X.509 parser would be preferable long term. Keep malformed-byte
tests in the Qt transaction-description test target.

### 13. src/qt/walletframe.cpp — PSBT file-size check/read race

**Boundary and prerequisites.** gotoLoadPSBT() checks the selected filename
with GetFileSize(..., MAX_FILE_SIZE_PSBT) and then closes that observation. It
reopens the path and reads to EOF with istreambuf_iterator without a second
limit (walletframe.cpp:193-226). A local process that can replace or grow the
selected file between those operations can make the GUI read an arbitrarily
larger file.

**Deterministic local test.** Extract the file-loading portion into a helper
that accepts a barrier, or use a temporary file and a writer thread. Pause
after the size check, replace/grow the file, release the read, and assert that
a hardened implementation rejects data beyond 100 MiB. The current code reads
the grown contents. Keep the test local; a FIFO/special file is not required.

**Expected behavior and confidence.** This is a direct TOCTOU/unbounded-read
defect, but exploitation requires local path write/replace permission and a
user to select the path in the GUI. **Source-established; high confidence in
the local attacker model.**

**Minimal fix and regression.** Open once, check the size using that same file
descriptor, read at most MAX_FILE_SIZE_PSBT bytes plus one, and reject if the
limit is crossed. Avoid reopening by pathname; use QFile/fstat or an equivalent
bounded stream. Add the barrier-based test beside wallet-frame Qt tests.

### 14. src/qt/rpcconsole.cpp — private descriptors retained in history

**Boundary and prerequisites.** The console filters only a hard-coded list of
private-key commands (rpcconsole.cpp:74-83). RPCParseCommandLine() replaces
arguments with an ellipsis only when the top-level command is in that list
(rpcconsole.cpp:142-183). importdescriptors accepts a desc string and
descriptorprocesspsbt evaluates descriptors with expand_priv=true, so both can
receive an xprv/private descriptor. The returned filtered command is stored as
console history by the Qt UI.

**Deterministic local test.** Extend src/qt/test/rpcnestedtests.cpp with
RPCParseCommandLine(nullptr, ..., "importdescriptors ...xprv...", false,
&filtered) and the equivalent descriptorprocesspsbt command. Assert that
filtered contains an ellipsis and no private key. The current list leaves the
descriptor intact. A GUI test can additionally verify that history is not
written to settings after relocking.

**Expected behavior and confidence.** This is local secret retention, not
remote RPC disclosure: the operator has already typed the descriptor into the
console, but another local user or a settings/history backup can obtain it
after the wallet is locked. expand_priv=true is the source evidence that
private descriptor material is accepted. **Source-established disclosure gap;
high confidence.**

**Minimal fix and regression.** Add both RPC names to the sensitive-command
filter at minimum. A more durable fix is schema-aware redaction of descriptor
arguments containing private keys, with nested-command coverage. Update
rpcnestedtests.cpp with xprv examples and assert no secret survives filtering.

### 15. src/qt/guiutil.cpp / wallet transaction history — BIP21 label formula

**Boundary and prerequisites.** parseBitcoinURI() accepts the URI label
parameter unchanged (guiutil.cpp:149-203). WalletModel persists the label
through setAddressBook() when the recipient is used, and the transaction model
returns that address-book label for LabelRole; TransactionView exports that
role through the same unneutralized CSVModelWriter
(transactionview.cpp:319-345). A URI such as
bitcoin:<address>?label=%3D1%2B1 can therefore produce a formula in a
transaction-history CSV.

**Deterministic local test.** Parse a loopback-only URI with label =1+1, use a
mock wallet/address-book update, export a transaction model row, and assert
that the CSV cell is the formula string under the current code. This can be
combined with candidate 11's writer test; the additional test proves the BIP21
ingestion path.

**Expected behavior and confidence.** The data path is source-established, but
the user must receive/use the URI, export history, and open it in a spreadsheet.
**Source-established behavior; contextual/lower severity.**

**Minimal fix and regression.** Fix the central CSV writer as in candidate 11,
then retain a BIP21-to-export regression in the Qt GUI/wallet test suite so a
future exporter cannot bypass the sanitizer.

### 16. src/wallet/migrate.cpp — self-referential zero-length overflow page

**Boundary and prerequisites.** During legacy Berkeley DB migration, an
OverflowRecord follows PageHeader::next_page until zero
(migrate.cpp:687-707). It appends each page's data and rejects only when the
accumulated data exceeds item_len; it does not require progress or remember
visited page numbers. A valid-looking zero-length overflow page whose
next_page points to itself causes a non-terminating loop. The attacker must
provide a crafted legacy wallet/database that the operator opens or migrates.

**Deterministic local test.** Extend src/wallet/test/fuzz/wallet_bdb_parser.cpp
with a minimized BDB fixture: a leaf overflow record points to an
OVERFLOW_DATA page with hf_offset == 0 and next_page equal to its own page
number. Run the parser in a subprocess with a short watchdog and assert that
it rejects. A focused fixture builder is preferable to modifying a live wallet.

**Expected behavior and confidence.** The loop has no progress condition and
the source path is sufficient to establish the hang. **Source-established
availability defect; high confidence, with local-wallet reachability.**

**Minimal fix and regression.** Track visited overflow page IDs per record and
reject duplicates/cycles; require the final accumulated length to equal
item_len; bound page count by the maximum data size and database page range.
Keep the fixture in the BDB parser fuzz/unit target and retain the watchdog.

### 17. src/wallet/migrate.cpp — repeated internal-page expansion

**Boundary and prerequisites.** The BDB tree walk uses a DFS vector of
(page,expected_level) pairs (migrate.cpp:642-668) and validates only the level.
It does not track page IDs or a work budget. An internal page can refer to the
same child repeatedly, and nested duplicates can multiply the number of page
reads/record visits without bound relative to the unique database pages. The
input is again a crafted legacy wallet/database supplied to migration.

**Deterministic local test.** Create a valid-format internal-page fixture with
many sibling records pointing to one child and, for the amplification case,
repeat the pattern at two or more levels. Run DumpWallet/the read-only BDB
parser under a timeout while counting page reads. Compare unique page count to
DFS work and assert that a hardened parser rejects duplicate references or
stops at a defined budget. The existing wallet BDB fuzz harness is the safest
place to minimize the fixture.

**Expected behavior and confidence.** Berkeley B-tree invariants normally make
duplicate child references invalid, but the current parser does not enforce
that invariant. **Reproduction pending; medium-high confidence.**

**Minimal fix and regression.** Maintain a visited-page set and reject a page
seen twice; additionally cap total work to a function of last_page/page size
and reject out-of-range page numbers. Add a minimized duplicate-reference seed
to the BDB parser fuzz target.

### 18. src/wallet/crypter.cpp, crypter.h, walletdb.cpp — unbounded KDF rounds

**Boundary and prerequisites.** CMasterKey::nDeriveIterations is an unsigned
32-bit wallet-database field serialized without a maximum (crypter.h:33-60).
LoadEncryptionKey() loads it directly (walletdb.cpp:393-418), and wallet
unlock/decryption passes it to CCrypter::SetKeyFromPassphrase()
(wallet.cpp:602-625). The lower-level BytesToKeySHA512AES() takes a signed int
and loops count-1 times (crypter.cpp:15-39), so large unsigned values can
demand billions of hashes or undergo unsigned-to-signed conversion.

**Deterministic local test.** Do not run a billion-round loop in the test
process. Add a validation-only unit test that loads a serialized CMasterKey
with values just above the chosen maximum and asserts a clean wallet error.
For the current behavior, run SetKeyFromPassphrase() in a subprocess with a
small watchdog and rounds=INT_MAX, or instrument the loop counter in a test
build to stop after a few iterations and prove that the supplied value is used.
The natural locations are src/wallet/test/wallet_crypto_tests.cpp and the
crypter fuzz target.

**Expected behavior and confidence.** A malformed/tampered legacy wallet can
make unlock consume extreme CPU; values above INT_MAX additionally expose an
unsafe signed conversion. The attacker must cause the user to open the wallet
file (or otherwise write its database); this is not a remote P2P primitive.
**Source-established resource/validation defect; high confidence.**

**Minimal fix and regression.** Define a documented MAX_DERIVE_ITERATIONS,
reject values above it during database load and before every derivation, and
return a wallet error rather than silently capping an attacker-controlled
value. Change the internal helper to an unsigned/count type and a monotonic
loop, while retaining the maximum for compatibility and CPU safety. Add
boundary tests for zero, the default, the maximum, maximum+1, and values that
would overflow int.

## Triage summary

The strongest source-established defects are the BIP70 out-of-bounds read
(12), the Berkeley DB overflow cycle (16), wallet KDF validation/CPU exhaustion
(18), private-descriptor history retention (14), the PSBT file read race (13),
and the two secret-in-argv release scripts (2 and 3). The HTTP queue/parser
issues (6 and 7), BDB duplicate expansion (17), and both PSBT field-boundary
issues (9 and 10) have compelling source evidence but should be measured with
the focused fixtures before assigning severity. External-signer substitution
(8) is a real intent-preservation gap whose practical threat model depends on
whether the signer is trusted and whether it displays/gets confirmation of
outputs. CSV formula candidates (11 and 15), temporary-file candidates (1 and
5), and the CAPTCHA helper (4) are valid hardening leads with more contextual
prerequisites.

## Recommended implementation order

1. Add bounded parsing/rejection tests and fixes for 12, 16, 18, 9, and 10.
2. Add private-command redaction tests for 14 and transaction-identity tests
   for 8.
3. Bound HTTP receive buffers/queues and add drip-feed/pipeline tests for 6
   and 7.
4. Fix the PSBT file race (13) and centralize CSV formula neutralization (11,
   15).
5. Harden release/CI temporary files and signer secret handling (1–3, 5),
   then add the bounded CAPTCHA helper (4).

All changes should be reviewed against the repository threat model and run in
the existing unit, fuzz, functional, Qt, wallet, and release-test targets
listed above. This report intentionally does not claim that any of those fixes
has been applied.
