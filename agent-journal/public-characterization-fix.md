# Campaign #101 — public-characterization-fix

Base: d2cd7ba57f (journal commit for #55 cycle-1; ledger-lineage anchor
audit/resurrection @ 5d0155254c). Branch: audit/public-characterization
(fresh from d2cd7ba57f). Start state: tracked-clean; pre-existing
untracked scratch (node_modules/, test/cache/, package*.json,
txindex-size.log, .ls.swp) and the intentional unstaged deletion of
agent-journal/campaign-goals-99.md (catalog replaced by
campaign-goals.md at the 2026-07-28 migration; left untouched).

## Cycle 1 (2026-07-29): PSBT fuzz harness — hybrid consumption fix for the truncation blocker characterized in #50

### Draw
Random draw over the 21-goal eligible pool (14 pending + 7 CYCLE-1,
nothing just-cycled this turn): raw=10440797539259245241
(`od -An -N8 -tu8 /dev/urandom`, masked to 63 bits), index 12 -> #101.

### Subject selection (public-boundary characterization step)
#50 cycle-1 (journal fuzz-introspector-blockers.md) CONFIRMED a
harness-realism blocker and queued the fix with an acceptance test;
applying + verifying that fix is exactly this campaign's two-step
protocol (characterization already landed; this is the minimal-fix
step). Hypothesis: replacing the PSBT fuzz target's
`ConsumeRandomLengthString()`-only document consumption with a hybrid
(bool-selected truncating OR whole-buffer mode) makes whole valid
documents reachable by the fuzzer without giving up truncation
exploration, and the previously unreachable per-input/per-output
functions become coverable.

### Mechanism (from #50 c1, re-verified this cycle)
src/test/fuzz/psbt.cpp fed `fdp.ConsumeRandomLengthString()` into
DecodeRawPSBT at both call sites (:37 first document, :157 merge
document). ConsumeRandomLengthString terminates the string at the
first 0x5c byte followed by a non-0x5c byte (backslash-escape
convention, FuzzedDataProvider.h:153-179), so any binary PSBT
containing such a pair — i.e. virtually every PSBT with real
transactions — never arrives whole. DecodeRawPSBT then rejects, and
the entire per-input/per-output half of the target (lines 86-145:
GetUTXO, IsNull, FillSignatureData, FromSignatureData,
PSBTInputSigned, CountPSBTUnsignedInputs) is dead for fuzzer-built
inputs. Hand-built seeds proved the gate: psbt_min (19 B, no 0x5c
pair) decodes and covers 1372 edges in isolation; psbt_1in (136 B,
contains 0x5c pairs) covers only 528.

### Fix (test-only, one hunk per call site + comment)
Both sites now do:
  fdp.ConsumeBool() ? fdp.ConsumeRandomLengthString()
                    : fdp.ConsumeRemainingBytesAsString();
