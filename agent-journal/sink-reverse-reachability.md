# Campaign #100 — sink-reverse-reachability

Base: 0d33ecabe7 (journal commit for #78 cycle-1 on
audit/translation-validation; ledger-lineage anchor audit/resurrection
@ 5d0155254c). Branch: audit/sink-reachability. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-28): bloom-filter sinks — ctor div-by-zero test-only; empty-filter mod-by-zero already guarded (CVE-2013-5700)

### Draw
Random draw over the 53-goal pool (34 pending + 19 CYCLE-1; #78
excluded as just-cycled): raw=3667681032778981497, index 24 -> #100.

### Sink A: CBloomFilter parameterized-ctor division/log
Sink (src/common/bloom.cpp:34,40): vData sizing computes
`-1/LN2SQUARED * nElements * log(nFPRate)` — log(0) for nFPRate=0;
nHashFuncs divides by nElements — div-by-zero for nElements=0.
Seed (not proof): l0rinc's upstream PR 35818 (bloom-zero-elements).
Reverse walk: the parameterized ctor is called from unit tests and
the bloom_filter fuzz target ONLY (upstream PR's own statement,
verified by grep: the sole production construction site is the COPY
ctor at net_processing.cpp:5073). FILTERLOAD does NOT reach it:
deserialization is raw-member READWRITE (bloom.h:67) into the default
ctor, then IsWithinSizeConstraints, then copy.
Verdict: LATENT / test-only. Not exploitable; the upstream ctor
hardening is optional robustness, not a reachable defect here.

### Sink B: empty-filter modulo/index
Wire path: NetMsgType::FILTERLOAD (net_processing.cpp:5057) ->
vRecv >> filter (raw vData/nHashFuncs/nTweak/nFlags) ->
IsWithinSizeConstraints (bounds only) -> new CBloomFilter(filter) ->
relay via contains()/insert(). An empty vData would make Hash() mod
(vData.size()*8) a division by zero and vData[nIndex>>3] an OOB
access.
Fact: both are guarded — insert() early-returns on vData.empty()
(bloom.cpp:55, "Avoid divide-by-zero (CVE-2013-5700)"), contains()
returns true on empty (bloom.cpp:77). The match-all consequence
(empty filter relays everything) is BIP37 protocol semantics
(fail-open by design), not a defect.
Verdict: DISMISSED — already guarded upstream; the wire path's only
remaining cost is sanctioned match-all relay.

### Verdict (cycle)
- Sink A: LATENT (test-only; upstream PR 35818 is a hardening seed).
- Sink B: DISMISSED (guarded; CVE-2013-5700 fix verified present and
  correct).
- No fix committed (no reachable defect; duplicating the upstream PR
  locally would diverge from its review track — recorded as a watch
  item instead).

### Exact commands
- grep/sed: bloom.cpp:25-99 (ctor, insert, contains, Hash,
  IsWithinSizeConstraints); bloom.h:44-81 (members, default ctor,
  SERIALIZE_METHODS); net_processing.cpp:5057-5075 (FILTERLOAD);
  production-ctor caller grep across src/

### Limitations / queue
- One sink family audited (bloom). Queued: allocation-size sinks from
  wire-derived CompactSize (vector resize paths in net/message),
  shift/narrowing sinks in feefrac, and the txgraph saturation family
  as a follow-on sweep.
- The upstream 35818 watch item: if it lands, take upstream's; do not
  pre-empt locally.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-30): wire-CompactSize allocation sinks — all bounded ~5 MiB, mutation-verified; DISMISSED

### Draw
Harvested-queue draw (seed_raw=9787481964664031861,
masked=564109927809256053, n=6, idx=3) landed on VerifyCryptedKeys
— DESCOPED per the 2026-07-28 scope note (wallet-only, no core
reachability path); recorded as a queue correction, not a result.
Redraw (seed_raw=4836711809555485631, masked same, n=5, idx=1) ->
sink-allocs -> #100 (second cycle; c1 queue cell "allocation-size
sinks from wire-derived CompactSize"). Branch: audit/sink-reverse-c2
from b1b8803cc2 (#108 c4 journal tip).

### Hypothesis
A wire-derived CompactSize count could drive an up-front allocation
disproportionate to bytes delivered (memory amplification sink) in a
P2P-reachable deserialization path.

### Code audit (trust boundary: untrusted P2P bytes -> DataStream ->
Unserialize)
- Transport envelope: MAX_PROTOCOL_MESSAGE_LENGTH = 4 MB (net.h:65).
- Generic vectors: VectorFormatter 5 MiB batched reserve with the
  explicit DoS comment (serialize.h:~686-695, MAX_VECTOR_ALLOCATE =
  5000000, serialize.h:39).
- Basic-byte std::vector and prevector: incremental blocks of
  1 + 4999999/sizeof(T) (serialize.h:~884, ~927) — note: a SEPARATE
  literal, not MAX_VECTOR_ALLOCATE.
- All wire string reads in net_processing use LIMITED_STRING
  (strSubVer MAX_SUBVERSION_LENGTH :3702, feature_id
  MAX_FEATUREID_LENGTH :4058); no plain std::string wire reads.
