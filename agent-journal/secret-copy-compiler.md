# Secret-copy and compiler-optimization audit

## Cycle 149

Status: confirmed and fixed; the cycle is ready for handoff.

### Gate and scope

- Goal: `secret-copy-compiler` (catalog goal 44), selected by exact `shuf -i 0-98 -n 1` -> `44`.
- Branch: `uber-cycle-149-secret-copy-compiler-20260730`.
- Cycle-start HEAD: `6fbe4b1bebc9e3ae92bc901d2fba747206c0f047`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence: `1082 42`.
- Tracked and staged state was clean at the gate. The catalog/protocol hashes matched the uber state ledger. The unrelated wallet test process PID `777094` and all pre-existing untracked artifacts were preserved.
- Prior work excluded from this cycle: the RPC cookie, MuSig entropy and MuSig keypair ordinary-stack findings; the ECDH shared-secret type cleanup; the completed compiler/optimization differential; and the completed constant-time/declassification cells. This cycle instead audited secret-derived state retained by a reusable crypto context and implicit context copies.

### Hypothesis and source evidence

`CHKDF_HMAC_SHA256_L32` stores the HKDF pseudorandom key in its private `m_prk[32]` member. The class had an implicit destructor and implicit copy/move operations, so a context held the secret-derived PRK in ordinary caller storage until reuse or stack overwrite, and callers could silently duplicate it. The class is used by BIP324, the crypto unit tests, and the HKDF fuzz target. BIP324 manually wiped its local whole object, but the test and fuzz callers had no equivalent cleanup.

The class was introduced by commit `551d489416339dae8f9d896013cd060a21406e2b` without an ownership or cleanup contract. The existing project pattern is type-level cleanup for `ECDHSecret`, `ChainCode`, secure key storage, and forward-secure cipher contexts. The BIP324 caller's manual wipe was therefore evidence of the secret but not a complete API guarantee.

### Independent pre-fix proof

The regression `crypto_tests/hkdf_hmac_sha256_l32_clears_secret_on_destruction` placement-constructs the context in aligned byte storage, derives one output from a fixed `0xa5` IKM, explicitly destroys the context, and checks every storage byte is zero. Before the production change:

```text
TMPDIR=/data/my_storage/tmp /data/my_storage/tmp/cycle89-build/bin/test_bitcoin \
  --run_test=crypto_tests/hkdf_hmac_sha256_l32_clears_secret_on_destruction \
  --log_level=test_suite --report_level=short --color_output=false
exit=201
crypto_tests/..: check std::all_of(storage.begin(), storage.end(), ...) has failed
```

This is a direct lifetime oracle, independent of BIP324's existing explicit wipe. It observes the object representation only after the object lifetime has ended and does not depend on a particular secret value remaining in a stack slot.

### Repair

- Added `CHKDF_HMAC_SHA256_L32` destruction cleanup using `memory_cleanse(m_prk, sizeof(m_prk))`.
- Deleted copy and move construction/assignment so callers cannot create avoidable PRK-bearing context copies.
- Removed BIP324's representation-dependent `memory_cleanse(&hkdf, sizeof(hkdf))`; the context now owns its cleanup while the neighboring ECDH and output buffers retain their existing explicit wipes.
- Added the placement-storage regression to `src/test/crypto_tests.cpp`.

The public behavior, HKDF vectors, BIP324-derived keys, and output buffers are unchanged. No caller currently copies or moves the context; the deleted operations make that invariant explicit rather than changing a used call path.

### Compiler and runtime verification

- `CCACHE_DIR=/data/my_storage/tmp/cycle149-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j4`: passed.
- The optimized test binary's `nm -C` output contains `CHKDF_HMAC_SHA256_L32::~CHKDF_HMAC_SHA256_L32()`. `objdump -drC` shows the destructor passes the object address to `memory_cleanse` with `esi = 0x20` before returning. This confirms the compiler retained the non-elidable cleanup call; it is not an inferred inline memset.
- `crypto_tests,bip324_tests` with seed `14902`: 26 cases, 30,198 assertions, passed.
- `crypto_tests,bip324_tests,key_tests` with seed `14904`: 40 cases, 30,961 assertions, passed.
- Rebuilt the Clang 19 ASan/UBSan/libFuzzer binary with `cmake --build /data/my_storage/tmp/cycle131-build-libfuzzer --target fuzz -j4`: passed.
- `FUZZ=crypto_hkdf_hmac_sha256_l32 /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz -runs=2000 -seed=14903 -timeout=5 -rss_limit_mb=2048`: completed 2,000 runs with no sanitizer finding or crash.
- `git diff --check`: passed.

### Dismissed and deferred candidates

- The already-fixed `ECDHSecret`, RPC cookie, MuSig entropy, MuSig keypair, HMAC stack buffers, and BIP324 caller wipe were not reopened.
- `CSHA512` hasher objects can retain buffered input after hashing, and the wallet passphrase KDF uses one locally. That is a real-looking but separate candidate: a generic destructor would affect many copyable, performance-sensitive public-data hashers, while a wallet-call-site cleanup needs its own regression and optimization evidence. It remains the highest-priority next cell rather than an unverified claim in this commit.
- Secure allocator-backed vectors and AES/ChaCha context destructors already provide cleanup for their owned key material; no independent missing cleanup was proven in this cycle.

### Handoff

The source/test/journal change is one self-contained finding. After the separate uber state close commit, the next run must perform a fresh gate and exact selector draw. Prefer the deferred `CSHA512` wallet-passphrase buffer only with a caller-specific proof, or choose the next catalog goal if the fresh selector produces a stronger unexplored surface. Preserve `/data/my_storage/tmp/cycle149-*` artifacts and PID `777094`.
