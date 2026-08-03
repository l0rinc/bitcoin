# Random Goal Run Ledger

## Cycle 320 Selection

- Selected index: `14`
- Goal slug: `secret-control-flow`
- Goal title: Secret-dependent control-flow and memory-access audit
- Selection command: `shuf -i 0-119 -n 1`
- Catalog SHA-256: `028ce2f22b37f6b2a61fb2345915f18062fdd66143c2336c4a3a66178b363d40`
- Base commit: `6631878f49453c8857104accc914fc3661392ff8`
- Branch: `uber-cycle-320-secret-control-flow-20260802`
- Timestamp: `2026-08-03T03:23:36Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-14`

## Cycle 319 Result

- Goal 70 (`compiler-optimization-differential`) completed on branch
  `uber-cycle-319-compiler-optimization-differential-20260802`. Selection commit
  was `9f8d780825`; the journal-only handoff commit was `fd1238745d`.
- Current-source libsecp focused `ecdh`, `musig`, and `ellswift` tests passed at
  32 iterations under GCC O2, GCC LTO, and Clang 19 actual O3. Full `tests` and
  `noverify_tests` suites also passed under GCC O2 and Clang 19 actual O3 with
  fixed seeds `319010` and `319011`; all four exited 0.
- Generated-rule evidence confirmed GCC LTO flags and explained the apparent
  Release O3 mismatch: nested libsecp intentionally normalizes base Release
  O3 to O2, while `SECP256K1_APPEND_CFLAGS=-O3` produces actual final O3
  commands. No source, compiler, CMake, LTO, hardening, or constant-time defect
  was independently confirmed. No source fix was justified.
- Learning added Goal 119, `optimization-mode-flag-parity`, with seed journal
  `agent-journal/optimization-mode-flag-parity.md`. Goal/catalog commit was
  `13014e33b3` (`goals: add optimization mode parity audit`). The catalog now
  has 120 contiguous goals `0..119`; catalog SHA-256 is
  `ab29054cb7ab84cae8ca1afd6a0e43bd955650231aa219f9cd21da205da2b424`,
  manifest SHA-256 is
  `028ce2f22b37f6b2a61fb2345915f18062fdd66143c2336c4a3a66178b363d40`,
  generator SHA-256 remains
  `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and
  random prompt SHA-256 remains
  `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`.
- Limitations: no current full top-level Bitcoin build, Clang 14/PGO rerun,
  BOLT, Valgrind, or Alive2 because the relevant cells are already covered or
  unavailable and storage is critically constrained. Preserve unrelated user
  files and existing scratch/protected processes.
- Next queue: fetch/rebase, run the fresh 120-goal gate, and select exactly one
  index from `0..119`. Goal 119 should inspect generated rules and cache values
  only when a distinct mode, generator, toolchain, or source change exists.

## Cycle 319 Selection

- Selected index: `70`
- Goal slug: `compiler-optimization-differential`
- Goal title: Compiler, optimization, LTO, PGO, and BOLT differential
- Selection command: `shuf -i 0-118 -n 1`
- Catalog SHA-256: `163eef986907fafe04cc3e27c87a892ed785fcc754f706cd4f318a521516deb4`
- Base commit: `9297c44a83d4e684ab584a30c44e7b96eebb3da2`
- Branch: `uber-cycle-319-compiler-optimization-differential-20260802`
- Timestamp: `2026-08-03T03:07:05Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-70`

## Cycle 318 Selection

- Selected index: `47`
- Goal slug: `build-ci-parity`
- Goal title: Build-system and CI parity audit
- Selection command: `shuf -i 0-117 -n 1`
- Catalog SHA-256: `82fe4d3eefbf266fa38a76d9949344be6e8c454a881c0411fa567fa010c34019`
- Base commit: `39eebea9546c197d2795d1b39139e23d0000a51d`
- Branch: `uber-cycle-318-build-ci-parity-20260802`
- Timestamp: `2026-08-03T02:58:43Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-47`

## Cycle 318 Result

- The selected build/CI parity campaign compared the CMake Windows target
  graph, native MSVC helper, MinGW cross helper, workflow matrices, presets,
  and historical checker changes. The native standard job enables
  `BUILD_KERNEL_LIB=ON` and `BUILD_UTIL_CHAINSTATE=ON`; CMake has attached an
  application manifest to `bitcoin-chainstate` since its 2024 introduction.
- `.github/ci-windows.py` nevertheless skipped `bitcoin-chainstate.exe` with
  a stale `no manifest present` exception. The cross-build checker already
  validated the same executable, and the analogous `test_kernel.exe` skip was
  removed in a later kernel-test change. This was a confirmed native-only CI
  coverage defect, not a production binary defect.
- Fixed in `10aaa92d97` (`ci: validate native Windows chainstate manifest`),
  authored by `Lőrinc <pap.lorinc@gmail.com>`. The fix removes only the stale
  chainstate exclusion. `python3 -m py_compile .github/ci-windows.py
  .github/ci-windows-cross.py`, the AST/source contract harness, and
  `git diff --check` passed. A Windows `mt.exe` run is unavailable on this
  Linux host; cross-checker parity and CMake/history evidence are retained.
- Learned rule: generated or embedded artifacts need a three-way check between
  source target attachments, native/cross verifier exception lists, and
  historical target additions. Added Goal 118,
  `windows-artifact-check-parity`, with seed journal
  `agent-journal/windows-artifact-check-parity.md`.
- Goal/catalog commit: `4c35fa1f32` (`goals: add Windows artifact parity
  audit`). The catalog has 119 contiguous goals `0..118`; catalog SHA-256 is
  `163eef986907fafe04cc3e27c87a892ed785fcc754f706cd4f318a521516deb4`,
  manifest SHA-256 is
  `f18edecea1171bbf1c108c38ab601c533e36540b751459a7bf97ed64577625f5`,
  generator SHA-256 remains
  `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and
  random prompt SHA-256 remains
  `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`.
