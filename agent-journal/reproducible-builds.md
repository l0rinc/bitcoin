# Campaign #76 — reproducible-builds

Base: fef20d1b9b (journal commit for #36 cycle-2 on audit/cross-tool-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/reproducible-builds. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-28): A/B same-host different-dir rebuild — bit-identical code; 1-byte DWARF comp_dir delta attributed; Guix packaging verified

### Draw
Random draw over the 64-goal repaired pool (41 pending + 23 CYCLE-1;
#36 excluded as just-cycled): raw=4368789968832384158, index 30 -> #76.

### Host feasibility note
No Guix/docker on this host (aarch64, disk 99% full) — full Guix
attestation rebuild is out of scope. Cells taken: (1) same-host A/B
different-build-dir binary comparison (the rebuild-and-compare cell),
(2) Guix packaging determinism text audit (the release-path cell).

### Cell 1: A/B binary comparison
Setup: build-repro-a and build-repro-b, identical cmake (Ninja,
Release -O2, tests/bench/gui OFF, wallet ON, IPC OFF, ccache ON),
targets bin/bitcoind bin/bitcoin-cli bin/bitcoin, HEAD fef20d1b9b,
clean worktree.
Results (sha256):
- bitcoin: A==B (84d693ea7405...) — bit-identical unstripped.
- bitcoin-cli: A==B (313800985cbb...) — bit-identical unstripped.
- bitcoind: A!=B — exactly 21 bytes: 20 bytes at offsets 781-800
  (.note.gnu.build-id) + 1 byte at 0xd865ee (.debug_str).
Attribution (diffoscope-style, manual):
- The 1 content byte: the string "/mnt/my_storage/bitcoin/build-repro-a"
  vs "...-b" — DW_AT_comp_dir of the secp256k1.c object (adjacent
  .debug_str entries: xonly_pubkey_tweak_add, secp256k1.c paths).
  Mechanism: this tree sets -fmacro-prefix-map only (CMakeLists.txt:
  523-526, covers __FILE__), NOT -ffile-prefix-map, so DWARF comp_dir
  (the build dir) is embedded raw; and secp256k1.c is among ccache's
  45 uncacheable calls (ccache -s: 45/22560), so it is compiled fresh
  in every build dir while all other objects are cache hits carrying
  identical strings from their first compile. That is why ONLY
  bitcoind differed (bitcoin/bitcoin-cli's objects were all hits) —
  ccache masked the same effect elsewhere.
- The 20 build-id bytes are a downstream hash of that 1 byte.
Proof of no other difference: strip --strip-debug on both -> only the
20 build-id bytes differ; objcopy --remove-section
.note.gnu.build-id -> byte-identical (994394732cc9...). All
.text/.rodata/.data sections identical in the unstripped pair too
(21 differing bytes total, none in executable sections).

### Cell 2: Guix packaging determinism (text audit, verified sound)
- LC_ALL=C exported in setup/build/package/codesign.sh (locale-safe
  sort ordering).
- setup.sh:9 exports TAR_OPTIONS="--no-same-owner --owner=0 --group=0
  --numeric-owner --mtime=@SOURCE_DATE_EPOCH --sort=name"; build.sh
  SOURCES setup.sh (line 9) and package.sh (line 207) in the same
  shell, so the linux/darwin tar paths in package.sh inherit full
  mtime/owner/order normalization (no local touch needed there).
- win zip path normalizes per-file (touch --date=@SOURCE_DATE_EPOCH,
  find | sort | zip -X@).
- prelude.bash guards against a pre-set SOURCE_DATE_EPOCH leaking in
  (ERR unless FORCE_SOURCE_DATE_EPOCH).
No gap found in the release path.

### Verdict
- DISMISSED as defect: the only same-host nondeterminism is one
  DWARF comp_dir byte from an uncacheable secp256k1 compile in
  unstripped dev binaries; release (Guix) determinism is achieved by
  fixed container paths + strip/split-debug, and mapping comp_dir away
  in dev builds would trade debuggability (real paths in gdb) for a
  cosmetic byte — not worth a tree change (campaign bars trading
  developer clarity).
- Cell filled: same-host A/B rebuild -> code sections bit-identical;
  boundary characterized (debug-info-only, one object class, ccache
  interaction documented).

### Exact commands
- cmake -B build-repro-{a,b} -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DBUILD_GUI=OFF -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF
  -DENABLE_WALLET=ON -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DWITH_USDT=OFF
- ninja -C build-repro-{a,b} bin/bitcoind bin/bitcoin-cli bin/bitcoin
- sha256sum / cmp -l / readelf -S / readelf -n (build-id)
- python3 byte-context read at the cmp-reported offset
- strip --strip-debug; objcopy --remove-section .note.gnu.build-id
- grep/sed audit of contrib/guix/libexec/{setup,build,package,codesign}.sh
  and contrib/guix/guix-build

### Limitations / queue for cycle 2
- True Guix container rebuild (attestation comparison) not possible on
  this host — the definitive cell remains open for a Guix-capable host.
- depends/ source-hash spot verification (download + sha256 vs
  packages/*.mk) not done this cycle — queued.
- Why secp256k1.c is ccache-uncacheable (which flag) not root-caused
  (45 uncacheable calls visible in ccache -s; CCACHE_DEBUG rebuild
  would name them) — queued; if it is a fixable flag, caching it would
  ALSO mask the comp_dir effect consistently.
- Cross-compiler reproducibility (gcc vs clang artifacts) is out of
  scope by definition (different toolchains, different bytes).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
