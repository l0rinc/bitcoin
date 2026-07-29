# Campaign #102 — durable-suspicion-replay

## Cycle 1 (2026-07-29): suspicion-index delivered (A1-A11) + L1 blind replay confirms unreachability, sharper UB decomposition

### Draw
Random draw over the 3-goal eligible pool (2 pending + 1 CYCLE-1,
#46 excluded as just-cycled): raw=7007359828371084343, index 1 ->
#102 (first cycle). Branch: audit/durable-suspicion-replay from
f273bc6d13 (#46 c1 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #102's
campaign-focus block holds finding-composition text — same offset
artifact class; title+slug authoritative.

### Deliverable 1: suspicion/artifact index
agent-journal/suspicion-index.md — 11 entries (A1-A11): preserved
scripts, seed corpora, perf traces, latent notes, each with
provenance, trust boundary, status, confidence, and resume command.
On-disk existence of every /tmp artifact verified at write time.

### Deliverable 2: cross-model blind replay of L1 (highest-risk
unresolved artifact)
Protocol: fresh subagent given ONLY the code pointers and the
question (CBloomFilter ctor zero-input math + production
reachability), NOT the old verdict. Compare with the prior L1
verdict (findings-index: "math UB at nElements=0/nFPRate=0;
test-only reachability; production uses copy ctor only; FILTERLOAD
uses raw deserialization + IsWithinSizeConstraints").

Replay result: CONFIRMS the old verdict and SHARPENS it:
- UB decomposition: nElements==0 -> INTEGER division by zero at
  src/common/bloom.cpp:40 (nHashFuncs line, 0/0 — fires for any
  nElements==0 regardless of nFPRate); nFPRate==0 -> log(0) = -inf
  (defined for doubles), but the (unsigned int)(+inf) float->int
  cast is UB at bloom.cpp:34. (The old note conflated both into
  "div-by-zero/log(0)"; the replay's line-level decomposition is
  the precise version.)
- Reachability REFINED: production never calls the sizing ctor
  (FILTERLOAD: default-ctor + wire-deserialize + post
  IsWithinSizeConstraints, behind NODE_BLOOM which is DEFAULT-OFF);
  the only other production construction is a copy ctor. And the
  old "test-only reachability" weakens further: even the fuzz
  harness clamps nElements >= 1 (bloom_filter.cpp:85-86) and unit
  tests use nonzero literals — the UB is unreachable even in
  tests/fuzz today. It would fire only for a FUTURE caller.
- File moved since the old note: src/bloom.{h,cpp} ->
  src/common/bloom.{h,cpp} (replay verified at current paths).
- Upstream fix: l0rinc PR 35818 "bloom: avoid undefined sizing
  calculations" — OPEN as of 2026-07-28 (watch stands).

### Verdict
- L1: CONFIRMED-LATENT, replay-verified (two independent passes now
  agree: unreachable from any current caller incl. fuzz/tests).
  No local change (upstream fix pending; no reachable caller to
  protect locally; minimal-diff rule).
- Index: delivered. Verdict on the campaign method: blind replay
  cost one subagent run and produced a strictly sharper record —
  worth repeating for future LATENT items (A11 next).

### Exact commands
- blind replay via explore subagent (prompt: code pointers +
  question only; transcript in session history)
- PR state: api.github.com/repos/bitcoin/bitcoin/pulls/35818
- artifact existence check: ls -d /tmp/btc{50,101}* /tmp/lw_* etc.

### Limitations / queue for cycle 2
- A11 (kernel input_index assert) is the next replay candidate.
- The index is /tmp-based (volatility noted per entry; the seeds
  of record are reconstructible from the journals' constructors).
- No cross-MODEL (different vendor) replay available on this host;
  the replay is cross-context (zero-prior) which is the available
  independence form.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