- Rebase and final gate remain pending for this cycle. `/data` has about 1.2G
  free and `/` has no free space. Protected PIDs `777094`, `956381`,
  `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` remain alive and
  untouched; unrelated untracked files remain preserved.

## Cycle 317 Selection

- Selected index: `107`
- Goal slug: `bip324-short-id-parity`
- Goal title: BIP324 short-message ID and long-form interoperability audit
- Selection command: `shuf -i 0-116 -n 1`
- Catalog SHA-256: `960a42b979b9e4c5ed2cd86f78b8f2fdb3a83fb06b97d30f67eaff07a352e7f2`
- Base commit: `23be1ca3a470a0b511a4c87e8e3ff43bfa043239`
- Branch: `uber-cycle-317-bip324-short-id-parity-20260802`
- Timestamp: `2026-08-03T02:44:33Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-107`

## Cycle 317 Result

- The matrix compared the deployed BIP324 v1.0.2 table with current Core,
  rust-bitcoin, btcd, and Core's Python v2 peer. Core's table covers IDs 1--28,
  reserved slots 29--36, and BIP434 FEATURE 37; its transport parser accepts
  both short and valid long forms. The repeated empty slots make an internally
  constructed empty message name map to reserved ID 29, which is an edge
  contract for the next harness campaign, not a proven network defect.
- Current rust-bitcoin `19436dde9ae7f56b9b999560120a66ad08958810` still has the
  Cycle 307 external gap: long zero-payload `mempool` reaches its Unknown
  decoder while short `0f` reaches MemPool. This is a recurrence at a newer
  upstream commit and was not counted as a new Core finding. Current btcd
  `05585e037ba0690572208dbc46d121a49cc0c4c9` omits compact-block IDs because
  its wire package has no corresponding message types; supported entries use
  long fallback and short decoding.
- Core's `v2transport_test` exercises only selected short IDs plus long `tx`,
  and the Python peer always emits short form for commands in `SHORTID` even
  though it can receive long form. No local production defect was proven.
- Learned suspicious surface: make BIP324 alternate-form fixtures explicit in
  both directions, cover FEATURE/version and undefined IDs, and ensure the
  test helpers can force noncanonical-but-valid long forms. Added Goal 117,
  `bip324-alt-form-harness`, with seed journal
  `agent-journal/bip324-alt-form-harness.md`.
- Evidence limits: no rustc/cargo/go are installed; the existing Core net test
  binary aborted during fixture setup because `/` and `/data` had no free
  space. Exact source snapshots and plaintext fixtures are preserved in the
  cycle journal. The known rust-bitcoin report and intentional btcd omission
  must not be rediscovered without changed behavior.

## Cycle 316

- Selected index: `106`
- Goal slug: `generated-source-boundaries`
- Goal title: Generated-source escaping and provenance boundary audit
- Selection command: `shuf -i 0-115 -n 1`
- Catalog SHA-256: `33b236d32ac47a56bd35bf3c9fff0121988df90c1c1e72dac1ee0ed12577e773`
- Base commit: `0374c23bb1989f87332ed9d9d2e90c417de86f95`
- Branch: `uber-cycle-316-generated-source-boundaries-20260802`
- Timestamp: `2026-08-03T02:27:31Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-106`

## Cycle 316 Result

- The selected generated-source campaign excluded the repaired Cycle 306
  Wycheproof and Silent Payments comment-closure defect. The distinct
  hypothesis was that libmultiprocess `mpgen` copied schema `include` and
  `includeTypes` Text annotations directly into quoted C++ header directives.
- A hostile Cap'n Proto fixture containing an escaped quote, newline, and
  declaration made the old generator return 0 and emit a valid proxy header
  containing `int generated_marker = 1;`. The generated header passed a C++20
  syntax check, proving source injection at the downstream compiler boundary.
  A second fixture isolated `includeTypes`; clean relative paths continued to
  generate and compile successfully.
- Confirmed and fixed in `f8973a2540` (`mpgen: reject unsafe include
  annotations`), authored as `Lőrinc <pap.lorinc@gmail.com>`. `mpgen` now
  rejects empty paths, quotes, backslashes, and ASCII control bytes for both
  annotation IDs before opening proxy output streams. CMake regression tests
  require the diagnostic and absence of proxy outputs; normal `mptest` output
  remains valid.
- Validation passed: `ninja -C /data/my_storage/tmp/cycle246-mp-inject
  mptest`; both `mpgen_rejects_unsafe-includes` and
  `mpgen_rejects_unsafe-include-types` via CTest; the existing `mptest` CTest;
  and `git diff --check`.
- Learned suspicious surface: `mpgen` still emits namespace, wrapper,
  exception, and renamed identifier Text annotations as C++ tokens. Added
  Goal 116, `mpgen-cpp-annotation-contract`, with seed journal
  `agent-journal/mpgen-cpp-annotation-contract.md`. Catalog/goal commit is
  `8fb04c6192` (`goals: add mpgen annotation contract audit`).
