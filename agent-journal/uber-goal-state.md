# Uber goal state (authoritative ledger)

MIGRATED from uber-rotation.md 2026-07-28: catalog replaced by the user
(campaign-goals-99.md deleted, campaign-goals.md = 110 goals, goal
numbers/slugs stable for 0-98). This file is authoritative from here;
uber-rotation.md is historical only. New requirements adopted:
URGENT.md live list + agent/all-findings append-only archive.

---

# Uber-goal rotation ledger

The uber-goal works the 99 campaigns in agent-journal/campaign-goals-99.md
(shared boilerplate + per-goal campaign-focus text). Each turn: pick the next
campaign by the rotation policy, run a bounded cycle per its protocol, commit
journal updates, then record state here and rotate. Campaign prompts are
reconstructed as: shared boilerplate + the goal's campaign-focus section.

## Rotation policy
- Severity-first ordering, then least-recently-run. Skip or defer campaigns
  whose prerequisites are missing on this machine (e.g. dm-flakey for 72,
  KLEE/CBMC for 77 — note as deferred, don't grind).
- A few cycles (typically 1–4 turns) per campaign, then rotate — never park
  on one campaign until the user says so. Bounded slices; exact handoff in
  the campaign's own journal each turn.
- Every finding gets one independent commit as Lőrinc <pap.lorinc@gmail.com>;
  journal-only commits for verdicts. Never claim the tree exhausted.
- Branch: one campaign branch per campaign (audit/<slug>) from the ledger
  base; journals committed there. Reviews/ stays on audit/resurrection.
- The three watch crons keep firing; answer them in their own turns.

## Campaign state
| # | campaign | state | last cycle | notes |
|---|----------|-------|-----------|-------|
| 26 | cross-subsystem-bug-shapes | DONE | 2026-07-27 | S1/S2/S5 mapped; journal on audit/resurrection |
| 33 | external-vulnerability-variants | DONE | 2026-07-27 | E1-E10; 0 local variants; audit/external-vulns |
| 85 | bitcoin-consensus-mutation | DONE | 2026-07-27 | 6 mutants all killed; journal on audit/resurrection |
| 86 | bitcoin-chainstate-symmetry | DONE | 2026-07-27 | all 7 areas locked; C2 defect = own PR 35714; C6 caveat noted |
| 89 | bitcoin-p2p-accounting | DONE | 2026-07-27 | P1-P7 dismissed; journal on audit/resurrection |
| 97 | cpp-defect-taxonomy | DONE | 2026-07-27 | all 18 classes + leftovers; audit/cpp-taxonomy |
| 98 | float-sanitizer-fuzz-exclusions | DONE | 2026-07-27 | 1 finding (99d98861fc); audit/float-sanitizer |
| (all others) | — | PENDING | — | unrun |
| 88 | bitcoin-wallet-recovery | DONE | 2026-07-27 | 6 dismissed; W4 finding fixed (0e7a8fabb5) |
| 87 | bitcoin-mempool-accounting | DONE | 2026-07-27 | all 6 dismissed (strong existing oracles) |
| 82 | secp-field-scalar-matrix | DONE | 2026-07-27 | both backends green same-seed; tests/noverify pair green |
| 83 | secp-group-ecmult | DONE | 2026-07-27 | parity oracles green both backends + exhaustive order-13 |
| 84 | secp-nonce-session | DONE | 2026-07-27 | all 5 dismissed; zero-before-checks enforcement verified |

## Done since seeding
| 5/52 | boundary/integer | DONE | 2026-07-27 | B1-B4 inclusive-boundary consistent; audit/boundary-integer |
| 62 | rejected-finding-resurrection | DONE | 2026-07-27 | R1 attack on P2.1 FAILED (dismissal strengthened) |
| 56 | stale-pr-resurrection | DONE | 2026-07-27 | 33916 gap confirmed, target queued; scan empty |
| 96 | todo-deferred-work | DONE | 2026-07-27 | 66 TODOs, top falsified: 1 masked, 1 own-PR, rest design |
| 20 | micro-optimization | DONE | 2026-07-27 | IsRoutable dedup -8.8% wall (769822b5a6) |
| 0 | continuous-bug-mining | CYCLE-1 | 2026-07-27 | bump fuzz target delivered (da8b249776) |
| 4 | public-interface-contracts | CYCLE-1 | 2026-07-27 | maxmempool assert fix (36156ad934) |
| 8 | locking-threading | DONE | 2026-07-27 | all 4 areas dismissed |
| 15 | public-object-validation | DONE | 2026-07-27 | all paths gate consistently |
| 4 | public-interface-contracts | CYCLE-2 | 2026-07-27 | RPC bounds consistent; c1 finding RETRACTED (5a16d316af) |
| 6 | serialization-untrusted-input | CYCLE-1 | 2026-07-27 | compact-block + addrv2 bounded |
| 27 | error-path-state | DONE | 2026-07-27 | failure contracts clean (2 shapes) |
| 7 | resource-exhaustion-variants | CYCLE-1 | 2026-07-27 | HTTP accounting bounded |
| 3 | current-pr-leftovers | DONE | 2026-07-27 | TopUpWithDB fix (2a4e8edcfc); maxmempool FP retracted |
| 9 | hit-frequency-coverage | CYCLE-1 | 2026-07-27 | coins subsystem fully covered |
| 11 | sanitizer-valgrind | DONE | 2026-07-27 | ASan/LSan/valgrind clean |
| 12 | static-analysis-true-positives | DONE | 2026-07-27 | CSA clean; 1 FP documented |
| 13 | secret-lifetime-zeroization | DONE | 2026-07-27 | lattice complete, boundaries documented |
| 14 | secret-control-flow | DONE | 2026-07-27 | BIP324 lattice complete |
| 18 | exhaustive-algebraic | CYCLE-2 | 2026-07-28 | script (8e7513bb1c) + Coin composition (4c27dad486) round-trips closed, both mutation-verified; fuzz-gap dismissed |
| 19 | benchmark-integrity | CYCLE-3 | 2026-07-28 | prevector fix (138ef3c044); mutation-sweep + units/batching + timed-region all honest; queue exhausted |
| 28 | weak-test-oracles | CYCLE-2 | 2026-07-28 | amount battery 5/5 killed; merkleblock battery 2 survivors fixed (50e9d14750), 1 equivalent |
| 16 | api-misuse-resistance | CYCLE-2 | 2026-07-28 | 5+7 kernel C API @pre doc fixes (b6b48987a5, 8b0e92b4a2); #33943 precedent journaled |
| 61 | stateful-contract-fuzzing | CYCLE-3 | 2026-07-28 | sigcache oracle (a4ff67417e); c2/c3 assessments all already-strong; triage guidance updated |
| 30 | security-logging | CYCLE-3 | 2026-07-28 | 2 injection fixes; amplification dismissed by design (0 bytes at default, 14.6KB/round at -debug) |
| 31 | cross-layer-contracts | CYCLE-3 | 2026-07-28 | +gettxoutsetinfo use_index restriction (9396f0b414); 3 doc/RPC layer fixes total |
| 29 | dead-stale-code | CYCLE-2 | 2026-07-28 | util/common + node/wallet/script scans clean; SAFE_CHARS_FILENAME parked |
| 17 | build-matrix-modules | CYCLE-3 | 2026-07-28 | c1-c3 all clean (fbe821c003, own branch) |
| 18 | exhaustive-algebraic | QUEUE-COMPLETE | 2026-07-28 | c1-c3 round-trips all mutation-verified (c382122eeb, own branch) |
| 31 | cross-layer-contracts | CYCLE-4 | 2026-07-28 | testnet-key examples labeled (b2a0c38154, own branch) |
| 36 | cross-tool-analysis-matrix | CYCLE-1 | 2026-07-28 | UBSan unit suite: 1 UB found+fixed (22aa75a2eb, own branch) |
| 37 | build-dead-zones | CYCLE-1 | 2026-07-28 | parity/empty-arm/#if 0/DEBUG_LOCK clean (442db1aa87, own branch) |
| 39 | generated-artifact-determinism | CYCLE-1 | 2026-07-28 | raw+json regen byte-identical (4bdf490240, own branch) |
| 43 | option-api-lifecycle | CYCLE-1 | 2026-07-28 | -prevoutfetchthreads lifecycle clean (331048ba1e, own branch) |
| 45 | constant-time-declassification | CYCLE-1 | 2026-07-28 | AES ctaes + CBC padding constant-time; declassifications intended-only |
| 47 | build-ci-parity | CYCLE-1 | 2026-07-28 | 4 registration/preset cells clean (f874c8a9c3, own branch) |
| 51 | differential-metamorphic | CYCLE-1 | 2026-07-28 | full-UTXO undo oracle (47e5bf2f95, own branch) |
| 59 | supply-chain-security-gates | CYCLE-1 | 2026-07-28 | script_assets sha256 pin (4124803dff, own branch) |
| 65 | contributor-branch-radar | CYCLE-1 | 2026-07-28 | l0rinc radar (bfd36c032b, own branch) |
| 66 | backport-correctness | CYCLE-1 | 2026-07-28 | 18/18 fixes unreachable from ledger tip (ce7810b484, own branch) |
| 68 | architecture-abi-parity | CYCLE-1 | 2026-07-28 | no arch skips; endian-safe (b11311d7cf, own branch) |
| 71 | deterministic-simulation | CYCLE-1 | 2026-07-28 | tip-extension fault class (b427b59b54, audit/fuzz-target-gaps) |
| 73 | network-state-machine | CYCLE-1 | 2026-07-28 | BIP324 replay/reorder oracle (754add19dd, own branch) |
| 90 | historical-knowledge-recipes | CYCLE-1 | 2026-07-28 | 14 recipes (abb74fe38c, own branch) |
| 91 | compiler-binary-hardening | CYCLE-1 | 2026-07-28 | BTI enforcement toolchain-inactive (12ec75620a, own branch) |
| 94 | bindings-ffi-parity | CYCLE-1 | 2026-07-28 | View/Range lifetime doc fix (0a6c377ddb, own branch) |
| 95 | database-semantics-differential | CYCLE-1 | 2026-07-28 | leveldb exact sync (c784869048, own branch) |
| 75 | build-throughput-cacheability | CYCLE-1 | 2026-07-28 | no-op 0.19s stable; generator cascade restat-bounded; fan-out leaf-only; warm 2.85s / cold 30.78s incremental |
| 22 | full-sync-ibd-profile | CYCLE-2 | 2026-07-28 | two-node P2P IBD: bloom reset-per-tip 40.6% CPU, clean-flag fix c8f53e58d9 (-58% user); checks-on 17.0s vs 2.96s re-confirmed |
| 1 | comment-code-contract | CYCLE-1 | 2026-07-28 | LockPoints bound comment wrong (b1c267c9f1); AddCoin/lock/nullptr-parent claims verified true |
| 0 | continuous-bug-mining | CYCLE-2 | 2026-07-28 | TODO evidence sweep: 56 production items, 0 defects, 7 verified risk-map cells |
| 60 | reviewer-preference-skill | CYCLE-1 | 2026-07-28 | l0rinc seam: 7 rules (R1-R7), held-out 2/3+1 refined; reviewer map n=5 |
| 36 | cross-tool-analysis-matrix | CYCLE-2 | 2026-07-28 | clang-18 differential green; 4 clang-only warnings triaged (3 fuzz-only-helper, 1 test-annotation) |
| 76 | reproducible-builds | CYCLE-1 | 2026-07-28 | A/B rebuild: code bit-identical; 1-byte secp comp_dir DWARF delta attributed; Guix packaging sound |
| 9 | hit-frequency-coverage | CYCLE-2 | 2026-07-28 | net_processing sancov: all gaps harness-scope; 0/23 alarm resolved as inlining artifact via per-line PC check |
| 47 | build-ci-parity | CYCLE-2 | 2026-07-28 | install manifest declarative-single-source; 17-file install set exact match |
| 81 | spec-vector-drift | CYCLE-1 (retro) | 2026-07-28 | BIP324+RFC8439 vectors byte-exact (0f6c2640b7, own branch; row restored) |
| 81 | spec-vector-drift | CYCLE-2 | 2026-07-28 | BIP173/350 bech32(m): no drift across unit/key_io/functional layers |
| 76 | reproducible-builds | CYCLE-2 | 2026-07-28 | depends pins exact (qrencode primary 404, fallback serves pinned bytes); secp ccache = absolute-I key divergence, not uncacheable |
| 21 | rebuild-recovery-profile | CYCLE-2 | 2026-07-28 | tx-heavy reindex-chainstate: 6.8s user, 85% secp256k1 EC math; checks negligible at 610 blocks; harness lessons logged |
| 2 | assertion-invariant-audit | CYCLE-1 | 2026-07-28 | fork production Assumes all construction-tautologies; cache-overflow fix verified; no input-validation-by-assert |
| 65 | contributor-branch-radar | CYCLE-2 | 2026-07-28 | perf seam: CheckBlock dup-check 1.85x branch locally absent (equivalence plausible), prevector-36 + serialization seeds recorded |
| 79 | fuzz-corpus-stewardship | CYCLE-1 | 2026-07-28 | cross-seed transfer +39-55% on 3 P2P siblings; merge-minimize -36% size zero-loss |
| 10 | fuzz-target-gaps | CYCLE-2 | 2026-07-28 | load_wallet harness delivered (fd74c4a7c2); bring-up crash = harness-oracle bug, production clean |
| 48 | property-oracle-expansion | CYCLE-1 | 2026-07-28 | CompactSize exhaustive battery delivered (8b7d8ac878), 1/1 boundary mutant killed |
| 67 | release-version-differential | CYCLE-1 | 2026-07-28 | v28.2/v0.20.1 ↔ HEAD sync+handshake clean both directions; BIP324 fallback correct |
| 24 | disk-io-amplification | CYCLE-1 | 2026-07-28 | regtest IBD: ~2x byte amplification (undo+index), bounded 16MB prealloc; no pathological growth |
| 63 | loupe-style-pipeline | CYCLE-1 | 2026-07-28 | fee-estimator zero-state waste confirmed+fixed (675011ba86): 20.4%->0 samples, -34% IBD user |
| 35 | mutation-testing | CYCLE-1 | 2026-07-28 | ReadVarInt overflow guards test-blind (M2/M3 survived); oracle delivered+verified (083afedbf1) |
| 92 | abi-alignment-aliasing | CYCLE-1 | 2026-07-28 | kernel enum name-maps value-independent; by-value structs static-linkage contract; aliasing sweep clean |
| 78 | translation-validation | CYCLE-1 | 2026-07-28 | Assume-erasure contract validated at binary: fork hardening zero-cost in release |
| 100 | sink-reverse-reachability | CYCLE-1 | 2026-07-28 | bloom ctor div-by-zero test-only latent; empty-filter guarded (CVE-2013-5700 present) |
| 7 | resource-exhaustion-variants | CYCLE-2 | 2026-07-29 | UTXO-scan/resize race: fixed in-tree (e049f064e1 unique-lock cursor); upstream master verified still racy |
| 94 | bindings-ffi-parity | CYCLE-2 | 2026-07-29 | enum mapping static_assert tables (073d543f26), reorder tripwire fires at :268 |
| 64 | finding-dedup-recurrence | CYCLE-1 | 2026-07-29 | findings-index.md built; 5 fixes confirmed NOT in lineage (F1,F2,F3,F7,F9) |
| 104 | analogical-vulnerability-translation | CYCLE-1 | 2026-07-29 | EMPTY-TRUTHINESS-FLIP fails on PartialMerkleTree (fail-closed + Assume contracts) |
| 90 | historical-knowledge-recipes | CYCLE-2 | 2026-07-29 | R15-R22 added (sancov inlining, reindex gating, MiniWallet API, io sampling, mutant-first, pool mechanics, dict fuzzing, flag persistence) |
| 71 | deterministic-simulation | CYCLE-2 | 2026-07-29 | crash-resume durability invariant delivered (6c6e7d9f87), 3k scans clean |
| 32 | whole-history-leftovers | CYCLE-1 | 2026-07-29 | multiply-first percentage shape: no exploitable survivor (range/memory-bounded) |
| 69 | backend-differential | CYCLE-1 | 2026-07-29 | secp wide-multiply int128-vs-int64 differential: 4/4 suites pass, no divergence |
| 103 | finding-composition | CYCLE-1 | 2026-07-29 | capability graph: no realizable chain; 3 edges tested broken (L2 fixed, F4+F5 fixed, F8 bounded) |
| 53 | timing-side-channel | CYCLE-1 | 2026-07-29 | AES-CBC padding dudect: Welch t 1.53/1.69/-1.14 (no leak), confirms #45 code-read |
| 74 | memory-pressure-allocator | CYCLE-1 (retro) | 2026-07-28 | mempool accounting honest (1.13x RSS/usage @8k); glibc drain retention not a leak (2ef390de05, row restored) |
| 74 | memory-pressure-allocator | CYCLE-2 | 2026-07-29 | LockedPool oversize alloc -> graceful bad_alloc via RPC (fault-injected); no residue |
| 105 | project-bug-autopsy-recurrence | CYCLE-1 | 2026-07-29 | txgraph saturation family autopsy: recurrence mapped, no uncovered sibling |
| 45 | constant-time-declassification | CYCLE-2 | 2026-07-29 | walletpassphrase: KDF-only throttle 0.10s/attempt measured, no lockout by design |
| 109 | whole-feature-public-path | CYCLE-1 | 2026-07-29 | compact-block relay mapped; all boundaries guarded incl. wrong-vs-malicious split |
| 66 | backport-correctness | CYCLE-2 | 2026-07-29 | 5 out-of-lineage fixes backported+verified (e15c4025e5, 84a3913096, 508d9edfca, 75c0616c24, b73b7c5d39) |
| 39 | generated-artifact-determinism | CYCLE-2 | 2026-07-29 | full JSON sweep 60/60 byte-identical (9 tree + 51 univalue) |
| 37 | build-dead-zones | CYCLE-2 | 2026-07-29 | config-dead zones clean: ZMQ/USDT/chainstate-util all properly gated |
| 95 | database-semantics-differential | CYCLE-2 | 2026-07-29 | dbwrapper contracts hold: WriteBatch atomicity + HEAD_BLOCKS crash protocol, scan-only iterators |
| 108 | adversarial-artifact-generation | CYCLE-1 | 2026-07-29 | hostile V1 peer artifact: 4/4 classes classified correctly (magic/checksum/oversize/valid) |
| 41 | history-seed-archaeology | CYCLE-1 | 2026-07-29 | fee_estimates.dat version gate fails closed both directions (v28.2 rejects 309900 non-fatally) |
| 50 | fuzz-introspector-blockers | CYCLE-1 | 2026-07-29 | PSBT blocker = harness truncation (ConsumeRandomLengthString); ~500 serialize edges unreachable |
| 57 | local-reasoning-contracts | CYCLE-1 | 2026-07-29 | m_all_zero contracts observable+guarded; broken-discipline mutation caught at bloom_tests:535 |
| 68 | architecture-abi-parity | CYCLE-2 | 2026-07-29 | char-signedness sweep clean: no sign-sensitive plain-char use; serialize forbid guards the class |
| 93 | system-fault-injection | CYCLE-1 | 2026-07-29 | mid-flush crash injection: recovery rolls forward identically (3 crashes, 415 blocks, no corruption) |
| 59 | supply-chain-security-gates | CYCLE-2 | 2026-07-29 | workflow byte-identical to upstream (mutable tags, no permissions block); posture upstream-accepted |
| 75 | build-throughput-cacheability | CYCLE-2 | 2026-07-29 | header-cost via -ftime-trace: validation.h 6.1s (20%), no anomaly |
| 73 | network-state-machine | CYCLE-2 | 2026-07-29 | handshake EOF sweep: 7/7 v2 offsets + v1 partial close clean, zero half-open peers |
| 99 | clean-room-reimplementation | CYCLE-1 | 2026-07-29 | CompactSize clean-room differential: 804 cases, 0 mismatches |
| 38 | failure-cleanup-crash-safety | CYCLE-1 (retro) | 2026-07-28 | EncryptWallet mkey rollback fix (9894fb8b6c, row restored) |
| 38 | failure-cleanup-crash-safety | CYCLE-2 | 2026-07-29 | txindex interrupted-build: empty-block build uninterruptible (~3s); resume mechanics present |
| 55 | alternative-implementation-diff | CYCLE-1 | 2026-07-29 | noble-secp256k1 ECDSA differential: 2019/2019+5/5 RFC6979 vectors match |
| 107 | conformance-test-transplant | CYCLE-1 | 2026-07-29 | RFC 4231 case 5 transplanted (0d36c6cd80); all 7 cases both hashes |
| 101 | public-characterization-fix | CYCLE-1 | 2026-07-29 | PSBT fuzz hybrid consumption fix (d086164661): 9 starvation-gated functions covered, control re-lists all 9, iso seed 528->2857 edges |
| 1 | comment-code-contract | CYCLE-2 | 2026-07-29 | net_processing strong claims: 9/9 verified TRUE (prune-read, SetupAddressRelay, diff-encoding, tx-inventory-empty); no defect |
| 43 | option-api-lifecycle | CYCLE-2 | 2026-07-29 | -capturemessages: append-across-restart proven (488->976 prefix-intact); capture IO failure aborts node (rc=-6), upstream-identical |
| 101 | public-characterization-fix | CYCLE-2 | 2026-07-29 | truncation-gate sibling sweep: class clean, psbt sole member (13 uses classified; script/banman coverage-confirmed) |
| 49 | critical-history-sweep | CYCLE-1 | 2026-07-29 | advisory sweep: 35202/54605/46598 fixes+oracles present (35202 functional run green); 52911 dup of #33 E1 |
| 6 | serialization-untrusted-input | CYCLE-2 (retro) | 2026-07-28 | txoutproof negative-oracle battery (9d1244e6b1, row restored); backported into lineage 4b8fa7c937, green at HEAD |
| 6 | serialization-untrusted-input | EXHAUSTED | 2026-07-29 | wallet-record cell dismissed (machinery-bounded, O5 harness owns the seam); all cells accounted |
| 58 | helper-reuse | CYCLE-1 (retro) | 2026-07-28 | mempool hex-tx-array decode-loop dedup (4f97fbfe1e, row restored); backported a7067512e8+3e887dbbf7, green at HEAD |
| 58 | helper-reuse | CYCLE-2 | 2026-07-29 | base64-PSBT decode-or-throw dedup x6 (b1e55802f6); rpc_psbt.py green |
| 80 | fuzz-engine-differential | CYCLE-1 | 2026-07-29 | PSBT C++/Python differential 400 cases: A=0 (no over-acceptance), E=142 round-trip-equal, C=123 all reference-lax |
| 91 | compiler-binary-hardening | CYCLE-2 | 2026-07-29 | kernel-lib export: secp-leak refuted (subtree hidden, 0-export probe); C++ surface config-gated (OFF: 1431; Guix ON hardens) |
| 60 | reviewer-preference-skill | CYCLE-2 | 2026-07-29 | line-level mining: R8-R13 (partial-pin rejection, convention-anchored nits, AssertLockHeld, type-proven guard pushback); held-out 2/3+1 |
| 40 | multi-agent-adjudication | CYCLE-1 | 2026-07-29 | L4 RESOLVED: dup-check equivalence proven (prover+breaker converge, adjudicator verified); 1-input arm vacuously safe |
| 23 | perf-flamegraph-investigation | CYCLE-1 (retro) | 2026-07-28 | reorg-repair cross-check gated (83f9989a68, row restored); backported 93c29aac55 |
| 23 | perf-flamegraph-investigation | CYCLE-2 | 2026-07-29 | backport verified at HEAD: mempool_tests green, ComplexMemPool 184.7 ms/op (matches c1 repaired 181.4; stale-binary 278.5 scare resolved by rebuild) |
| 42 | ci-review-bot-followup | CYCLE-1 | 2026-07-29 | DrahtBot TSan on upstream 35744 = unmerged shared-lock rework; tree not exposed (UniqueLock fix e049f064e1, resize-cursor test green) |
| 34 | uncovered-code-classification | CYCLE-1 (retro) | 2026-07-28 | merkleblock 81->86/86 unit lines (32d5d1dcc4, row restored); backported 068152320f (union with F2 tests) |
| 34 | uncovered-code-classification | CYCLE-2 | 2026-07-29 | backport green at HEAD; blockstorage guards have dedicated tests; MoneyRange asserts unreachable by construction (compressor throws) |
| 106 | semantic-twin-inconsistency | CYCLE-1 | 2026-07-29 | hex-decode twin map: tx/PSBT strict-by-design (proven), block/header/proof benign-lax; dismissed, no contract violated |
| 70 | compiler-optimization-differential | CYCLE-1 | 2026-07-29 | LTO+Wodr build green, zero ODR warnings, full unit suite green under LTO binary |
| 44 | secret-copy-optimization | CYCLE-1 | 2026-07-29 | key paths secure-container-clean; memory_cleanse elision-resistant at -O3 -flto (2 disassembly probes) |
| 25 | performance-regression-bisect | CYCLE-1 (retro) | 2026-07-28 | +2.4% MemPoolAddTransactions bisected to 3ae78dbd25 (row restored) |
| 25 | performance-regression-bisect | CYCLE-2 | 2026-07-29 | regression persists at HEAD (250.75 ms/op); consumer split caps feerate-only fix upside (2/4 need bitsets); journal-only |
| 46 | api-output-on-failure | CYCLE-1 | 2026-07-29 | kernel C API: 5 surfaces output-on-failure-defined (verify/block_read/process_block/_header/to_bytes); input_index assert = upstream WIP note |
| 102 | durable-suspicion-replay | CYCLE-1 | 2026-07-29 | suspicion-index A1-A11 delivered; L1 blind replay confirms unreachable (sharper UB split: int 0/0 at bloom.cpp:40, inf-cast at :34) |
| 54 | raii-resource-leaks | CYCLE-1 | 2026-07-29 | fopen/raw-new sweeps clean: RAII pervasive; readwritefile/dbwrapper manual paths upstream-matching, bad_alloc-theoretical only |
| 51 | differential-metamorphic | CYCLE-1 (retro) | 2026-07-28 | full-UTXO undo oracle (47e5bf2f95, row restored); backported 4807d408fe |
| 51 | differential-metamorphic | CYCLE-2 | 2026-07-29 | multi-block undo composition oracle added (negative control fired [101!=102], reverted green); suite green |
| 76 | reproducible-builds | CYCLE-3 | 2026-07-29 | full depends sweep: 22 URL-verified + 28 cached sources pin-clean + make download green; qrencode primary still dead (fallback exact); xorg.freedesktop.org unreachable=environmental |
| 60 | reviewer-preference-skill | CYCLE-3 | 2026-07-29 | reusable skill encoded: reviews/reviewer-rules.md (R1-R13 + reviewer map, indexed) |
| 21 | rebuild-recovery-profile | CYCLE-3 | 2026-07-29 | Taproot-OP_TRUE mix profiled: tweak math = validation floor (~86% EC), 11.2k vs 9.0k tx/s vs P2PK; no defect |

## Next-up queue
1. Random draw (user-mandated policy since 2026-07-28): recorded seed over
   pending + CYCLE-1 pool, exhausted excluded; this cycle:
   raw=6086368032283125981 -> idx 18 (of 27) -> #107.
   raw=6091627946289443426 -> idx 24 (of 26) -> #75.
   raw=10440797539259245241 -> idx 12 (of 21) -> #101.
   raw=817997966924375334 -> idx 14 (of 20) -> #1.
   raw=3880495123191271155 -> idx 15 (of 20) -> #43.
   raw=6188991159828561970 -> idx 18 (of 19) -> #101.
   raw=5200339953805149283 -> idx 7 (of 18) -> #49.
   raw=5880676013471384719 -> idx 13 (of 17) -> #6.
   raw=2597777539898758520 -> idx 8 (of 16) -> #58.
   raw=7161189119308982694 -> idx 9 (of 15) -> #80.
   raw=3164618978385005329 -> idx 13 (of 14) -> #91.
   raw=4139937993477073163 -> idx 12 (of 13) -> #60.
   raw=8140190821735971651 -> idx 3 (of 12) -> #40.
   raw=5886040343995211851 -> idx 0 (of 11) -> #23.
   raw=2944374403711530272 -> idx 2 (of 10) -> #42.
   raw=2032011409874351500 -> idx 1 (of 9) -> #34.
   raw=4582464614074250662 -> idx 6 (of 8) -> #106.
   raw=726564917869240643 -> idx 4 (of 7) -> #70.
   raw=4945649014858672681 -> idx 1 (of 6) -> #44.
   raw=3460624873609219110 -> idx 0 (of 5) -> #25.
   raw=7914243779083168148 -> idx 0 (of 4) -> #46.
   raw=7007359828371084343 -> idx 1 (of 3) -> #102.
   raw=9209411641343266232 -> idx 0 (of 2) -> #54; then #51 sole
   remaining eligible entry (singleton, no RNG needed).
   POOL-EMPTY NOTE (2026-07-29): after #51 c2 the random pool is
   EMPTY (0 pending, 0 CYCLE-1). Further cycles draw from the
   re-rank queue (CYCLE-2+ campaigns) per the rotation policy, or
   reopen campaigns when new evidence appears.
   RE-RANK draw 1 (8 open CYCLE-2+ cells): raw=2099841385913238651
   -> idx 3 -> #76 c3.
   RE-RANK draw 2 (7 cells): raw=2554837129443345158 -> idx 6 ->
   #60 c3.
   RE-RANK draw 3 (6 cells): raw=3519535482585431460 -> idx 0 ->
   #21 c3.
   HYGIENE (from #64 c1): DONE 2026-07-29 (#66 c2 backport of all 5).
   SCOPE NOTE (2026-07-28 objective refresh): weight core campaigns
   (consensus/coins/P2P/compact blocks/serialization/crypto/chainstate/
   validation/indexes/storage); wallet/IPC/GUI/RPC only to prove
   reachability of an in-scope core defect.
   POOL-REPAIR NOTE (2026-07-28): the incremental pools used from the
   #75 draw onward carried stale CYCLE-2+ entries (4, 28, 61), and a
   draw of #61(c3) over that 27-entry pool (raw=2149655188711527484,
   idx 16) was DISCARDED. The pool was rebuilt from the ledger handoff
   per the documented rule (pending + exactly-CYCLE-1, minus
   EXHAUSTED/QUEUE-COMPLETE/deferred #72/#77, minus the just-cycled
   campaign): 41 pending + 23 CYCLE-1 = 64 entries. Campaigns at
   CYCLE-2+ are reachable via the re-rank queue, not the random pool.
   POOL-REPAIR 2 (2026-07-28): a draw of #98 over that 64-entry pool
   was DISCARDED — #98 has a DONE table row the handoff list omitted.
   The handoff DONE list is now rebuilt MECHANICALLY from table rows
   (see Handoff); the eligible pool is 33 pending + 22 CYCLE-1 = 55.
2. then re-rank: 21 c2 (tx-heavy reindex), #22 queue (fee-estimator
   UpdateMovingAverages per-block gating; tx-heavy import),
   #1 queue (net_processing/txmempool claim sweep, fork-added comments),
   #60 queue (line-level review comments, contentious-PR NACK pass),
   #45 queue (passphrase rate-limiting semantics), #43 queue
   (-capturemessages lifecycle), #76 queue (full depends download
   sweep; 45-uncacheable itemization; qrencode upstream-watch),
   #73 queue (handshake EOF sweep), #75 queue (clean-build wall,
   CI cache keys, header-cost -ftime-report).

