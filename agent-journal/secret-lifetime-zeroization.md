# Journal: secret-data lifetime and zeroization audit (campaign 13)

Uber-goal rotation. Branch: audit/secret-lifetime-zeroization from
audit/resurrection @ 979bae3a50. Method: label secret classes, trace
copies/error paths/destructors, check cleanse survives at container level.
Campaign rule: no broad memset churn — verify the mechanism instead.

## Cycle 1: the zeroization lattice — COMPLETE, DISMISSED

Containers (all self-cleansing at destruction):
- CKey::keydata — secure_unique_ptr<std::array<32>> (key.h:60);
  secure_unique_ptr's deleter cleanses then frees (allocators/secure.h:58-74).
- KeyPair::m_keypair — secure_unique_ptr<std::array<96>> wrapping the
  opaque secp256k1_keypair entirely (key.h:299-304).
- CKeyingMaterial — secure_allocator<unsigned char> vector; deallocate
  cleanses before LockedPool free (secure.h:38-40, crypter.h:63).
- SecureString — secure_allocator basic_string (secure.h:53).
- LockedPool: allocator deallocate cleanses BEFORE pool free — double
  coverage.

Secret paths verified:
- Passphrase: SecureString end-to-end RPC → Unlock/ChangeWalletPassphrase;
  plain_master_key/master_key temporaries are CKeyingMaterial (scope-exit
  cleanse).
- Seed: CKey seed_key (GenerateRandomKey, wallet.cpp:3666) →
  master_key.SetSeed — both secure containers, cleansed at scope exit.
- CKey copies: copy-assign stays inside secure containers (key.h:80-85);
  moves are pointer moves (no copy).
- KeyPair (BIP340): opaque struct lives inside the secure container.

Boundaries (documented, upstream-accepted, not defects):
- Passphrase transit through UniValue/JSON plain std::string — the known
  RPC layer boundary (same as upstream; CLI/docs cover it).
- Serialized-key import streams hold plaintext transiently (key.h:107-111).
- BIP32 chaincode: PUBLIC by design (present in xpubs) — not secret.

Verdict: no avoidably-long secret lifetime found; mechanism coherent.
Rotation: uber-ledger marks #13 DONE, next #14.