- The catalog now contains 117 contiguous goals (`0..116`); catalog SHA-256
  is `960a42b979b9e4c5ed2cd86f78b8f2fdb3a83fb06b97d30f67eaff07a352e7f2`,
  manifest SHA-256 is
  `cb1fc63e2029ce37fea901c82bbd855d382f2f3eb48ccbea8ff243fc8e2b5766`,
  generator SHA-256 remains
  `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and
  random prompt SHA-256 remains
  `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`.
- After the finding and goal commits, `git fetch origin master` followed by
  `git rebase origin/master` was a no-op. HEAD is
  `8fb04c6192bc751942c79ff9e0846d8073d67690`, origin/master is
  `556988790a7f961693a8fd93f73725baea66476a`, and divergence is `0 1458`.
  `/data` has approximately 1.3G available; protected PIDs
  `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and
  `1346200` are alive and untouched.
- Verdict: **confirmed and fixed**. After this state-close commit, run a
  fresh gate, fetch and rebase `origin/master`, draw exactly one selector with
  `shuf -i 0-116 -n 1`, create the next dedicated `uber-cycle-317-*` branch,
  and continue from Goal 116's queue.

## Cycle 315

- Selected index: `51`
- Goal slug: `invariant-differential`
- Goal title: Invariant, differential, and metamorphic audit
- Selection command: `shuf -i 0-114 -n 1`
- Catalog SHA-256: `51e561f5a1ea1c1db165416a0ff29b698a9e380279179a3878745517ca7ace7c`
- Base commit: `fdd1720c84961aaf40eb16f3bf460856b0f84f22`
- Branch: `uber-cycle-315-invariant-differential-20260802`
- Timestamp: `2026-08-03T02:04:31Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-51`

## Cycle 315 Result

- The invariant-differential queue excluded prior chain-parameter, transaction-request, layered UTXO-cache, formatter, BIP32, and Taproot cells. The distinct metamorphic hypothesis was that oversized compressed-script decoding could depend on the destination object's prior script.
- The historical OOM guard in `5d0434d13d` says an oversized script is replaced with a short invalid script, but `ScriptCompression::Unser` appended `OP_RETURN`. A deterministic 10007 encoded script-size value with exactly 10001 payload bytes made decoding into `CTxOut{1, OP_TRUE}` produce `OP_TRUE OP_RETURN`, while a fresh destination produced `OP_RETURN`. The unchanged targeted binary failed with status 201; the log is `/data/my_storage/tmp/cycle315-compress-before.log`.
- The production reachability trace covers `Coin::Unserialize`, undo records, `CDBIterator::GetValue`'s copy/commit path, `CCoinsViewDBCursor::GetValue`, and the reused `Coin` in UTXO snapshot export. The issue is bounded to malformed persisted or direct serialized input; common fresh point lookups limit normal exposure but do not justify state-dependent formatter output.
- Confirmed and fixed in `8d3a058c57` (`compressor: replace oversized scripts during decode`), authored by `Lőrinc <pap.lorinc@gmail.com>`. `ScriptCompression::Unser` now clears the destination before appending the documented replacement. The regression decodes identical bytes into fresh and pre-populated outputs. The fixed targeted case and all 8 `compress_tests` cases passed with `*** No errors detected`; after log: `/data/my_storage/tmp/cycle315-compress-after.log`.
- Learned Goal 115, `compressed-script-replacement-invariant`, with seed journal `agent-journal/compressed-script-replacement-invariant.md`. It expands the suspicious surface to malformed/truncated/noncanonical compressed-script encodings, cursor reuse, undo recovery, snapshot export, bounded consumption, and output-on-failure contracts. Goal/catalog commit is `4b56ca47b` (`goals: add malformed UTXO decode campaign`).
- The catalog now contains 116 contiguous goals (`0..115`); catalog SHA-256 is `33b236d32ac47a56bd35bf3c9fff0121988df90c1c1e72dac1ee0ed12577e773`, manifest SHA-256 is `cd14bbd499291ae3bfa358ae830f1dc28e812acaea1f0997ba601298d2e22179`, generator SHA-256 remains `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and random prompt SHA-256 remains `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`.
- No full rebuild or sanitizer run was attempted because `/data` remains full with approximately 1.3G available; the existing protected workloads remain alive and untouched. After the state-close commit, fetch and rebase `origin/master`, run a fresh gate, draw exactly one selector with `shuf -i 0-115 -n 1`, create the next dedicated branch, and continue.

## Cycle 314

- Selected index: `96`
- Goal slug: `todo-deferred-work`
- Goal title: TODO, FIXME, stub, and deferred-work challenge audit
- Selection command: `shuf -i 0-113 -n 1`
- Catalog SHA-256: `7012173cab79a6d83c1b465e41c6015bbe42c408dbee283b7191b8771404b2c5`
- Base commit: `52f7c73943d6b0b957db00f33adf2718ddb012ab`
- Branch: `uber-cycle-314-todo-deferred-work-20260802`
- Timestamp: `2026-08-03T01:41:53Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-96`

## Cycle 314 Result

- The deferred-work inventory excluded vendored dependencies and Goal 29's already-reviewed stale-code cells. The distinct live marker was the Taproot `TRDescriptor` FIXME: known `tr()` script paths were estimated as keypath-only by wallet input sizing.
- `src/script/sign.cpp` appends the selected leaf script and control block to a successful script-path witness, while `src/wallet/spend.cpp` consumes the descriptor byte bound and separately adds the witness element count. For `tr(NUMS_H,pk(NUMS_H))`, the old bound was 66 bytes and one element; the corrected bound is 135 bytes and three elements, changing the illustrative input estimate from 58 to 75 vbytes.
- Finding commit `2ac99aac74` (`descriptor: account for Taproot script path satisfaction size`) is authored by `Lőrinc <pap.lorinc@gmail.com>`. `TRDescriptor` now takes the maximum over keypath and every known leaf, including compact-size-prefixed script and depth-derived control block; `RawTRDescriptor` remains keypath-only because its tree is unknown. The commit includes the focused cycle journal and removes only the stale explanatory comments from the Taproot functional test workaround.
- The rebuilt scratch binary `/data/my_storage/tmp/cycle314-test_bitcoin` passed `descriptor_tests/taproot_script_path_satisfaction_size` and all 14 `descriptor_tests` cases. Both runs ended with `*** No errors detected`; `git diff --check` passed.
- Learned Goal 114, `taproot-satisfaction-fee-boundaries`, with seed journal `agent-journal/taproot-satisfaction-fee-boundaries.md`. Catalog/goal commit is `b6d9c2a751` (`goals: add Taproot fee-bound differential campaign`). The catalog now contains 115 contiguous goals (`0..114`); catalog SHA-256 is `51e561f5a1ea1c1db165416a0ff29b698a9e380279179a3878745517ca7ace7c`, manifest SHA-256 is `27695e51702f85048b251b0e1793037b06c9492aa6d925a7ae1564f0481821d6`, generator SHA-256 remains `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and random prompt SHA-256 remains `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`.

