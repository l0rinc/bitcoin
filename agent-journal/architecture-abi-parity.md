# Campaign #68 — architecture-abi-parity

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/arch-abi. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): platform-skip inventory + endianness-sensitive decoder audit — clean (static evidence)

### Draw
Random draw over the 42-goal eligible pool: raw=17321715160289371101,
index 31 -> #68.

### Evidence class (per campaign labeling)
STATIC / compile-only. QEMU user-mode and any x86 sysroot are absent
on this aarch64 host (verified: no qemu-x86_64, no
/usr/x86_64-linux-gnu); this host IS the non-x86 config (Cortex-A76),
so native ARM64 execution is itself one of the campaign's cells.

### Cells
1. Platform-only skipped tests: grep for arch guards
   (#if __x86_64__/__aarch64__/__i386__/SSE-macros) across src/test and
   src/wallet/test: ZERO hits. The unit suite is arch-neutral by
   construction — backend selection is runtime-dispatched
   (SHA256AutoDetect), not compile-gated, so nothing is silently
   skipped on ARM64.
2. ASMap decoder (util/asmap.cpp, consumes the embedded binary blob):
   bit-level decoding via ConsumeBitLE over std::byte spans (no
   multi-byte reinterpret/memcpy reads anywhere in the file) —
   endian-free by construction; uint32_t throughout with an explicit
   int64_t cast for the jump-vs-EOF comparison (:199) — width-safe on
   32/64-bit; endpos = size*8 unoverflowable at 1.5MB.
3. Obfuscation/blk XOR (session-verified during #30 c4): byte-wise
   position-based XOR — endian-free.
4. uint256/base_blob: byte-array storage with explicit BE/LE helpers
   (no native-layout exposure in serialization) — endian-free by
   design.

### Verdict
- DISMISSED: no arch-conditional test skips; the audited binary
  decoders are endian- and width-safe. Native ARM64 execution
  (this session's ~40 green suites) is the executed evidence for the
  ARM64 cell; x86 cells remain unevidenced on this host.

### Limitations
- No executed x86_64/i686/big-endian evidence: needs QEMU+sysroot or
  CI. Big-endian (s390x) is the remaining blind spot for the XOR/
  asmap cells despite their byte-wise design.
- Char signedness not swept tree-wide (decoder-scope only: std::byte
  everywhere audited).

### Exact commands
- `grep -rn '#if.*__x86_64__\|#ifdef __aarch64__\|SSE' src/test src/wallet/test` (0)
- reads: util/asmap.cpp (full), init.cpp XOR sites, uint256.h

### Next queue for this campaign
- If QEMU lands: run test_bitcoin crypto_tests + a regtest chain
  replay under qemu-x86_64 and diff against native (serialized output
  parity).
- s390x CI cell for the byte-wise decoders (artifact-only here).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): char-signedness sweep — all production plain-char uses sign-safe by construction

Base: 8fe7642936 (journal commit for #37 cycle-2 on
audit/build-dead-zones-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/arch-abi-c2 (c1 journal carried). Start
state: clean (untracked scratch only).

### Draw
Random draw over the 30-goal pool (17 pending + 13 CYCLE-1; #57
excluded as just-cycled): raw=2005772117516005166, index 26 -> #68.
c1's open cell: "Char signedness not swept tree-wide (decoder-scope
only)". QEMU still absent on this host (checked).

### Sweep (production src, subtrees excluded)
char on aarch64-gcc is unsigned; on x86-64-gcc signed. Any code
whose behavior depends on the sign of plain char is an arch
differential. Full sweep for plain-char variables in arithmetic or
signed comparisons:
- serialize.h:241-264: plain-char serialization is statically
  FORBIDDEN (deleted overloads) — the project's own guard.
- crypto/hex_base.cpp:64-68: HexDigit indexes
  p_util_hexdigit[(unsigned char)c] — platform-identical indexing.
- util/strencodings.cpp: decode64/32 tables index uint8_t(c);
  ProcessMantissaDigit takes ASCII-only paths; IsHex's
  HexDigit(c) < 0 test is safe via the unsigned-cast index above.
- Remaining plain-char uses: buffers (buf[4096] et al.), thread-name
  arrays, set_perm letter — sign-insensitive.

### Verdict
- DISMISSED: no sign-sensitive plain-char use in production src; the
  tree is char-signedness-safe on both platform families. The static
  serialization forbid prevents the class entirely at the type level.

### Exact commands
- grep sweeps for plain-char arithmetic/comparison patterns over
  production src; reads: strencodings.{h,cpp}, hex_base.{h,cpp},
  serialize.h:241-264

### Limitations / queue
- Enum underlying-type signedness (enum : char) not swept — adjacent
  class, queued if a case appears.
- QEMU/x86 and s390x cells remain unevidenced (host-limited, c1).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-30): enum underlying-type signedness sweep — class empty in-tree; DISMISSED

### Draw
Re-harvested-queue draw (seed_raw=12307389541691523835,
masked=3084017504836748027, n=3, idx=2) -> abi-enum-signedness ->
#68 (third cycle; c2 queue cell "enum : char adjacent class").
Branch: audit/architecture-abi-c3 from 0a4178cb88 (#55 c2 tip).

### Hypothesis
An enum with a plain-char underlying type (signedness
implementation-defined: unsigned on aarch64-gcc, signed on
x86-gcc) could carry sign-sensitive values or comparisons that
diverge across arches — the adjacent class to c2's plain-char
sweep.

### Sweep (grep evidence)
- `enum (class)? X : (signed )?char` (excluding unsigned char):
  ZERO hits in src/ (the class is empty in-tree).
- `enum ... : unsigned char`: zero hits.
- Explicit underlying types actually used: 38x uint, 6x int,
  2x size_t, 1x unsigned, and 8 enum-underlying-enum kernel
  wrappers (enum class LogCategory : btck_LogCategory et al.,
  kernel/bitcoinkernel_wrapper.h:25-64).
- No negative enumerators in the kernel wrapper enums; no
  -fshort-enums in the build config (CMakeLists/cmake/ greps) —
  the C-API enums keep gcc's default int underlying type on both
  arches, and all uses are equality comparisons.

### Verdict
DISMISSED: the enum-underlying-type signedness class is empty
in-tree; adjacent explicit-type uses are arch-invariant. Nothing
sign-sensitive to fix or gate.

### Exact commands
- grep -rnE 'enum +(class +)?\w+ *: *(signed +)?char\b' src/
- underlying-type tabulation (counts above); negative-enumerator
  and -fshort-enums greps as recorded.

### Limitations / queue
- QEMU/x86 and s390x cells remain unevidenced (host-limited, c1).
- The enum-underlying-enum wrapper pattern is legal C++ and
  gcc-consistent here; clang parity on that construct is covered
  by the c2 clang differential note.

## Rotation note
Three cycles; endian, char-signedness, and enum-underlying classes
all clean. Host-limited cells remain.

## Cycle 4 (2026-08-02, draw 229, raw=8430095800060831913 (63-bit), idx 1/2): unaligned-access parity sweep — wire layer memcpy-only, zero reinterpret_cast in serialize/streams; byte-casts alignment-irrelevant; DISMISSED

### Sweep
- Endian helpers (crypto/common.h:22-91): every ReadLE/WriteLE/
  ReadBE/WriteBE is memcpy-based — alignment-safe by
  construction on any arch (aarch64 unaligned-tolerance never
  needed).
- serialize.h / streams.h: ZERO reinterpret_cast — the wire/
  deserialization layer has no aliasing or alignment hazard.
- Remaining reinterpret_cast<uint*_t> hits (netaddress.cpp:294/
  :300, bitcoinkernel.cpp:616): byte-pointer casts over struct
  members and external buffers — char-width access, alignment-
  irrelevant on every arch. qt hit out of scope.

### Verdict
DISMISSED: no unaligned-access assumption in production paths;
the parity class is empty at the wire layer. Combined with c1
(endian), c2 (char signedness), c3 (enum underlying): the
native-host parity surface is clean.

### Exact commands
- grep memcpy/reinterpret_cast refs above (crypto/common.h,
  serialize.h, streams.h, netaddress.cpp, bitcoinkernel.cpp).

### Limitations / queue
- QEMU/x86/s390x cells remain host-limited (c1 note).
- secp256k1 subtree alignment is its own test matrix's domain
  (not re-swept).
