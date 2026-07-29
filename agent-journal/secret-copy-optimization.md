# Campaign #44 — secret-copy-optimization

## Cycle 1 (2026-07-29): secure-container audit + memory_cleanse elision probes — cleanse proven elision-resistant at -O3 -flto

### Draw
Random draw over the 6-goal eligible pool (5 pending + 1 CYCLE-1,
#70 excluded as just-cycled): raw=4945649014858672681, index 1 ->
#44 (first cycle). Branch: audit/secret-copy-optimization from
027edb4d3f (#70 c1 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #44's
campaign-focus block (secret dataflow/declassification) matches its
title — no offset here.

### Cell 1: secret-container coverage on key paths — CLEAN (read)
- CKey stores keydata in secure_unique_ptr<KeyType> (key.h:61-66);
  copies go through operator= into the target's own secure
  allocation (key.h:77-90); destruction cleanses via
  secure_allocator deallocate (support/allocators/secure.h:39).
- BIP32 derive: vout (HMAC output, secret-derived) is a
  secure_allocator vector (key.cpp:325); the tweak path feeds
  vout.data() directly into secp256k1 — no plain-container copy.
- DER import writes directly into the secure keydata
  (key.cpp:309); export's caller buffer is the secure CPrivKey
  (key.cpp:180-187). No non-secure crossing found on the audited
  paths.

### Cell 2: compiler-elision of cleansing — PROVEN RESISTANT (2 probes)
memory_cleanse = memset + `__asm__ __volatile__("" : : "r"(ptr) :
"memory")` barrier (support/cleanse.cpp). Compiled with gcc 13.3
-O3 -flto=auto:
- Probe 1 (foldable): memset(0x42) + cleanse + free -> gcc folds to
  calloc+free; the 0x42 write dies but the ZERO-STATE guarantee at
  free is preserved (calloc zeroes). Semantics hold.
- Probe 2 (opaque source: argv bytes -> memcpy -> cleanse -> free):
  disassembly shows `stp xzr,xzr,[x19]` / `stp xzr,xzr,[x19,#16]`
  (32 bytes of zero-stores) emitted BEFORE the free call — the
  cleanse is inlined but NOT elided; the barrier works as
  documented (Adam Langley idiom, quoted in cleanse.cpp).
Probes preserved in the journal record (re-run: g++-13 -O3 -flto
-I src probe.cpp src/support/cleanse.cpp; objdump -d).

### Cell 3: residue classes (explicitly non-findings, with rationale)
- HMAC ctx rkey[64] (crypto/hmac_sha256.cpp:17-20): the memsets are
  PADDING at Init, not cleansing; the object holds the padded key
  until reuse/dealloc — upstream-matching design; the HMAC "key"
  here is derivation material already held in secure containers
  elsewhere. Per the rotation's non-inflation rule (uncleared
  counters/buffers without a standalone cryptographic meaning are
  not severity by themselves), recorded and not reported.
- Raw memsets in secp256k1 subtree (musig/extrakeys): subtree
  initialization zeroing; secp's real cleansing goes through its own
  memclear — upstream domain.
- script/keyorigin.h:44 fingerprint memset: fingerprint is PUBLIC
  by design (BIP32).

### Verdict
DISMISSED: secret paths use secure containers at every audited
crossing; the cleanse primitive is elision-resistant under the
tree's most aggressive local optimization (-O3 -flto); remaining
uncleared residues are upstream-matching and non-severity.

### Exact commands
- reads: key.h:36-110, key.cpp:75-95/120-165/180-190/305-340,
  secure.h:20-70, cleanse.cpp
- probes: g++-13 -O3 -flto=auto -I src probe{,2}.cpp
  src/support/cleanse.cpp; objdump -d | grep -A26 '<main>:'

### Limitations / queue for cycle 2
- clang-18 disassembly cross-check of the same probes (the barrier
  is documented for gcc; clang honors the same idiom — verify
  rather than assume).
- Wallet-side secret flows (vchSecret/mkey paths, #38's family)
  not re-audited — that campaign covered its own rollback fix;
  a dedicated crossing-map there is queued if a cycle lands here.
- Signing-time secp256k1 context copies (nonce function buffers)
  are subtree-internal; out of local scope.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
