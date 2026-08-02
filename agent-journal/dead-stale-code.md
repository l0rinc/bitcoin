# Campaign #29 — dead-stale-code

Base: audit/resurrection @ 31f7f2e601 (rotation ledger commit for #31).
Branch: audit/dead-stale-code. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): three dead-code sweeps — all clean; one inconclusive enum variant parked

### Sweep 1: uncalled free functions (util/common)
Method: regex-extract plausible free-function definitions from
src/util/*.cpp + src/common/*.cpp, then `grep -rlF` each name across
src/ and count containing files.
Result: 1-file hits are static/anonymous-namespace helpers used within
their own TU (asmap Decode*, pcp helpers, args/settings internals);
2-file hits are header+impl pairs invoked through macros
(FormatException, StrFormatInternalBug) or the Qt layer (SettingName).
No dead production function found. Consistent with -Werror unused-function
hygiene for statics.

### Sweep 2: dead registered options
Method: extract 174 option names from AddArg calls across
init.cpp/src/init/*.cpp/qt and the tool binaries; for each, count
non-AddArg references to the quoted option string.
Initial raw scan flagged 101 — false positive: registration and
consumption commonly co-locate in one file (verified -asmap: registered
and consumed in src/init.cpp:1609-1612).
Refined scan: 5 remaining candidates, all -zmqpub*hwm — all consumed via
dynamically constructed names (`arg + "hwm"`,
src/zmq/zmqnotificationinterface.cpp:69). 0 dead options.

### Sweep 3: unused enum values / typedefs (spot)
- SafeChars: DEFAULT/UA_COMMENT/URI used in production;
  SAFE_CHARS_FILENAME has NO production use — only its own unit test
  (util_tests.cpp:1083) exercises it.
- MutableTransactionSignatureChecker / DeferringSignatureChecker: live
  (sign.cpp, psbt.cpp, interpreter users).
- Serialization macros VARINT_MODE/COMPACTSIZE/LIMITED_STRING/
  LIMITED_VECTOR: all live.

### Verdict
- DISMISSED (no removable dead code found this cycle). Journal-only.
- INCONCLUSIVE (parked): SAFE_CHARS_FILENAME — test-only reachability,
  but it is a public SanitizeString rule in a coherent 4-value family;
  no evidence it is unintentionally staged, and removing a documented
  public enum value needs a stronger reason than current non-use.
  Not removed (campaign: no harmless cleanups; history check left open).

### Exact commands
- python3 nm/grep cross-reference scripts (in cycle history; refined
  option scan: AddArg extraction + non-AddArg reference counting)
- `grep -rn '"-asmap"' src/` (spot verification)
- `grep -rn "hwm" src/zmq/` (dynamic-consumption verification)

### Limitations / leads
- Sweeps covered util/common/options only; node/wallet/script free
  functions not scanned this cycle (same method applies — cheap).
- Duplicated-implementation and stale-test classes not yet swept.
- bench/fuzz-only production helpers (the reverse direction: production
  code only reachable from tests) not systematically mapped — needs
  coverage tooling, queued.

### Next queue for this campaign
- Duplicated implementations: candidate pairs (uint256 base_blob vs
  blob wrappers; hex helpers in strencodings vs crypto/hex_base).
- Stale tests: tests exercising removed/no-longer-shipped behavior
  (grep for options/features deleted in release notes).

## Cycle 2 (2026-07-28): free-function scan over node/wallet/script — clean

### Method
Same scanner as cycle 1 (regex-extract plausible free-function
definitions, then count files containing each name across src/),
extended to src/node/*.cpp, src/wallet/*.cpp, src/script/*.cpp
(test/fuzz/bench subdirs excluded).

### Result
- 1-file hits: anonymous-namespace/static helpers used within their TU
  (descriptor factory fns registered via local tables, interpreter
  statics like EvalChecksig*/VerifyWitnessProgram, sqlite helpers,
  walletdb Load* internals, eviction comparators). All used in-TU;
  -Werror unused-function already guards this class.
- 2-file hits: header+impl pairs invoked through macros, typedefs, or
  cross-TU calls the name-grep sees (spend.h users, miniscript.h,
  feefrac.h SaturatingSubtract).
- Spot-verified the only plausible candidate: ApproximateBestSubset
  (wallet/coinselection.cpp) is static and CALLED in production at
  coinselection.cpp:782/784 (SelectCoinsBnB); the same-named test case
  (coinselector_tests.cpp:651) is coincidental. NOT dead.
- LegacyWalletIsMineInnerDONOTUSE is deliberately named/retained.

### Verdict
- DISMISSED (no removable dead production code in node/wallet/script).
  Journal-only. Both broad source layers now scanned clean
  (util/common in c1, node/wallet/script in c2).

### Exact commands
- python3 scanner (cycle history), `grep -rn ApproximateBestSubset src/`

