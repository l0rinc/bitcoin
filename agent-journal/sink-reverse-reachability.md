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