- ReadCompactSize range cap MAX_SIZE = 0x02000000 elements
  (serialize.h:36) + non-canonical rejection.

### Experiment (driver /tmp/sink_driver.cpp; allocation accounting via
operator new + --wrap=malloc/realloc/free, per-probe high-water reset)
Hostile buffer = CompactSize claiming 0x02000000 elements + 100
payload bytes; raw 32 MiB control validates the tracker exactly.
- g++ -O2 -std=c++20 -I src -Wl,--wrap=malloc,--wrap=realloc,
  --wrap=free /tmp/sink_driver.cpp src/support/cleanse.cpp
Results (peak transient allocation, post-catch leak):
  vector<uint8_t>   32M/100B: peak   5,000,133 B, threw, leak 0
  vector<uint64_t>  32M/100B: peak   5,000,133 B, threw, leak 0
  vector<FakeInv>   32M/100B: peak   5,000,101 B, threw, leak 0
  prevector<28,u8>  32M/100B: peak   5,000,133 B, threw, leak 0
  std::string       32M/100B: peak  33,554,566 B, threw, leak 0
  raw new[33554433] control : peak  33,554,433 B (tracker exact)
  legal-max controls (4M/4M, 5M/5M): pass, no throw
(FakeInv = CInv-shaped stand-in; prevector uses raw realloc, hence
the --wrap accounting; first driver revision had a monotonic-peak
artifact and blind realloc path — fixed before this table.)

### Mutation (causality) check
serialize.h copy with MAX_VECTOR_ALLOCATE 5000000 -> 50000000,
driver rebuilt against it (-I /tmp/mut -I src):
  vector<uint64_t>/FakeInv: peak -> 50,000,133/50,000,101 B
  (mutation KILLED on the generic path — bound is caused by the
  constant);
  vector<uint8_t>/prevector: peak UNCHANGED 5,000,133 B — the
  basic-byte paths use their own 1+4999999 literal (mutation
  SURVIVES there). Maintainability note only: the 5 MB bound is
  duplicated as a literal; a future MAX_VECTOR_ALLOCATE edit would
  silently miss the basic-byte paths. Both are 5 MB today and
  verified bounded — below the defect bar.

### Verdict
DISMISSED: every wire-reachable CompactSize-driven allocation path
is bounded to ~5 MiB transient per message (documented "attacker
must provide X MiB to make us allocate X+5 MiB" verified EXACTLY:
100 B payload -> 5,000,1xx B peak, zero post-catch leak). The only
up-front 32 MiB sink (plain std::string Unserialize) has no
wire-reachable caller (all LIMITED_STRING) and is exception-safe;
even hypothetically reachable it is <=8x over the 4 MB transport
cap, transient — not a defect.

### Exact commands
- build/run as above; mutation: sed copy to /tmp/mut/serialize.h,
  rebuild with -I /tmp/mut first.
- reachability greps: 'vRecv >>' (net_processing.cpp:3387-5713),
  LIMITED_STRING sites, REJECT-handler absence.

### Limitations / queue
- Driver measures virtual reservation via allocator interception,
  not RSS; commit-charge nuance (untouched mmap pages cost no RAM)
  only makes the bound tighter.
- Queued (from c1): shift/narrowing sinks in feefrac; txgraph
  saturation family. New: deduplicate the 5 MB literal if a
  maintenance cycle lands nearby (do not file as a defect).

## Rotation note
Two cycles; bloom + wire-CompactSize families closed. feefrac and
txgraph families remain.

## Cycle 3 (2026-07-30): txgraph saturation family — UNREACHABLE by bounded proof; DISMISSED

