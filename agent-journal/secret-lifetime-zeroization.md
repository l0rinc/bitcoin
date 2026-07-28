# Secret-data lifetime and zeroization audit

## Cycle 42: RPC cookie entropy temporary

Status: confirmed and fixed in the current worktree; functional validation is blocked by the host disk-space guard.

### Gate and scope

- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base and pre-finding HEAD: `eee7f5beb825734df0908a1f3f3093f1a70fe237`
- Worktree gate: tracked/staged state was clean; unrelated untracked agent artifacts were preserved.
- Scope: secret-bearing random buffers and their cleanup on success and error returns.
- Upstream relation at the cycle gate: `origin/master...HEAD = 2 852`.

### Hypothesis

`GenerateAuthCookie` creates a 32-byte RPC cookie password in an ordinary stack array. The value is an authentication credential, not merely public random input. The array remains live through conversion, file creation, rename, permission handling, and the final return, and it is not cleansed on any path. A secure allocator-backed temporary should preserve the cookie format and returned password while cleansing the raw bytes at scope exit.

### Evidence

Before the change, `src/rpc/request.cpp` used `unsigned char rand_pwd[COOKIE_SIZE]`, filled it with `GetRandBytes`, and converted it with `HexStr`. The function can return `Disabled` before opening the file, `Error` after open, rename, or permission failure, or `Ok` after assigning the hexadecimal password to the caller. None of those paths cleared the raw array.

The authentication path reaches this function from RPC HTTP initialization. The secure allocator in `src/support/allocators/secure.h` locks allocations and calls `memory_cleanse` in `deallocate`, including vector destruction on every return. Existing key and wallet code already uses `std::vector<unsigned char, secure_allocator<unsigned char>>` for sensitive temporary buffers. The caller-visible hexadecimal `std::string` is retained because it is the required RPC credential; this change covers only the unnecessary raw entropy temporary.

### Change

Added the secure allocator include and changed `rand_pwd` to a 32-byte `std::vector<unsigned char, secure_allocator<unsigned char>>`. `GetRandBytes` and `HexStr` retain their existing behavior and the cookie file format is unchanged.

### Verification

- `git diff --check`: passed.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed; rebuilt `src/rpc/request.cpp` and linked `bin/test_bitcoin`.
- `build_unit_clang19/bin/test_bitcoin --run_test=rpc_tests --log_level=test_suite`: passed; 22 RPC test cases, `*** No errors detected`.
- `build_func_clang19/test/functional/test_runner.py rpc_users.py --tmpdirprefix=/tmp/bitcoin-rpc-cookie-cycle42`: blocked before test execution because `/` had 64M free and the daemon rejected the regtest datadir as too low on disk space.
- `build_func_clang19/test/functional/test_runner.py rpc_users.py --tmpdirprefix=/data/my_storage/bitcoin/.tmp-rpc-cookie-cycle42`: reached block-database initialization but the daemon still reported `Disk space is too low!`; `/data` had 41G free. This is an environment/resource limitation, not a test assertion failure.

### Verdict and limits

Confirmed: the raw cookie password had an avoidable ordinary-stack lifetime, and the secure vector provides the project-standard cleanup mechanism without changing authentication behavior. This audit does not claim that all copies of the credential are secure: the required caller-owned hexadecimal password and file contents remain ordinary strings/files, and those are separate API/storage contracts. No broad string or logging redesign is in scope.

### Next queue

1. Audit the raw MuSig nonce-generation entropy temporary in `src/musig.cpp`.
2. Audit the local `secp256k1_keypair` in MuSig partial signing, which carries secret key material through multiple failure returns.
3. Revisit compiler-visible cleanse evidence and existing ctime/checkmem coverage after the current source findings.

## Cycle 42: MuSig nonce-generation entropy temporary

Status: confirmed and fixed in the current worktree.

### Hypothesis

`CreateMuSig2Nonce` generated the 32-byte session randomness required by MuSig2 in a plain `uint256`. That value is secret nonce-generation entropy and must be unique and secret, but the ordinary stack object survived all subsequent parsing, libsecp calls, serialization, and error returns without an explicit cleanse. The secure allocator should cover the caller-owned buffer until the function exits.

### Evidence

Before the change, `src/musig.cpp:146-147` declared `uint256 rand`, filled it with `GetStrongRandBytes`, and passed `rand.data()` to `secp256k1_musig_nonce_gen`. The public libsecp contract at `src/secp256k1/include/secp256k1_musig.h` describes `session_secrand32` as unique, uniformly random, and secret. The implementation invalidates the input on successful nonce generation, but a caller buffer still needs cleanup on wrapper failure paths and after the call.

`CreateMuSig2Nonce` is reachable from `src/script/sign.cpp` during MuSig2 signing and the resulting secret nonce is retained in the provider. `src/musig.cpp` already includes `support/allocators/secure.h`, and the same module uses `make_secure_unique` for `secp256k1_musig_secnonce`. Existing key code uses the secure allocator for random temporary vectors, establishing the local pattern.

