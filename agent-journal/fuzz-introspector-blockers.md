# Campaign #50 — fuzz-introspector-blockers

Base: 8ad188e4fe (journal commit for #95 cycle-2 on
audit/db-semantics-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/introspector-blockers. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): PSBT blocker map — magic is weak; the real blocker is harness truncation (ConsumeRandomLengthString starves long valid documents)

### Draw
Random draw over the 32-goal pool (19 pending + 13 CYCLE-1; #41
excluded as just-cycled): raw=7696717260708278889, index 9 -> #50.

### Blocker 1: magic prefix — MEASURED WEAK
Hypothesis: "psbt\xff" magic is a structural blocker. Refuted by
measurement: 3000 runs empty corpus -> cov 467; 3000 with
"psbt\xff" dictionary -> cov 464 (no delta; 5 bytes is discoverable).

### Blocker 2: decode-success gate — REAL, seed-solvable
The whole serialize/analysis half of the target sits behind
`if (!psbt_res) return;` (psbt.cpp:37-39). UNCOVERED_FUNC after
6000 runs (empty+dict): PartiallySignedTransaction::Serialize 0/82,
PSBTInput::Serialize 0/344, PSBTOutput::Serialize 0/152 — decode
almost never succeeds from random strings. A 19-byte minimal valid
PSBT (psbt\xff + GLOBAL_UNSIGNED_TX of an empty tx) opens the gate:
PST::Serialize<VectorWriter> and AnalyzePSBT become covered.

### Blocker 3 (the real one): harness input-structure truncation
psbt.cpp:37 feeds `fuzzed_data_provider.ConsumeRandomLengthString()`
into DecodeRawPSBT — the provider emits a RANDOM-LENGTH PREFIX of
the input, so long documents almost never arrive whole. Measured
with hand-built seeds (validated through decodepsbt RPC first):
- psbt_min (19 B, zero-input, valid): 1372 edges in isolation —
  decodes, round-trips, analyzes.
- psbt_1in (136 B, one input with non_witness_utxo, valid per
  decodepsbt RPC including the full input map): 528 edges in
  isolation — truncation makes whole-document decode rare.
Consequence: PSBTInput::Serialize (344 edges) and
PSBTOutput::Serialize (152 edges) remain at 0/344 and 0/152 even
with valid seeds — the harness's consumption model, not the seeds,
is the blocker. (Bring-up: my first 1-in seed had the txid endianness
backwards — decodepsbt caught it; corrected and RPC-verified.)

### Verdict
- CONFIRMED (blocker identified and mechanism-proven): the PSBT
  target's serialize side (~500 edges) is unreachable because
  ConsumeRandomLengthString truncates long valid documents. This is
  a HARNESS-REALISM blocker (campaign class: input structure), not a
  production defect and not a seed problem.
- Recommended minimal fix (next cycle, one line): consume the full
  buffer as the document (ConsumeRemainingBytes-style) or prefix
  documents with an explicit length contract, so valid long PSBTs
  survive whole; re-run this before/after table to close it.

### Exact commands
- FUZZ=psbt build_fuzz/bin/fuzz -runs=N [-dict] [-print_coverage=1]
  /tmp/btc50_{empty,dict,seed,i0,i1,iso}
- decodepsbt RPC verification of both hand-built seeds
- seed constructors recorded in the journal history (python struct +
  sha256d without reversal for the serialized txid)

### Limitations / queue
- The one-line harness fix is NOT applied this cycle (scout stage);
  queued with the exact before/after table above as its acceptance
  test.
- Key-origin/HD-keypath serializers (SerializeHDKeypath et al.)
  blocked behind the same gate — same fix covers them.
- Corpus seeds preserved: /tmp/btc50_seed/{psbt_min,psbt_1in}.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): SigningProvider-bearing signing section — 3 signing functions now covered (+191 edges on the iso seed)

### Draw
Re-rank draw (last of the 8 open CYCLE-2+ cells; singleton after
draws 1-7): #50 c2, queued from #101 c1 ("needs a
SigningProvider-bearing target") and #101 c2 ("script_sign.cpp:130
is the natural pattern source"). Branch:
audit/introspector-blockers-c2 from 9a07355c19 (#75 c3
bookkeeping).

### Hypothesis
A provider-driven signing section in the psbt fuzz target (pattern
from script_sign.cpp) covers the three functions the c1 fix could
not reach: SignPSBTInput (127 edges), UpdatePSBTOutput (43),
PSBTInputSignedAndVerified (111) — closed without a new target.

