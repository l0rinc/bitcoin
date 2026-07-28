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
