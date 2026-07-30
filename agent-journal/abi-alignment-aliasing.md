# Campaign #92 — abi-alignment-aliasing

Base: 5586ecb7b4 (journal commit for URGENT.md #35-c1 item on
audit/mutation-testing; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/abi-alignment (fresh). Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): kernel C-API enum/struct ABI + aliasing sweep — all three cells clean

### Draw
Random draw over the 55-goal pool (36 pending + 19 CYCLE-1; #35
excluded as just-cycled): raw=17023545825859031061, seed masked to 63
bits (7800173789004255253), index 23 -> #92.

### Cell 1: enum ABI value-fragility — REFUTED by design
Hypothesis: the btck_* C enums (SynchronizationState, Warning,
ValidationMode, ChainType, LogCategory) could silently drift from
their C++ counterparts. Fact: every mapping in
src/kernel/bitcoinkernel.cpp is a NAME-BASED switch
(cast_state:212, cast_btck_warning:225, log-category switch ~150-210,
chain-type switch ~844) — value-independent by construction; a
renumbering of either side cannot misroute. New enumerators are
covered by -Wswitch (no default case by design, with assert(false)
trailing). -Werror is off in this config, so a new value warns rather
than fails the build — upstream-intentional; the assert covers
debug/fuzz paths. No numeric-equality static_asserts exist, but none
are needed given name-mapping. Not a defect.

### Cell 2: by-value C structs — contract understood, no violation
btck_ValidationInterfaceCallbacks, btck_NotificationInterfaceCallbacks,
btck_LoggingOptions cross the ABI by value with no size/version
field; the implementation reads fields directly
(bitcoinkernel.cpp:805ff). A header/consumer drift would misread —
but libbitcoinkernel ships STATIC (lib/libbitcoinkernel.a +
include/bitcoinkernel.h, #47 c2 install-manifest cell), so the
consumer always rebuilds against the shipped header; drift is a
rebuild-time concern, and the API is explicitly experimental. All
other kernel types are opaque heap pointers — no layout contract at
all. Per the campaign bar (no contract + reachable caller), no defect.

### Cell 3: aliasing sweep — clean
All reinterpret_cast sites in serialization/crypto/primitive hot
paths are standard-permitted char/unsigned-char/std::byte aliases
(span.h:96-103 UCharCast family; transaction_identifier.h:63-65
uint256 byte views over contiguous storage). No byte*->wide-type
casts anywhere in the swept files; unaligned loads go through memcpy
(crypto/common.h ReadLE/WriteLE family).

### Verdict
DISMISSED across all three cells: the kernel C-API ABI is
opaque-pointer + name-mapped enums + static-linkage structs; the
aliasing surface is standard-permitted. No unreachable contract
claims made.

### Exact commands
- grep/sed reads: bitcoinkernel.h typedef/enum surface;
  bitcoinkernel.cpp cast_state/cast_btck_warning/category+chaintype
  switches; reinterpret_cast sweep over src/{span.h,crypto/,
  serialize.h,uint256.*,consensus/,primitives/}

### Limitations / queue
- Struct-layout regression battery (sizeof/offsetof asserts for the
  by-value C structs as an early-warning tripwire) is a possible c2
  deliverable even without a current defect — queued as a test cell.
- C++ ABI across shared-lib boundaries (libbitcoinkernel is static;
  no shared variant shipped) — out of scope until a shared build
  exists.
- Threading/lifetime contracts of the callbacks (user_data_destroy
  ordering) partially covered by #16 c2's @pre doc pass.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): by-value struct layout battery delivered (oracle O13) — compile-time mutation killed

### Draw
Re-harvested-queue draw (seed_raw=15796267858001076908,
masked=6572895821146301100, n=3, idx=0) -> abi-layout-battery ->
#92 (second cycle; c1 queue cell "struct-layout regression
battery"). Branch: audit/abi-alignment-c2 from 8965d3872d
(#63 c5 journal tip).

### Scope (from c1's enum/type map)
The public C API's by-value structs with bodies (all other btck_
types are opaque pointers — no layout to pin):
1. btck_ValidationInterfaceCallbacks (6 pointers, 48 B),
2. btck_NotificationInterfaceCallbacks (9 pointers, 72 B),
3. btck_LoggingOptions (5 ints, 20 B).

### Battery delivered (src/test/kernel/test_kernel.cpp,
btck_abi_layout_battery)
static_assert on sizeof + every field offset for all three
structs, plus runtime BOOST_CHECK echoes so the battery also
appears in test output. Compile-time tripwire form: any field
reorder/retype is a build failure, not a runtime surprise for
downstream consumers.

### Mutation verification (compile-time)
Swapped block_checked <-> pow_valid_block in the header:
static assertion failed at test_kernel.cpp:1498-1499 with the
exact offsets named; restore -> builds + full test_kernel green.

### Verdict
ORACLE DELIVERED + mutation-verified: the by-value C ABI layout
now has an early-warning tripwire; c1's enum name-map conclusion
(value-independent) extends to layout-pinned structs.

### Exact commands
- cmake --build build-before --target test_kernel -j4;
  build-before/bin/test_kernel [--run_test='*abi_layout*']
- mutation: python swap above; rebuild; restore.

### Limitations / queue
- The battery pins aarch64/x86-64 pointer sizes (8/8); a 32-bit
  build would need its own row if ever shipped (currently no such
  target in the matrix).
- Callback threading/lifetime contracts (user_data_destroy
  ordering) remain partially covered by #16 c2's doc pass.

## Rotation note
Two cycles; enum maps pinned value-independent, struct layouts
pinned by tripwire.
