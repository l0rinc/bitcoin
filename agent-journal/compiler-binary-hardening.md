# Campaign #91 — compiler-binary-hardening

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/compiler-hardening. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): binary hardening inspection — full stack present; BTI enforcement inactive by toolchain; secp256k1 subtree asymmetry

### Draw
Random draw over the 41-goal eligible pool: raw=824133286738821982,
index 38 -> #91. Prior-art check: reviews/2026-07-26-compile-time-
hardening-candidates.md covers the CONSTEXPR axis (different work,
no overlap).

### Binary inspection (build-before/bin/bitcoind, aarch64 — binaries, not flags)
- PIE: yes (pie executable, FLAGS_1 PIE). NX stack: yes (GNU_STACK RW).
- RELRO + BIND_NOW (full RELRO): yes.
- Stack canary: yes (__stack_chk_fail/guard refs).
- FORTIFY_SOURCE=3: yes (6 _chk refs: fprintf/memcpy/memset/read/
  snprintf/vsnprintf — plausible given span/string abstractions).
- -fstack-protector-all, -fstack-clash-protection, -Wstack-protector,
  -fcf-protection=full (x86 CET; inert here), -Wl,-z,relro/now/
  separate-code: all in link lines.
- -mbranch-protection=standard: in compile lines; `bti j` instructions
  present in the binary (objdump count >0 in main-tree objects,
  libbitcoin_crypto.a 23, leveldb objects confirmed).
- _GLIBCXX_ASSERTIONS: absent (upstream-consistent; distro wrappers add it).

### Finding 1: BTI compiled in but NOT ACTIVATED (toolchain-level)
The .note.gnu.property (AArch64 BTI/PAC) that tells the kernel/loader
to enforce BTI is ABSENT from the binary (readelf shows only
build-id + ABI-tag). Causality experiment (minimal, /tmp):
- object compiled with -mbranch-protection=standard: note PRESENT
  (AArch64 feature: BTI, PAC);
- trivial all-BTI executable linked with the system gcc driver: note
  ABSENT;
- relink with -Wl,-z,force-bti: note PRESENT (BTI), with ld warnings
  that Scrt1.o/crti.o/crtn.o lack BTI notes.
=> The AND-rule (output property = AND of inputs) drops the note
because glibc/gcc STARTUP FILES on this system (glibc 2.35-era crt*)
have no BTI note. Any normally-linked binary on this toolchain loses
enforcement regardless of project flags. Severity: low-moot here
(Cortex-A76 is ARMv8.2 — no BTI hardware; instructions are NOPs);
on ARMv8.5+ hosts the attempted hardening would be silently inactive
unless linked with -z force-bti (upstream design decision; crt
warnings expected and benign).

### Finding 2: secp256k1 subtree escapes the hardening set (wiring asymmetry)
- leveldb/crc32c/main-tree objects: -mbranch-protection=standard
  present in compile lines, bti instructions in objects.
- secp256k1 objects: 0 branch-protection flag, 0 bti instructions.
  Root cause: cmake/secp256k1.cmake builds the subtree with its own
  RelWithDebInfo CFLAGS + sanitize_interface only — it never inherits
  core_interface, so the crypto core gets LESS hardening than leveldb
  (no branch-protection, no stack-protector-all, no stack-clash).
  The isolation is upstream's deliberate subtree pattern, but it is
  inconsistent across subtrees (leveldb inherits, secp does not).
- Note: finding 1 is NOT caused by finding 2 (proven by the all-BTI
  experiment); fixing secp256k1's flags would not restore the note on
  this toolchain (crt files also lack it).

### Verdict
- Findings of fact, journal-only. No checkbox hardening committed:
  campaign rule requires a demonstrated failure blocked or diagnostic
  gained, which neither finding provides on this host (no BTI
  hardware; no concrete secp256k1 OOB proposed).
- Report-worthy upstream: (a) document that -mbranch-protection
  compiles in but enforcement needs -z force-bti on crt-note-less
  toolchains; (b) subtree flag asymmetry (secp256k1 vs leveldb).

### Exact commands / artifacts
- readelf -lW/-dW/-nW build-before/bin/bitcoind; nm (canary/_chk)
- objdump -d (bti counts: main objects, libleveldb.a member, libsecp256k1.a)
- /tmp/r91_main.c experiment (object vs linked note; -z force-bti)

