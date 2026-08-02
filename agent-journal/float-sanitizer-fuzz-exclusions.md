# Journal: float edge values / sanitizer resurrection / fuzzer exclusions

Campaign: three linked passes (float edges → sanitizer suppressions → fuzzer
guard exclusions). Branch: audit/float-sanitizer, created from
audit/resurrection @ 83da9a2d95 (clean tree; untracked: node_modules,
package*.json, txindex-size.log — ignored scratch).
Prior campaign state: goal 86 journal has C1-C4 locked, C5-C7 open
(agent-journal/bitcoin-chainstate-symmetry.md) — paused, not abandoned.
Build: build-before (Release gcc, working). Machine: Cortex-A76, disk ~99%
full — no new full build dirs; reuse build-before, incremental only.

## Scope ledger (ranked queue — verdict per candidate)

### Pass 2 first (cheapest to inventory, highest historical yield)
| # | candidate | hypothesis | verdict |
|---|-----------|-----------|---------|
| X1 | test/sanitizer_suppressions/{lsan,tsan,ubsan} | stale entries whose cause is fixed | open |
| X2 | valgrind.supp | stale entries | open |
| X3 | src/crypto/sha256_sse4.cpp no_sanitize("address") ×2 | intentional overread still real? | open |
| X4 | minisketch NO_SANITIZE_MEMORY | third-party, check upstream parity | open |
| X5 | CI sanitizer configs (ci/test/) — recover modes, omitted categories (memory, cfi, dataflow) | omitted sanitizer coverage | open |

### Pass 1 (float edges)
| # | candidate | hypothesis | verdict |
|---|-----------|-----------|---------|
| F1 | RPC/JSON double parsing (UniValue get_real/getNum) → fee rates, amounts, -rpc* args | inf/NaN/subnormal acceptance where bounds assumed | open |
| F2 | CAmount/fee conversion paths (AmountFromValue etc.) | NaN/inf → int cast UB | open |
| F3 | GUI/bench/tools double usage | formatting/casting edges | open |

### Pass 3 (fuzzer guard exclusions)
| # | candidate | hypothesis | verdict |
|---|-----------|-----------|---------|
| G1 | fuzz harness catches/early-returns hiding production throws | removal crashes → real boundary bug | open |
| G2 | Assume gates in fuzz targets | overly narrow input domains | open |

## Verdicts

### X1-a: ubsan integer inventory + two high-value entries DISMISSED

Full inventory: ubsan (integer class only — no plain-UB suppressions in-repo),
lsan (3 Qt-external entries), tsan (deadlock test + qt wildcard + zmq +
3 intermittent with issue links). Suppression culture is maintained:
fa832103aa (2022) removed entries after fixing chain.o; fd30e9688e (2023)
narrowed file-level to symbol-level.

1. unsigned-integer-overflow:CompressAmount/DecompressAmount — JUSTIFIED.
   Ser path (compressor.h:104-109) has Assume(amount <= MAX_MONEY) before
   CompressAmount, so the wrap is out-of-domain by contract; valid amounts
   provably cannot overflow (max intermediate ~1.9e16 < 2^64). Unser path
   (110-119) explicitly throws on DecompressAmount(v) > MAX_MONEY, so
   wrapped values from a corrupt DB are caught. Residual reachability is
   fuzz-only. DISMISSED.
2. implicit-integer-sign-change:EvalScript — JUSTIFIED. The firing idiom is
   stacktop(-i)/stacktop(-1): size() + n converts the negative int to
   size_t, wrapping to size()+n — well-defined unsigned arithmetic
   (C++ [conv.integral]), guarded by explicit depth checks before every use
   (e.g. interpreter.cpp:1125,1139,1147). Pervasive intentional idiom in the
   interpreter; removal would require churn-heavy rewrites upstream has no
   appetite for. Already narrowed to symbol level. DISMISSED.

### X1-b: PR 35773 justified; bip32 constants clean

- PR 35773 (MarcoFalke, open) suppresses
  implicit-unsigned-integer-truncation:SaltedCoinsCacheHasher::operator() —
  fires only on 32-bit (uint64 hash → 32-bit size_t); intentional hash
  truncation; explicit-cast alternative is zero-behavior churn against
  upstream's symbol-suppression culture. Justified. (PR comment style:
  minimal body + reproducer diff; accepted approach: document intentional
  truncations via symbol suppressions.)
