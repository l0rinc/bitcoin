# Campaign #59 — supply-chain-security-gates

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/supply-chain-gates. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): script_assets_test.json unpinned download — CONFIRMED + FIXED (4124803dff)

### Draw
Random draw over the 39-goal eligible pool: raw=10776981825607958642,
index 24 -> #59.

### Precedent check (avoid duplicating fork work)
audit/supply-chain already exists with the fork author's two 2026-07-18
pinning commits (48d6460315 shellcheck/mlc, 09a2c19d8e cross-build SDK
tarballs) — verified sound (hash pins + substitution-simulated
verifier), verified UNMERGED into the main lineage (consistent with
campaign #66's tracking finding). Worked on a new branch, extending
their pattern to what they left unpinned.

### Trust trace (unpinned downloads remaining on this base)
- ci/test/03_test_script.sh:79 script_assets_test.json (script test
  vectors WITH expected verdicts, from mutable raw/main) — CHOSEN.
  Threat: substituted/weakened vectors silently produce false passes
  in every CI run; only a second, independent compromise (a reviewed
  commit changing the in-repo pin) can defeat the fix.
- ci/test/01_base_install.sh:32 llvm-snapshot.gpg.key — assessed as
  inherent TOFU: the key and the packages share one trust root
  (apt.llvm.org over HTTPS); a hash pin adds no independent anchor and
  breaks on key rotation. Documented, not fixed.
- qa-assets git clone (fuzz corpora, --depth=1, same file) — corpus
  corruption fails fuzzers loudly (crash artifacts), not silently;
  recorded as a lead, not fixed.

### Defect / fix
CONFIRMED: the one remaining unpinned integrity-bearing download in
ci/test on this base. Fixed with the fork's own precedent pattern:
echo "<sha256>  <path>" | sha256sum --check after the curl.
Pin: cd789a58ec45916e1721cdd14e82ca4c93100959f1cef4e229b22e3bf539f095
(current asset 2026-07-28, 9,243,520 bytes, shape-verified).

### Demonstrated bad artifact (campaign bar)
- genuine: sha256sum --check -> OK
- substituted (valid JSON + weakened content appended): FAILED,
  1 computed checksum did NOT match — rejected before use.
- bash -n on the modified script: clean. (shellcheck unavailable
  locally; the change mirrors the pinned precedent's shape.)

### Verdict
- CONFIRMED + FIXED (4124803dff). Master-relative severity: low-moderate
  (CI-only integrity gate; the same gap exists in upstream's
  03_test_script.sh — the fix is upstream-applicable, matching the
  fork author's already-upstreamed-style precedent).

### Limitations
- Cannot rule out an asset already weakened when first pinned
  (spot-verified size+shape only).
- The pin needs manual bumps on legitimate asset updates (same
  contract as depends/shellcheck pins).
- Container image pins, workflow permissions matrix, and release
  verification (guix/verify) not swept this cycle.

### Exact commands
- `curl -sfL <asset> -o /tmp/r59_assets.json && sha256sum ...`
- substitution sim: /tmp/r59_sim (genuine OK, substituted FAILED)
- `bash -n ci/test/03_test_script.sh`

### Next queue for this campaign
- .github/workflows permissions matrix + action pinning (ci.yml is
  the only workflow; assess permissions: blocks and third-party action
  SHA pins).
- qa-assets corpus pinning options (per-corpus manifests vs
  crash-tolerant status quo).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): workflow permissions + action pinning — byte-identical to upstream; posture is upstream-accepted, no divergence

Base: 63dd3eeb26 (journal commit for #93 cycle-1 on audit/system-fault;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/supply-chain-c2 (c1 journal carried). Start state: clean
(untracked scratch only).

### Draw
Random draw over the 28-goal pool (15 pending + 13 CYCLE-1; #93
excluded as just-cycled): raw=7622586580083083163, index 23 -> #59.
Queued cell from c1: ".github/workflows permissions matrix + action
pinning".

### Audit
- ci.yml is the ONLY workflow (610 lines).
- Permissions: NO permissions: block anywhere — the workflow runs on
  the repository default GITHUB_TOKEN scope (permissive by default).
- Action pinning: all 13 third-party `uses:` are major-version TAGS
  (actions/checkout@v6, cache/{restore,save}@v5,
  upload-artifact@v7, download-artifact@v8) — mutable, not SHA-pinned.
  Local composite actions (.github/actions/*) are repo-internal.
- Diff vs upstream master (fetched raw today): BYTE-IDENTICAL.
  The fork adds nothing, removes nothing.

### Verdict
- DISMISSED as a fork-local defect: the posture (mutable tags +
  default token permissions) is upstream's CURRENT accepted state,
  and the upstream attempt to change it (l0rinc PR 35754, reviewed in
  #60 c1's mining: Sjors' circular-trust objection, maflcko/sedited
  maintenance-burden pushback) is still unmerged. Diverging locally
  would create exactly the rebase burden those reviews warned about
  with no upstream direction yet. Recorded as a watch item, not a fix.
- The c1 script_assets pin (4124803dff, merged into the lineage via
  #66 c2) is the right-sized version of the same idea, already done.

### Exact commands
- grep 'permissions:|uses:' .github/workflows/ci.yml
- curl raw.githubusercontent.com/bitcoin/bitcoin/master/.github/
  workflows/ci.yml && diff (identical, 610/610)

### Limitations / queue
- qa-assets corpus pinning (per-corpus manifest vs crash-tolerant
  status quo) unassessed — queued (it's the next pinning decision,
  same trust-model question as 35754).
- guix/verify release attestation chain not re-swept (#76 covered
  the scripts; the attestor side is out-of-tree).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