## Handoff
Updated after every rotation. Campaign-DONE/QC/EXHAUSTED/deferred
(mechanically rebuilt from table rows 2026-07-28): 3, 5, 6(EXHAUSTED), 8, 11, 12,
13, 14, 15, 18(QUEUE-COMPLETE), 19(EXHAUSTED), 20, 26, 27, 33, 52,
56, 62, 72(deferred), 77(deferred), 82, 83, 84, 85, 86, 87, 88, 89,
96, 97, 98.
Cycles done (random-pool state): 0(c1,c2), 1(c1,c2), 4(c1,c2), 6(c1,c2,c3),7(c1), 2(c1), 9(c1,c2), 10(c1,c2), 7(c1,c2), 16(c1,c2), 17(c1,c2,c3), 21(c1,c2,c3), 22(c1,c2),
28(c1,c2), 29(c1,c2), 30(c1,c2,c3), 31(c1,c2,c3,c4), 34(c1,c2), 36(c1,c2), 23(c1,c2), 25(c1,c2),
37(c1), 39(c1,c2), 40(c1), 42(c1), 44(c1), 46(c1), 54(c1), 51(c1,c2), 21(c1,c2), 43(c1,c2), 45(c1,c2), 47(c1,c2), 49(c1), 50(c1), 48(c1), 53(c1), 55(c1), 57(c1), 58(c1,c2), 59(c1,c2), 60(c1,c2,c3),
61(c1,c2,c3), 63(c1), 64(c1), 65(c1,c2), 66(c1,c2), 67(c1), 68(c1,c2), 69(c1), 70(c1), 71(c1,c2), 73(c1,c2), 74(c1,c2), 75(c1),
76(c1,c2,c3), 80(c1), 81(c1,c2), 90(c1,c2), 91(c1,c2), 92(c1), 93(c1), 94(c1,c2), 95(c1,c2), 99(c1), 100(c1), 101(c1,c2), 102(c1), 103(c1), 104(c1), 105(c1), 106(c1), 107(c1), 108(c1), 109(c1).
Technique note for future secp cycles: subtree-only scratch builds with
SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64 + tests/noverify -j4 give a
full cross-backend differential in ~35s on this host.
Declassification note (#45 c1): a padding check that masks padsize to
zero on failure is the whole Vaudenay defense — verify the mask, the
full-length scan, and that failure/success timing is identical before
claiming a leak.
Build-throughput note (#75 c1): `ninja -t deps` inversion gives exact
header fan-out with zero tree mutation; validate one point with
touch + `ninja -n` + `touch -d` mtime restore (dry-run always includes
the 14-edge build-info cascade — subtract it, it restats away on real
runs). ccache-warm timings are NOT compile costs — use CCACHE_DISABLE=1
for cold numbers.
IBD-profile note (#22 c2): on regtest the IsInitialBlockDownload latch
flips false after the FIRST connected block (minwork=0 + fresh
timestamps), so any per-tip-change work gated on !is_ibd runs for the
whole sync — profile with -checkblockindex=0 AND a perf record, or the
consistency machinery (5.7x wall) hides the real pipeline. This tree's
bitcoind does not auto-create a missing -datadir; mkdir first.
Cross-tool note (#36 c2): clang -Wunneeded-internal-declaration/
-Wunneeded-member-function fires on every fork helper referenced only
from a discarded `if constexpr (G_ABORT_ON_FAILED_ASSUME)` block —
by design, NOT dead code; check the caller's guard before proposing
removal. gcc has no equivalent warning and no thread-safety analysis.
Coverage note (#9 c2): sancov UNCOVERED_FUNC 0/N for a small static
function with few call sites is usually an INLINING artifact (edges
executed at the call sites; out-of-line symbol never called) — confirm
with per-line UNCOVERED_PC before classifying. Single-file -runs=1
-print_coverage output can omit the function entirely; never treat a
grep fallback as coverage evidence. A padded NetMsgType dictionary
(-dict, 28 tokens from src/protocol.h) lifted process_messages edge
coverage 42% and is reusable across P2P targets.
Reindex-harness note (#21 c2): plain -reindex on a synced datadir is
block-INDEX-only; the validation cell needs -reindex-chainstate.
OOM-guard note (#80 c1): any scratch differential/fuzz script that
feeds fuzzed counts into Python reference parsers MUST set
resource.setrlimit(RLIMIT_AS, 4 GiB) and catch MemoryError per case —
the framework's deserializers trust untrusted CompactSize counts and
will otherwise consume the whole host (kernel OOM-kill at 14.8 GB RSS
on 2026-07-29; only the script died, no restart). Pair every
differential with a positive-control assertion (E>0-style) — a
systematic harness error (hex passed to a base64 RPC) showed up
instantly as zero double-accepts.
Completion gating must not poll pre-wipe-true state (610/False matches
BEFORE the wipe) — gate on the debug.log markers ('Wiping LevelDB' ->
final 'UpdateTip.*height=N version') and distrust gates against copied
datadir logs (stale matches). /usr/bin/time user-CPU is the
gate-independent metric. MiniWallet (~80 tx/s, no wallet DB) beats
wallet-RPC (~0.1-26 tx/s with silent failure modes; fresh regtest
needs -fallbackfee) for tx-heavy chain construction; the fork's
framework needs called_by_framework=True on generate().
