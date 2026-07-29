# Campaign #39 — generated-artifact-determinism

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/generated-artifact. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): artifact inventory — all regenerable outputs byte-identical; no drift

### Draw
Random draw over the 49-goal eligible pool: raw=16085357564447640226,
index 15 -> #39.

### Inventory + results
1. doc/man/*.1 man pages: intentional PLACEHOLDERS in-tree ("Please
   follow the instructions in contrib/devtools/README.md to generate
   the manual pages after a release"). Release-time pipeline:
   contrib/devtools/gen-manpages.py + help2man. help2man not installed
   on this host — determinism untestable locally; the checked-in state
   is documented placeholder, not stale artifact.
2. src/node/data/ip_asn.dat -> build-generated ip_asn.dat.h
   (cmake/script/GenerateHeaderFromRaw.cmake; pure: file(READ HEX) +
   regex format + file(WRITE), no timestamp/locale/ordering inputs).
   Regenerated into /tmp and cmp against build-before's:
   BYTE-IDENTICAL under LC_ALL=C and LC_ALL=tr_TR.UTF-8.
   Zero-diff verification command recorded below.
3. src/test/data/*.json -> *.json.h
   (cmake/script/GenerateHeaderFromJson.cmake; same pure pattern).
   Spot-check tx_valid.json: BYTE-IDENTICAL.
4. src/node/data/ip_asn.dat itself: coordinated release artifact from
   the asmap-data repo (Kartograf/RPKI pipeline), provenance and
   replacement procedure documented in doc/asmap-data.md. Not locally
   regenerable by design; not stale-flagged (the refresh cadence is a
   release process, documented).
5. src/bitcoin-build-info.h: intentionally version-stamped
   (BUILD_GIT_COMMIT "5d0155254c19" at measurement time) —
   non-deterministic by design, generated per build by
   cmake/script/GenerateBuildInfo.cmake. Documented behavior.
6. secp256k1 ecmult static tables: Sage generators in-tree
   (src/secp256k1/sage/gen_*.sage); Sage not installed locally —
   provenance present, regeneration not possible on this host.

### Verdict
- DISMISSED: every artifact that CAN be regenerated locally reproduces
  byte-identically (2 raw + 1 json checks, 2 locales); the rest have
  documented provenance or are intentionally version-stamped. No stale
  artifacts, no undocumented manual edits, no unstable ordering.

### Zero-diff verification commands
- raw: `/usr/bin/cmake -DRAW_SOURCE_PATH=src/node/data/ip_asn.dat
  -DHEADER_PATH=/tmp/x.h -DRAW_NAMESPACE=node::data -P
  cmake/script/GenerateHeaderFromRaw.cmake && cmp /tmp/x.h
  build-before/src/node/data/ip_asn.dat.h`
- json: same with -DJSON_SOURCE_PATH=src/test/data/<name>.json and
  GenerateHeaderFromJson.cmake.

### Limitations
- Second-compiler/OS regeneration not done (single host); the
  generators are text transforms, so cross-compiler variance is not
  expected (locale was the realistic hazard and was tested).
- secp256k1/minisketch Sage tables unregenerated (toolchain absent).
- Manpage determinism untested (help2man absent); the placeholder
  state makes this moot in-tree.

### Next queue for this campaign
- If Sage/help2man land on this host: secp table + manpage
  determinism passes.
- src/test/data/*.json full sweep (all 10 files, same cmp loop).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): full JSON sweep — 60/60 regenerated headers byte-identical

Base: 0afa208f43 (journal commit for #66 cycle-2 on audit/backport-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/generated-artifact-c2 (c1 journal carried). Start state:
clean (untracked scratch only).

### Draw
Random draw over the 37-goal pool (24 pending + 13 CYCLE-1; #66
excluded as just-cycled): raw=5387637136342518518, index 27 -> #39.
Queued item from c1: "src/test/data/*.json full sweep".

### Sweep (exact build parameters from build.ninja)
For each JSON, run GenerateHeaderFromJson.cmake with the same
-DJSON_SOURCE_PATH/-DHEADER_PATH/-P invocation the build uses, output
to a fresh path, cmp against the checked-in generated header:
- src/test/data/*.json (9 files: base58_encode_decode,
  bip341_wallet_vectors, blockfilters, key_io_invalid, key_io_valid,
  script_tests, sighash, tx_invalid, tx_valid): 9/9 IDENTICAL.
- src/univalue/test/*.json (51 files: fail1-fail45, pass1-4,
  round1-7): 51/51 IDENTICAL.
60/60 byte-identical. No drift, no locale/host variance on this host.

### Verdict
- DISMISSED: the JSON->header generation is deterministic across the
  entire corpus (c1's 2-file spot check extended to full coverage).
- The generated-artifact family now has complete in-tree coverage:
  raw (c1) + json (c2, both trees).

### Exact commands
- per file: /usr/bin/cmake -DJSON_SOURCE_PATH=<abs> -DHEADER_PATH=/tmp/x.h
  -P cmake/script/GenerateHeaderFromJson.cmake && cmp
- build.ninja:2940-2959 (parameter provenance)

### Limitations
- Sage-derived secp tables and help2man manpages remain toolchain-
  blocked on this host (as c1 recorded) — the only open cells, both
  external-tool gated.
- asmap.raw.h re-verified by the same loop pattern (already c1).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
