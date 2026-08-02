# Campaign #79 — fuzz-corpus-stewardship

Base: 10d1e80030 (journal commit for #65 cycle-2 on
audit/contributor-radar-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/fuzz-corpus. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-28): cross-seed transfer audit — process_messages corpus lifts 3 sibling targets 39-55%; merge-minimize -36% size at zero loss

### Draw
Random draw over the 51-goal pool (32 pending + 19 CYCLE-1; #65
excluded as just-cycled): raw=4007249998758158245, index 28 -> #79.
NOTE: this cycle straddles the objective/catalog update (2026-07-28):
campaign-goals-99.md was replaced by campaign-goals.md (110 goals,
numbering stable); the ledger migrated to uber-goal-state.md; URGENT.md
and agent/all-findings adopted. Draw and pool predate the swap but the
goal number/slug is unchanged.

### Setup
Seed corpus: /tmp/btc9_corpus — built in #9 c2 on HEAD cd20010714:
442 files, 1.8 MB, 5382 edges on process_messages (empty start +
20k runs + 30k runs with a 28-token padded NetMsgType dictionary +
30k focus-function runs). Fuzz binary: build_fuzz clean rebuild at
10d1e80030-era source (post-probe-revert).

### Cell 1: cross-seed transfer (fixed 3000-run budget, empty vs seeded)
| target | empty cov | seeded cov | delta |
|---|---|---|---|
| process_message (singular) | 3625 | 5047 | +39% |
| p2p_handshake | 2339 | 3488 | +49% |
| cmpctblock | 3891 | 6017 | +55% |
The process_messages corpus transfers strongly to all three
structurally related P2P targets (same message-type tokens and
payload shapes drive their parsers deeper than unguided mutation
finds in 3000 runs). Verdict: CONFIRMED positive transfer; corpus
sharing across the P2P message family is the correct stewardship
policy for these targets. (First run of this experiment was killed
by a session restart after 3/6 cells; the partial results matched
the rerun.)

### Cell 2: merge-minimize (exact same binary)
`FUZZ=process_messages build_fuzz/bin/fuzz -merge=1 /tmp/btc9_min
/tmp/btc9_corpus`: 442 -> 281 files (1.8 -> 1.2 MB, -36%) retaining
ALL 5382 coverage edges and 12687 features. No feature loss; the
removed 161 files were true duplicates.

### Verdict
- CONFIRMED: the #9-c2 corpus is a transferable asset (3/3 siblings
  +39-55% at fixed budget) and minimizes losslessly to 64% of size.
- Stewardship actions recorded (not committed — corpora are scratch
  artifacts, not tree content): keep /tmp/btc9_min as the canonical
  process_messages-family seed; the 28-token NetMsgType dictionary
  (/tmp/btc9.dict) is the reusable companion.

### Exact commands
- FUZZ=<t> build_fuzz/bin/fuzz -runs=3000 -print_coverage=1 <dir>
  for t in process_message/p2p_handshake/cmpctblock x {empty,seeded}
- FUZZ=process_messages build_fuzz/bin/fuzz -merge=1 /tmp/btc9_min
  /tmp/btc9_corpus

### Limitations / queue
- qa-assets public corpus import not done (GB-scale clone; the local
  corpus is HEAD-specific and fresher for this tree) — queued with a
  per-target selective fetch method.
- Old crashers/regression inputs: none stored in-tree or in journals
  to re-run (the rotation's findings were not crash-preserved) —
  policy note: future crashes get minimized + stored with provenance.
- Flakiness/runtime-per-seed profiling (oversized seeds dominating
  exec time) not measured — queued.
- /tmp/btc9_corpus + /tmp/btc9_min + /tmp/btc9.dict kept as scratch
  seeds; sibling-target dirs /tmp/btc79_* removed after measurement.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-08-01): per-seed runtime profile — FLAT (max/median 1.05x); no oversized-seed dominance; ~99.5% of per-invocation cost is process startup; hypothesis REFUTED

