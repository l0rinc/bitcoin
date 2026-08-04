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

## Cycle 3 (2026-07-29): RPC/cookie auth declassification sweep — constant-time at the secret comparisons; one de-minimis shape

### Draw
Re-rank draw over the 3 remaining CYCLE-2+ open cells:
raw=1921359177284103813, index 0 -> #45 (third cycle; c1/c2 queue
cell "Cookie/RPC auth declassification — username-enumeration shapes
in auth error paths"). Branch: audit/constant-time-declass-c3 from
3a05e3441f (#1 c3 bookkeeping).

### Audit (httprpc.cpp auth path, full read :50-101, :216)
- CheckUserAuthorized (:63-82): per-rpcauth-entry, username compare
  via TimingResistantEqual (:66); on match, HMAC-SHA256(pass, salt)
  then TimingResistantEqual against the stored hash (:73-77).
  TimingResistantEqual itself (strencodings.h:202-209) is
  branch-free over the FIRST argument's length (the stored value),
  so attacker-controlled lengths do not steer the timing; the
  documented length-proportional time leaks only the config value's
  length.
- Error identity: single 401 for any failure (:216) — no
  wrong-user vs wrong-pass message distinction.
- Shape recorded (de minimis): the per-entry loop runs HMAC only
  when the username matches (:67-74), so response time differs
  between existing and non-existing usernames (username-existence
  oracle). Reachability: RPC is localhost/authorized-network by
  default; the username is not the credential (the salted password
  hash is); upstream-identical code. Classified de minimis, not a
  defect.
- Cookie auth shares the same path (synthetic __cookie__ entry) and
  the same constant-time comparisons.

### Verdict
DISMISSED: secret comparisons are constant-time; the single-401
error shape hides user existence at the message level; the
HMAC-on-match timing shape is de minimis at this trust boundary
(local RPC) with usernames non-secret. No code change.

### Exact commands
- reads: httprpc.cpp:50-101/216, strencodings.h:195-209,
  rpc/request.cpp (cookie generation)

### Limitations / queue
- Remote timing measurement of the HMAC-on-match delta not
  attempted (loopback HTTP noise dwarfs ~1us HMAC; the shape is
  recorded as informational).
- The 25000-rounds static constant note (c2) stands.
- #53's AES-CBC padding cell stays cross-linked (closed there).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 4 (2026-08-01): secp256k1 ctime_tests under valgrind memcheck — full suite, 0 errors; DISMISSED

### Draw
RE-RANK draw 154 over the 8-cell pool: raw=9940712460200409047,
masked 717340423345633239 -> idx 7 -> #45 secp ctime cell (c1
queue; valgrind confirmed available at /usr/bin/valgrind).
Branch: audit/constant-time-c4 from 4340ba0f87.

### Method
Subtree scratch build: cmake -S src/secp256k1 -B /tmp/btc45c4/build
-DSECP256K1_BUILD_CTIME_TESTS=ON (Valgrind AUTO detected, VALGRIND
macro defined; window/comb defaults ECMULT_WINDOW_SIZE=15
COMB_BLOCKS=43 COMB_TEETH=6). Run:
valgrind [--quiet] --error-exitcode=42 bin/ctime_tests.

### Result
- Under memcheck: ERROR SUMMARY: 0 errors from 0 contexts; rc=0
  (exit 42 would fire on any secret-dependent access).
- Full ctime suite compiled in: sign/recover/ecdh/eckey/ecmult
  generator paths at production backend parameters.
- Fire-proof note: run WITHOUT valgrind the binary prints only its
  usage hint (the tests are memcheck-instrumented, not self-
  reporting) — the valgrind ERROR SUMMARY line is the correct
  oracle, and --quiet suppresses it (use rc + non-quiet grep).
- Verdict: DISMISSED. The shipping secp256k1 backend is
  memcheck-clean on the entire ctime suite at production
  parameters on this host (aarch64).

### Exact commands
- cmake line above; valgrind --error-exitcode=42
  /tmp/btc45c4/build/bin/ctime_tests (log /tmp/btc45c4_run.log).

### Limitations / queue
- msan variant not run (needs an instrumented toolchain;
  memcheck is the valgrind-family gate upstream CI uses).
- #45 queue: empty (c1 AES/CBC, c2 passphrase throttle, c3 RPC
  auth, c4 secp ctime). Campaign COMPLETE on current surface.

## Rotation note
Four cycles; declassification surface closed.

## Cycle 5 (2026-08-02, draw 266, raw=13490391729748515582, suspicion-mined from the 26-PR sweep): PR 35688 empty-HMAC UB — CONFIRMED with first-invalid UBSan trace at HEAD + ADOPTED; UBSan probe silent post-fix; crypto_tests green with empty-key vectors