## Cycle 313

- Selected index: `67`
- Goal slug: `release-version-differential`
- Goal title: Release-to-release behavioral and consensus differential
- Selection command: `shuf -i 0-112 -n 1`
- Catalog SHA-256: `9704269e8b150f9f1c2d9acaa83b49dd40b862cbd15defb0e948d41099f9175d`
- Base commit: `e6ea2efbcacfb8df2fcefac60be7628bbe76ba9d`
- Branch: `uber-cycle-313-release-version-differential-20260802`
- Timestamp: `2026-08-03T01:27:51Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-67`

## Cycle 313 Result

- The selected release-differential queue excluded prior RPC, script/vector, synthetic-mainnet, reorg/restart, prune, wallet-migration, P2P-transcript, and compact-block cells. The distinct hypothesis was the BIP32 seed-length contract added after v31.1.
- v31.1 accepted a 15-byte CExtKey seed and generated a valid private key. The source-matched temporary contract test failed with exit code 201 because NonFatalCheckError was not raised; a second old-release control passed after asserting key.key.IsValid(). Current rejects both 15- and 65-byte seeds, and the permanent regression passed 2 assertions.
- The permanent test and release-differential journal were committed in ccbf9d6de6 (test: cover BIP32 seed length contract), authored as Lőrinc <pap.lorinc@gmail.com>. The current full bip32_tests suite passed 10 cases and 709 assertions. No production change was needed because current already contains upstream fix 2cf9d79d84.
- Learned Goal 113, bip32-seed-contract-parity, with seed journal agent-journal/bip32-seed-contract-parity.md. Goal/catalog/seed commit is a98d52265e (agent: add BIP32 seed contract parity goal). The catalog now contains 114 contiguous goals (0..113); catalog SHA-256 is 7012173cab79a6d83c1b465e41c6015bbe42c408dbee283b7191b8771404b2c5, manifest SHA-256 is 5fa8ce1c051d5b212d7289a5a0f234215f689d410b5b2f9046ce45ddd7d861b9, generator SHA-256 remains 297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03, and random prompt SHA-256 remains 56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2.

## Cycle 312

- Selected index: `7`
- Goal slug: `resource-exhaustion-variants`
- Goal title: Untrusted-interface resource-exhaustion variant analysis
- Selection command: `shuf -i 0-111 -n 1`
- Catalog SHA-256: `ffadac8632c7f62b066b42bcf04b77ce2bb75dca3d8f7917546f141ad289c2dc`
- Base commit: `260f3a2081b3893120c09eb8d8fe11da7802a755`
- Branch: `uber-cycle-312-resource-exhaustion-variants-20260802`
- Timestamp: `2026-08-03T01:11:54Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-7`

## Cycle 312 Result