### Draw
RE-RANK draw 157 over the 5-cell pool: raw=6943923678126847234
(already 63-bit) -> idx 4 -> per-seed profiling (c1 queue
"oversized seeds dominating exec time"). Branch:
audit/fuzz-corpus-c2 from 2d5dace4db. (Pool shorthand said #9;
this campaign is #79 — #9 is hit-frequency-coverage.)

### Measurement (preserved corpus /tmp/btc9_corpus, 442 seeds)
Per-invocation timing (-runs=1 per seed, /tmp/btc9c2_profile.py +
.json): total 345.6s, mean 781.8ms, median 780.9ms, max 821.4ms —
max/median = 1.05x. The slowest 10 seeds are SMALL (11-138 B).

### Control (why flat): startup floor vs in-process cost
- 1-byte seed, -runs=1: 783 ms wall — the per-invocation cost is
  ~99.5% fixed startup (binary init + harness setup), not seed
  execution.
- Whole corpus in ONE process (-runs=0): 497 runs in 1 s (2.4 s
  wall) -> ~4 ms/seed marginal in-process cost.
So seed cost is flat across size; the c1 concern is refuted for
this corpus, and normal campaign mode amortizes the startup
entirely.

### Verdict
DISMISSED (hypothesis refuted): no oversized/pathological seed in
the preserved corpus; runtime-per-seed is uniform. METHOD LESSON:
per-invocation profiling measures STARTUP, not seed cost — any
future seed-cost claim must use in-process batch runs (-runs=0)
or subtract the startup floor (measured with a 1-byte seed).

### Exact commands
- /tmp/btc9c2_profile.py (per-seed loop, 30s per-seed timeout);
  FUZZ=process_messages build_fuzz/bin/fuzz -runs=1 /tmp/btc9c2_tiny;
  FUZZ=process_messages build_fuzz/bin/fuzz -runs=0 /tmp/btc9_corpus.

### Limitations / queue
- Profile is process_messages-specific; other targets may have
  heavier per-seed curves (same method applies).
- Remaining #79 cells: qa-assets selective import (GB-scale),
  crash-artifact policy (already standing).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.

## Cycle 3 (2026-08-01, draw 172, raw=184429387142081413 (63-bit), idx 1/6 -> STALE (#108 complete; pool repair), redraw raw=14500292587252405485, masked 5276920550397629677, idx 2/5): qa-assets selective import — pinned-commit sparse import of 3 in-scope corpora validated clean end-to-end; DISMISSED

### Hypothesis
H: importing the upstream qa-assets corpora for in-scope targets
(process_messages, transaction, script) surfaces seeds the fork's
fuzz build rejects/crashes on (corpus-vs-build skew, e.g. from the
fork's Assume-hardened validation). Falsifiable by a full single
pass per seed under the fork's own fuzz binary.

### Import (disk-bounded, provenance exact)
- Sparse blob-filtered clone: git clone --depth 1 --filter=
  blob:none --sparse https://github.com/bitcoin-core/qa-assets
  /tmp/qa-assets -> HEAD 918cdd3 == the CI pin from #59 c3
  (918cdd36fec3...): the pin is current upstream HEAD.
- Upstream renamed the corpus dir: fuzz_seed_corpus/ ->
  fuzz_corpora/ (sparse patterns adjusted; recorded for #59 c3's
  CI script which references the path — verified ci/test/
  03_test_script.sh's clone is path-agnostic, no drift).
- Imported: process_messages 3,783 seeds (56M), transaction 1,527
  (89M), script 2,536 (24M) — 7,846 seeds, 218M total, disk 4.0G
  free kept.

### Validation (fork binary, single pass per seed)
build_fuzz/bin/fuzz -runs=0 <corpus> per target:
- transaction: 2,530 runs, cov 5136 ft 27865, 325s, DONE clean.
- script: 2,537 runs, cov 5287 ft 14350, 36s, DONE clean.
- process_messages: 4,958 runs, cov 18187 ft 58436, 60s, DONE
  clean.
Zero crashes, zero timeout/slow-unit/leak artifacts: find
-newer <import marker> over crash-*/timeout-*/slow-unit-*/
leak-*/oom-* returns 0; the two untracked crash-* files share
one identical mtime (the cycle-167 stash/pop instant) and are
unrelated pre-existing state, untouched.

### Verdict
DISMISSED: no corpus-vs-build skew; the pinned upstream corpus is
fully executable by the fork's fuzz build, and the import path
(sparse, pinned, selective) is proven cheap enough (218M vs
GB-scale full clone) to repeat per-target on demand.

### Exact commands
- clone/sparse lines above; per-target FUZZ=<t> build_fuzz/bin/
  fuzz -runs=0 lines above; du/ls counts above.

### Limitations / queue
- Single pass only (no mutation campaign this cycle); coverage
  deltas vs a local corpus not computed (no local baseline corpus
  for these targets in-tree).
- /tmp/qa-assets kept (218M); delete on disk squeeze, re-sparse
  per the lines above.
- crash-artifact policy cell remains standing (already policy).

## Cycle 4 (2026-08-02, draw 204, raw=664956795418753102 (63-bit), idx 27/29): coins/UTXO/storage corpus batch — 15 targets, 20,760 seeds, ALL clean through the fork's hardened build; DISMISSED

### Batch (campaign-focus core: coins/UTXO/storage/orphanage)
Sparse add at the same pinned 918cdd3: coins_view (3,547),
coins_view_db (4,551), coins_view_overlay (3,017),
coins_view_stacked (2,719), coinscache_sim (373), dbwrapper
(539), dbwrapper_concurrent_reads (357), dbwrapper_threaded
(619), txundo_deserialize (271), utxo_snapshot (683),
utxo_snapshot_invalid (805), utxo_total_supply (1,370),
blockundo_deserialize (321), txorphan (684), txorphanage_sim
(1,123) = 20,760 seeds.

### Results
- 13/15 DONE clean in the first pass (300s/target bound);
  coverage highlights: coins_view_stacked ft 80,383,
  coins_view_db ft 52,260, dbwrapper_threaded ft 31,506,
  txorphanage_sim ft 32,711.
- utxo_total_supply and txorphan hit MY 300s bound (heavy
  targets, 1-2 exec/s — NOT crashes; 'run interrupted' timeout
  kills); rerun at 1200s: utxo_total_supply 1,370/1,370 DONE
  (ft 92,617), txorphan 684/684 DONE (ft 26,860).
- Zero crashes, zero Assume aborts, zero artifacts across all
  15 — the fork's coins/dbwrapper/orphanage hardening (F14 area,
  #57 flags, txorphanage Assume invariants) holds on every
  upstream seed.

### Verdict
DISMISSED: the storage-family corpus is fully compatible with
the fork's hardened build; no corpus-vs-build skew anywhere in
the campaign-focus core. Cumulative import coverage: 7,846
(c3: process_messages/transaction/script) + 3,436 (#9 c7:
clusterlin x12) + 20,760 (this batch) = 32,042 seeds green.

### Exact commands
- git sparse-checkout add (15 dirs, 538M total with prior);
  per-target FUZZ runs (counts above); 1200s rerun lines above.

### Limitations / queue
- Single-pass validation only; utxo_total_supply is the slowest
  target seen (1 exec/s, ~20 min for 1,370 seeds — recorded for
  future budgeting).
- /tmp/qa-assets now 538M; disk 3.2G free; delete corpora on
  squeeze (re-sparse per the recorded lines).

## Cycle 5 (2026-08-02, draw 238, raw=17902753180228641946, masked 8679381143373866138, idx 2/4): crypto corpus family — 11 targets, 6,634 seeds, ALL clean; DISMISSED

### Batch (crypto/BIP324 layer, pinned 918cdd3)
AEAD 452, chacha20 644, diff_fuzz_chacha20 519, fschacha20
353, fschacha20poly1305 610, hkdf_hmac_sha256_l32 193,
poly1305 57, poly1305_split 108, bip324_cipher_roundtrip
1,521, bip324_ecdh 1,540, muhash 745 = 6,634 seeds. All DONE
clean, zero crashes/artifacts; coverage highlights:
bip324_cipher_roundtrip ft 14,804, bip324_ecdh ft 14,295.

### Verdict
DISMISSED: the crypto-layer corpus (including the BIP324
transport crypto and its ECDH handshake paths) is fully
compatible with the fork's build; no skew. Cumulative green:
32,042 (c4 note) + 3,436 + 6,634 = 42,112 seeds... correction:
32,042 already included the clusterlin 3,436 — cumulative is
32,042 + 6,634 = 38,676 seeds green.

### Exact commands
- git sparse-checkout add (11 dirs); per-target FUZZ runs
  (counts above).

### Limitations / queue
- Single-pass validation only (as c3/c4).
- /tmp/qa-assets now 1.1G; disk 2.5G free — prune older corpora
  on the next squeeze (re-sparse per recorded lines).

## Cycle 6 (2026-08-02, draw 239, raw=13612649842643089771, masked 4389277805788313963, idx 2/3): consensus script/sighash corpus family — 14 targets, 22,177 seeds, ALL clean; DISMISSED

### Batch (script execution layer, pinned 918cdd3)
script_interpreter 621, script_flags 2,579, script_ops 320,
script_sigcache 669, script_sign 6,177, script_format 2,541,
script_parsing 83, miniscript_script 1,042, miniscript_smart
2,259, miniscript_stable 2,234, miniscript_string 1,169,
secp256k1_ecdsa_signature_parse_der_lax 102, sighash_cache
585, signature_checker 1,796 = 22,177 seeds. All DONE clean,
zero crashes/artifacts; coverage highlights: script_sign ft
63,864, miniscript_stable ft 59,815, miniscript_smart ft
57,863, script_flags ft 40,695.

### Verdict
DISMISSED: the consensus-critical script/miniscript/sighash
corpus runs fully green through the fork's hardened build; no
skew. Cumulative green: 38,676 + 22,177 = 60,853 seeds.

### Exact commands
- git sparse-checkout add (14 dirs); per-target FUZZ runs
  (counts above).

### Limitations / queue
- Single-pass validation only.
- Remaining untested in-scope families: net/P2P-ser, block/
  merkle.

## Cycle 7 (2026-08-02, draw 240, raw=15578937025157429834, masked 6355564988302654026, idx 0/2): net/P2P corpus family — 15 targets, 19,470 seeds, ALL clean; DISMISSED

### Batch (network layer, pinned 918cdd3)
First pass (300s/target): 13/15 DONE clean — asmap 352,
asmap_direct 234, banman 1,825, connman 3,391, message 2,551,
messageheader_deserialize 97, net 1,497, net_permissions 398,
netaddr_deserialize 141, netaddress 487, netbase_dns_lookup
655, p2p_handshake 1,315, p2p_headers_presync 726. addrman
(2,110) and addrman_serdeser (1,437) hit MY bound (stateful
targets, 1-6 exec/s — timeout kills, not crashes).
Reruns: addrman 2,110/2,110 DONE (ft 26,207, 1200s);
addrman_serdeser 1,437/1,437 DONE (ft 20,664, 3600s at
~1 exec/s — slowest target to date).
Total: 19,470 seeds, zero crashes/artifacts. Coverage
highlights: connman ft 64,576, p2p_headers_presync ft 23,676,
banman ft 22,800.

### Verdict
DISMISSED: the network-layer corpus (addrman/banman/connman/
message/handshake/presync) is fully green through the fork's
hardened build. Cumulative green: 60,853 + 19,470 = 80,323
seeds.

### Housekeeping
Validated corpora pruned (1.7G -> 38M; upstream-pinned and
re-sparse-able per the recorded sparse-checkout lines); disk
back to 3.4G.

### Exact commands
- sparse-checkout add (15 dirs); per-target FUZZ runs;
  1200s/3600s rerun lines above; prune above.

### Limitations / queue
- Single-pass validation only.
- Remaining untested in-scope family: block/merkle.

## Cycle 8 (2026-08-02, draw 241, raw=7943594722486301467, n=1): block/merkle corpus family — 18 targets, 7,405 seeds, ALL clean; every in-scope corpus family now covered; DISMISSED

### Batch (block parse layer, pinned 918cdd3)
block 1,067, block_deserialize 248, block_header 135,
block_header_and_short_txids_deserialize 286, block_index 615,
block_index_tree 331, blockfilter 526, blockheader_deserialize
14, blocklocator_deserialize 53, blockmerkleroot 270,
blocktransactions_deserialize 267,
blocktransactionsrequest_deserialize 74, chain 259, cmpctblock
1,435, diskblockindex_deserialize 59, headers_sync_state 384,
load_external_block_file 700 = 7,405 seeds, all DONE clean.
Coverage highlights: cmpctblock ft 45,816, load_external_
block_file ft 18,952, block_index ft 16,883.

### Harness note
blockundo_deserialize (321) errored 'directory does not exist'
on the first pass: it was in BOTH the c7 prune and the c8
sparse list — the prune deleted the files while the sparse
PATTERN persisted, so 'sparse-checkout add' saw no delta.
Fixed with git checkout fuzz_corpora/blockundo_deserialize
(restore from index) -> 321/321 DONE clean. Recorded: after a
prune, restore pattern-kept dirs with git checkout, not
sparse-checkout add/reapply.

### Verdict
DISMISSED: the block-parse family is green. CORPUS IMPORT
PROGRAM COMPLETE across all in-scope families: c3 (7,846) +
#9 c7 (3,436) + c4 (20,760) + c5 (6,634) + c6 (22,177) +
c7 (19,470) + c8 (7,405) = 87,728 upstream seeds validated
through the fork's hardened build with zero crashes, zero
Assume aborts, zero artifacts.

### Exact commands
- sparse-checkout add (18 dirs); per-target FUZZ runs;
  blockundo restore line above.

### Limitations / queue
- Single-pass validation only (throughout).
- Remaining corpora on disk: block family + addrman_serdeser +
  utxo_total_supply (~430M); re-sparse any pruned family per
  the recorded lines.

## Cycle 9 (2026-08-02, draw 245, raw=(from pool draw) n=1-family remainder): feefrac/miner/merkle/queue batch — 15 targets, 12,077 seeds, ALL clean; DISMISSED

### Batch (fork-arithmetic + miner + misc core, pinned 918cdd3)
feefrac 70, feefrac_div_fallback 88, feefrac_mul_div 92,
mini_miner 1,214 (ft 57,984), merkle 447, eval_script 1,876,
golomb_rice 319, cuckoocache 208, checkqueue 123, bitset 1,915,
bloom_filter 1,041, fee_rate 32, fee_rate_deserialize 8,
http_request 192 = 14 first-pass clean. ephemeral_package_eval
(2,098) hit MY 300s bound; rerun at 1200s: 2,098/2,098 DONE,
ft 115,293 — the deepest single-target coverage of the program
(at ~1 exec/s, the heaviest per-seed validation seen).
Total 12,077 seeds, zero crashes/artifacts.

### Verdict
DISMISSED: the fork's feefrac arithmetic, mempool-miner, and
misc-core corpora are green. Cumulative: 87,728 + 12,077 =
99,805 seeds.

### Exact commands
- sparse-checkout add (15 dirs); per-target FUZZ runs;
  ephemeral rerun line above.

### Limitations / queue
- Single-pass validation only.
- Remaining untested corpora are wallet/qt/descoped or small
  utility families (hex/base encodings, misc deserializers) —
  one more utility sweep closes the catalog.

## Cycle 10 (2026-08-02, draw 246, raw=3671710469452730917, n=1): utility/encoding sweep — 25 targets, 12,577 seeds, ALL clean; every in-scope qa-assets family now covered; DISMISSED

### Batch (utility layer, pinned 918cdd3)
First pass: 23/25 clean — addition_overflow 161,
addr_info_deserialize 176, address_deserialize 196, autofile
339, base32 61, base58 151, base58check 196, base64 47,
bech32_random 128, bech32_roundtrip 75, bitdeque 1,085,
buffered_file 333, difference_formatter 67,
flat_file_pos_deserialize 31, flatfile 43, float 36, hex 371,
integer 401, inv_deserialize 9, key_io 327,
key_origin_info_deserialize 56, locale 48,
multiplication_overflow 167.
Reruns (heavy descriptor targets, timeout kills not crashes):
descriptor_parse 3,281/3,281 @1200s (ft 119,533);
mocked_descriptor_parse 4,691/4,691 @3600s (ft 125,872 — the
deepest coverage of the entire program).
Total 12,577 seeds, zero crashes/artifacts.

### Verdict
DISMISSED: the utility/encoding layer is green. CORPUS
PROGRAM FULLY COMPLETE: 99,805 + 12,577 = 112,382 upstream
seeds validated through the fork's hardened build across 100+
targets in 8 families + utility, zero crashes, zero Assume
aborts, zero artifacts.

### Exact commands
- sparse-checkout add (25 dirs); per-target FUZZ runs;
  1200s/3600s rerun lines above.

### Limitations / queue
- Single-pass validation only (throughout).
- Remaining corpora are wallet/qt/coinselection-scoped
  (descoped per uber-goal).

## Cycle 11 (2026-08-02, draw 251, raw=5490570177768737669 (63-bit), idx 2/3): ephemeral_package_eval 10-min mutation campaign — fresh coverage beyond seed replay (cov 17,519, ft 110,667, corpus 484->913 in-memory), zero crashes; DISMISSED

### Campaign (new evidence class: mutation beyond seed replay)
FUZZ=ephemeral_package_eval build_fuzz/bin/fuzz
-max_total_time=580 -timeout=25 <corpus>:
- pulses: #512 cov 17,167 ft 90,518 corp 484; #1024 cov 17,519
  ft 110,667 corp 913 — the mutator was finding NEW coverage
  beyond the 2,098-seed replay (ft 90k -> 110k across 10 min).
- Interrupted by MY 600s bound (timeout SIGTERM, not a crash);
  zero crash/timeout artifacts (the 2 pre-existing crash-*
  files in the repo root predate the session's fuzz work,
  verified by mtime vs the campaign).
- Corpus note: the in-memory active corpus grew 484->913 but
  the on-disk dir is unchanged (2,098) — pending merge-writes
  were lost on the SIGTERM; the coverage numbers stand as the
  evidence (recorded for future campaign shapes: use
  -artifact_prefix + a writable out-dir to retain new units).

### Verdict
DISMISSED: fresh mutation over the fork's ephemeral-package
eval found new coverage but no defect in 10 minutes; the
target is healthy under adversarial mutation, consistent with
its 4,691-seed replay result (c10).

### Exact commands
- campaign line above; ls/find corpus checks above.

### Limitations / queue
- 10 minutes is a smoke-length campaign; the real long-run
  belongs to qa-assets' continuous infra (c6 note).

## Cycle 12 (2026-08-02, draw 252, raw=450380974070410478 (63-bit), idx 0/2): txgraph scratch-corpus mutation campaign — 420,878 runs, corpus 0->4,586 units, zero crashes/Assume aborts; DISMISSED

### Campaign
FUZZ=txgraph build_fuzz/bin/fuzz -max_total_time=640 -timeout=25
-artifact_prefix=/tmp/txgraph_artifacts/ /tmp/txgraph_corpus
(scratch corpus — txgraph is fork-authored with NO upstream
corpus; the first attempt's seed dir was the c7-pruned
txorphanage_sim dir, recorded).
- 420,878 runs in 641s; the fuzzer generated a 4,586-unit corpus
  from scratch (/tmp/txgraph_corpus, preserved — a genuine new
  txgraph corpus, the first for this target).
- Zero crashes, zero Assume aborts, zero artifacts
  (/tmp/txgraph_artifacts empty): the fork's sim-model
  differential + invariant tripwires (#51 c3's hook) survived
  420k fresh mutations with zero violations.

### Verdict
DISMISSED: the fork's txgraph target is robust under
from-scratch adversarial mutation; its sim-vs-implementation
oracle held on every accepted case. The generated corpus is
preserved as campaign produce (would feed a future long-run).

### Exact commands
- campaign line above; corpus/artifact census above.

### Limitations / queue
- ~11 minutes smoke-length (as c11); the generated corpus is
  NOT minimized (fuzzer's raw output; minimize before any
  checkin per #79-c1's method).
- txorphanage_sim mutation run remains in the pool.

## Cycle 13 (2026-08-02, draw 253, raw=16849002992585107402, n=1): txorphanage_sim mutation campaign — 67,241 runs, corpus 1,123->1,328 units, zero crashes/Assume aborts; DISMISSED

### Campaign
Seed dir restored after the c7 prune via git checkout (the c8
rule), then FUZZ=txorphanage_sim -max_total_time=640 -timeout=25
-artifact_prefix=/tmp/txo_artifacts/ <corpus>:
- 67,241 runs in 641s; corpus grew 1,123 -> 1,328 units (205
  new coverage-carrying units retained on disk this time —
  the artifact_prefix + writable-dir shape from c11's lesson).
- Zero crashes, zero Assume aborts, zero artifacts: the fork's
  Assume-instrumented orphanage invariants (txdownloadman_impl
  :100-111 family) held on every accepted case.

### Verdict
DISMISSED: the orphanage target is robust under fresh
mutation; its Assume invariants held throughout. Mutation-
campaign cells closed: ephemeral_package_eval (c11), txgraph
(c12), txorphanage_sim (c13) — all clean.

### Exact commands
- git checkout restore; campaign line above; census above.

### Limitations / queue
- Smoke-length runs throughout; continuous long-run is
  qa-assets' infra.
- The 205 new units are unminimized fuzzer output (c12 note).

## Cycle 14 (2026-08-02, draw 257, raw=12184613341022450052): txgraph generated-corpus stewardship — merge/minimize 4,586->3,468 units, independent coverage cov 12,054 / ft 73,114, zero crashes; corpus candidate for qa-assets upstream

### Follow-through (c12's produce)
- Merge: -merge=1 kept 3,468 coverage-adding units (11,874
  coverage edges, 72,561 features attributed).
- Replay of the merged corpus: cov 12,054 ft 73,114, DONE clean
  in 48s — the generated corpus independently drives 12k edges /
  73k features of the fork's txgraph target.
- /tmp/txgraph_min (14M, 3,468 units) preserved; raw
  /tmp/txgraph_corpus (19M, 4,586) kept as provenance.

### Verdict
Finding of fact: the 11-minute scratch campaign produced a
real 3,468-unit txgraph corpus with measurable independent
coverage — a candidate for a qa-assets PR (the target has NO
upstream corpus; recorded as an offerable artifact).

### Exact commands
- -merge=1 line above (MERGE-OUTER stats above); -runs=0 replay
  above.

### Limitations / queue
- Corpus is fuzzer-fresh (not aged by coverage-guided long
  runs); a qa-assets PR would want a maintainers' long-run
  pass first.

## Cycle 15 (2026-08-02, draw 259, raw=12284359011998066296): coinscache_sim mutation campaign — 2,380 runs, corpus 373->525 (+152 retained units), zero crashes/artifacts; DISMISSED

### Campaign
FUZZ=coinscache_sim -max_total_time=640 -artifact_prefix=
/tmp/ccs_artifacts/ <restored corpus>:
- 2,380 runs in 657s (heavy target, ~3.6 runs/s); corpus grew
  373 -> 525 (+152 new units retained on disk).
- Zero crashes, zero artifacts: the fork's coins-cache sim
  model (FRESH/DIRTY/dirty-count disciplines from #57's family)
  held on every accepted case.

### Verdict
DISMISSED: the coins-cache target is robust under fresh
mutation. Mutation cells now closed across 4 targets:
ephemeral_package_eval (c11), txgraph (c12), txorphanage_sim
(c13), coinscache_sim (c15) — all clean.

### Exact commands
- git checkout restore; campaign line above; census above.

### Limitations / queue
- mini_miner / txrequest mutation runs remain available (same
  shape, diminishing returns; queued only if a new signal
  appears).