### Change

Changed `rand` to `std::vector<unsigned char, secure_allocator<unsigned char>>(32)`. The random bytes, pointer passed to libsecp, public nonce output, and failure behavior are unchanged; destruction now cleanses and unlocks the entropy buffer on every return.

### Verification

- `git diff --check`: passed.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed; rebuilt `src/musig.cpp` and linked `bin/test_bitcoin`.
- `build_unit_clang19/bin/test_bitcoin --run_test=key_tests,bip328_tests --log_level=test_suite`: passed; 16 selected test cases, `*** No errors detected`.
- `build_unit_clang19/src/secp256k1/bin/tests`: passed; bundled libsecp tests completed with exit code 0 after 76.758 seconds.

### Verdict and limits

Confirmed: the wrapper exposed a secret MuSig session-randomness buffer with an avoidable ordinary-stack lifetime. The secure vector fixes that lifetime without changing the libsecp API or output. This finding does not claim that libsecp internal temporaries or the public nonce need the same treatment; those are governed by libsecp contracts and public output status.

### Updated next queue

1. Audit the local `secp256k1_keypair` in MuSig partial signing, which carries secret key material through multiple failure returns.
2. Revisit compiler-visible cleanse evidence and existing ctime/checkmem coverage after the current source findings.
3. Continue with secret copies in adjacent signing, wallet, and callback paths only after searching this journal and history for duplicates.

## Cycle 42: MuSig partial-signing keypair storage

Status: confirmed and fixed in the current worktree; the focused build/tests pass and the broad unit suite has unrelated environment/configuration failures.

### Hypothesis

`CreateMuSig2PartialSig` creates a `secp256k1_keypair` after receiving a valid private key and then performs multiple aggregate, parse, tweak, session, signing, verification, and serialization operations. The keypair contains the private key and public key, but the raw local object has no cleanup on any of the many `std::nullopt` returns.

### Evidence

The public contract in `src/secp256k1/include/secp256k1_extrakeys.h` defines `secp256k1_keypair` as an opaque, copyable 96-byte structure holding a secret and public key. The implementation in `src/secp256k1/src/modules/extrakeys/main_impl.h` serializes the secret scalar into `keypair->data[0..31]` and the public key into the remaining bytes. `secp256k1_keypair_create` zeroes the structure only for an invalid creation result; it does not own or cleanse caller storage after a successful creation.

In the pre-change wrapper, `src/musig.cpp:167-168` put that object in ordinary function storage. The first subsequent failure can occur in aggregate-key validation, pubnonce lookup/size/parse, nonce aggregation, tweak application, session creation, partial signing, signature verification, or serialization. The function is reachable from `src/script/sign.cpp` for MuSig2 partial signing. The module already uses `make_secure_unique<secp256k1_musig_secnonce>()`, and `KeyPair` in `src/key.h` uses the same secure allocator for an equivalent 96-byte keypair representation.

### Change

Changed the local object to `make_secure_unique<secp256k1_keypair>()` and passed `keypair.get()` to the unchanged libsecp APIs. `SecureUniqueDeleter` invokes the existing allocator cleanup over all 96 bytes when the function returns, including every failure path. No C API, signature result, nonce invalidation order, or public behavior changes.

### Verification

- `git diff --check`: passed.
- `cmake --build build_unit_clang19 --target test_bitcoin -j2`: passed; rebuilt `src/musig.cpp` and linked `bin/test_bitcoin`.
- `build_unit_clang19/bin/test_bitcoin --run_test=key_tests,bip328_tests`: passed; 16 selected test cases, `*** No errors detected`.
- `build_unit_clang19/bin/test_bitcoin`: completed with exit code 201 after running 1190 cases. The failures were outside this change: `MempoolCheckSaturatingFeeDiagram` reported `9999 != -9223372036854765808`; a block-write test hit a `/tmp` filesystem setup error; and the configured build lacks external-signing support for an external-signer wallet test. The focused tests and compile gate remained green.
- Earlier in this cycle, `build_unit_clang19/src/secp256k1/bin/tests` passed with exit code 0 after 76.758 seconds; no libsecp source was changed by this finding.

### Verdict and limits

Confirmed: MuSig partial signing retained a private-key-bearing C structure in ordinary storage across numerous failure edges. Secure ownership is already the repository pattern for both CKey material and the equivalent KeyPair abstraction. This audit does not expand to every public or non-secret libsecp temporary; it targets the wrapper-owned keypair lifetime.

### Updated next queue

1. Revisit compiler-visible cleanse evidence and existing ctime/checkmem coverage for the three fixed buffers.
2. Audit secret copies in adjacent signing, wallet, and callback paths, searching this journal and history first.
3. Re-rank the remaining secret-lifetime surfaces using sanitizer, assembly, and call-path evidence rather than broad pattern matching.