- The selected resource-exhaustion campaign excluded all previously closed generic vector, locator, cfilter, BIP35, relay, receive-buffer, REST `getutxos`, duplicate-descriptor, and address/inventory cells. The distinct hypothesis was `GETBLOCKTXN`: `BlockTransactionsRequest` used generic vector allocation before `DifferenceFormatter` could enforce its strictly increasing `uint16_t` index domain.
- Independent pre-fix probe evidence used only a 32-byte block hash and a five-byte CompactSize count declaring 2,500,000 indexes. The old parser produced `capacity=2500000 size=1` and then failed at EOF, reserving 5,000,000 bytes from a 37-byte message. The fixed probe rejected the count with `Vector length limit exceeded`, leaving `capacity=0 size=0`.
- Confirmed and fixed in `b8ae707f47399124c65b42e56423f124cfb9a706` (`p2p: bound block transaction request indexes`), authored as `Lőrinc <pap.lorinc@gmail.com>`. `BlockTransactionsRequest` now uses `LimitedVectorFormatter<65536, DifferenceFormatter>`; the regression verifies oversized-count rejection, empty output, and zero capacity.
- Validation: the existing scratch build `/data/my_storage/tmp/cycle246-wallet` rebuilt `test_bitcoin`; the focused regression passed 1 case and 3 assertions; the full `blockencodings_tests` suite passed 31 cases and 372 assertions; and `git diff --check` passed. No fresh daemon socket test was run because the host filesystems are full and protected workloads were preserved.
- Learned suspicious surface: custom difference, varint, CompactSize, and related vector formatters may expose the same mismatch between declared count, representable element domain, allocation timing, output-on-failure, and malformed-input accounting. Added Goal 112, `difference-formatter-input-bounds`, with seed journal `agent-journal/difference-formatter-input-bounds.md`.
- Goal/catalog/seed commit: `d90c20520861a1ea728e28e2611b910a93371403` (`agent: add formatter boundary audit goal`). The catalog now contains 113 contiguous goals (`0..112`); catalog SHA-256 is `9704269e8b150f9f1c2d9acaa83b49dd40b862cbd15defb0e948d41099f9175d`, manifest SHA-256 is `4507b42251321675eca0bde823d8b9256afa0345c5fbcbb5adf7d3e0df727767`, generator SHA-256 is `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and random prompt SHA-256 is `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`.

## Cycle 311

- Selected index: `17`
- Goal slug: `build-matrix-modules`
- Goal title: Build-matrix and module-configuration audit
- Selection command: `shuf -i 0-110 -n 1`
- Catalog SHA-256: `77cacbe449eee1955686e5fbebac74b1db6c85fb9270bc84e0e3aec609a06b52`
- Base commit: `9ac0b27e2ec97d612941a9c9fdd1ab33c8f66edf`
- Branch: `uber-cycle-311-build-matrix-modules-20260802`
- Timestamp: `2026-08-03T01:02:47Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-17`

## Cycle 311 Result

- The selected build-matrix-modules campaign excluded prior wallet/IPC/GUI-off and reduced-export cells and focused on the independently configurable kernel-library and chainstate options. The concrete hypothesis was that `BUILD_UTIL_CHAINSTATE=ON` with `BUILD_KERNEL_LIB=OFF` could advertise and test a target that CMake did not generate.
- Pre-fix configure in `/data/my_storage/tmp/cycle311-chainstate-without-kernel-1` exited 0 and printed `bitcoin-chainstate ... ON` with `libbitcoinkernel ... OFF`. It generated `ENABLE_BITCOIN_CHAINSTATE=true`, but target help contained no chainstate or kernel target; requesting `bitcoin-chainstate` exited 1 with `ninja: error: unknown target 'bitcoin-chainstate'`. History traced the mismatch to `7990463b105`, which nested the pure chainstate executable under the kernel-library guard while retaining independent options.
- Confirmed and fixed in `657e4c64eb` (`cmake: reject chainstate without kernel library`), authored by `Lőrinc <pap.lorinc@gmail.com>`. `CMakeLists.txt` now rejects the impossible combination during option interaction with `BUILD_UTIL_CHAINSTATE requires BUILD_KERNEL_LIB=ON.` The repaired invalid cell in `/data/my_storage/tmp/cycle311-chainstate-without-kernel-2` exits 1 before producing a misleading summary or capability file.
- Independent valid-cell verification in `/data/my_storage/tmp/cycle311-chainstate-valid-1` with both options ON exits 0, prints both capabilities ON, writes `ENABLE_BITCOIN_CHAINSTATE=true`, and exposes `bitcoin-chainstate`, `bitcoinkernel`, `libbitcoinkernel`, and `libbitcoinkernel.a` in target help. `git diff --check` passed. A full compile was not attempted because `/data` and `/` are full; the configuration graph is sufficient for this guard defect and protected workloads were preserved.
- Learned suspicious surface: the kernel library, chainstate utility, kernel tests, install components, pkg-config/header exports, functional capability flags, Windows/cross recipes, and fuzz/reduced-export overrides may still diverge. Added Goal 111, `kernel-chainstate-config-parity`, with seed journal `agent-journal/kernel-chainstate-config-parity.md`.
- Goal/catalog/seed commit: `eefdbe5211` (`goals: add kernel chainstate configuration parity audit`). The catalog now contains 112 goals with IDs `0..111`; catalog SHA-256 is `ffadac8632c7f62b066b42bcf04b77ce2bb75dca3d8f7917546f141ad289c2dc`, manifest SHA-256 and the next exact selector will be recorded at cycle close. No sanitizer, cross-compiler, or full kernel build was run because of storage limits.

## Cycle 310

- Selected index: `15`
- Goal slug: `public-object-validation`
- Goal title: Public object parsing and validation variant analysis
- Selection command: `shuf -i 0-109 -n 1`
- Catalog SHA-256: `0f3f6c11ee008c76cb88250fdbc6f6abd713e72206f1c3b94daeb8e9983ed172`
- Base commit: `199365c2793ca9ece8692dc65946864a5fe1d1d8`
- Branch: `uber-cycle-310-public-object-validation-20260802`
- Timestamp: `2026-08-03T00:17:20Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-15`

## Cycle 310 Result

- The selected public-object-validation queue excluded the previously fixed compact-header, Taproot x-only, descriptor-inference, PSBT serialized-key, direct-xpub, and raw-P2PK cells. The remaining direct parser variants were checked against history and compatibility tests. The concrete new hypothesis was that `CExtPubKey::operator<` treated only public key and chain code as ordered-container identity even though `operator==` also includes depth, parent fingerprint, and child number.
- A valid deterministic xpub fixture cloned one BIP32 vector key and changed only depth to `1`, fingerprint to `11 22 33 44`, and child to `1`. The old `std::set<CExtPubKey>` collapsed the unequal objects, and the old `std::map<CExtPubKey, CExtKey>` update pattern used by `gethdkeys` retained one key while overwriting its private-value metadata. Production descriptor parsing of `wsh(multi(2,xpub1,xpub2))` likewise extracted one xpub. The pre-fix regression expectation failed with `1 != 2` for the set and private map.
- Confirmed and fixed in `7bc2afcf91` (`pubkey: preserve extended key metadata in ordered containers`), authored by `Lőrinc <pap.lorinc@gmail.com>`. The comparator now orders public key, chain code, depth, fingerprint, and child number, matching `operator==`; version bytes remain deliberately excluded from the generic equality and comparator, while PSBT retains its complete serialized-key comparator. The permanent BIP32 regression also checks neutering each returned xprv back to the matching xpub and descriptor extraction of both keys.
- Repaired validation: `TMPDIR=/data/my_storage/tmp/cycle310-public-object ninja -C /data/my_storage/tmp/cycle246-wallet test_bitcoin -j4` exited 0; the focused `bip32_tests/extpubkey_metadata_identity` passed with no errors; and `bip32_tests,descriptor_tests,psbt_tests,wallet_rpc_tests` passed 38 selected cases with seed `31003` and no errors. `git diff --check` passed. No sanitizer, 32-bit, or functional daemon run was attempted because `/data` and `/` remain full; existing protected workloads were preserved.
- Learned suspicious surface: extended-key identity differs by context between derivation-material grouping, complete BIP32 metadata, complete wire serialization, and key-origin output. Added Goal 110, `extpubkey-identity-matrix`, with seed journal `agent-journal/extpubkey-identity-matrix.md`. The catalog now contains 111 contiguous goals (`0..110`); catalog SHA-256 is `77cacbe449eee1955686e5fbebac74b1db6c85fb9270bc84e0e3aec609a06b52`, manifest SHA-256 is `8e0263e80bfad7fe1fba978f76637b78d51c411c422cc14252dd723b92be1714`, random prompt SHA-256 is `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`, and generator SHA-256 is `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`.

## Cycle 309

- Selected index: `94`
- Goal slug: `bindings-ffi-parity`
- Goal title: Bindings, FFI, and language-wrapper parity audit
- Selection command: `shuf -i 0-108 -n 1`
- Catalog SHA-256: `6284d0369462c9c426d557943b9c4b71fd20e06658f7993aba04f1811ecb686a`
- Base commit: `d8bb0bf76632b6436aaea56b54e526db9d5b3363`
- Branch: `uber-cycle-309-bindings-ffi-parity-20260802`
- Timestamp: `2026-08-03T00:01:37Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-94`

## Cycle 309 Result

- Finding: `btck_chainstate_manager_import_blocks` accepted malformed raw C
  path arrays. With count `1`, a null path entry and a null lengths array both
  returned success instead of `-1`; the implementation also used a `uint32_t`
  loop index for a public `size_t` count.
- Verdict: confirmed and fixed in `d3dc50785a` (`kernel: validate import path
  arrays`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Fix: reject missing arrays for nonzero counts, reject null path elements,
  iterate with `size_t`, and use `fs::PathFromString` for length-delimited
  paths. The C++ wrapper's valid vector conversion remains unchanged.
- Before evidence: the focused kernel test with only the regression added
  exited `201`; both new checks observed `0` instead of `-1` and `9/11`
  assertions passed.
- After evidence: the focused kernel test passed `11/11` assertions; the full
  kernel suite passed 20 cases and `3726/3726` assertions. The build was
  `/data/my_storage/tmp/cycle278-kernel-build` with `CCACHE_DISABLE=1`.
- Learned suspicious surface: raw `libbitcoinkernel` C consumers can express
  nullability, callback, borrowed-lifetime, status/exception, and width states
  that the C++ wrapper normalizes away. Added Goal 109,
  `kernel-c-abi-boundary-matrix`, with seed journal
  `agent-journal/kernel-c-abi-boundary-matrix.md`.
- Goal/catalog/seed commit: `f36360f865` (`goal: add kernel C ABI boundary
  matrix`). Catalog now contains 110 goals with IDs `0..109`; catalog SHA-256
  is `0f3f6c11ee008c76cb88250fdbc6f6abd713e72206f1c3b94daeb8e9983ed172`,
  manifest SHA-256 is
  `ce1280118624afc5fc58847e7b59bc3cabb3a08eeb978748df67f7e0bc5013f4`,
  generator SHA-256 is
  `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`, and
  random-run prompt SHA-256 is
  `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`.
- Limitations: no sanitizer or 32-bit build was run because `/data` and `/`
  are full and protected long-running jobs remain alive. Scratch state is
  isolated under `/data/my_storage/tmp/cycle309-kernel-*`.

## Cycle 308

- Selected index: `6`
- Goal slug: `serialization-untrusted-input`
- Goal title: Serialization, deserialization, and untrusted-input sweep
- Selection command: `shuf -i 0-107 -n 1`
- Catalog SHA-256: `633bb1216c6ddf55f6e4bdeda2a99dfb3eb8bb1878b0e6370dd49521c05069c9`
- Base commit: `49b264dd560a024702c81ca5c3493e180e5322d3`
- Branch: `uber-cycle-308-serialization-untrusted-input-20260802`
- Timestamp: `2026-08-02T23:36:21Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-6`

## Cycle 308 Result

- Finding: `wallet::DatabaseBatch::Read()` deserialized persisted values
  directly into caller-owned objects. The exact malformed value
  `01 00 00 00 02 03` changed a sentinel `PartiallyDecoded::first` to `1`
  before returning `false` in both SQLite and in-memory SQLite backends.
- Verdict: confirmed and fixed in `72c003fef9` (`walletdb: preserve outputs on
  decode failure`), authored as `Lőrinc <pap.lorinc@gmail.com>`.
- Fix: decode into a temporary, value-initialize trivially default-constructible
  outputs, and commit only after successful deserialization. The focused
  regression passed 16/16 assertions after the fix; the combined
  `db_tests,walletdb_tests,walletload_tests` run passed 12 cases and 643
  assertions.
- GCS candidate: the local block-filter index skips semantic Golomb-Rice
  validation after checking its coupled database hash. A huge-N payload is a
  learned bounded-work/recovery hypothesis, not a current finding, because
  ordinary one-sided file corruption fails the stored hash and a malformed
  hash-consistent record requires coupled local corruption or a writer defect.
- Learned suspicious surface: persisted GCS filter payload validation and
  file/LevelDB recovery symmetry. Added contiguous Goal 108,
  `persisted-gcs-filter-validation`, with seed journal
  `agent-journal/persisted-gcs-filter-validation.md`.
- Goal/catalog/seed commit: `7edd1f9112` (`goal: add persisted GCS filter
  validation campaign`). Catalog now contains 109 goals with IDs `0..108`;
  catalog SHA-256 is `6284d0369462c9c426d557943b9c4b71fd20e06658f7993aba04f1811ecb686a`,
  manifest SHA-256 is `280600d2c11b4437ff7d0bccee1211231aaa7c3620075cf03c0ecb08b7562c68`,
  generator SHA-256 is `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`,
  and random-run prompt SHA-256 is
  `56f2d4093caa99fcc54c8709bd18b5548228bde2d96a2b485ab9fe3a1cd55c2`.
- Validation limitation: `/data` and `/` are full, so no sanitizer rebuild or
  broad wallet suite was attempted; protected long-running jobs were kept
  alive and all scratch state is isolated under `/data/my_storage/tmp/cycle308-*`.

## Cycle 304

- Selected index: `52`
- Goal slug: `integer-overflow`
- Goal title: Integer overflow, narrowing, signedness, and division audit
- Selection command: `shuf -i 0-103 -n 1`
- Catalog SHA-256: `6ab27a9d21d866210694348713a6cecc2c4bf407fa4615986c8f482fba95747f`
- Base commit: `0f220b92529f28210a09ad965f8c49eff29b9297`
- Branch: `uber-cycle-304-integer-overflow-20260802`
- Timestamp: `2026-08-02T22:27:05Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-52`

## Cycle 304 Result

- Finding: `-maxconnections=4294967296` narrowed from `int64_t` to `int`,
  became zero automatic connections, and allowed startup; a representable
  `INT_MAX` value also made the old file-descriptor request exceed the `int`
  API domain.
- Verdict: confirmed and fixed.
- Finding commit: `a6abfded86` (`init: reject overflowing maxconnections`)
- Focused validation: `node_init_tests`, 4 cases and 6 assertions passed.
- Independent validation: Clang 19 `implicit-conversion` build completed;
  oversized direct startup returned the explicit range error, and `INT_MAX`
  startup completed with the expected system-limit reduction. The broad
  sanitizer configuration also emitted unrelated pre-existing libsecp256k1,
  crypto, and CRC32C diagnostics; none referenced `src/init.cpp`.
- Learned suspicious surface: integer option narrowing in `-par`, duration
  options, wallet fee sizes, and block-filter/index height arithmetic.
- Added contiguous goal: `104` (`integer-option-boundaries`)
- Goal/seed/catalog commit: `d93bc4098e`
- Catalog count after extension: 105 goals, IDs `0..104`
- Catalog SHA-256: `3b62db081945f5375ac7f152e31ace458c1c807f89380f79ac088944aecc3ffa`
- Manifest SHA-256: `6f7281d74b0f621ff0f28b50813ebcedeedf0ff3c66bc6e4cebe7aee9e47fd42`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`

