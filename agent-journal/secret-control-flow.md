# Journal: secret-dependent control-flow and memory-access audit (campaign 14)

Uber-goal rotation. Branch: audit/secret-control-flow from
audit/resurrection @ 239d39cc35. Method: trace secret taint into
branches/loops/indexes/error exits; compare constant-time vs variable-time
helpers; dataflow proof first, dynamic tools second.

## Cycle 1: BIP324 v2 transport crypto (newest secret-dependent code)

### DISMISSED — complete lattice

- ECDH + HKDF: BIP324Cipher::Initialize uses secp256k1 constant-time ECDH;
  ALL key-derivation artifacts explicitly cleansed (ecdh_secret, okm,
  HKDF state — bip324.cpp:73-76) and the node private key destroyed
  post-handshake (m_key = CKey(), line 77).
- Branches in Initialize are ROLE-FLAG driven (initiator/self_decrypt,
  public protocol roles at 51-61) — never secret-value driven.
- Session-key containers: FSChaCha20 and FSChaCha20Poly1305 DELETE
  copy/move "to protect the secret" (chacha20.h:138-142, poly1305.h
  similar) — secret copies impossible by type.
- Cleansing at every level: ChaCha20Aligned dtor cleanses key state
  (chacha20.cpp:42-45), ChaCha20 dtor cleanses keystream buffer (348-351).
- Algorithms are inherently constant-time (ChaCha/Poly1305 arithmetic,
  no secret-indexed tables).

### Secret-taint verdict
No secret-dependent branch/loop/index found. Key lifetime is cleansed
end-to-end with type-level no-copy protections.

## Campaign 14 cycle complete
Rotation: uber-ledger marks #14 DONE, next #18.