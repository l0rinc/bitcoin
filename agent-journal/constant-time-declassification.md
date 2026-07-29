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

## Cycle 2 (2026-07-29): walletpassphrase online-attempt semantics — KDF-only throttle (0.10s/attempt measured), no lockout, by design

### Draw
Random draw over the 40-goal pool (23 pending + 17 CYCLE-1; #105
excluded as just-cycled): raw=13832910584231443478, seed masked to 63
bits (4609538547376667670), index 30 -> #45. Queued item from c1:
"walletpassphrase online-attempt rate limiting / lockout semantics".
Base: d145133bd3 (journal commit for #105 cycle-1 on
audit/autopsy-recurrence; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/constant-time-c2.

### Semantics map (code)
- Attempt path: walletpassphrase RPC (encrypt.cpp:50-70) ->
  CWallet::Unlock (wallet.cpp:615) -> DecryptMasterKey per master key
  -> CCrypter::Decrypt (AES-256-CBC of the master key).
- Cost per attempt: the KDF, BytesToKeySHA512AES with
  DEFAULT_DERIVE_ITERATIONS = 25000 (crypter.h:46-48, "just under 0.1
  seconds on a 1.86 GHz Pentium M"). Stored rounds come from the
  wallet DB at encryption time.
- No attempt counter, no lockout, no escalating delay anywhere
  (grep over crypter.cpp/rpc/encrypt.cpp: none exist).
- Wrong-passphrase detection = decrypt failure; no oracle beyond the
  claimant's own key (c1's no-Vaudenay point).

### Measurement (isolated scratch wallet)
regtest node, encryptwallet, three wrong-passphrase RPC attempts:
0.10s / 0.10s / 0.10s wall each (RPC overhead ~1ms of it). ~10
attempts/s/connection maximum on this host.

### Verdict
- DISMISSED: online-attempt semantics are the documented design —
  unbounded attempts throttled only by the per-attempt KDF cost
  (~0.1s measured, matching the in-code comment). The operative
  defense is passphrase strength against the OFFLINE attack (stolen
  wallet.dat), and 25000 SHA512-AES rounds is the whole margin;
  online lockout would add account-lockout DoS surface for no
  security gain at this throttle.
- The c1 queue item is fully answered; no code change warranted.

### Exact commands
- greps/seds: encrypt.cpp:50-70, wallet.cpp:615-632, crypter.cpp:41-108,
  crypter.h:46-48
- timed RPC attempts on an encrypted scratch wallet (/tmp/btc45_w,
  removed after)

### Limitations / queue
- The 25000-rounds constant is static at encryption; wallets created
  on slower hosts carry the same rounds — not a defect, documented.
- Cookie/RPC auth declassification (username-enumeration shapes in
  auth error paths) remains queued from c1.
- #53 c1 already empirically closed the AES-CBC padding timing cell
  (Welch t 1.53/1.69/-1.14) — cross-linked, not repeated.

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