## Cycle 307

- Selected index: `55`
- Goal slug: `alternative-implementation`
- Goal title: Alternative-implementation compatibility-difference audit
- Selection command: `shuf -i 0-106 -n 1`
- Catalog SHA-256: `fb4f3f314db4d15105120db4109ddb2bcfda208e26290a400ddb9028644d7a62`
- Base commit: `bf8b74bacfde5bb19be05bb960acbedf68b54afa`
- Branch: `uber-cycle-307-alternative-implementation-20260802`
- Timestamp: `2026-08-02T23:19:53Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-55`

## Cycle 307 Result

- Finding: the pinned rust-bitcoin V2 message decoder recognizes BIP324 short IDs 1-28, but its long-command dispatcher routes optimized messages such as `mempool`, `tx`, `cmpctblock`, and `blocktxn` to `NetworkMessage::Unknown`. The valid long-form `mempool` fixture is `00 6d 65 6d 70 6f 6f 6c 00 00 00 00 00`; short form `0f` decodes as `MemPool`.
- Core verdict: dismissed. Core's reserved short IDs 29-36 were intentionally added as ignored extension slots by `6a129983c9b`; ID 37 is BIP434 FEATURE and is version-gated. btcd's missing compact-block IDs are an unsupported-message boundary because the pinned wire package has no compact-block message types.
- External verdict: report-ready rust-bitcoin long-form interoperability gap at pinned commit `607e8b2fe0d8f1ebe06923dbbc0ca6afdf00d1d1`; no local source/test change justified.
- Learned suspicious surface: BIP324 short/long message-type parity, extension-ID freshness, bidirectional wire fixtures, and version-gated message registries. Added Goal 107, `bip324-short-id-parity`, with seed journal `agent-journal/bip324-short-id-parity.md`.
- Verification: BIP324 v1.0.2 specification, Core source/history, rust-bitcoin source, and btcd source inventory. Rust/Go execution was unavailable. Core `net_tests` execution was blocked by full `/` and `/data` filesystems during chain fixture setup.