### Limitations
- Only bitcoind inspected at ELF level (not bitcoind-qt, not
  libbitcoinkernel.so — the shared lib consumers would inherit the
  same toolchain note behavior).
- Windows/macOS/FreeBSD hardening (DYNAMICBASE etc., present in the
  same CMake block) not inspectable on this host.
- Sanitizer-based integer/conversion checks are a different axis
  (campaigns 11/98 territory).

### Next queue for this campaign
- libbitcoinkernel.so ELF inspection (export hardening + note state).
- If a v8.5+ host becomes available: measure whether the missing note
  matters in practice (glibc BTI enablement logging).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): libbitcoinkernel export hardening — config-gated; secp-leak hypothesis REFUTED by measurement

### Draw
Random draw over the 14-goal eligible pool (11 pending + 3 CYCLE-1,
#80 excluded as just-cycled): raw=3164618978385005329, index 13 ->
#91 (second cycle; c1 queue cell "libbitcoinkernel.so ELF
inspection"). Branch: audit/compiler-hardening-c2 from 4099ee4f7d
(#80 c1 bookkeeping). Journal pulled forward from 12ec75620a (c1,
side branch only).

### Scope note
build-before builds the kernel STATIC (libbitcoinkernel.a, 128
objects after adding the convenience target's deps; bitcoinkernel
target default). Rather than a full shared reconfigure (disk 100%),
the cell was answered statically + with one targeted link probe.

### BTI note census (corrected; c1's asymmetry quantified in the
kernel artifact)
125/128 objects carry the AArch64 BTI+PAC property note; the 3
without are exactly the secp256k1 subtree objects (secp256k1.c.o,
precomputed_ecmult.c.o, precomputed_ecmult_gen.c.o) — c1's finding-2
asymmetry measured inside the kernel deliverable. (First census read
0/128 — my grep pattern didn't match readelf's "AArch64 feature:"
format; corrected by re-running with the right pattern and recording
the method slip.) Linked-level enforcement remains toolchain-
inactive per c1 finding 1 (crt files note-less).

### Export surface: hypothesis REFUTED, config-dependence measured
- Hypothesis: secp256k1's 82 API symbols (GLOBAL in .symtab) would
  leak into libbitcoinkernel.so (object library -> --exclude-libs
  inapplicable). REFUTED: readelf shows them GLOBAL HIDDEN — the
  subtree's own CMAKE_C_VISIBILITY_PRESET=hidden
  (cmake/secp256k1.cmake:14) marks everything hidden; a probe link
  (gcc -shared secp256k1.c.o precomputed_*.o) exported 0 dynamic
  symbols. No secp leak in EITHER config.
- C++ side IS config-gated: this build has REDUCE_EXPORTS=OFF
  (upstream default, CMakeLists.txt:134), so the hidden preset and
  -Wl,--exclude-libs,ALL are inactive: 1431 default-visibility
  global defined symbols across the 128 objects would export from a
  .so linked now. Release/Guix builds set -DREDUCE_EXPORTS=ON
  (contrib/guix/libexec/build.sh:125), hiding C++ internals; the
  BITCOINKERNEL_API attribute (bitcoinkernel.h:21) keeps the public
  API visible in both configs.
- Stack canary present in objects (UND __stack_chk_fail/guard).

### Verdict
Findings of fact, journal-only (same bar as c1: no demonstrated
failure blocked by a local change). Dev-build symbol visibility is
not a security boundary; the shipped (Guix) configuration is
hardened; upstream-matching option design.

### Exact commands
- ar x libbitcoinkernel.a (128 .o); per-object readelf -n census
- nm --defined-only -g (1431 count; 82 secp); readelf -sW visibility
- probe: gcc -shared secp256k1.c.o precomputed_ecmult{,_gen}.c.o
  -Wl,--exclude-libs,ALL; nm -D -> 0
- config: CMakeLists.txt:134/602-606, cmake/secp256k1.cmake:14,
  bitcoinkernel.h:21, contrib/guix/libexec/build.sh:125

### Limitations / queue for cycle 3
- The .so-link-level dynamic table in a real REDUCE_EXPORTS=ON build
  was not measured (needs a full reconfigure; disk 100%). If a
  future cycle has disk headroom: configure build-kernel-shared with
  -DREDUCE_EXPORTS=ON, nm -D the .so, expect API-only + hidden
  internals.
- bitcoind-qt / bench / test binaries' ELF posture uninspected.
- Windows/macOS hardening flags remain uninspectable on this host.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