### Change (test-only)
psbt fuzz target: added a signing pass after the existing
round-trip/merge checks — FillableSigningProvider with up to 2
fuzz-consumed keys (ConsumePrivateKey), PrecomputePSBTData, then
per-input SignPSBTInput + PSBTInputSignedAndVerified and per-output
UpdatePSBTOutput on a fresh mutable copy (psbt_sign). Includes:
common/types.h (PSBTFillOptions), key.h, script/signingprovider.h,
test/fuzz/util.h (ConsumePrivateKey).

### Evidence (build_fuzz, same corpus as #101 c1)
- make -C build_fuzz -j4 fuzz: clean.
- FUZZ=psbt ... -runs=3000 -print_coverage=1 /tmp/btc101_seed:
  ZERO UNCOVERED_FUNC matches for SignPSBTInput /
  UpdatePSBTOutput / PSBTInputSignedAndVerified (all three were
  listed at 0/127, 0/43, 0/111 in #101 c1's AFTER measurement);
  no crash, exit 0.
- Isolation acceptance chain on psbt_1in_whole (-runs=0):
  528 (truncated harness) -> 2857 (hybrid fix, #101 c1) ->
  3048 edges (this change, +191).
- 2000-run corpus total: cov 3475 ft 5057.

### Verdict
CONFIRMED oracle extension: the signing-family coverage gap queued
in #101 is closed with a target-local section (no new harness);
coverage table is the regression evidence. Test-only change;
production PSBT code untouched; master-relative severity none.

### Limitations / queue
- Keys are random, not matched to the PSBT's scripts, so actual
  signature production is rare — the signing MACHINERY is covered,
  deep successful-sign paths (complete=true arms) are not
  guaranteed. A key-script-correlated seed (e.g. derived from the
  provider's keys) is the next depth step if a cycle lands here.
- PSBTv2 signing paths (v2-only fields) not specifically seeded.

## Rotation note
One bounded cycle complete; the re-rank queue from the pool-empty
note is now fully consumed (all 8 cells done). Next draws rebuild
the queue from journal queues/URGENT/next-up.

## Cycle 3 (2026-07-29): key-script-correlated PSBT signing seed — layout replay-verified, RPC signability proven

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=6465375341789668838, index 2 -> #50 (third cycle; c2 queue
cell "key-script-correlated seed"). Branch:
audit/introspector-blockers-c3 from d068762887 (#80 c4
bookkeeping).

### Seed design (matches the harness consumption exactly)
233 bytes: [163B PSBT v0: unsigned spend of a P2PKH(K)-funded
outpoint + non_witness_utxo] [0x5c 0x00 terminator] [K 32B]
[junk K2 32B] [key2-comp=1, key1-comp=1, merge-mode=0(whole),
doc1-mode=1(random-length)]. The terminator makes doc1 parse as
exactly the PSBT; front-consumed bytes then land the provider key
exactly on K (layout replay: doc1 == PSBT byte-exact, key1 == K,
compressed=1, all bytes consumed).

### Verifications
- Public API signability: decodepsbt valid; descriptor wallet
  (pkh(descsum_create(WIF)), active=False) -> walletprocesspsbt
  complete=True; finalizepsbt complete=True with signed tx hex
  (0200000001 43cccd...). "Tests successful". Harness path:
  uses_wallet=True needed (framework default False ->
  -disablewallet); importprivkey is legacy-only (descriptor
  wallets -> importdescriptors + descsum checksum + active=False
  for non-ranged; all recorded as framework lessons).
- Fuzz harness: FUZZ=psbt .../fuzz -runs=500 over the correlated
  corpus: clean.
- Layout replay (python simulation of the target's consumption):
  doc1 == PSBT byte-exact; key1 == K; terminator consumed; tail
  bools land in the right order.

### Verdict
CONFIRMED deliverable: a corpus seed that drives the
SignPSBTInput complete arm (sign + PSBTInputSignedAndVerified
with a real signature) — the depth step queued in c2. The seed's
signing property is proven by the public RPC; the harness layout
by byte-exact replay; coverage deltas are doc-structure-dependent
(so not the metric here). Seed preserved: /tmp/psbt_corr_seed
(+doc at /tmp/psbt_corr_seed_doc, WIF at /tmp/corr_wif.txt).

### Exact commands
- python3 seed constructor (ECKey pubkey, hash160, fund/spend/
  PSBT assembly; 0x5c-pair absence checked)
- python3 /tmp/btc50_corr.py (RPC proof)
- FUZZ=psbt build_fuzz/bin/fuzz -runs=500 /tmp/btc50_corr_corpus
- python3 layout replay (this journal's method section)

### Limitations / queue
- PSBTv2 correlated variant not built (same construction with v2
  globals).
- A corpus-directory layout (many correlated variants) is the
  natural qa-assets-style follow-up.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-07-29): PSBTv2 correlated seed — drives the complete sign arm end-to-end; surfacing + fixing the missing ECC init (SEGV on first valid key)

### Draw
Re-rank draw over the rebuilt 3-cell queue (60-c6, 50-c4, 49-c3):
raw=1016919037349801110, index 1 (of 3) -> #50 (fourth cycle; c3
queue cell "PSBTv2 correlated variant"). Branch:
audit/introspector-blockers-c4 from 2a9f35bcc6 (#80 c6 journal tip).

### Hypothesis
A PSBTv2 doc with a P2PKH(K)-funded non_witness_utxo and the
harness-correlated key layout drives SignPSBTInput's complete arm in
the psbt fuzz target (v2 globals path; c3 did v0).

### Seed (corrected layout — see c3 correction below)
/tmp/psbt_v2_corr_seed (257 B, sha256
ea60e88a42cb2f227e475bc758d42a82355d3d605aad4e9d3cb249777147309d):
[185 B PSBTv2 doc: unsigned spend of a P2PKH(K)-funded outpoint,
non_witness_utxo present, K=0x01*32]
[0x5c 0x00][0x5c 0x00] double terminator
[K 32B][K2=0x07*32 32B][tail bools key2-comp=1, key1-comp=1,
merge-mode=1, doc1-mode=1].
doc1 parses as exactly the 185 B doc (doc contains no 0x5c-pair that
would truncate — checked for both v0/v2 docs); merge parses empty;
key1 lands exactly on K. Front bytes 185+2+2+32+32=253 + 4 end bools
= 257 = file size: all consumed.

### c3 layout correction (important)
c3's v0 seed used a SINGLE terminator with merge-mode=0
(ConsumeRemainingBytesAsString): the whole-mode merge string then ate
the provider keys, so the provider never held K as seeded. c3's
replay skipped merge consumption, so the flaw was invisible there;
c3's RPC signability proof was independent of the harness layout and
remains valid. Fixed BOTH seeds this cycle to the double-terminator
layout (doc1-mode=1 random-length, merge-mode=1 random-length);
corrected v0 seed = /tmp/psbt_corr_seed (235 B, sha256
4da6bc8ca8feba91c2a51338ce1766950c702aa930ca6c8b6c2eded2b945e72a).

### Defect found + fixed: psbt target had no ECC init
- Mechanism: the signing pass (08590b364d, #50 c2) made the target do
  ECC work, but the target had no .init; secp256k1_context_sign stays
  null. The FIRST valid fuzz key crashes inside
  FillableSigningProvider::AddKey -> CKey::GetPubKey ->
  secp256k1_ec_pubkey_create(NULL, ...).
- Reachability: needs a decodable doc AND >=64 front bytes for two
  keys; only reachable via the hybrid whole-doc consumption
  (d086164661) — random truncation-mode fuzzing essentially never
  produces both, which is why the corpus campaigns stayed green.
- Failing-before (build_fuzz, ASan+UBSan, pre-fix HEAD target,
  FUZZ=psbt .../fuzz -runs=1 /tmp/psbt_corr_seed):
  key.cpp:198:42 runtime error: null pointer passed as argument 1
  which is declared to never be null; AddressSanitizer SEGV on
  0x000000000000 READ; first-invalid frame
  secp256k1_ecmult_gen_context_is_built (ecmult_gen_impl.h:23) <-
  secp256k1_ec_pubkey_create (secp256k1.c:643) <- CKey::GetPubKey
  (key.cpp:198) <- FillableSigningProvider::AddKey
  (signingprovider.h:323) <- psbt_fuzz_target (psbt.cpp:212).
- Fix (test-only, smallest): initialize_psbt() with a static
  ECC_Context (idiom from bip324.cpp:21) + FUZZ_TARGET(psbt,
  .init = initialize_psbt). 9 insertions, 1 deletion.
- Passing-after (same build, fixed target): both corrected seeds
  -runs=2 clean; both crash artifacts clean; 500-run corpus (v0+v2
  seeds) clean in 97 s.
- Severity: test-infra only (fuzz harness crash); no production,
  consensus, or wallet impact. Fork-local: both enabling commits are
  this campaign's.

### Signing proof (two independent verifiers)
1. In-target trace (temporary instrumentation, reverted after use):
   provider.GetKey(ToKeyID(PKHash)) hit=1 for the utxo spk
   76a91479b000887626b294a914501a4cd226b58b23598388ac;
   SignPSBTInput returns PSBTError::OK (enum value 7) which per
   psbt.cpp:766 requires sig_complete=true (the input is unsigned, so
   the psbt.cpp:662 early-OK path is excluded);
   PSBTInputSignedAndVerified=1 with final_script_sig=106 B — the
   fuzz-key signature verifies against the utxo under the consensus
   script interpreter. (Note: the fuzz target's out_sigdata shows
   complete=0/partial=0 BY DESIGN — SignPSBTInput only copies the
   missing-info fields to out_sigdata, psbt.cpp:759-764.)
2. Public RPC (independent implementation path): descriptor wallet
   pkh(descsum_create(WIF(K))) active=False; walletprocesspsbt on the
   185 B v2 doc -> complete=True; finalizepsbt -> complete=True,
   signed tx 020000000143cccd... (prevout 43cccd6a... matches the
   doc). Script /tmp/btc50_corr_v2.py (v2 variant of c3's).

### Exact commands
- FUZZ=psbt build_fuzz/bin/fuzz -runs=2 /tmp/psbt_v2_corr_seed
  /tmp/psbt_corr_seed  (final code: clean)
- FUZZ=psbt build_fuzz/bin/fuzz -runs=500 /tmp/psbt_c4_corpus
  (Done 500 runs in 97 second(s), clean)
- python3 /tmp/btc50_corr_v2.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc50_v2rpc  (CORR-SIGN-OK)
- failing-before: git checkout -- src/test/fuzz/psbt.cpp && rebuild
  && FUZZ=psbt .../fuzz -runs=1 /tmp/psbt_corr_seed (SEGV above);
  restored and rebuilt green after.

### Verdict
CONFIRMED deliverable + CONFIRMED harness defect: the PSBTv2
correlated seed drives the complete sign arm end-to-end (sign +
consensus-rule verify, two independent verifiers), and the campaign
surfaced+fixed a real fork-local fuzz-harness crash (missing ECC
init) with failing-before/passing-after evidence. Coverage is not
the metric: the new ECC init's secp256k1 selftest covers ecdsa_sign
at startup for any seed, so signing cannot be coverage-attributed —
behavioral proof used instead.

### Artifacts
/tmp/psbt_c4_artifacts/: crash-821b... (= corrected v0 seed,
byte-identical), crash-ccfd... (6 B artifact from the same session,
provenance uncertain, clean post-fix), both corrected seeds. Replay
helpers /tmp/btc50_corr.py (v0), /tmp/btc50_corr_v2.py (v2).
WIF /tmp/corr_wif.txt. Pre-fix file copy /tmp/psbt_fixed.cpp.

### Limitations / queue
- Seeds live in /tmp scratch (disk-pressure policy); a corpus-dir
  layout for qa-assets-style import remains queued from c3.
- Second key K2 is junk (no second input consumes it); multi-input
  multi-key correlated docs are the next depth step.
- crash-ccfd provenance uncertain; kept for the record.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted (multi-input correlated docs, taproot/witness variants).

## Cycle 5 (2026-07-29): 2-input PSBTv2 correlated seed — BOTH provider keys drive their own complete sign arms (K and K2)

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=15284566479080594961, masked to 63 bits 6061194442225819153,
index 1 (of 3) -> #50 (fifth cycle; c4 queue cell "multi-input
multi-key correlated docs"). Branch:
audit/introspector-blockers-c5 from 1341c7ac93 (#65 c5 journal tip).

### Hypothesis
A PSBTv2 doc with 2 inputs funded by P2PKH(K) and P2PKH(K2)
respectively drives SignPSBTInput's complete arm TWICE in one fuzz
run — once per provider key (c4's K2 was junk padding).

### Seed (extends c4's corrected layout)
/tmp/psbt_v2_2in_seed (471 B, sha256
233858af13b1a4573f3ba7462251ed76b3d157063f3b47a89e4169bf0e7e4e28):
[399 B PSBTv2 doc: one funding tx (119 B) with vout0=P2PKH(K),
vout1=P2PKH(K2) at 50000 sat each; spend inputs reference (txid,0)
and (txid,1), each carrying the SAME funding tx as
non_witness_utxo; one OP_TRUE output at 99000 sat]
[0x5c 0x00][0x5c 0x00][K 32B][K2 32B][key2-comp,key1-comp,
merge-mode,doc1-mode = 1,1,1,1].
Constructor: /tmp/btc50_2in.py (framework PSBTMap serializer).
Doc has no 0x5c+non-0x5c pair; front 467 + 4 end bools = 471 = file
size (all consumed).
K = 0x01*32 -> spk 76a91479b000887626b294a914501a4cd226b58b23598388ac
K2 = 0x07*32 -> spk 76a914a3c6b1ee4a49d9f2af3b3802974744fba924164a88ac

### Signing proof (two independent verifiers)
1. In-target trace (temporary per-input instrumentation, reverted):
   SIGDBG input=0 kid=1 hit=1 sign_err=7(OK) verified=1 final_ss=106
   SIGDBG input=1 kid=1 hit=1 sign_err=7(OK) verified=1 final_ss=106
   Both inputs: provider key hit, SignPSBTInput OK (sig_complete),
   PSBTInputSignedAndVerified=1 under the consensus script
   interpreter, 106-byte final scriptSig each.
2. Public RPC (independent path): descriptor wallet with BOTH
   pkh(descsum_create(WIF)) descriptors (active=False);
   walletprocesspsbt complete=True; finalizepsbt complete=True
   (020000000249c97d7a... — both prevouts of the shared funding tx).
   Script /tmp/btc50_corr2_v2.py.

### Post-restore verification (final code)
FUZZ=psbt build_fuzz/bin/fuzz -runs=300 over
{2in-seed, c4 v2 seed, c4 v0 seed}: clean; target restored byte-
identical to the c4 fix commit (git diff empty).

### Exact commands
- python3 /tmp/btc50_2in.py (constructor; prints pubkeys/spks/shas)
- python3 /tmp/btc50_corr2_v2.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc50_2in_rpc (CORR2-SIGN-OK)
- instrumented: cmake --build build_fuzz --target fuzz; FUZZ=psbt
  .../fuzz -runs=2 /tmp/psbt_v2_2in_seed (trace above)
- final: -runs=300 /tmp/psbt_c5_corpus (clean)

### Verdict
CONFIRMED deliverable: the correlated-seed family now covers the
multi-key arm — a single fuzz input drives two independent complete
sign+verify cycles with two distinct fuzz-consumed keys. The c4
"K2 is junk" depth gap is closed.

### Limitations / queue
- Taproot/witness (v0/v1) correlated variants still open — next
  depth cell (P2WPKH needs witness_utxo semantics and the
  require_witness_sig path; P2TR needs schnorr keys and the
  PSBT_IN_TAP_* fields).
- qa-assets-style corpus-dir import still queued from c3.

## Rotation note
Five cycles; multi-key cell closed. Not exhausted (witness/taproot
variants).

## Cycle 6 (2026-07-29): P2WPKH PSBTv2 correlated seed — require_witness_sig arm driven to a verified final witness

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=16460576047879496241, masked 7237204011024720433, index 1
(of 4) -> #50 (sixth cycle; c5 queue cell "witness variants").
Branch: audit/introspector-blockers-c6 from 0abe707222 (#49 c6
journal tip).

### Hypothesis
A PSBTv2 doc whose input carries ONLY witness_utxo (P2WPKH(K))
drives the require_witness_sig path (psbt.cpp:684-690, :743) that
the P2PKH seeds structurally skip (non_witness_utxo sets
require_witness_sig=false).

### Seed (c4 layout, witness variant)
/tmp/psbt_v2_wit_seed (211 B, sha256
4e99ca4c108a1e8f6f234d0073aa4e349a56090e2b1a1985601259cd3479c45f):
[139 B PSBTv2 doc: funding tx pays 50000 to
001479b000887626b294a914501a4cd226b58b235983 (P2WPKH of
K=0x01*32); spend input carries witness_utxo ONLY; 49000 OP_TRUE
output] [0x5c 0x00][0x5c 0x00][K 32B][junk K2 32B][4 bools=1].
Constructor /tmp/btc50_wit.py; no 0x5c-pair; front 207 + 4 = 211.

### Signing proof (two independent verifiers)
1. In-target trace (temporary instrumentation, reverted):
   SIGDBG input=0 kid=0 hit=0 sign_err=7 verified=1 final_ss=0
   final_wit=1 — the kid/hit probe misses BY DESIGN (P2WPKH
   extracts as WITNESS_V0_KEYHASH, not PKHash; the probe is
   P2PKH-specific). The authoritative outcomes: SignPSBTInput OK
   (impossible under require_witness_sig unless a witness signature
   was produced — psbt.cpp:743 returns INCOMPLETE otherwise),
   PSBTInputSignedAndVerified=1 under the consensus interpreter,
   final witness SET, scriptSig empty. The require_witness_sig arm
   is confirmed driven.
2. Public RPC: descriptor wallet wpkh(descsum_create(WIF))
   active=False; walletprocesspsbt complete=True; finalizepsbt
   complete=True with hex 020000000001010ca6b723... — the 0001
   marker/flag proves witness data in the final tx.

### Post-restore control
200-run corpus over the 4-seed family (v0, v2, 2-input, wit)
clean on the byte-identical restored target (git diff empty).

### Exact commands
- python3 /tmp/btc50_wit.py (constructor; prints spk/shas)
- python3 /tmp/btc50_corrw_v2.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc50_wit_rpc (CORRW-SIGN-OK)
- FUZZ=psbt build_fuzz/bin/fuzz -runs=2 /tmp/psbt_v2_wit_seed
  (instrumented trace); -runs=200 /tmp/psbt_c5_corpus (final)

### Verdict
CONFIRMED deliverable: the correlated-seed family now covers the
legacy-P2PKH, multi-key, and P2WPKH-require_witness_sig arms of
SignPSBTInput, each with two independent verifiers.

### Limitations / queue
- P2TR remains open (schnorr keys + PSBT_IN_TAP_* fields; the
  sighash DEFAULT vs ALL branch at psbt.cpp:700-712 is the
  interesting untested arm there).
- qa-assets corpus-dir import still queued.

## Rotation note
Six cycles; witness-v0 cell closed. Not exhausted (P2TR).

## Cycle 7 (2026-07-29): P2TR PSBTv2 correlated seed — raw-output-key keypath arm driven (SignTaproot output-key fallback + SIGHASH_DEFAULT)

### Draw
Re-rank draw over the remaining 2-cell queue:
raw=6221299261676566590, index 0 (of 2) -> #50 (seventh cycle;
c6 queue cell "P2TR"). Branch: audit/introspector-blockers-c7
from 4828e826da (#9 c4 journal tip).

### Mechanism analysis (before construction)
FillableSigningProvider::GetTaprootSpendData only consults
tr_trees (a plain AddKey does NOT register taproot spend data),
so a tweaked-keypath P2TR is unsignable by this harness. BUT
SignTaproot has the output-key fallback (sign.cpp:607
make_keypath_sig(output, nullptr)): CreateSchnorrSig ->
GetKeyByXOnly(output) (signingprovider.h:186-193, covers both
Y-parities) -> CKey::SignSchnorr with merkle_root=nullptr = RAW
schnorr under the untweaked output key. So a P2TR output whose
output key IS K's x-only pubkey (no BIP341 tweak) signs with a
plain provider key. Consensus-side this is a valid keypath spend
(output-key tweak status is a wallet-side convention; verification
checks the sig against the output key directly).

### Seed (c6 layout, taproot variant)
/tmp/psbt_v2_tr_seed (223 B, sha256
39fb43787958c26ce84280b946bc917ae760c5e841b4f7c4e954cf0f2f618b74):
[151 B PSBTv2 doc: funding tx pays 50000 to
51201b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d
5dd078f (P2TR of x-only(K), K=0x01*32, negated=True); spend input
witness_utxo-only; 49000 OP_TRUE output]
[0x5c 0x00][0x5c 0x00][K 32B][junk K2 32B][4 bools=1].
Constructor /tmp/btc50_tr.py (compute_xonly_pubkey).

### Signing proof (two independent verifiers)
1. In-target trace (temporary instrumentation, reverted):
   SIGDBG input=0 spk=51201b84... sign_err=7(OK) verified=1
   tap_key_sig=0 final_ss=0 final_wit=1 — OK + consensus-verified
   + final witness set (keypath sig finalized into the witness;
   m_tap_key_sig reads 0 post-finalization). The
   WITNESS_V1_TAPROOT SignStep -> SignTaproot -> output-key
   fallback arm is confirmed driven, with the taproot
   SIGHASH_DEFAULT branch taken (psbt.cpp:700: no sighash_type,
   IsPayToTaproot -> DEFAULT).
2. Public RPC: descriptor wallet rawtr(descsum_create(WIF))
   active=False; walletprocesspsbt complete=True; finalizepsbt
   complete=True (020000000001011f2e0848... — segwit tx spending
   the funding prevout). rawtr (BIP386 untweaked) matches the
   harness's raw-key semantics exactly.

### Post-restore control
150-run corpus over the 5-seed family (v0, v2, 2-in, wit, tr)
clean; target byte-identical (git diff empty).

### Exact commands
- python3 /tmp/btc50_tr.py (constructor; prints xonly/spk/shas)
- python3 /tmp/btc50_corrt_v2.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc50_tr_rpc (CORRT-SIGN-OK)
- FUZZ=psbt build_fuzz/bin/fuzz -runs=2 /tmp/psbt_v2_tr_seed
  (trace); -runs=150 /tmp/psbt_c5_corpus (final)

### Verdict
CONFIRMED deliverable: the correlated-seed family now covers
P2PKH, multi-key P2PKH, P2WPKH (require_witness_sig), and P2TR
raw-key keypath — every SignPSBTInput complete arm reachable
through a plain FillableSigningProvider. The remaining
unreachable arm is taproot SCRIPT-PATH (needs tr_trees/script
leaves — provider extension, not a seed shape).

### Limitations / queue
- Taproot script-path arm: needs the fuzz target to also populate
  provider.tr_trees (a TaprootBuilder from fuzz bytes) — a harness
  extension candidate if a cycle lands here.
- Tweaked (BIP86) keypath: needs tr_spenddata via PSBT_IN_TAP_
  BIP32_DERIVATION or provider extension; RPC-side covered by tr()
  if ever needed.
- qa-assets corpus-dir import still queued.

## Rotation note
Seven cycles; the seed family is complete through keypath taproot.
Not exhausted (script-path harness extension).

## Cycle 8 (2026-07-29): taproot SCRIPT-PATH arm — harness extension (FlatSigningProvider + fixed-2G tree) + seed; wit_items=3 proof

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=13781584894610127864, masked 4558212857755352056, index 1
(of 3) -> #50 (eighth cycle; c7 queue cell "script-path harness
extension"). Branch: audit/introspector-blockers-c8 from
351b676c47 (#24 c5 journal tip).

### Mechanism analysis
The c2-era harness used FillableSigningProvider — the wallet
keystore class, which has NO taproot tree storage and no
GetTaprootSpendData override: script-path is structurally
unreachable with it. FlatSigningProvider carries the public
tr_trees map. Two-arm subtlety: with internal key = a provider
key, SignTaproot's keypath branch (sign.cpp:602-604) wins FIRST
and the script-path loop never runs — so the registered tree's
internal key must be UNOWNED. Fixed choice: internal = x-only of
2G (02c6047f94...9ee5 — a constant valid point, private key never
consumed); leaf = xonly(keys[1]) OP_CHECKSIG. Keypath unsignable,
script-path forced.

### Harness extension (this cycle's buildable change, 22+/2-)
Provider switched to FlatSigningProvider (keys + pubkeys maps
populated for the two consumed keys — lookup-equivalent for all
prior arms), plus tr_trees registration of the fixed-2G tree when
both keys are valid, plus <script/interpreter.h> for
TAPROOT_LEAF_TAPSCRIPT.

### Seed
/tmp/psbt_v2_trsp_seed (223 B, sha256
bdbb7b37430a2f021c98b70df7137a734dce30f6809a182d4e89dc72ecc87111):
[151 B PSBTv2 doc: funding tx pays 50000 to
5120c91c51656cd53945885bc870c63863b0c50cbb9ca6cc16c7c463ac722
0f5d040 (taproot_construct(2G-xonly, [xonly(K2) OP_CHECKSIG]) —
matches the C++ builder byte-for-byte); witness_utxo-only input;
49000 OP_TRUE output] + c4 trailer. Constructor /tmp/btc50_trsp.py.

### Signing proof (two independent verifiers)
1. In-target trace (temporary instrumentation, reverted):
   sign_err=7(OK) verified=1 final_wit=1 wit_items=3 — the
   3-element witness [schnorr sig, leaf script, control block] is
   definitive for the SCRIPT-PATH arm (keypath witness has 1
   item). First attempt with internal=K proved the keypath-wins
   subtlety (wit analysis drove the 2G redesign).
2. Public RPC: descriptor tr(2G-xonly-hex, pk(WIF(K2))) wallet;
   walletprocesspsbt complete=True; finalizepsbt complete=True
   (0200000000010193a088a0... — prevout matches the funding tx).

### Final-code control
200-run corpus over the 6-seed family (v0, v2, 2-in, wit, tr,
trsp) clean; family re-verified on the Flat provider (all prior
arms still verified=1).

### Exact commands
- python3 /tmp/btc50_trsp.py (taproot_construct tree)
- python3 /tmp/btc50_corrsp_v2.py --configfile=... (CORRSP-SIGN-OK)
- FUZZ=psbt build_fuzz/bin/fuzz -runs=2 /tmp/psbt_v2_trsp_seed
  (wit_items=3 trace); -runs=200 /tmp/psbt_c5_corpus (final)

### Verdict
CONFIRMED deliverable: every SignPSBTInput signing arm reachable
via a key-holding provider is now driven — P2PKH, multi-key,
P2WPKH, P2TR keypath, P2TR script-path — each with two
independent verifiers. The psbt signing-section coverage program
is COMPLETE at the provider-boundary.

### Limitations / queue
- Deeper trees (multi-leaf, hidden branches) and MuSig2 fields:
  harness can grow leaves by the same pattern; PSBT-level musig
  fields are #80's territory.
- qa-assets corpus-dir import still queued.

## Rotation note
Eight cycles; script-path cell closed. The campaign's remaining
cell is the corpus import; further arms need new PSBT features.

## Cycle 9 (2026-07-29): qa-assets-style corpus-dir import — 8-seed family dir merge-validated

### Draw
Re-rank draw over the remaining 3-cell queue:
raw=2479466053413897937, index 1 (of 3) -> #50 (ninth cycle; the
c3-carried cell "qa-assets-style corpus import"). Branch:
audit/introspector-blockers-c9 from 449a7fda9c (#80 c8 journal
tip).

### Corpus dir (the deliverable)
/tmp/psbt_family_corpus/ — 8 files, each with recorded sha256 in
the c4-c8 journals:
- psbt_corr_seed (v0 P2PKH; byte-identical to crash-821b — the
  pre-ECC-init SEGV reproducer, kept as the regression seed)
- psbt_v2_corr_seed (v2 P2PKH)
- psbt_v2_2in_seed (2-input multi-key)
- psbt_v2_wit_seed (P2WPKH require_witness_sig)
- psbt_v2_tr_seed (P2TR raw keypath)
- psbt_v2_trsp_seed (P2TR script-path, fixed-2G tree)
- crash-821b... (dup of psbt_corr_seed; merge dedups it)
- crash-ccfd... (6 B artifact from the c4 crash session,
  provenance uncertain, kept for the record)

### Validation
- libFuzzer merge: FUZZ=psbt .../fuzz -merge=1 /tmp/psbt_merge_out
  /tmp/psbt_family_corpus -> "MERGE-OUTER: successful in 1
  attempt(s)", 8 in / 7 out (byte-dup deduped), cov=11449 edges,
  ft=18864. The dir is a well-formed corpus for qa-assets import.
- The family's distinct value vs the upstream psbt corpus is the
  #9 c3 measurement: upstream drives only anyone-can-spend and
  early-OK arms; this dir drives every key-requiring complete arm
  (P2PKH, multi-key, P2WPKH, P2TR keypath, P2TR script-path).

### Verdict
CONFIRMED deliverable: the corpus-dir layout is complete and
merge-validated; contents, shas, and merge output recorded. The
import path to qa-assets is now mechanical (copy + PR).

### Exact commands
- cp seeds + crash artifacts to /tmp/psbt_family_corpus;
  sha256sum (recorded above)
- FUZZ=psbt build_fuzz/bin/fuzz -merge=1 /tmp/psbt_merge_out
  /tmp/psbt_family_corpus (output above)

### Limitations / queue
- The dir lives in /tmp scratch (disk policy); import = copy.
- 200-run stability over the dir already green (c8 final control).

## Rotation note
Nine cycles; the long-carried import cell is closed. #50's
remaining cells need new PSBT features or harness depth.