ConsumeBool pulls from the END of the buffer
(FuzzedDataProvider::ConsumeIntegralInRange, "Pull bytes off the end
of the seed data"), so existing corpus documents are not shifted; the
last input byte selects the mode (LSB 0 => whole-document mode). To
exercise whole-document mode with the RPC-verified valid seeds, they
are re-pointed by appending one 0x00 mode byte (psbt_min_whole,
psbt_1in_whole) — the document bytes are unchanged. Truncation
exploration is preserved (LSB 1 => old behavior), and the fuzzer
mutates the mode byte like any other.

### BEFORE measurement (old harness binary, grown #50 corpus)
FUZZ=psbt build_fuzz/bin/fuzz -runs=3000 -print_coverage=1
/tmp/btc50_seed (139-element corpus grown from psbt_min/psbt_1in
during #50). UNCOVERED_FUNC still lists, despite the valid seeds:
- PSBTInput::GetUTXO 0/19, GetOutPoint 0/3, IsNull 0/11,
  FillSignatureData 0/162, FromSignatureData 0/145
- PSBTOutput::FillSignatureData 0/78, FromSignatureData 0/51,
  IsNull 0/9
- PSBTInputSigned 0/5
(PSBTInput::Serialize/PSBTOutput::Serialize themselves are covered by
shorter corpus entries that happen to decode; the per-input semantic
functions above are what the truncation gate kills.)
Out-of-target-scope functions correctly remain uncovered:
SignPSBTInput 0/127, UpdatePSBTOutput 0/43,
PSBTInputSignedAndVerified 0/111, wallet FillPSBT family, RPC
DecodePSBTInputs/Outputs (the fuzz target never calls these).

### AFTER measurement (hybrid harness, same corpus + 2 re-pointed seeds)
FUZZ=psbt build_fuzz/bin/fuzz -runs=3000 -print_coverage=1
/tmp/btc101_seed (= /tmp/btc50_seed copy + psbt_min_whole,
psbt_1in_whole). All nine target-called functions that the truncation
gate starved are now COVERED (absent from UNCOVERED_FUNC):
- PSBTInput::GetUTXO (19), GetOutPoint (3), IsNull (11),
  FillSignatureData (162), FromSignatureData (145)
- PSBTOutput::FillSignatureData (78), FromSignatureData (51),
  IsNull (9); PSBTInputSigned (5)
- plus the analysis-copy helpers (PSBTInputAnalysis copy ctor,
  _M_default_append, __relocate_object_a) that AnalyzePSBT reaches
  only with a non-empty input vector.

Isolation acceptance number (#50's queued table): psbt_1in_whole
alone (-runs=0) => cov: 2857 ft: 2847, vs 528 edges for the same
136-byte document under the old harness (5.4x; psbt_min's whole-doc
1372 also exceeded, because inputs/outputs now flow through the
per-element loops).

CONTROL (new binary, OLD corpus without the *_whole seeds,
-runs=3000): all nine functions reappear UNCOVERED with identical
edge counts (0/19, 0/3, 0/11, 0/162, 0/145, 0/78, 0/51, 0/9, 0/5),
and PSBTInputSignedAndVerified 0/111 reappears as well — confirming
its AFTER-list absence was outlining noise. The single controlled
variable is whether a whole valid document reaches DecodeRawPSBT;
the old corpus under the new harness still does not decode whole
(each entry's last byte became the mode selector, shortening the
document by one byte), which is also why the *_whole seeds append a
dedicated 0x00 mode byte instead of relying on existing tails.

Inlining-artifact notes (#9 c2 lesson; UNCOVERED_FUNC granularity is
build-dependent):
- AFTER newly lists Serialize<DataStream> instantiations
  (PSBTInput 0/344, PSBTOutput 0/152, PST 0/82) and the
  deserialize-ctors + ConsumeDeserializableConstructor<PST>
  (fuzz/util.h:146): these belong to other targets linked into the
  same binary, ran in NEITHER build, and merely changed outlining
  between builds. The VectorWriter Serialize instantiations the psbt
  target actually uses stay covered in both builds.
- SignPSBTInput (127) and PSBTInputSignedAndVerified (111) dropped
  off the AFTER uncovered list without being target-reachable (the
  target has no SigningProvider and never signs — verified by code
  inspection of the call graph); same outlining artifact in the
  opposite direction. Not claimed as coverage gains.
- PSBTInput::HasSignatures 0/9 appears in AFTER only; likewise never
  target-called (PSBTInputSigned checks final scripts directly).
Out-of-scope functions correctly still uncovered in both builds:
UpdatePSBTOutput 0/43, wallet FillPSBT family, RPC
DecodePSBTInputs/Outputs 0/369+0/185, ExternalSigner::SignTransaction.

### Exact commands
- make -C build_fuzz -j4 fuzz
- cp -a /tmp/btc50_seed /tmp/btc101_seed; append 0x00 mode byte ->
  psbt_min_whole, psbt_1in_whole
- BEFORE: FUZZ=psbt build_fuzz/bin/fuzz -runs=3000 -print_coverage=1
  /tmp/btc50_seed (old binary)
- AFTER: same, /tmp/btc101_seed (new binary)
- CONTROL: same, /tmp/btc50_seed (new binary)
- ISO: FUZZ=psbt build_fuzz/bin/fuzz -runs=0 -print_final_stats=1
  /tmp/btc101_iso (psbt_1in_whole only) => cov: 2857 ft: 2847

### Verdict
CONFIRMED (harness-realism defect fixed, two-step closed): #50's
characterization predicted the nine per-input/output functions would
become coverable once whole documents reach DecodeRawPSBT; the hybrid
harness + re-pointed seeds cover all nine (483 in-target edges
unlocked), the old-corpus control re-lists all nine, and the isolated
136-byte seed jumps 528 -> 2857 edges. Test-only change; production
PSBT serialization/parsing untouched; no master-relative severity
(master's psbt fuzz target has the identical truncation-only harness —
this is an upstreamable one-hunk-per-site improvement).

## Rotation note
#101 cycle-1 complete with a confirmed fix; campaign returns to the
pool. Next eligible queue for this campaign: characterize the other
truncation-gated fuzz targets (same ConsumeRandomLengthString pattern
-> grep src/test/fuzz for remaining single-mode document consumers,
e.g. any target passing ConsumeRandomLengthString into a full
deserializer), or take the #50 c2 follow-up (SigningProvider-bearing
target for SignPSBTInput/UpdatePSBTOutput) under that campaign.

### Limitations
- Coverage, not a production defect: the change touches only the fuzz
  harness; production PSBT code is unchanged (no master-relative
  severity; master has the identical harness).
- -print_coverage UNCOVERED_FUNC is subject to inlining artifacts
  (#9 c2 lesson); the verdict relies on functions entering/leaving the
  list wholesale, not on edge-count deltas.
- SignPSBTInput/UpdatePSBTOutput/PSBTInputSignedAndVerified stay
  unreachable by design (target never signs); covering them needs a
  SigningProvider-bearing target — out of scope, queued as a possible
  #50 c2 follow-up.

## Cycle 2 (2026-07-29): truncation-gate sibling sweep — class is clean; psbt was the sole member

### Draw
Random draw over the 19-goal eligible pool (14 pending + 5 CYCLE-1,
#43 excluded as just-cycled): raw=6188991159828561970, index 18 ->
#101 (second cycle; c1 queue cell "grep other fuzz targets for the
same single-mode document pattern"). Branch:
audit/public-characterization-c2 from dc72b5940f (#43 c2
bookkeeping). Start state: tracked-clean.

### Hypothesis
Other fuzz targets pass ConsumeRandomLengthString() into a
full-document deserializer, leaving valid long documents unreachable
(same gate class as the psbt target fixed in c1).

### Sweep (all uses, mechanism-level classification)
No-arg uses (3): psbt.cpp (FIXED c1), banman.cpp:114, script.cpp:211.
Bounded-arg uses (10): base_encode_decode.cpp:64/106 (100),
bech32.cpp:69 (limit+1), block_index.cpp:217 (100),
connman.cpp:155/224 (64, MESSAGE_TYPE_SIZE),
crypto_hkdf_hmac_sha256_l32.cpp:21/26 (1024, 128),
http_request.cpp:64/179 (16).
Classification rule: the backslash-escape terminator only gates
VALID whole documents when (a) the consumer is a full-document
parser AND (b) valid documents can contain the 0x5c+non-0x5c pair.
- banman.cpp:114: string is written as a deliberately CORRUPTED
  banlist.json — arbitrary content is the purpose; a valid escape-
  free JSON contains no 0x5c and arrives whole. NO GATE.
- script.cpp:211: DecodeDestination — base58/bech32 alphabets
  exclude 0x5c; valid destinations arrive whole. NO GATE.
- All 10 bounded uses: consumers are codecs whose valid alphabets
  exclude 0x5c (base64/base58/bech32/hex), length-capped ASCII
  tokens (HTTP names/values, net msg type), or arbitrary salt/info
  bytes (HKDF — content-agnostic). NO GATE.

### Executable confirmation (build_fuzz, -print_coverage=1, 2000
runs, empty corpus)
- FUZZ=script: no UNCOVERED_FUNC for DecodeDestination/
  EncodeDestination — consumer covered, no gate.
- FUZZ=banman: no UNCOVERED_FUNC for the load path; loop-body
  functions (Ban 0/5, IsBanned 0/25+0/17, ClearBanned 0/17,
  AssertBanTransition 0/41) uncovered ONLY because an empty corpus
  yields ConsumeBool()==false at LIMITED_WHILE entry — an
  empty-corpus artifact unrelated to the truncation class (grown
  qa-assets corpora enter the loop trivially).

### Verdict
DISMISSED: the truncation-gate class has no siblings; the psbt
target (c1) was the sole member. No fix needed anywhere else. The
alphabet-membership rule above is the reusable classifier for
future ConsumeRandomLengthString additions (review gate: if a new
target feeds it to a binary-document parser, apply the c1 hybrid).

### Limitations / queue
- The remaining PSBT coverage gap (SignPSBTInput 0/127,
  UpdatePSBTOutput 0/43, PSBTInputSignedAndVerified 0/111) needs a
  SigningProvider-bearing target; script_sign.cpp:130 already builds
  a FillableSigningProvider and is the natural pattern source.
  Queued to #50's next cycle (fuzz-blockers owns new-harness work).
- banman empty-corpus loop starvation noted for the record; not a
  defect (seed corpora cover it).

## Cycle 3 (2026-07-29): truncation-gate re-sweep — c2's table was 13/65 uses; completed classification, verdict HOLDS (no new gate)

### Draw
Re-rank singleton (last queue cell, labeled 101-c2; actual next
cycle is c3): #101. Branch: audit/public-characterization-c3 from
cdb282919c (#49 c4 journal tip).

### Hypothesis
Uses of ConsumeRandomLengthString that c2's sweep table missed feed
a full-document parser where valid documents can contain
0x5c+non-0x5c — a live truncation-gate sibling.

### Method
Mechanical re-sweep: grep -rn "ConsumeRandomLengthString"
src/test/fuzz/*.cpp on the CURRENT worktree (lineage) and
git grep on master; diff against c2's 13-entry table; classify each
missed use per c2's alphabet-membership rule (gate only when the
consumer is a full-document parser AND valid documents can contain
the pair).

### Finding about c2 (record correction)
The fuzz tree is byte-identical to c2's base except psbt.cpp
(git diff dc72b5940f..HEAD -- src/test/fuzz = only the campaign's
own 2 commits), yet the current grep finds ~65 uses across 21
files vs c2's 13 across 9. c2's table omitted: kitchen_sink(2),
locale(1), message(4), net_permissions(1), netbase_dns_lookup(1),
rpc(7), socks5(3), string(4), strprintf(3), system(16),
torcontrol(1), plus 6 further http_request sites and script:183
(vector variant). The c2 VERDICT was stated too broadly ("all
uses") for the table actually presented.

### Classification of the missed uses (all NO GATE)
- socks5: username/password/destination are protocol inputs to the
  Socks5() handshake function, not documents parsed from the
  string; truncated hostname == another hostname.
- message: message text (arbitrary), base64 signature + address
  (alphabets exclude 0x5c, same as script.cpp:211 already covered).
- kitchen_sink / rpc: error-string oracles, RPC argument payloads
  (arbitrary, or base64/hex/base32/base58-encoded — exclude 0x5c),
  RPC command name (token).
- http_request (6 more): header names/values, reply body,
  content-length token — protocol fields, capped; truncation yields
  different valid fields (consistent with c2's own 2-site verdict).
- system(16): config network name, arg names/values — tokens.
- strprintf(3): format string + args — content-agnostic.
- locale(1): 5-char locale token. net_permissions(1): permission
  spec alphabet (alnum + ,@.:) excludes backslash.
  netbase_dns_lookup(1): DNS names exclude 0x5c.
- torcontrol(1): tor reply lines (arbitrary by design).
- script:183: ConsumeRandomLengthStringVector — witness stack
  elements, arbitrary bytes.
- Master's smaller set (~48 uses, same families): classification
  transfers identically.

### Verdict
DISMISSED (class remains clean): no full-document parser among the
~50 missed uses accepts 0x5c-bearing valid documents; psbt stays
the sole class member. The c2 record is corrected: the table is now
the complete ~65-use classification.

### Exact commands
- grep -rn "ConsumeRandomLengthString" src/test/fuzz/*.cpp
- git grep -n "ConsumeRandomLengthString" master -- src/test/fuzz/*.cpp
- git diff dc72b5940f..HEAD --stat -- src/test/fuzz/ (tree-identity
  proof for the c2-completeness finding)

### Limitations / queue
- Review gate stands: any FUTURE target feeding
  ConsumeRandomLengthString to a binary-document parser must take
  the c1 hybrid (watch in review cycles).
- #101's own queue is now empty; further cycles need a fresh
  signal (new target, new parser class, or a delegated cell).

## Rotation note
Three cycles; truncation-gate class closed with a corrected,
complete table. Queue-empty for this campaign.
