# Campaign #55 — alternative-implementation-diff

Base: e1d258e71d (journal commit for #38 cycle-2 on
audit/failure-cleanup-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/alt-impl-diff. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): noble-secp256k1 ECDSA differential — 2019/2019 RFC6979 vectors match (+5/5 no-entropy entries)

### Draw
Random draw over the 22-goal pool (16 pending + 6 CYCLE-1; #38
excluded as just-cycled): raw=7089479505672159737, index 9 -> #55.

### Sibling and shared vectors
Sibling: paulmillr/noble-secp256k1 (TypeScript secp256k1 reference).
Shared vectors: test/vectors/secp256k1/ecdsa.json @ main (fetched
2026-07-29): 2019 valid + 5 extraEntropy-empty entries, format
{m: 32-byte msgHash, d: privkey, signature: compact r||s}.

### Differential
Driver (C, secp256k1_ecdsa_sign with nonce_function_rfc6979, NULL
ndata, against build-before's libsecp256k1.a): for each vector, sign
m with d and compare the compact signature byte-for-byte.
- valid: 2019/2019 MATCH.
- extraEntropy entries (all 5 have empty extraEntropy fields): 5/5
  MATCH with the same default path.

### Process notes (honest, for the replay trail)
1. First vector file (rfc6979.json) has a different schema — its
   k0/k1/k15 are aux-entropy variants consumed by their DRBG test,
   not plain signatures; two independent implementations (in-tree
   secp AND a from-RFC python ECDSA) disagreed with it until the
   consuming test file (test/secp256k1.test.ts:17,135-144) showed
   the actual vector path (secp256k1/ecdsa.json with prehash:false).
2. My first differential run hashed the message a second time
   (double-hash) — 10/10 systematic mismatch; semantics fixed by
   reading the sibling's test consumer (m used directly as msgHash).
3. An independent RFC6979+ECDSA reimplementation in python agreed
   with the in-tree secp's behavior before the convention fix —
   ruling out an in-tree implementation divergence; the mismatch was
   harness-side throughout.

### Verdict
- DISMISSED (differential): the in-tree libsecp256k1's RFC6979 ECDSA
  is byte-identical to the sibling implementation on 2019+5 shared
  vectors. No divergence, no local or sibling bug; adapter lessons
  recorded above.
- CONFIRMED (conformance): cross-implementation byte-exact signing.

### Exact commands
- curl noble-secp256k1 test/vectors/secp256k1/ecdsa.json
- gcc driver vs build-before/src/secp256k1/lib/libsecp256k1.a;
  python rfc6979 reimplementation (recorded in conversation)

### Limitations / queue
- extraEntropy vectors with real entropy (ndata path) — the 5 fetched
  were all entropy-empty; ndata-covered vectors queued.
- schnorr/BIP340 sibling vectors (noble's BIP340 set) vs in-tree
  schnorrsig — queued.
- btcd/rust-bitcoin tx-serialization vector differentials — heavier;
  queued.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
