# Campaign #45 — constant-time-declassification

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/constant-time. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): AES wallet-encryption boundary on ARM64 — constant-time end to end, no leaking declassification

### Draw
Random draw over the 33-goal eligible pool: raw=13722416236855635760,
index 14 -> #45. Boundary chosen because this host is ARM64: x86 AES-NI
does not apply, so wallet-key encryption runs whatever the portable
path is — the constant-time question is real here.

### Dataflow proof (secret-derived values, each marked)
- AES core: src/crypto/ctaes — bit-sliced AES ("no tables or
  data-dependent branches whatsoever", README; source verified: no Te
  lookup tables, no key-dependent branches). Key schedule + encrypt +
  decrypt all constant-time on every platform incl. aarch64.
  DECLASSIFICATION: none needed; the design is the defense.
- CBC padding check (aes.cpp:111-127, "attempt to run in constant-time"):
  full 16-iteration scan, no early exit; `padsize *= !fail` masks
  malformed padding to zero so the comparison mask
  `(i > 16 - padsize) & (*out-- != padsize)` reads only the last block
  (never out-of-bounds — the classic Vaudenay protection);
  `fail |=` accumulates branch-free; `written * !fail` returns 0 on
  failure with identical timing. CONSTANT-TIME by construction.
- CCrypter::Decrypt (crypter.cpp:94-109): padding mode on; returns
  bool. DECLASSIFICATION: "key correct or not" — intended unlock
  semantic; wrong passphrase and corrupted ciphertext are
  indistinguishable (any wrong key yields garbage padding -> same
  failure), so the boolean carries no Vaudenay oracle: decryption
  happens with the claimant's own key, never with a server-held secret
  key under attacker-chosen ciphertexts.
- KDF: SetKeyFromPassphrase — SHA512 (constant-time) with public fixed
  iteration count; IV = memcpy of nIV (public by design).
- RPC boundary: walletpassphrase unlock failures report only
  "incorrect passphrase" — same declassification, intended.
  (Rate-limiting of online passphrase attempts is a separate property;
  noted, not audited this cycle.)

### Verdict
- DISMISSED: no variable-time crossing found on the wallet-encryption
  path; every declassification is either unnecessary-by-design or
  carries only the intended unlock semantic. The ARM64 cell (this
  host's native execution of ~40 suites incl. crypter paths) is the
  executed evidence; the static proof above is the dataflow evidence
  the campaign requires alongside dynamic passes.

### Why existing analysis missed nothing
#13 (secret-lifetime-zeroization) and #14 (secret-control-flow)
completed the BIP324/zeroization lattices; the AES/padding boundary
was unexamined and is now closed. No defect to fix.

### Limitations
- Compiler-output spot check (objdump for injected data-dependent
  branches in ctaes) not performed — the branch-free property is
  structural in bit-sliced straight-line code; recorded as static
  evidence class.
- Online passphrase rate-limiting and lockout policy: separate
  property, queued below.
- AES-GCM/other modes: not used in-tree (only CBC), nothing to audit.

### Exact commands
- reads: src/crypto/ctaes/*, src/crypto/aes.cpp:94-128,
  src/wallet/crypter.cpp:89-118.

### Next queue for this campaign
- walletpassphrase online-attempt rate limiting / lockout semantics.
- Cookie/RPC auth declassification beyond #53's tag-timing cell
  (username-enumeration shapes in error paths).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