- shift-base bip32 constant BIP32_HARDENED_KEY_LIMIT is uint32_t{1} << 31 —
  unsigned, well-defined, compile-time. Clean.

### FINDING (confirmed): stale shift-base:FormatHDKeypath suppression — REMOVED

History: added file-wide (shift-base:util/bip32.cpp) by f0f8b1a076 (2021-01,
fuzz integer-sanitizer noise); narrowed to symbol by fd30e9688e (2023-11).
Cause: the 2023 function body used explicit shifts `(i << 1) >> 1` and
`i >> 31` for the hardened bit. A later refactor (current HEAD,
util/bip32.cpp:62-77) replaced them with masks
(i & BIP32_NON_HARDENED_KEY_MASK / i & BIP32_HARDENED_KEY_LIMIT) — no shift
operations remain in the function; the only shift left in the file is the
compile-time constexpr uint32_t{1} << 31, never instrumented.

Proof (two independent verifiers):
1. Source + history: current FormatHDKeypath contains no shift; the mask
   refactor removed every runtime shift the 2023 body had.
2. Executable: harness compiling src/util/bip32.cpp with clang 18
   -fsanitize=integer -fno-sanitize-recover=all, running FormatHDKeypath
   100k iterations over edge keypaths {0, 1, 0x7FFFFFFF, 0x80000000,
   0x80000001, 0xFFFFFFFF, 31} → exit 0, no diagnostic, output
   "/0/1/2147483647/0'/1'/2147483647'/31" correct.
Review precedent: fa832103aa (2022) "Avoid integer sanitizer warnings in
chain.o" removed suppressions after the cause was fixed — same class.
Validation: bip32_tests pass (build-before); diff is exactly -1 line.

### X3: sha256_sse4 no_sanitize("address") — JUSTIFIED (toolchain bugs, not stale)