## Cycle 306 Result

- Finding: all three libsecp256k1 vector generators copied external JSON
  comments into C block comments without escaping `*/`. Minimal hostile
  ECDSA, ECDH, and Silent Payments fixtures emitted a live `int
  generated_marker` into the generated test-vector initializer and failed C
  syntax compilation before the fix.
- Verdict: confirmed and fixed.
- Finding commit: `1ec5c95460` (`secp256k1: sanitize generated vector comments`)
- Fix: shared `sanitize_c_comment()` replaces `*/` with `* /`; all three
  generators use it, and `tools/wycheproof_utils.py` is now listed in
  `Makefile.am` distribution inputs.
- Focused verification: hostile generated headers compiled under `cc
  -std=c11 -fsyntax-only` after the fix; all three returned `compile=0`.
- Regeneration verification: ECDSA, ECDH, and Silent Payments production
  headers remained byte-identical before and after the fix. Hashes were
  `1e3c11ff4c5c83cbd0d79b3ede6a47309e2074f0f9432f3aef09e3bb2c9004c5`,
  `040085b0859e4cc41105bfecec825c76c616fe89a703f8221807c6433ba9f3d2`, and
  `8d88aead1f2f359aca31ac8c803001c55b1231be187839cafbe9c6959cedbbcc`.
