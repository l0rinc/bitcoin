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