### Defect (F3 empty-span family, crypto arm)
CHMAC_SHA256/512 constructors memcpy key bytes unconditionally;
an empty key vector hands memcpy(nullptr, 0) — UB by the C
standard's nonnull contract, and UBSan flags it even though the
copy length is zero. The in-tree fuzz target worked around it by
force-resizing empty inputs (crypto.cpp guards) — the footprint
was already visible.
- FAILING-BEFORE (first-invalid trace): UBSan probe
  (CHMAC_SHA256{nullptr, 0} + CHMAC_SHA512{nullptr, 0}):
  hmac_sha256.cpp:16 + hmac_sha512.cpp:16 'runtime error: null
  pointer passed as argument 2, which is declared to never be
  null'.

### Adoption (audit/adopt-empty-hmac)
- Cherry-picked b80907909c (std::copy for empty ranges + empty-
  key RFC-style vectors) with two conflict resolutions recorded:
  (a) test comment union — kept BOTH the new empty-key vectors
  AND our RFC-4231 case-5 documentation (#107 c1 lineage);
  (b) fuzz-target union — dropped ONLY the empty-guards, kept
  the fork's added split/chunking calls unconditionally.
- Also picked the branch tip a6b1b82f0d (eval_script stale
  empty-guard removal — same family).
- PASSING-AFTER: crypto_tests green (empty-key vectors + case 5
  cohabit); UBSan probe with the fixed sources is SILENT
  (EMPTY-HMAC-OK, no runtime error).

### Verdict
CONFIRMED + ADOPTED: genuine null-at-size-0 UB at HEAD (the F3
family's crypto arm), fixed with the author's minimal std::copy
change; empty keys now exercised instead of avoided. Upstream
vehicle: PR 35688.

### Suspicion-mining
- S7: std::copy's empty-range semantics make the guard-removal
  safe everywhere the same shape appears; the eval_script arm
  (a6b1b82f0d) is the same family's second instance — checked
  ConsumeRemainingBytes already returns early.
- S8: the fork's chunking differential calls now run on empty
  inputs too (wider coverage, no new oracle needed).

### Exact commands
- UBSan probe (failing) above; cherry-picks + resolutions above;
  crypto_tests + probe (passing) above; empty-seed fuzz run
  pending below.

### Limitations / queue
- Empty-seed fuzz run pending the build_fuzz rebuild.

### Garnish (cycle 266 completion)
Empty-seed run against the REBUILT fuzz binary (the one carrying
the dropped guards): FUZZ=crypto -runs=2000 /tmp/empty_seed
executes clean — the fuzz path now exercises empty inputs
directly instead of skipping them (previously guard-resized).
Cycle 266 fully closed.

## Cycle 338 (2026-08-04) — REOPENED by secp256k1 subtree update + upstream BIP32 SetSeed assert

Draw r156 (raw=7213982357476630317 -> #45). Two new inputs since
cycle 266:

1. Upstream 2cf9d79d84 (BIP32 SetSeed length assert, 16..64 bytes)
   vs our 5b0e492d19 (Derive input rejection + write-after-success):
   COMPLEMENTARY layers (SetSeed vs Derive), both in-tree post-rebase
   (key.cpp:413 verified). No duplication, no conflict.
2. c26d4e2d6f secp256k1 subtree update. Constant-time review of the
   non-module diffs:
   - scalar get_bits*: VERIFY_CHECK precondition rewrites
     (offset <= 256 - count replaces offset + count <= 256 —
     overflow-safe formulation, same accepted domain), same-limb
     checks added; scalar_low_impl offset>=32 fix affects only
     EXHAUSTIVE_TEST_ORDER builds. No timing behavior change;
     get_bits_var remains documented non-CT in offset/count, as before.
   - extrakeys: VERIFY-mode even-Y check on x-only save (defensive).
   - secp256k1.c: one-line selftest loop change.
   VERDICT: DISMISSED (no declassification/timing delta).

SIDE OBSERVATION (not a #45 defect): the subtree's own CMakeLists
defaults SECP256K1_ENABLE_MODULE_SILENTPAYMENTS=ON and Core's wrapper
(cmake/secp256k1.cmake) pins only ECDH/RECOVERY/MUSIG — so the BIP352
module is now COMPILED INTO Core builds (build-after libsecp256k1.a
carries 16 silentpayments symbols; build-before carried 0). Zero Core
callers exist (grep across src/ outside the subtree): unreachable =>
no runtime surface today. This is upstream-master state too (no Core
override at 17c5e33e9c), not fork divergence. WATCH CELL: if upstream
adds the first Core caller, the module's CT/secret-handling properties
(shared-secret derivation, label tweaks) become live review surface;
the compiled-in-but-unused state is an upstream packaging choice worth
noting in any future upstream-facing summary.
