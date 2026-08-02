# Journal: public object parsing and validation variant analysis (campaign 15)

Uber-goal rotation. Branch: audit/public-object-validation from
audit/resurrection @ 34c7eab157. Method: compare every equivalent
parse/validation path per object class against malformed/non-canonical
inputs and failure-state guarantees.

## Cycle 1 verdicts

### CPubKey / XOnlyPubKey parse paths: DISMISSED — garbage-in by design, gate at every consumption

CPubKey intentionally permits unvalidated bytes at construction
(pubkey.h:178 contract: "see IsFullyValid()"). Every consumption path
re-validates:
- IsFullyValid → ec_pubkey_parse (pubkey.cpp:343-347).
- CPubKey::Verify: IsValid gate + ec_pubkey_parse → false on failure
  (300-314); lower-S normalize documented (311-313).
- XOnlyPubKey::IsFullyValid/VerifySchnorr/CheckTapTweak/CreateTapTweak:
  xonly parse-gated, false/nullopt on failure (247-297).
- Consensus interpreter CheckSig goes through the same parse path.
Output-on-failure: consistent false/nullopt, no partially-written state.

### Descriptor key parsing (ParsePubkeyInner): DISMISSED — explicit rejections

script/descriptor.cpp:1948-1980: whitespace rejected; hex → CPubKey with
hybrid rejection (1956), IsFullyValid gate (1960), uncompressed rejection
unless permitted (1961-1968); 32-byte x-only keys only in P2TR context,
reconstructed to full key and re-validated (1969-1978). Origin/fpr hex
paths parse-gated (2190+).

### Signature strictness split: BY CONTRACT (noted)

ECDSA lax-DER in wallet/RPC paths (pubkey.cpp:308, lower-S normalize)
vs strict-DER in consensus script validation — two documented strictness
levels serving different purposes; not a divergence bug.

## Campaign 15 cycle complete

All compared paths gate validation consistently; malformed inputs reject
with safe output state. No variants found. Rotation: uber-ledger marks
#15 DONE, next #4 cycle 2 (RPC help bounds).