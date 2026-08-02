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

## Cycle 2 (2026-07-29): A5 replay against the v28.2 release binary — same abort, independent verifier form

### Draw
Re-rank draw over the rebuilt 7-cell queue:
raw=2757803267336390011, index 4 -> #102 (second cycle; c1 queue
"replay round for A-items"). Branch: audit/durable-suspicion-c2
from c1b1a23a64 (#95 c4 bookkeeping).

### Replay subject
A5 (-capturemessages aborts node on capture IO failure; classified
upstream-matching by SOURCE comparison in #43 c2). Replay form:
BEHAVIORAL — the actual v28.2 release binary under the same fault,
no source argument needed.

### Replay result
- v28.2 -capturemessages on regtest: capture dirs created.
- Fault applied (message_capture replaced by a regular file):
  node DIED within seconds. debug.log: "EXCEPTION: filesystem
  error: cannot create directories: Not a directory
  [.../message_capture/127.0.0.1_18444] ... bitcoin in msghand".
- Same shape as HEAD (uncaught exception escaping the msghand
  thread -> abort), with the throw at create_directories in this
  build (v28.2 predates some AutoFile hardening) — the uncaught
  propagation is identical.

### Verdict
A5 UPGRADED from source-matching to behavior-verified: the abort
is reproduced on the actual upstream release binary. No change to
the classification (debug-only option, upstream behavior) — the
journal's evidence tier rises.

### Exact commands
- /tmp/btc102_replay.sh (v28.2 binary, fault injection, state check)
- releases/v28.2/bin/bitcoind (Bitcoin Core v28.2.0)

### Limitations / queue
- The replay used the create_directories fault (HEAD replay used
  the fopen-EISDIR fault); both throw into the same uncaught site —
  the difference is noted, the class is identical.
- A8 (perf attribution replay) is superseded by HEAD measurements;
  remaining replay candidates: none pressing (A-items are green).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-08-02, draw 186, raw=8926299595551660088 (63-bit), idx 44/47): A11 replay — kernel input_index asserts confirmed at TODAY's upstream head; sharpened: ZERO in-tree callers (not even tests); replay queue now empty

### Replay (fresh context, claims from #46 c1 re-derived)
A11: plain asserts on public C API parameters —
btck_transaction_get_input_at (bitcoinkernel.cpp:578) and
btck_script_pubkey_verify (:717): assert(input_index <
vin.size()); asserts-on -> abort; NDEBUG downstream -> UB; no
status path, no documented precondition.
1. Upstream identity at 556988790a (fetched this cycle): the same
   two asserts, upstream lines 540/676. WIP-API status unchanged.
2. Caller census: grep across src/ finds NO caller outside
   bitcoinkernel.{h,cpp}/wrapper — not even test_kernel. The
   functions are exercised only by EXTERNAL kernel consumers, so
   the OOB path is unreachable from anything this tree ships.
3. Behavioral tier: asserts present in this tree's builds (c1
   verified the abort class); the NDEBUG-UB arm is a downstream-
   build property, unchanged.

### Verdict
A11 CONFIRMED-LATENT, replay-verified with one sharpening (zero
in-tree callers incl. tests). No local change (minimal-diff rule;
upstream WIP; watch via #42 stands). Replay queue: EMPTY
(A-items green, A5 behavior-verified, A8 superseded, A11 done).

### Exact commands
- git show origin/master:src/kernel/bitcoinkernel.cpp | grep
  assert(input_index (540/676); grep caller census above.

### Limitations / queue
- The replay queue is empty; new suspicions arrive via other
  campaigns' Limitations tails (standing rule).

## Cycle 4 (2026-08-02, cycle 255): pre-existing crash-* artifact analysis — 30+ targets scanned, ZERO reproduction; stale-harness verdict; files left untouched

### Artifacts
Two untracked crash files in the repo root (predate this
session's work; preserved-by-convention, never staged):
- crash-ad6d...814 (31 B, starts 16 ec 27 27..., 'un' tail)
- crash-e411...b11 (13 B).

### Reproduction scan (negative)
Each file against 30+ fuzz targets across all families
(process_messages, txgraph, txorphanage_sim,
ephemeral_package_eval, mini_miner, txdownloadman, transaction,
tx_deserialize, script_flags, merkleblock_deserialize,
partialmerkletree_deserialize, block, tx_package_ephemeral,
mini_miner_opreturn, ephemeral, txrequest, txorphan_protected,
txorphan, utxo_total_supply, coins_view_stacked, coinscache_sim,
headers_sync_state, p2p_headers_presync, load_wallet, psbt,
psbt_base64_decode, wallet_fees, crypto, diff_fuzz_chacha20,
strprintf, num3072_mul, feefrac): ZERO crashes, zero asserts,
zero sanitizer reports. Nothing current reproduces.

### Verdict
DISMISSED (stale artifacts): most likely seeds from a prior
session's custom /tmp harness (the load_wallet bring-up family
fd74c4a7c2 mentions /tmp/lw_crash_flags_seed) — they reproduce
nothing against the current build and carry no live-defect
signal. Left in place, untracked, per the user's-file rule.

### Exact commands
- xxd dumps above; per-file x per-target -runs=1 loop above.

### Limitations
- Not scanned against EVERY target (33 of ~100+); the families
  most consistent with the input shapes are all in the scan.
- Provenance is inference (no metadata in libFuzzer artifacts).
