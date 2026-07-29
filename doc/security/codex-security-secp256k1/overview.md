# Codex Security triage: secp256k1

## Result

This was a discovery-only scan of `/data/my_storage/secp256k1`, not a completed
Codex Security report. The scan produced no `scan-manifest.json`,
`findings.json`, `coverage.json`, or final report, so none of the four records
below is a Codex-confirmed vulnerability. They are source-review candidates
with the dispositions recorded here.

The scan inventory contained 188 in-scope files. Five review receipts cover all
188 files (71 + 46 + 38 + 28 + 5), with no unreviewable items. The candidate
ledger and the original receipts are committed beside this file. The generated
baseline build is intentionally not copied: it is about 207 MiB of binaries,
objects, and CMake test output. The complete scan remains at
`/data/my_storage/codex-security-results/secp256k1-full`.

The baseline build was configured with the Silent Payments module enabled and
ran **220/220 CTest tests successfully**. That validates existing behavior; it
does not prove that the performance candidate is acceptable.

## Candidate triage

### 1. Silent Payments output rescanning — performance question, not confirmed

**Code path.** `src/modules/silentpayments/main_impl.h:606-754` accepts
transaction-shaped output data. It caps the recipient-group counter at
`SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT` (2,323), but each surviving
counter value scans all `n_tx_outputs` again. With label lookup enabled, each
non-match also creates two Jacobian label candidates and can invoke batch
inversion and the caller's label callback. The accompanying benchmark uses
23,250 outputs (a block-sized worst case), reverses matches to maximize work,
and exercises up to 2,323 matches (`src/modules/silentpayments/bench_impl.h`).

**Boundary and trigger.** A wallet or other embedder scans transaction outputs
that came from the network. A transaction arranged to make many group values
survive, with matches late in the output list and labels enabled, drives roughly
`min(n_tx_outputs, 2323) * n_tx_outputs` comparisons plus elliptic-curve work.
The public API's count and output validation remain in force.

**Assessment.** The bound is finite and was added specifically to avoid
unbounded quadratic behavior. We did not measure the worst-case benchmark in
this run, so this is a medium-confidence availability/performance concern, not
a memory-safety or cryptographic finding. It may be an accepted protocol cost
if measured wallet scan times are small enough; it becomes a DoS finding only
when a realistic block/transaction can consume excessive scheduler time.

**Reproduction and test.** Build the benchmark with
`-DSECP256K1_ENABLE_MODULE_SILENTPAYMENTS=ON -DSECP256K1_BUILD_BENCHMARK=ON`,
then run `SECP256K1_BENCH_ITERS=1 bin/bench silentpayments_scan_worstcase` (and
the no-match case) on
the target hardware. Preserve measurements for 2,323 and 23,250 outputs in a
benchmark regression or an explicit documented work budget. A minimally
invasive mitigation, if measurements justify one, is to add an early-abort
work budget or cache/group output computations before changing the protocol
limit.

### 2. Lax private-key DER pointer arithmetic — source-proven UB in a contrib helper

**Code path.** `contrib/lax_der_privatekey_parsing.c:11-53` checks lengths by
forming expressions such as `privkey + lenb`, `privkey + len`, `privkey + 3`,
and `privkey + 2 + privkey[1]` and then comparing them with `end`. For a valid
short buffer containing `{ 0x30, 0x82 }`, the parser advances to one-past the
object and then forms `privkey + 2`; a large encoded length similarly forms a
pointer far beyond the object. C pointer arithmetic outside the original array
object is undefined even if the pointer is only compared.

**Boundary and impact.** The input is a caller-supplied serialized private-key
buffer to an explicitly lax parser. Repository search found this helper in the
test/contrib surface, not in the installed library's normal public parsing path.
Malformed bytes can therefore produce UB in callers that opt into this helper,
but this is not evidence of a remotely reachable libsecp256k1 runtime issue.

**Reproduction and fix.** Compile the helper and a two-byte `{0x30, 0x82}`
caller under UBSan (`-fsanitize=undefined`) and assert a defined failure return;
also test an encoded length larger than the backing object. Replace pointer-add
bounds tests with subtraction-based checks (`remaining >= needed`) before every
advance, and add the malformed cases to the contrib/parser test. Treat invalid
caller pointers and undersized objects as outside the API contract.

### 3. macOS Valgrind action installs mutable Homebrew HEAD — CI supply-chain gap

**Code path.** `.github/actions/install-homebrew-valgrind/action.yml:7-28`
does `brew tap LouisBrunner/valgrind`, trusts the formula, fetches its current
`HEAD`, and on a cache miss runs `brew install --HEAD`. The fetched Git revision
is written to a cache fingerprint only after Homebrew has selected it; no
expected commit, release digest, or signature is checked. The macOS workflow
invokes this action at `.github/workflows/ci.yml:488`.

**Boundary and impact.** This is a build-host supply-chain boundary. A
compromised tap or mutable HEAD could execute formula/build code with the
runner's permissions. The scan did not establish a compromise, and the impact
depends on the runner's permissions and available secrets.

**Fix and regression check.** Pin a reviewed tap commit or Valgrind release,
verify its expected commit/digest before `brew trust`, `brew fetch`, or
`brew install`, and derive the cache key from that expected revision. Exercise
the composite action with a mocked `brew` that changes HEAD and ensure the
action fails closed. This is CI hardening rather than a library code change.

### 4. Test-vector comment injection — generator/workflow issue

**Code path.** The Silent Payments and Wycheproof generators accept a
caller-selected JSON file (`tools/tests_silentpayments_generate.py:311-318`,
`tools/tests_wycheproof_generate_ecdh.py:70-73`, and
`tools/tests_wycheproof_generate_ecdsa.py:14-17`) and interpolate each vector's
`comment` directly inside a C comment (`...:255`, `...:133`, and `...:79`). A
comment containing `*/`, followed by declarations or preprocessor directives
and a reopening `/*`, becomes active generated C when the header is compiled.

**Boundary and impact.** The input is test data controlled by the developer or
the upstream-vector refresh process, not a runtime library message. If an
untrusted or compromised JSON corpus reaches the generator, it can alter test
source and execute on a developer/build host or subvert verification. The
current tracked corpora were parsed completely and their generated headers
reproduced byte-for-byte; no current vector contains the terminator.

**Reproduction and fix.** Put a temporary vector comment containing `*/` and a
small declaration into a fixture, run the relevant generator, and compile the
generated header to demonstrate the source break-out. Sanitize comment text
(at minimum replace `*/`, preferably encode all generated text) and add a
malicious-comment regression fixture. Keep upstream vector commits/digests
pinned and review changes to the generated headers.

## Recommended order

1. Measure the Silent Payments worst-case benchmark on wallet hardware and set
   an explicit work-budget decision.
2. Correct the contrib DER parser's pointer arithmetic and add UBSan coverage.
3. Pin and authenticate the Homebrew Valgrind dependency in macOS CI.
4. Sanitize generator comments and add a hostile-vector fixture.

No source files in the secp256k1 checkout were changed by this investigation.

## Re-run locally

The patched CLI is installed in the Bitcoin checkout. From that checkout, run:

```sh
cd /data/my_storage/bitcoin
npx --no-install codex-security scan ../secp256k1 \
  --mode deep \
  --output-dir /data/my_storage/codex-security-results/secp256k1-next \
  --max-cost 100
```

Use a new output directory for each run (or add `--archive-existing`). The
stored Codex credentials are reused automatically. `npx --no-install` is
intentional: it selects the locally installed patched package and never
downloads a different version. Add `--path src,include,contrib,tools,.github`
for a narrower source/configuration scan.