### Draw
Rebuilt-queue draw (seed_raw=10344607599657158645,
masked=1121235562802382837, n=7, idx=5) -> txgraph-saturation ->
#100 (third cycle; c1 queue cell "txgraph saturation family").
Branch: audit/sink-reverse-c3 from 3c53c2e4e8 (#100 c2 journal tip).

### Hypothesis
Attacker-controlled mempool contents could drive txgraph's FeeFrac
sums to saturation/overflow, producing wrong fee ordering, a bad
block-template package, or a failed Assume.

### Audit (src/txgraph.cpp, src/util/feefrac.h, src/util/overflow.h)
- The only saturating sums in txgraph are CheckedFeePerWeightSum
  (:35-60), used ONLY inside G_ABORT_ON_FAILED_ASSUME debug blocks
  (:3381, :3488), each iterating transactions of a SINGLE
  cluster/chunk.
- Cluster size is hard-bounded: MAX_CLUSTER_COUNT_LIMIT = 64
  (txgraph.h:18), SetType = BitSet<64> (:161).
- Overflow handling is explicit even if reached: CheckedAdd detects,
  SaturatingAdd clamps, overflow flags gate the exactness Assume
  (AssumeMatches), and ChunkLinearizationInfo collapses saturated
  clusters into one conservative connected chunk (:1143-1153)
  instead of exposing a disconnected package.
- feefrac.h arithmetic itself: Mul/MulFallback produce a full
  96-bit product (int128 or exact (high,low32) decomposition —
  a*b = (high + (low>>32))*2^32 + uint32(low) holds for negative
  operands under two's complement); DivFallback's negative left-
  shift is well-defined under C++20 [expr.shift]; ByRatio cross-
  multiplication cannot overflow by construction (96-bit).
- Existing coverage: src/test/fuzz/feefrac.cpp fuzz target (UBSan
  build) + feefrac_tests.cpp.

### Bounded proof of unreachability (runnable computation, python3)
- fee (int64, sats): cluster fee sum <= 64 * MAX_MONEY
  = 134,400,000,000,000,000 < int64 max 9,223,372,036,854,775,807
  (margin 68.6x). Tighter: mempool txs spend disjoint UTXOs (no
  in-mempool double-spend), so ANY subset sum of mempool fees <=
  total supply 2.1e15 sats (margin 4.39e3).
- size (int32, WU): cluster size sum <= 64 * MAX_STANDARD_TX_WEIGHT
  = 25,600,000 < int32 max 2,147,483,647 (margin 83.9x).
- Constants verified in-tree: MAX_MONEY (consensus/amount.h:26),
  MAX_STANDARD_TX_WEIGHT = 400,000 (policy/policy.h:38),
  MAX_CLUSTER_COUNT_LIMIT = 64 (txgraph.h:18).

### Verdict
DISMISSED: saturation is defense-in-depth, unreachable by a wide
margin under the fork's own cluster limit and consensus/policy
bounds, and safe-by-construction even if hypothetically reached
(flagged clamps + conservative chunk collapse). No UB, no ordering
inversion, no reachable Assume failure. No test gap requiring a
local oracle (fuzz target + unit tests exist upstream-inherited).

### Exact commands
- grep/sed reads cited above; margin computation:
  python3 -c '<see journal body>' (recorded inline above).

### Limitations / queue
- Proof assumes cluster membership is capped at 64 for ALL paths
  into CheckedFeePerWeightSum (both call sites iterate one
  cluster's entries; AddTransaction callers enforce the limit
  before linking — F13's -limitclustercount validation is the
  config surface). If a future path links >64 into one cluster,
  the 68.6x fee margin still holds (needs >4000 max-fee txs).
- Queued (remaining from c1): shift/narrowing sinks in feefrac —
  PARTIALLY absorbed here (Mul/DivFallback + shift analysis above);
  the ctime/int64-backend differential remains a separate queued
  cell under backend-differential.

## Rotation note
Three cycles; bloom, wire-CompactSize, and txgraph saturation
families closed. feefrac remainder folded into backend queue.

## Cycle 4 (2026-08-01): feefrac shift/narrowing sinks — Mul/Div (__int128) vs fallback exact over the boundary corpus + UBSan clean; DISMISSED; campaign COMPLETE

### Draw
RE-RANK draw 165 over the 8-cell pool: raw=10771425690861073957,
masked 1548053654006298149 -> idx 5 -> #100 feefrac ctime/int64
backend differential (c3 queue). Branch: audit/sink-reverse-c4
from 0a4178cb88.

### Mechanism
util/feefrac.h: native __int128 Mul/Div vs portable MulFallback/
DivFallback (used on non-__int128 platforms). Suspicious corners
in the fallback: `a >> 32` and `low >> 32` (signed arithmetic
shifts of negatives), `(n.first % d) << 32` (left shift of a
negative mod — UB in C++14/17, DEFINED in C++20).

### Differential (/tmp/btc100c4_probe.cpp)
Corpus: 23 a-values (0, ±small, ±2^31 edges, ±2^32, INT64_MAX/MIN
halves and extremes) x 12 b-values (±1, ±2, ±1000, INT32 edges),
products filtered to int64-range (the documented contract), then
Div over 8 divisors x both round modes.
- Mul vs MulFallback: 0 differences (totally-ordered equality,
  high<<32|low reconstructed).
- Div vs DivFallback: 0/3200 differences.
- Same probe under -fsanitize=undefined: zero reports (the shift
  corners are C++20-defined).

### Verdict
DISMISSED: the two backends are exactly interchangeable over the
contracted domain, and the fallback's shift/narrowing shapes carry
no UB on this standard. #100's sink family is closed (c1 bloom,
c2 wire-CompactSize allocations, c3 txgraph saturation proof, c4
feefrac backends) — campaign COMPLETE.

### Limitations / queue
- The int64-range filter means >int64 products are unexamined —
  the contract says callers must not produce them (FeeFrac values
  are fee/size pairs, bounded well below); recorded, not a gap.
- #100 queue: empty.

## Rotation note
Four cycles; the sink family is closed.

## Postscript (2026-08-02, draw 233, raw=17388002019312348372, masked 8164629982457572564, idx 1/3): >int64-products tail — ABSORBED by #48 c5: the ByRatio ordering oracle checked products up to ~2^91-2^126 (INT64_MAX/4 fees x INT32_MAX/2 sizes, 21e14 x 1e8) against an exact cpp_int reference with 0/55,201 mismatches — FeeFrac::Mul returns __int128, so >int64 products are represented and compared exactly; the tail is examined, not just contract-bounded. #100 remains COMPLETE.
