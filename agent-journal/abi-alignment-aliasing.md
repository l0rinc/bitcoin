# ABI layout, alignment, aliasing, and object-lifetime audit

## Cycle 268 completion

- Selector protocol: the fresh gate fetched `origin/master` before branch creation. The exact `shuf -i 0-98 -n 1` draw was `92` (`abi-alignment-aliasing`); distinct evidence remained open, so no reroll was needed. The dedicated branch is `uber-cycle-268-abi-alignment-aliasing-20260802`.
- Gate HEAD was `c556caf414c143479ae6ca0163944fd182bb0a09`; fetched `origin/master` was `556988790a7f961693a8fd93f73725baea66476a`; merge-base was `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence was `1325 45` (`HEAD...origin/master`). Existing untracked agent/user artifacts were preserved. The Cycle 106 `prevector`, native kernel callback-layout, KeyPair-cast, and `NoDestructor` cells were excluded, as were detached prior C API index/flag/array fixes. The existing `write_bytes` null-exception candidate from the bindings audit was also kept separate.
- Fresh hypothesis: `btck::write_bytes` in `src/kernel/bitcoinkernel_wrapper.h` forms `first + len` before considering whether the callback was asked to write zero bytes. Valid empty script, scriptSig, and witness-item paths can pass `len == 0` with an empty `std::vector`'s `data()` result. The old code therefore relied on a pointer-arithmetic operation whose zero-length case still requires a valid pointer value under the C++ object/lifetime contract.
- Independent evidence: GCC 12/libstdc++ and Clang 19/libstdc++ probes both returned `(nil)` for `std::vector<unsigned char>{}.data()`. Source tracing found empty witness items and empty scriptSig already reach this callback; the new `empty_script.ToBytes()` assertion makes the empty script path explicit. The pre-fix Clang 19 kernel test passed its existing empty-input behavior, and the available UBSan build did not diagnose the null-plus-zero expression, so this is a source-level portability/undefined-pointer-arithmetic finding rather than a reproduced crash.
- Fix: return immediately from the callback when `len == 0`, before converting or incrementing `buffer`. This preserves the empty output and exception behavior while avoiding pointer formation entirely. Add `BOOST_CHECK(empty_script.ToBytes().empty())` to `btck_script_pubkey` as a regression oracle; the existing empty witness-item and empty scriptSig checks cover the related paths.
- Verification: `cmake --build /data/my_storage/tmp/cycle107-kernel-clang19 --target test_kernel -j2` rebuilt successfully with Clang 19. The focused `btck_script_pubkey,btck_transaction_input` run passed 2 cases and 108/108 assertions. The full run initially hit the host's full `/tmp`; rerunning with `TMPDIR=/data/my_storage/tmp/cycle268-kernel-test` passed all 19 cases and 3719/3719 assertions. `git diff --check` passed.
- Classification: confirmed local ABI/object-lifetime contract risk in a public kernel wrapper, fixed without changing observable serialization. No 32-bit or alternate-standard-library execution was available. The sanitizer non-diagnostic result is a limitation, not evidence that the old pointer expression is portable. No claim is made that every ABI portability cell is closed.
- Handoff: the enclosing source/test commit records this finding and journal. After its commit, create the separate Cycle 268 state close, refresh the gate, draw a new goal with the exact selector protocol, and continue with remaining ABI cells. Do not repeat the empty-buffer callback cell without a new implementation, compiler/library, or recurrence boundary.

## Cycle 106 start

- Selector protocol: the first exact `shuf -i 0-98 -n 1` draw was `5`, which repeated the already-closed Cycle 102 boundary audit without new evidence; the recorded no-repeat reroll was `92` (`abi-alignment-aliasing`).
- Branch: `uber-cycle-106-abi-alignment-aliasing-20260729`.
- Cycle-start HEAD: `0c48a81989d802c59a8455caef49e21c8ec66285`; `origin/master`: `9611a356035be531d62bfc40879f388d5dc359c4`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `origin/master...HEAD` is `40 1001`.
- Gate: tracked/index state was clean, `git diff --check` passed, catalog hashes matched `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, and `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and known untracked artifacts were preserved. PID `777094` (`cycle93-build/bin/test_bitcoin --run_test=wallet_tests`) remained untouched.
- Scope: current Bitcoin Core and bundled libsecp256k1 packed layouts, unions, casts, placement construction, raw storage, over-alignment, strict aliasing, pointer provenance, object lifetime, and C/C++ ABI boundaries. Prior Cycle 102 boundary cells, Cycle 105 compiler/IPO output parity, prior libsecp backend matrices, and known binding-only work are excluded unless this cycle finds a distinct ABI or lifetime mechanism.
- Initial hypotheses: (1) a production raw-storage or cast path violates alignment, lifetime, or aliasing requirements on a reachable input; (2) a packed/union layout is consumed through a stronger alignment or type-punning assumption than its contract permits; (3) a public or internal C/C++ ABI has size, offset, calling-convention, or ownership drift across supported build modes; (4) sanitizer/compiler diagnostics are suppressed or skipped around a current defect.
- Evidence protocol: inventory concrete sites before testing; state the required object/alignment/ABI invariant; use compiler warnings, UBSan alignment/object-size, ASan, layout probes, and cross-compiler generated-size/offset comparisons; then reproduce the first invalid operation or prove the contract. Use scratch files and builds under `/data/my_storage/tmp`, never default datadirs, wallets, keys, or production databases.

## Cycle 106 evidence

### Inventory and rejected hypotheses

- `src/prevector.h` uses a packed union for inline/heap storage, then explicitly aligns the union to `char*` and statically requires `T` to fit the pointer alignment. The standalone `uint64_t`/`uint32_t` probe exercised direct storage, heap transition, insert, erase, resize, and shrink-to-fit. GCC 12 and Clang 19 builds with `-fsanitize=undefined,alignment,object-size -O1` both exited 0 and printed `prevector alignment/lifetime probe passed`.
- `src/kernel/bitcoinkernel.h` callback records were compiled independently as C and C++ with GCC 12 and Clang 19. All four outputs matched: validation size/alignment `48/8`, offsets `0,8,16,24,32,40`; notifications `72/8`, offsets `0,8,16,24,32,40,48,56,64`; logging `20/4`, offsets `0,4,8,12,16`. Native ABI drift was not reproduced. `-m32`/i686 syntax checks were blocked before parsing by missing multilib libc and C++ headers (`bits/libc-header-start.h` and `bits/c++config.h`).
- `src/key.cpp`'s `std::array<unsigned char, 96>` to `secp256k1_keypair*` cast remains a language-lawyer concern because the object is an opaque byte representation. GCC 12 with the production include paths and `-O2 -fstrict-aliasing -Wstrict-aliasing=3 -Wcast-align=strict -fsyntax-only` emitted no diagnostic, and the Clang UBSan build compiled and exercised the keypair path without an alignment or object-size report. This is recorded as no actionable runtime defect for this cycle, not as proof that every abstract-machine interpretation is settled.
- `src/leveldb/util/no_destructor.h` uses placement `new` in its constructor. The header included `<cstddef>`, `<type_traits>`, and `<utility>`, but not the declaration header for placement `operator new`. A direct translation unit including only this header and constructing an over-aligned instance failed under both compilers before the fix:

  - Clang 19: `error: no matching 'operator new' function for non-allocating placement new expression; include <new>`.
  - GCC 12: `error: no matching function for call to 'operator new(sizetype, char [64])'`.

  The existing LevelDB test includes `<utility>` first, which masked this omission through a transitive include.

### Finding ND-001: no_destructor.h omitted placement-new declaration

- Hypothesis: a consumer that includes `no_destructor.h` directly can fail to compile, and the header is not self-contained for its own placement-new expression.
- Trust boundary: source/header consumers and supported C++ compilers; no runtime input is required.
- Reproducer: `clang++-19 -std=c++20 -Wall -Wextra -Werror -fsanitize=undefined,alignment,object-size -fno-omit-frame-pointer -O1 -I. agent-journal/abi_cycle106_storage_probe.cpp ...` and the equivalent `g++-12` command, where the probe constructs `leveldb::NoDestructor<alignas(64) OverAligned>`.
- Fix: add `#include <new>` to `src/leveldb/util/no_destructor.h`.
- Independent verification: after the include, both commands compiled and the executables exited 0 with `NoDestructor over-alignment/lifetime probe passed`. The full isolated Clang 19 UBSan `test_bitcoin` target rebuilt and relinked successfully (`ninja -C /data/my_storage/tmp/cycle106-clang19-ubsan test_bitcoin -j8`, 6 incremental steps after the header edit). Post-fix focused runs passed: `key_tests,prevector_tests` 16 cases/981 assertions; `allocator_tests,dbwrapper_tests` 17 cases/2522 assertions. Earlier in the same build, `util_tests` (79/3991), `coins_tests` (37/1,218,037), and `netbase_tests,span_tests` (29/690) also passed with `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- Classification: confirmed local header self-containment/build portability defect. No unrelated production behavior changed.

## Cycle 106 completion

- Source finding: ND-001 is ready for one self-contained commit; no other source change is justified.
- Dismissed for this cycle: native C/C++ callback layout drift, prevector alignment/lifetime failure, and an actionable KeyPair runtime failure.
- Limitation: 32-bit ABI execution/compilation could not be performed because this host lacks multilib headers. Windows-specific `SingletonEnv` execution was not available on this Linux host.
- Handoff: after committing ND-001 and the journal, refresh the repository gate, preserve all pre-existing untracked artifacts, select a new goal with the exact `shuf -i 0-98 -n 1` protocol, and continue the uber loop. Search this journal before revisiting the same header omission or the native callback-layout cell.