- Consumer validation: Clang 19 and GCC ECDSA/Silent Payments modules passed;
  a fresh Clang 14 Release ECDH-enabled CMake build passed all ECDH tests,
  including `test_ecdh_wycheproof`.
- Learned suspicious surface: generated-source escaping and provenance across
  C/C++, Rust, shell, manpage, build, and metadata generators. Extend the
  catalog with goal `106`, `generated-source-boundaries`, and seed journal
  `agent-journal/generated-source-boundaries.md`.
- Goal/catalog commit: `72c09f6e2c` (`goal: add generated source boundary campaign`)
- Catalog count after extension: 107 goals, IDs `0..106`
- Catalog SHA-256: `fb4f3f314db4d15105120db4109ddb2bcfda208e26290a400ddb9028644d7a62`
- Manifest SHA-256: `5769fb6a16ca00af236d50c081377375d6c1bb1a2642fab182b4b7e99bc18573`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`

## Cycle 305

- Selected index: `69`
- Goal slug: `backend-differential`
- Goal title: SIMD, assembly, and portable-reference backend differential
- Selection command: `shuf -i 0-104 -n 1`
- Catalog SHA-256: `3b62db081945f5375ac7f152e31ace458c1c807f89380f79ac088944aecc3ffa`
- Base commit: `54afa66613a649a55974ec83c67efd12cb02052a`
- Branch: `uber-cycle-305-backend-differential-20260802`
- Timestamp: `2026-08-02T22:50:50Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-69`

## Cycle 306

- Selected index: `39`
- Goal slug: `deterministic-artifacts`
- Goal title: Generated-artifact and test-vector determinism audit
- Selection command: `shuf -i 0-105 -n 1`
- Catalog SHA-256: `ae927e6bca7b2406e318ac893962e481862f22ff46443228e6908131ff7dca13`
- Base commit: `ed71bceff261929dc735765286c95445bb89327d`
- Branch: `uber-cycle-306-deterministic-artifacts-20260802`
- Timestamp: `2026-08-02T23:07:27Z`
- Prompt source: `agent-goals/REUSABLE_AGENT_GOALS.md#goal-39`

## Cycle 305 Result

- Finding: `minisketch_decode()` narrowed its public `size_t max_elements`
  through `int`, and the decoder then evaluated signed `1 + max_count`.
  Generic and CLMUL builds returned `-1` for a valid one-element sketch when
  the caller supplied `INT_MAX`, `INT_MAX + 1`, or `SIZE_MAX`.
- Verdict: confirmed and fixed.
- Finding commit: `9a8cf446ba` (`minisketch: preserve large decode bounds`)
- Focused verification: generic and CLMUL external probes returned element 7
  for all three large bounds; pre-fix ASan/UBSan replay independently reported
  signed overflow at `src/minisketch/src/sketch_impl.h:401` in both backends.
- Broad validation: normal and sanitized no-VERIFY/VERIFY Minisketch suites
  passed at complexities 2 and 4 in generic and CLMUL trees.
- Learned suspicious surface: Minisketch serialized-size multiplication,
  capacity/max-elements arithmetic, and decode return counts.
- Added contiguous goal: `105` (`minisketch-api-size-arithmetic`)
- Goal/seed/catalog commit: `14d0a8782f`
- Catalog count after extension: 106 goals, IDs `0..105`
- Catalog SHA-256: `ae927e6bca7b2406e318ac893962e481862f22ff46443228e6908131ff7dca13`
- Manifest SHA-256: `233d8f1d52e98a4a3d8d134df4e7bd35792af2ea6814d5af02185cf8fdc20510`
- Generator SHA-256: `297256d5dc173c5be13ed1d1021d161576d319b12fe86d8711c5c3c6bedf2b03`
- Random-run prompt SHA-256: `56f2d4093caa99fcc54c8709bd18b55482208bde2d96a2b485ab9fe3a1cd55c2`