Both no_sanitize attributes on Transform() work around documented toolchain
defects, not memory bugs: clang compile failure under ASan
(llvm-project#92182, bitcoin#31913, added 141df0a288 2024-05) and GCC
runtime SEGV in the ASan self-test (bitcoin#34881, added fedeff7f20
2026-03). Conditional on ASan being active; the SSE4 inline asm is an Intel
YASM translation ASan cannot instrument. Actively maintained (2026 gcc
addition). Removing would break ASan builds. On this ARM host the file is
inert (#if __x86_64__). DISMISSED.

### X4: minisketch NO_SANITIZE_MEMORY — dead guard, third-party, leave

clmul_common_impl.h:23-29 defines no_sanitize("memory") only for clang < 11
(MSan could not reason through _mm_clmulepi64_si128). Current minimum
toolchains make the guard a no-op. Vendored subtree — changes belong to
upstream sipa/minisketch, not local churn. DISMISSED (upstream note if the
subtree ever updates).

### F2 (float edges → RPC/JSON double boundary): DISMISSED — boundary fully guarded, empirical matrix

Inventory: the entire RPC double-input surface is two handlers
(estimaterawfee threshold, rpc/fees.cpp:166; prioritisetransaction dummy,
rpc/mining.cpp:556-561) plus MaybeArg<double> plumbing (rpc/util.cpp:729).
All amount paths are double-free by design: AmountFromValue
(rpc/util.cpp:98-108) uses ParseFixedPoint on the raw string, never a
double; ParseFeeRate builds on it.

Empirical boundary matrix (harness against src/univalue, exact output):
- nan / NaN / Infinity / -Infinity → JSON parse REJECTS (read=0). NaN/inf
  literals can never reach a handler.
- 1e999 / -1e999 / 1.8e308 → parsed as number token, but get_real() itself
  THROWS "JSON double out of range". No ±inf reaches callers.
- 1.7976931348623157e308 (max finite) → fine.
- 1e-999 / 0.1e-4932 (decimal underflow) → get_real() = 0 (silent, benign:
  amounts don't use get_real; threshold=0 is a valid value).
Command: g++ -std=c++20 -I src/univalue/include harness.cpp
src/univalue/lib/{univalue,univalue_get,univalue_read}.cpp

Comparison-style fragility (NOT a defect, unreachable): estimaterawfee's
`threshold < 0 || threshold > 1` would pass NaN — but no NaN can cross the
boundary above. prioritisetransaction's `*dummy != 0` rejects NaN correctly
(NaN != 0 is true). Internal double generation (getnetworkhashps division,
fee-estimator stats) is output-side only. DISMISSED.

### G-pass slice 1 (fuzzer exclusions): 4 candidates, all legitimate

1. fuzz/headerssync.cpp:34 `Assume(!new_headers.empty())` — unfalsifiable:
   sole caller guards with `if (!deser_headers || deser_headers->empty())
   return;` at line 137. Valid harness precondition. DISMISSED.
2. fuzz/block.cpp:88 catch (NonFatalCheckError) — positive oracle:
   asserts caught_null_ref afterwards; tests exception-safety of
   TX_WITH_WITNESS serialization with a null tx ref. Not hiding anything.
   DISMISSED.
3. fuzz/coins_view.cpp:418-425 catch (CoinsViewCursorUnsupportedError) —
   positive oracle: asserts exact message and that the exception occurred.
   DISMISSED.
4. fuzz/deserialize.cpp:179-181 FeeFrac invariant exclusion
   (fee_rate_deserialize): deserializing arbitrary bytes into CFeeRate can
   produce FeeFrac with size<0 or (size==0 && fee!=0), violating the
   documented invariant (feefrac.h:20). VERDICT: legitimate exclusion —
   no production path ever deserializes raw CFeeRate fields: SERIALIZE_METHODS
   (feerate.h:128) exists but has no production persistence/wire user
   (estimator file uses EncodedDoubleFormatter with IsSaneEstimatorVector
   sanity checks; mempool.dat doesn't contain CFeeRate; RPC constructs
   CFeeRate from integrals, size=1000 always). The abort risk
   (ComparableFeeRate's Assume(size>0), feerate.h:43) is unreachable from
   disk or network. FRAGILITY NOTED (not a defect): if raw CFeeRate is
   ever persisted, feerate.h:128 has no invariant check on deserialize.

### X1-c: remaining ubsan integer entries — all JUSTIFIED (closes in-repo audit)

1. shift-base:streams.h — BitStreamWriter::Write (streams.h:358)
   `data << (64 - nbits)` discards caller bits above nbits BY API CONTRACT
   ("Write the nbits least significant bits"). Documented bit-discard.
   JUSTIFIED.
2. implicit-integer-sign-change:TxConfirmStats::removeTx
   (block_policy_estimator.cpp:548) — unsigned nBestSeenHeight - entryHeight
   wraps when entryHeight is ahead; implicit conversion to int yields
   negative (two's complement, C++20 modular), caught by the blocksAgo < 0
   guard at 551. Intentional defensive idiom, correct behavior. JUSTIFIED.
3. shift-base:InsecureRandomContext::rand64 + RandomMixin::randbits
   (random.h:220, 243) — `gen << bitbuf_size` intentionally discards high
   bits (only bottom bits of the fresh word are wanted); SplitMix64 mixer
   shifts/overflows are the algorithm definition. JUSTIFIED.
4. shift-base:arith_uint256.cpp — multi-limb shifts are the bignum
   implementation's design. JUSTIFIED.
5. crypto/ blankets (overflow/sign-change/truncation/shift) — SHA256/SHA1/
   RIPEMD/ChaCha/SipHash use mod-2^32/2^64 arithmetic by definition; every
   wrap is the algorithm. JUSTIFIED.
6. implicit-integer-sign-change:compressor.h — static_cast<uint64_t>(CAmount)
   in AmountCompression::Ser (compressor.h:106); negative CAmount is
   out-of-domain post-validation (Assume bounds above only — asymmetry
   noted, defensive coverage acceptable). JUSTIFIED.

In-repo ubsan suppression audit complete: 1 stale removed (FormatHDKeypath,
99d98861fc), all others justified. External-dependency entries (leveldb,
secp256k1, minisketch, boost/libc++/qt headers, FuzzedDataProvider) left
untouched by design.

### X5: CI sanitizer configuration — DISMISSED (comprehensive matrix, strict options)

- Jobs enabled in .github/workflows/ci.yml: native_asan
  (address,float-divide-by-zero,integer,undefined; ci.yml:452), native_fuzz
  (fuzzer,address,undefined,float-divide-by-zero,integer; :500), native_tsan
  (:524), native_fuzz_with_msan (:530), native_msan (:536), plus valgrind
  jobs. MSan uses track-origins=2 with libc++; TSan builds libc++.
- Options are strict, no recover-mode hiding: UBSAN halt_on_error=1 +
  print_stacktrace + report_error_type (03_test_script.sh:22);
  TSAN halt_on_error=1 (:21); ASAN detect_leaks + use-after-return +
  strict init order (:19). LSan/TSan/UBSan suppression files wired in.
- Omitted categories: CFI (requires LTO+clang, no known upstream adoption
  or pending findings class here) and DataFlowSanitizer (research taint
  tracker, not applicable). Neither is a resurrection candidate.
- No disabled sanitizer jobs, no -fsanitize-recover, no skipped
  platforms for sanitizers beyond documented container caps (SYS_PTRACE
  for ASan+LSan, issue-linked). DISMISSED.

### G-pass slice 2 (state-machine harnesses): model clamps legitimate

1. fuzz/txorphan.cpp "protected peer" clamp (345-349, 360-364): the harness
   never lets designated protected peers exceed their allotment so it can
   assert "within-limits peer is never evicted" as an oracle. The clamp
   makes the invariant assertable; production eviction itself is fully
   exercised via unprotected peers. LIMITED_WHILE caps are throughput
   bounds, not semantic exclusions. DISMISSED.
2. fuzz/txrequest.cpp:122,126 early returns — inside the harness's own
   model (Cleanup over its announcement matrix), not production TxRequest.
   Model-based comparison oracle. DISMISSED.
3. fuzz/txgraph.cpp / cluster_linearize.cpp Assumes — internal invariant
   checks on the harness's depgraph construction (IsConnected, Any);
   positive assertions, not input exclusions. DISMISSED.

### F3 (GUI/bench/tools doubles): DISMISSED

GUI money code (qt/bitcoinunits, optionsmodel) uses qint64 satoshis
throughout — no doubles in any money path. Bench nanobench doubles are
timing statistics (display-only). Tools take no double inputs. No float
anywhere near consensus or secret-dependent code.

### E1 (EncodedDoubleFormatter edge behavior): DISMISSED — validated at every consumer

Format: fee-estimator file stores doubles as uint64 via EncodeDouble/
DecodeDouble (monotonic encoding); arbitrary uint64 on disk CAN decode to
inf/NaN/subnormal. But every consumer validates before use:
- buckets: IsSaneBucketsVector — finite, strictly increasing, back ≥ 1e99
  (block_policy_estimator.cpp:70-79).
- estimator vectors: IsSaneEstimatorVector — finite && ≥ 0 (81-84).
- decay scalar: explicit 0 < decay < 1 throw (478-480).
NaN fails isfinite everywhere; inf fails finite or the bound checks.
Coverage of the exact path: fuzz/policy_estimator_io.cpp (fuzzed file
through real Read), test/serfloat_tests.cpp (encode/decode roundtrip),
policyestimator_tests.cpp:122 (explicit invalid-decay rejection).
DISMISSED.

### X2: valgrind.supp triage — all 5 entries justified, no memory-error suppressions

1. Shutdown(NodeContext&) leak — broad but exit-path only (OS reclaims);
   the only broad entry, noted as acceptable-by-location.
2-3. leveldb leaks (BackgroundCall, GetCoin path) — dependency, still-
   reachable allocation caches.
4-5. LogInstance / Logger::StartLogging still-reachable — process-lifetime
   singletons by design.
Critically: NO Memcheck:Addr*/Cond* (memory-error) entries exist — nothing
hides UB, only still-reachable leak noise. No resurrection candidate.
DISMISSED.

### G-slice-3 (wallet fuzz): no suspicious catches; oracle-precondition bailouts legitimate

- No catch blocks in wallet/test/fuzz (crypter, wallet_bdb_parser, fees,
  spend, coincontrol, scriptpubkeyman) — grep-verified.
- coinselection.cpp:122/124 early returns (coin_grinder_is_optimal): bail
  when CoinGrinder has no solution or did not complete optimally — the
  "other algorithms >= optimal" oracle is only meaningful with an optimal
  reference; documented inline. Production calls each algorithm
  independently; nothing hidden. DISMISSED.

## Cycle summary (end of cycle 2)

### M1 (mempool.dat persist format): DISMISSED — integer-only, re-validated per entry

Format (node/mempool_persist.cpp:93-159): version uint64 gated against two
known versions (unknown → graceful discard), then per-entry {TX_WITH_WITNESS,
int64 time, int64 feeDelta}. NO double/float fields anywhere. Torn write →
deserialization throw → outer catch → load discarded gracefully. Every
entry is re-validated through full AcceptToMemoryPool (139) with expiry
and fee-delta handling — corrupt entries cannot bypass policy/consensus
checks; re-validation IS the integrity mechanism. The one double
(100.0 * tried / total, line 114) is progress-logging only, and total==0
skips the loop (no div-by-zero). DISMISSED.

### K1 (kernel C-API float surface) + pass-1 closure

Kernel API has exactly ONE double: btck_NotifyBlockTip's
verification_progress (bitcoinkernel.h:368) — OUTBOUND (kernel → consumer),
produced by GuessVerificationProgress (validation.cpp:5710-5745) and
GetBackgroundVerificationProgress (5747-5758).
- Mainline chains: m_chain_tx_count==0 guarded → 0.0 (5718-5721);
  denominator positive (ChainTxData values positive on mainnet/signet/
  testnet; regtest reduces to exactly 1.0); clamped by std::min(·, 1.0)
  (5744). Provably in [0,1].
- Edge (custom chainparams, future ChainTxData.nTime + tiny tx_count):
  denominator can go negative → negative progress unclamped; background
  variant can exceed 1.0 past target. Display-only, no consensus/state
  impact, non-standard config only. OBSERVATION, not a defect.

PASS 1 CLOSED: all float surfaces audited — RPC/JSON (F2), config (zero
double options: ArgsManager has no GetDouble), GUI (qint64 money), bench
(statistics-only), tools (no float inputs), bindings/kernel (this entry),
fee-estimator serialization (E1), mempool persist (M1). No float anywhere
near consensus or secret-dependent crypto. Declined-as-churn: NaN-canonical
comparison in estimaterawfee (unreachable via the proven JSON boundary).

Campaign surface across all three passes now audited: suppressions
(ubsan/lsan/tsan/valgrind + no_sanitize + CI matrix), RPC/JSON/config/GUI/
bench float boundaries, fuzzer catches/Assumes/clamps in net, state-machine,
deserialize, and wallet harnesses, fee-estimator float+serialization.
Findings: 1 confirmed (stale shift-base:FormatHDKeypath removed,
99d98861fc); ~25 dismissed-with-evidence; 3 fragility notes
(m_blocks_for_inv_relay drain invariant — from goal 89; feerate.h:128 no
invariant check; estimaterawfee NaN-passing comparison style, unreachable).

### G-deep (serialization formatter exceptional values): DISMISSED

- ReadCompactSize (serialize.h:334-364): canonical-form enforcement on
  EVERY width branch (non-canonical → throw at 346/352/358) + MAX_SIZE
  range check by default; range_check=false only for generic number
  encoding, documented (331).
- ReadVarInt (446-464): overflow guards are tight for both unsigned and
  signed types — pre-shift bound at 453 (n > max>>7 → throw) and the
  continuation increment guarded at 458 (n == max → throw); no signed
  overflow reachable. Encoding is a bijection by construction, so no
  canonicality check needed on read.
- VarIntMode::NONNEGATIVE_SIGNED negative-value quirk ("improperly encode
  if negative", serialize.h:394-398): used for block-index nVersion/nHeight/
  nFile and FlatFilePos — all non-negative in real data (block versions are
  positive; heights/files ≥ 0). Documented limitation, unreachable.
  DISMISSED.

### DYNAMIC experiment 1: FeeFrac guard removal — VALID PRECONDITION (guard is necessary)

Setup: removed the exclusion at fuzz/deserialize.cpp:180-182 (mutant, since
reverted and verified clean). Seeds crafted as raw CFeeRate {int64 fee,
int32 size} LE and run through the REAL production boundary (CFeeRate
deserialize → serialize → operator==) in a standalone verifier compiled
with -DABORT_ON_FAILED_ASSUME to replicate the fuzz build's Assume
semantics (G_ABORT_ON_FAILED_ASSUME, check.h:28-34 — without it Assume is
a runtime no-op, which the first harness iteration proved: all seeds passed,
documenting why dynamic checks need the fuzz configuration).

Results (commands: g++ -std=c++20 -DABORT_ON_FAILED_ASSUME harness +
src/util/check.cpp + src/support/cleanse.cpp + src/clientversion.cpp):
- e_valid (1000/1000): pass, equal=1.
- b (fee=1, size=0): pass — IsEmpty() → ComparableFeeRate{0,1}; (1,0)
  silently EQUALS (0,0). By design (IsEmpty is size==0).
- c (fee=-1, size=1000): pass — negative ratios are in-domain.
- d (INT64_MIN/INT32_MIN): ABORT at feerate.h:43 Assume(feerate.size > 0).
- a (fee=1, size=-1): ABORT at feerate.h:43, same site.

Classification: VALID PRECONDITION. The abort fires in the harness's own
equality check on out-of-domain input; the Assume is the type's domain
enforcement. Production never deserializes raw CFeeRate (no persistence/
wire user — static pass G-slice-1), so no production defect exists. The
exclusion is NECESSARY: removing it lets out-of-domain bit patterns reach a
domain-asserting comparator. Guard kept; mutant reverted (git diff clean).

### DYNAMIC experiment 2: txorphan clamp inversion — UNNECESSARY (unit coverage proves the semantics)

The planned inversion (let protected peers exceed limits, watch eviction)
is already covered deterministically by src/test/orphanage_tests.cpp
peer_dos_limits: over-limit peers are evicted by LimitOrphans (line 124),
eviction targets ONLY the over-limit peer (254), FIFO within peer, and no
over-eviction beyond necessity (258); plus DoS_mapOrphans and
too_large_orphan_tx. Verified green on this branch:
build-before/bin/test_bitcoin --run_test=orphanage_tests → no errors.
The fuzz clamp's purpose (assert never-evicted-when-within-limits
continuously) is thereby justified: eviction-when-over-limits is unit-
proven, so the clamp loses no production coverage. DISMISSED.

## Campaign handoff (cycle 3 complete)

Branch: audit/float-sanitizer @ (this commit). Tree clean. All three passes
plus the dynamic phase are done:
- Pass 1 (float edges): CLOSED — RPC/JSON (empirical univalue matrix),
  config (no double options), GUI (qint64), bench, tools, kernel bindings
  (one outbound double, in-range), fee-estimator serialization, mempool
  persist.
- Pass 2 (sanitizer resurrection): CLOSED — 1 confirmed finding
  (99d98861fc stale shift-base:FormatHDKeypath removed); all other
  suppressions justified with mechanism evidence; CI matrix comprehensive.
- Pass 3 (fuzzer exclusions): CLOSED — 9 candidates statically + 1 dynamic
  guard-removal (FeeFrac: valid precondition, guard NECESSARY, aborts at
  feerate.h:43) + 1 dynamic experiment obviated by unit coverage
  (txorphan peer_dos_limits green).
Confirmed: 1. Dismissed: ~30. Fragility notes: 3 (feerate.h:128 no
invariant check; estimaterawfee NaN-passing comparison, unreachable;
AmountCompression::Ser Assume bounds above only).
Re-ranked queue for the next session: secp256k1 subtree suppressions
(belongs to secp campaigns); second-cycle dynamic experiments on other
harness catches IF new evidence appears; monitor PR 35773 (32-bit
truncation suppression) via crons. Do NOT treat the tree as exhausted —
this is a complete pass of the CURRENT queue, not a proof of absence.