### Next queue for this campaign
- Duplicated implementations: candidate pairs (uint256 base_blob vs
  blob wrappers; hex helpers in strencodings vs crypto/hex_base).
- Stale tests: tests exercising removed/no-longer-shipped behavior
  (grep for options/features deleted in release notes).

## Rotation note
Two bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-07-28): duplicated-pair + stale-test + parked-item — all DISMISSED with evidence

### Draw
Random draw over the 61-goal eligible pool: raw=12589355222101945619,
index 13 -> #29. Executed all three queued items from c1/c2.

### Item 1: duplicated implementations — DISSOLVED (layering/design)
- hex pair: HexStr exists ONLY in crypto/hex_base.{h,cpp} (10 production
  includers); util/strencodings.h has ZERO HexStr definitions (65
  includers use it for ParseHex/TryParseHex and higher-level encodings).
  The split is dependency layering (crypto/uint256 must not depend on
  util), not duplication. DISMISSED.
- uint256 pair: base_blob<256> (uint256.h:26, hash semantics) vs
  base_uint<256> (arith_uint256.h:230, arithmetic) — the deliberate
  dual-type design, upstream-identical (origin/master retains
  base_blob, verified by git show). Not dead, not duplicated. DISMISSED.

### Item 2: stale tests — DISSOLVED (framework helpers / previous releases)
Candidates from a removed-RPC name sweep over test/functional
(importmulti, signrawtransaction, importaddress, importpubkey, getinfo,
estimatefee, account API, setgenerate, getwork):
- feature_taproot.py:1515 signrawtransaction -> NodeSigner framework
  helper (feature_taproot.py:1878), not the RPC removed in v0.18.
- wallet_migration.py importaddress/importmulti (~8 sites) -> all go to
  self.old_node (previous-release binary where the RPCs exist);
  create_legacy_wallet (line 82) requires it. Correct by design.
No test exercises removed behavior against the CURRENT binary. DISMISSED.

### Item 3: SAFE_CHARS_FILENAME (parked c1) — history closes it
- Added by d678771c66 (2017-01-08, "Wallet: Sanitise -wallet parameter")
  FOR a production caller: SanitizeString on the -wallet parameter.
- That caller disappeared with the -wallet handling rework; the enum
  value + its unit test (util_tests.cpp:1083) remain.
- Upstream master still carries SAFE_CHARS_FILENAME (verified:
  git show origin/master:src/util/strencodings.h) — it is
  upstream-designated public API, not fork-local cruft.
- Verdict: DISMISSED (was intentionally staged; upstream retains; the
  c1 "no evidence of unintentional staging" is now history-backed).
  Removing an upstream public enum value remains unjustified.

### Verdict
- All three queued items DISMISSED with evidence. Journal-only cycle.
- Campaign status: util/common (c1), node/wallet/script (c2),
  duplicated/stale-test/parked (c3) all clean. Remaining unrun cell:
  bench/fuzz-only production helpers (needs coverage tooling).

### Exact commands
- grep -rln 'crypto/hex_base.h' src/ ; grep -c HexStr src/util/strencodings.h (0)
- git show origin/master:src/uint256.h | grep -c base_blob (1)
- removed-RPC name sweep over test/functional (list in item 2)
- git log -S 'SAFE_CHARS_FILENAME' ; git show d678771c66

### Next queue for this campaign
- bench/fuzz-only production helpers (reverse-dead-code direction) —
  needs coverage tooling; parked until a coverage build exists.
- SAFE_CHARS_FILENAME: nothing further (closed).

## Rotation note
Three bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 4 (2026-08-02, draw 228, raw=4931726890564104754 (63-bit), idx 1/3): reverse-dead-code sample — 9 helper candidates all have production callers; no coverage tool needed for the sample verdict; DISMISSED

### Sample (reverse-reachability, c3's parked direction)
9 header-declared helpers probed for production-only vs
test/bench/fuzz-only caller sets (grep -rl, split by path class):
- TrimString (prod 10), RemoveSuffixView (3, incl. dbwrapper
  Logv), ReplaceAll (6), ToLower (9), SplitString (9), HexStr
  (31), HexDigit (3, hex_base per c3), RenameOver (5): ALL have
  production callers.
- ToUpper (prod 2): production-reachable via
  common/messages.cpp:94 (mode_string normalization) — the
  thinnest sample, still live.
- FormatParagraphs: my bad candidate name — zero hits anywhere
  (does not exist in this tree; recorded as a candidate-name
  error, not a finding).

### Verdict
DISMISSED: no test-only production helper in the sample; the
reverse-dead-code direction shows no dead weight at the utility
layer. The full census still wants a coverage build (c3 note);
the sample method is recorded as the cheap first pass.

### Exact commands
- per-symbol grep -rl loop above; sed strencodings.cpp:391-402,
  common/messages.cpp:94.

### Limitations / queue
- 9-symbol sample, not exhaustive; the coverage-build census
  remains the definitive cell (parked on tooling).
