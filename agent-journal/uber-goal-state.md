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
| 36 | cross-tool-analysis-matrix | CYCLE-3 | 2026-07-31 | clang-18 UBSan full suite: 1128 cases 0 reports (117 __ubsan syms control); {gcc,clang}xUBSan consistent; DISMISSED |
| 36 | cross-tool-analysis-matrix | CYCLE-4 | 2026-07-31 | _GLIBCXX_ASSERTIONS full suite: 1128 cases 0 violations (ninja -t commands flag control); DISMISSED |
| 76 | reproducible-builds | CYCLE-1 | 2026-07-28 | A/B rebuild: code bit-identical; 1-byte secp comp_dir DWARF delta attributed; Guix packaging sound |
| 9 | hit-frequency-coverage | CYCLE-2 | 2026-07-28 | net_processing sancov: all gaps harness-scope; 0/23 alarm resolved as inlining artifact via per-line PC check |
| 47 | build-ci-parity | CYCLE-2 | 2026-07-28 | install manifest declarative-single-source; 17-file install set exact match |
| 47 | build-ci-parity | CYCLE-3 | 2026-08-01 | export-set consumer check: downstream compiles+links(g++, 0 undef)+runs via .pc; static lib self-contained; DISMISSED |
| 81 | spec-vector-drift | CYCLE-1 (retro) | 2026-07-28 | BIP324+RFC8439 vectors byte-exact (0f6c2640b7, own branch; row restored) |
| 81 | spec-vector-drift | CYCLE-2 | 2026-07-28 | BIP173/350 bech32(m): no drift across unit/key_io/functional layers |
| 81 | spec-vector-drift | CYCLE-3 | 2026-07-31 | BIP341 both levels byte-exact: C++ script_assets 141917/141917, Python wallet-vector port 0 mismatches; no drift |
| 81 | spec-vector-drift | CYCLE-4 | 2026-07-31 | BIP32 25/25 xprv/xpub match + engine green; base58+sighash byte-identical to upstream master; no drift |
| 76 | reproducible-builds | CYCLE-2 | 2026-07-28 | depends pins exact (qrencode primary 404, fallback serves pinned bytes); secp ccache = absolute-I key divergence, not uncacheable |
| 21 | rebuild-recovery-profile | CYCLE-2 | 2026-07-28 | tx-heavy reindex-chainstate: 6.8s user, 85% secp256k1 EC math; checks negligible at 610 blocks; harness lessons logged |
| 2 | assertion-invariant-audit | CYCLE-1 | 2026-07-28 | fork production Assumes all construction-tautologies; cache-overflow fix verified; no input-validation-by-assert |
| 65 | contributor-branch-radar | CYCLE-2 | 2026-07-28 | perf seam: CheckBlock dup-check 1.85x branch locally absent (equivalence plausible), prevector-36 + serialization seeds recorded |
| 79 | fuzz-corpus-stewardship | CYCLE-1 | 2026-07-28 | cross-seed transfer +39-55% on 3 P2P siblings; merge-minimize -36% size zero-loss |
| 10 | fuzz-target-gaps | CYCLE-2 | 2026-07-28 | load_wallet harness delivered (fd74c4a7c2); bring-up crash = harness-oracle bug, production clean |
| 48 | property-oracle-expansion | CYCLE-1 | 2026-07-28 | CompactSize exhaustive battery delivered (8b7d8ac878), 1/1 boundary mutant killed |
| 67 | release-version-differential | CYCLE-1 | 2026-07-28 | v28.2/v0.20.1 ↔ HEAD sync+handshake clean both directions; BIP324 fallback correct |
| 67 | release-version-differential | CYCLE-2 | 2026-07-31 | wtxid/txid inventory across v0.21 boundary: negotiation/inv/getdata all per BIP339; DISMISSED |
| 24 | disk-io-amplification | CYCLE-1 | 2026-07-28 | regtest IBD: ~2x byte amplification (undo+index), bounded 16MB prealloc; no pathological growth |
| 63 | loupe-style-pipeline | CYCLE-1 | 2026-07-28 | fee-estimator zero-state waste confirmed+fixed (675011ba86): 20.4%->0 samples, -34% IBD user |
| 35 | mutation-testing | CYCLE-1 | 2026-07-28 | ReadVarInt overflow guards test-blind (M2/M3 survived); oracle delivered+verified (083afedbf1) |
| 92 | abi-alignment-aliasing | CYCLE-1 | 2026-07-28 | kernel enum name-maps value-independent; by-value structs static-linkage contract; aliasing sweep clean |
| 78 | translation-validation | CYCLE-1 | 2026-07-28 | Assume-erasure contract validated at binary: fork hardening zero-cost in release |
| 100 | sink-reverse-reachability | CYCLE-1 | 2026-07-28 | bloom ctor div-by-zero test-only latent; empty-filter guarded (CVE-2013-5700 present) |
| 7 | resource-exhaustion-variants | CYCLE-2 | 2026-07-29 | UTXO-scan/resize race: fixed in-tree (e049f064e1 unique-lock cursor); upstream master verified still racy |
| 94 | bindings-ffi-parity | CYCLE-2 | 2026-07-29 | enum mapping static_assert tables (073d543f26), reorder tripwire fires at :268 |
| 64 | finding-dedup-recurrence | CYCLE-1 | 2026-07-29 | findings-index.md built; 5 fixes confirmed NOT in lineage (F1,F2,F3,F7,F9) |
| 104 | analogical-vulnerability-translation | CYCLE-3 (queue-empty) | 2026-07-31 | INTERPRETER-CONFUSION: descriptor/miniscript limits agree, all fail closed (valid nest cap 200 via ops 201; parse cap 3600; tr braces 128); DISMISSED |
| 90 | historical-knowledge-recipes | CYCLE-2 | 2026-07-29 | R15-R22 added (sancov inlining, reindex gating, MiniWallet API, io sampling, mutant-first, pool mechanics, dict fuzzing, flag persistence) |
| 71 | deterministic-simulation | CYCLE-2 | 2026-07-29 | crash-resume durability invariant delivered (6c6e7d9f87), 3k scans clean |
| 71 | deterministic-simulation | CYCLE-3 | 2026-07-30 | reorged-record resume oracle + mock duplicate-hash fidelity fix (7e88645b92); 3k clean (row restored 2026-07-31) |
| 71 | deterministic-simulation | CYCLE-4 | 2026-07-31 | extension-resume oracle (forced, fire-proofed) + ext-hash uniqueness fix + c1 oracle correction (flip can fire pre-scan); 3k clean |
| 71 | deterministic-simulation | CYCLE-5 (EXHAUSTED) | 2026-07-31 | progress-value fuzzing: monotonic/flat/adversarial gVP schedules, [0,1] + divide-guard oracles; early-consumption trap fixed |
| 32 | whole-history-leftovers | CYCLE-1 | 2026-07-29 | multiply-first percentage shape: no exploitable survivor (range/memory-bounded) |
| 69 | backend-differential | CYCLE-1 | 2026-07-29 | secp wide-multiply int128-vs-int64 differential: 4/4 suites pass, no divergence |
| 103 | finding-composition | CYCLE-1 | 2026-07-29 | capability graph: no realizable chain; 3 edges tested broken (L2 fixed, F4+F5 fixed, F8 bounded) |
| 53 | timing-side-channel | CYCLE-1 | 2026-07-29 | AES-CBC padding dudect: Welch t 1.53/1.69/-1.14 (no leak), confirms #45 code-read |
| 74 | memory-pressure-allocator | CYCLE-1 (retro) | 2026-07-28 | mempool accounting honest (1.13x RSS/usage @8k); glibc drain retention not a leak (2ef390de05, row restored) |
| 74 | memory-pressure-allocator | CYCLE-2 | 2026-07-29 | LockedPool oversize alloc -> graceful bad_alloc via RPC (fault-injected); no residue |
| 74 | memory-pressure-allocator | CYCLE-5 | 2026-07-31 | mlock-failure path live: degraded arena works unlocked (locked=0/total=262144), log-silent (upstream-identical); DISMISSED |
| 105 | project-bug-autopsy-recurrence | CYCLE-1 | 2026-07-29 | txgraph saturation family autopsy: recurrence mapped, no uncovered sibling |
| 45 | constant-time-declassification | CYCLE-2 | 2026-07-29 | walletpassphrase: KDF-only throttle 0.10s/attempt measured, no lockout by design |
| 109 | whole-feature-public-path | CYCLE-2 | 2026-07-31 | live compact-block matrix 4/4: hb-synced=cmpctblock only, miss=1x GETBLOCKTXN/BLOCKTXN, v1=v2; static map confirmed |
| 66 | backport-correctness | CYCLE-2 | 2026-07-29 | 5 out-of-lineage fixes backported+verified (e15c4025e5, 84a3913096, 508d9edfca, 75c0616c24, b73b7c5d39) |
| 39 | generated-artifact-determinism | CYCLE-2 | 2026-07-29 | full JSON sweep 60/60 byte-identical (9 tree + 51 univalue) |
| 37 | build-dead-zones | CYCLE-2 | 2026-07-29 | config-dead zones clean: ZMQ/USDT/chainstate-util all properly gated |
| 95 | database-semantics-differential | CYCLE-2 | 2026-07-29 | dbwrapper contracts hold: WriteBatch atomicity + HEAD_BLOCKS crash protocol, scan-only iterators |
| 108 | adversarial-artifact-generation | CYCLE-1 | 2026-07-29 | hostile V1 peer artifact: 4/4 classes classified correctly (magic/checksum/oversize/valid) |
| 108 | adversarial-artifact-generation | CYCLE-6 (COMPLETE) | 2026-08-01 | post-handshake v2 slowloris reaped at +20.0min (ping+send timeout dual lines); mocktime + framework peertimeout masks recorded |
| 41 | history-seed-archaeology | CYCLE-1 | 2026-07-29 | fee_estimates.dat version gate fails closed both directions (v28.2 rejects 309900 non-fatally) |
| 41 | history-seed-archaeology | CYCLE-6 | 2026-07-31 | banlist.dat/.json: all 6 cells as documented (round-trip/corrupt/garbage/legacy-dat/expired/write-fail-retry); persistence family CLOSED; campaign EXHAUSTED |
| 50 | fuzz-introspector-blockers | CYCLE-1 | 2026-07-29 | PSBT blocker = harness truncation (ConsumeRandomLengthString); ~500 serialize edges unreachable |
| 57 | local-reasoning-contracts | CYCLE-1 | 2026-07-29 | m_all_zero contracts observable+guarded; broken-discipline mutation caught at bloom_tests:535 |
| 68 | architecture-abi-parity | CYCLE-2 | 2026-07-29 | char-signedness sweep clean: no sign-sensitive plain-char use; serialize forbid guards the class |
| 93 | system-fault-injection | CYCLE-1 | 2026-07-29 | mid-flush crash injection: recovery rolls forward identically (3 crashes, 415 blocks, no corruption) |
| 59 | supply-chain-security-gates | CYCLE-2 | 2026-07-29 | workflow byte-identical to upstream (mutable tags, no permissions block); posture upstream-accepted |
| 75 | build-throughput-cacheability | CYCLE-2 | 2026-07-29 | header-cost via -ftime-trace: validation.h 6.1s (20%), no anomaly |
| 73 | network-state-machine | CYCLE-2 | 2026-07-29 | handshake EOF sweep: 7/7 v2 offsets + v1 partial close clean, zero half-open peers |
| 73 | network-state-machine | CYCLE-4 | 2026-07-31 | slow-drip ellswift reaped at 64s mid-handshake (V2 handshake timeout); real peer unaffected; DISMISSED |
| 73 | network-state-machine | CYCLE-5 (COMPLETE) | 2026-08-01 | node-initiated half-close: shape absent (zero shutdown() syscalls); peer view = full close, clean EOF 0.04s |
| 99 | clean-room-reimplementation | CYCLE-1 | 2026-07-29 | CompactSize clean-room differential: 804 cases, 0 mismatches |
| 38 | failure-cleanup-crash-safety | CYCLE-1 (retro) | 2026-07-28 | EncryptWallet mkey rollback fix (9894fb8b6c, row restored) |
| 38 | failure-cleanup-crash-safety | CYCLE-2 | 2026-07-29 | txindex interrupted-build: empty-block build uninterruptible (~3s); resume mechanics present |
| 55 | alternative-implementation-diff | CYCLE-1 | 2026-07-29 | noble-secp256k1 ECDSA differential: 2019/2019+5/5 RFC6979 vectors match |
| 55 | alternative-implementation-diff | CYCLE-2 | 2026-07-30 | RFC6979+ndata extraEntropy differential: 25/25 + 200/200 byte-identical, three implementations (row restored 2026-07-31) |
| 55 | alternative-implementation-diff | CYCLE-3 | 2026-07-31 | BIP340 sibling vectors: Python 19/19+8/8, in-tree subset byte-exact, noble==official rows 0-14; no drift |
| 55 | alternative-implementation-diff | CYCLE-4 (EXHAUSTED) | 2026-07-31 | rust-bitcoin fixtures 4/4: sighash subset 289/289, BIP341 identical, huge-witness decode, block round-trip byte-exact |
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
| 65 | contributor-branch-radar | CYCLE-3 | 2026-07-29 | leveldb knob batch assessed: block-cache-bypass premise VERIFIED (format.cc:106 + table.cc:182); others = bench ladders/diagnostic/vendored bump |
| 1 | comment-code-contract | CYCLE-3 | 2026-07-29 | txmempool+txgraph strong claims 8/8 TRUE; chunk-connectedness assert matches IsAcceptable guard exactly; core claim surface covered |
| 45 | constant-time-declassification | CYCLE-3 | 2026-07-29 | RPC auth sweep: comparisons constant-time, single-401; HMAC-on-match username oracle = de minimis at local RPC boundary |
| 75 | build-throughput-cacheability | CYCLE-3 | 2026-07-29 | clean-build wall: cold 1418.2s vs warm 59.6s (23.8x ccache lever); CI cache keys sound (content-key is the anchor) |
| 50 | fuzz-introspector-blockers | CYCLE-2 | 2026-07-29 | signing section covers SignPSBTInput/UpdatePSBTOutput/PSBTInputSignedAndVerified; iso seed 528->2857->3048 edges |
| 25 | performance-regression-bisect | CYCLE-3 | 2026-07-29 | txindex baseline: 3.7s catch-up for 41.4k txs (~validation-path speed); 35531 lineage = branch decision |
| 23 | perf-flamegraph-investigation | CYCLE-3 | 2026-07-29 | EvictionProtection (sort, 34us micro) + ConnectBlockAll (~98% EC verify) profiles clean; no fix candidate |
| 34 | uncovered-code-classification | CYCLE-3 | 2026-07-29 | ScriptCompression 3 malformed arms closed + mutation-verified (guard-drop kills arm 1) |
| 35 | mutation-testing | CYCLE-2 | 2026-07-29 | WriteVarInt per-line sweep: M2/M3/M4 all killed (34/137/137 failures); CTxUndo VARINT fuzz-covered |
| 48 | property-oracle-expansion | CYCLE-2 | 2026-07-29 | CompactSize 254-class exhaustive (130k+ cases); guard-weakening mutant killed 61440x |
| 60 | reviewer-preference-skill | CYCLE-4 | 2026-07-29 | template held-out on 35205: 3/3 blind predictions confirmed; R14 added (setter boundary validation) |
| 106 | semantic-twin-inconsistency | CYCLE-2 | 2026-07-29 | merkle-root twins: 6/6 multi-shape blocks agree; mutation flag C++-only (intentional); Python empty-vector unreachable |
| 40 | multi-agent-adjudication | CYCLE-2 | 2026-07-29 | A11 adjudicated: kernel input_index assert = defect-classified (policy + author's fix branch 6f23568be8); severity none today; parked |
| 80 | fuzz-engine-differential | CYCLE-2 | 2026-07-29 | raw-tx parser differential: A=0/300, B=0 round-trip-exact, C=240 all Python-lax; production never over-accepts |
| 49 | critical-history-sweep | CYCLE-2 | 2026-07-29 | remaining advisory cells: 54604 fork-interaction clean (PRIVBROADCAST rate-limited), 46597 32-bit cap present, 52922/21/13/14 markers verified |
| 91 | compiler-binary-hardening | CYCLE-3 | 2026-07-29 | shared kernel lib measured: 134 btck_ + std weak only, zero internal/secp exports; full RELRO+NX; no BTI note (c1's toolchain finding) |
| 106 | semantic-twin-inconsistency | CYCLE-3 | 2026-07-29 | vsize twins agree (formula + 6/6 functional); sighash numerics agree; PSBT DEFAULT\|ALL restriction = intentional policy |
| 43 | option-api-lifecycle | CYCLE-3 | 2026-07-29 | -v2transport settings.json lifecycle proven: honored, persisted (write-back intact), CLI-overridable |
| 34 | uncovered-code-classification | CYCLE-4 | 2026-07-29 | dbwrapper boundary layers safe; Cursor warmup asymmetry = author's PR 35654 pending (corrupt-only, parked) |
| 34 | uncovered-code-classification | CYCLE-5 | 2026-07-31 | BitsToBytes padding arms: dedicated test, 0xff-mutant killed (was sancov-granularity, not behavioral gap); in-tree queue CLOSED |
| 23 | perf-flamegraph-investigation | CYCLE-4 | 2026-07-29 | CompareMainTransactions = O(1) early-exit comparator; memcmp/atomic shares inherent-by-design; no fix candidate |
| 95 | database-semantics-differential | CYCLE-3 | 2026-07-29 | RocksDB swap builds+reindexes correctly; CPU parity (validation-bound), wall -43% (write parallelism, directional) |
| 42 | ci-review-bot-followup | CYCLE-2 | 2026-07-29 | wider DrahtBot sweep (15 PRs, 7 flagged): all failures PR-owned; 35793 shows in-tree assert working as designed |
| 44 | secret-copy-optimization | CYCLE-2 | 2026-07-29 | clang cross-check: barrier un-elidable in IR (calloc+barrier+free); cleanse idiom confirmed both compilers |
| 80 | fuzz-engine-differential | CYCLE-3 | 2026-07-29 | consensus acceptance differential: A=0/300 (no structural over-acceptance); E=53/D=99 agreement, C=148 parse/policy |
| 75 | build-throughput-cacheability | CYCLE-4 | 2026-07-29 | 45-uncacheable itemized: 57/58 = failed compilations (rotation's own mutants); IPC/capnp hypothesis refuted |
| 95 | database-semantics-differential | CYCLE-4 | 2026-07-29 | durability differential: kill -9 mid-reindex, both engines recover identical tip, zero corruption |
| 102 | durable-suspicion-replay | CYCLE-2 | 2026-07-29 | A5 capturemessages abort replayed on v28.2 binary: same msghand exception abort (behavior-verified, second verifier form) |
| 91 | compiler-binary-hardening | CYCLE-4 | 2026-07-29 | ELF census 7/7 binaries: PIE+NX+full RELRO+canary uniform, zero divergence; BTI note absent (c1 toolchain finding) |
| 80 | fuzz-engine-differential | CYCLE-4 | 2026-07-29 | PSBTv2 differential: v2 paths clean (A=0/400 mixed); E=107 round-trip-exact, C=124 Python-lax |
| 50 | fuzz-introspector-blockers | CYCLE-3 | 2026-07-29 | correlated PSBT signing seed: layout replay byte-exact + walletprocesspsbt complete=True; complete-arm driver |
| 51 | differential-metamorphic | CYCLE-3 | 2026-07-29 | fee-diagram incremental-vs-recompute: hook exists (txgraph fuzz sim+CompareChunks), 1000 runs green |
| 42 | ci-review-bot-followup | CYCLE-3 | 2026-07-29 | corecheck endpoint = real oracle; flags 35744 bench regressions (ComplexMemPool +15.6%, OrphanageEraseForPeer +33%) upstream-side |
| 46 | api-output-on-failure | CYCLE-2 | 2026-07-29 | import_blocks clean; callback reentrancy constraint real (cs_main held, deadlock on reentry) but undocumented (upstream-identical) |
| 21 | rebuild-recovery-profile | CYCLE-4 | 2026-07-29 | dbcache sensitivity: wall +60% at dbcache=4, user-CPU neutral (validation-bound); fork default sane |
| 60 | reviewer-preference-skill | CYCLE-5 | 2026-07-29 | maintainer merge-rationale mined: M1-M4 (terse depth-honest ACKs, self-contained descriptions, lifecycle, no info-hiding) |
| 58 | helper-reuse | CYCLE-3 | 2026-07-29 | 7th PSBT copy deduplicated: helper moved to rpc/rawtransaction_util (existing shared header); rpc_psbt green |
| 65 | contributor-branch-radar | CYCLE-4 | 2026-07-29 | rocksdb-brute assessed: bulk-fetch class subsumed by shipped -prevoutfetchthreads; stale WIP, nothing actionable |
| 80 | fuzz-engine-differential | CYCLE-5 | 2026-07-29 | rich PSBTv2 differential: A=0/400, E=115, R=4 guard-contained; BIP371 value-layout lesson (6-seed iterations) |
| 42 | ci-review-bot-followup | CYCLE-4 | 2026-07-29 | corecheck sweep: bench deltas noise-shaped (same +10-33% on unrelated bloom PR); regression reading corrected |
| 10 | fuzz-target-gaps | CYCLE-3 | 2026-07-29 | load_wallet widened (crypted/ACTIVE*SPK/BESTBLOCK); LoadActiveScriptPubKeyMan assert on corrupt DB documented (upstream-matching) |
| 80 | fuzz-engine-differential | CYCLE-6 | 2026-07-29 | MuSig2 PSBT seeding: differential clean (A=0, E=100); format-from-source worked first try |
| 95 | database-semantics-differential | CYCLE-5 | 2026-07-31 | write-flush-windowed kill: _Exit inside all 4 batch commits (idx/coins/shutdown); identical tip recovery, 0 corruption; DISMISSED |
| 35 | mutation-testing | CYCLE-5 | 2026-07-31 | CTxUndo hostile-field layers classified (decode-reject/apply-reject/trust-boundary); range-check mutant killed fail-before/pass-after |
| 35 | mutation-testing | CYCLE-6 | 2026-07-31 | latent uncompilable SizeComputer overload repaired (DEFAULT mode, upstream-inherited); boundary battery; M_a/M_b/M_c killed 8/134/15 |
| 76 | reproducible-builds | CYCLE-4 | 2026-08-01 | 45-uncacheable itemization: 0 uncacheable in all 6 live families (logfile-verified); lifetime 83 = deleted-config residue; DISMISSED |

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
   RE-RANK draw 4 (5 cells): raw=8499369058190745183 -> idx 3 ->
   #65 c3.
   RE-RANK draw 5 (4 cells): raw=9064950354337441572 -> idx 0 ->
   #1 c3.
   RE-RANK draw 6 (3 cells): raw=1921359177284103813 -> idx 0 ->
   #45 c3.
   RE-RANK draw 7 (2 cells): raw=203410285419299450 -> idx 0 ->
   #75 c3.
   RE-RANK draw 8 (1 cell, singleton): #50 c2. The 8-cell re-rank
   queue is now fully consumed; next draws rebuild from journal
   queues/URGENT/findings-index resume points.
   RE-RANK draw 9 (rebuilt 10-cell queue): raw=7506511565727394747
   -> idx 7 -> #25 c3.
   RE-RANK draw 10 (9 cells): raw=8627252179256411442 -> idx 6 ->
   #23 c3.
   RE-RANK draw 11 (8 cells): raw=523542359493460158 -> idx 6 ->
   #34 c3.
   RE-RANK draw 12 (7 cells): raw=4506871108920428011 -> idx 1 ->
   #35 c2.
   RE-RANK draw 13 (6 cells): raw=4934760897020020062 -> idx 0 ->
   #48 c2.
   RE-RANK draw 14 (5 cells): raw=2955923148731138849 -> idx 4 ->
   #60 c4.
   RE-RANK draw 15 (4 cells): raw=2632070695870877747 -> idx 3 ->
   #106 c2.
   RE-RANK draw 16 (3 cells): raw=6403733973450834911 -> idx 2 ->
   #40 c2.
   RE-RANK draw 17 (2 cells): raw=4732580478433972031 -> idx 1 ->
   #80 c2.
   RE-RANK draw 18 (1 cell, singleton): #49 c2. All rebuilt-queue
   cells now consumed; next queue from journal queues/URGENT.
   RE-RANK draw 19 (rebuilt 10-cell queue): raw=3025727333361852290
   -> idx 0 -> #91 c3.
   RE-RANK draw 20 (9 cells): raw=2549763356383794637 -> idx 1 ->
   #106 c3.
   RE-RANK draw 21 (8 cells): raw=6417013600593057308 -> idx 4 ->
   #43 c3.
   RE-RANK draw 22 (7 cells): raw=4758704522666985144 -> idx 2 ->
   #34 c4.
   RE-RANK draw 23 (6 cells): raw=6107678024432235403 -> idx 1 ->
   #23 c4.
   RE-RANK draw 24 (5 cells): raw=1659704294019222637 -> idx 2 ->
   #95 c3.
   RE-RANK draw 25 (4 cells): raw=8478006373347410799 -> idx 3 ->
   #42 c2.
   RE-RANK draw 26 (3 cells): raw=2732842920470447767 -> idx 1 ->
   #44 c2.
   RE-RANK draw 27 (2 cells): raw=5930901419059390541 -> idx 1 ->
   #80 c3.
   RE-RANK draw 28 (1 cell, singleton): #75 c4.
   RE-RANK draw 29 (rebuilt 8-cell queue): raw=138518960771160384
   -> idx 0 -> #95 c4.
   RE-RANK draw 30 (7 cells): raw=2757803267336390011 -> idx 4 ->
   #102 c2.
   RE-RANK draw 31 (6 cells): raw=5848245164434789317 -> idx 3 ->
   #91 c4.
   RE-RANK draw 32 (5 cells): raw=1166893013096824274 -> idx 4 ->
   #80 c4.
   RE-RANK draw 33 (4 cells): raw=6465375341789668838 -> idx 2 ->
   #50 c3.
   RE-RANK draw 34 (3 cells): raw=7567157135810338807 -> idx 1 ->
   #51 c3.
   RE-RANK draw 35 (2 cells): raw=3639072378229674997 -> idx 1 ->
   #42 c3.
   RE-RANK draw 36 (1 cell, singleton): #46 c2. All rebuilt-queue
   cells consumed again.
   RE-RANK draw 37 (rebuilt 6-cell queue): raw=1546480656633297830
   -> idx 2 -> #21 c4.
   RE-RANK draw 38 (5 cells): raw=127326143727536095 -> idx 0 ->
   #60 c5.
   RE-RANK draw 39 (4 cells): raw=6707348440583223643 -> idx 3 ->
   #58 c3.
   RE-RANK draw 40 (3 cells): raw=2099837013193560362 -> idx 2 ->
   #65 c4.
   RE-RANK draw 41 (2 cells): raw=6241307819045081386 -> idx 0 ->
   #80 c5.
   RE-RANK draw 42 (1 cell, singleton): #42 c4. All rebuilt-queue
   cells consumed.
   RE-RANK draw 43 (rebuilt 5-cell queue): raw=8893763982643078528
   -> idx 3 -> #10 c3.
   RE-RANK draw 44 (4 cells): raw=7792775892002329801 -> idx 1 ->
   #80 c6.
   RE-RANK draw 45 (3 cells): raw=1016919037349801110 -> idx 1 ->
   #50 c4.
   RE-RANK draw 46 (2 cells): raw=471461078390554833 -> idx 1 ->
   #49 c3. Remaining: 60-c6 singleton, then rebuild.
   RE-RANK draw 47 (1 cell, singleton): #60 c6. Queue consumed;
   next draw rebuilds from journal queues/URGENT/findings-index.
   RE-RANK draw 48 (rebuilt 5-cell queue): raw=3510230010931241503
   -> idx 3 -> #24 c2.
   RE-RANK draw 49 (4 cells): raw=8930217931205760543 -> idx 3 ->
   #65 c5.
   RE-RANK draw 50 (3 cells): raw=15284566479080594961, masked
   6061194442225819153 -> idx 1 -> #50 c5.
   RE-RANK draw 51 (2 cells): raw=9203260543596453984 -> idx 0 ->
   #49 c4. LINEAGE NOTE: master is NOT an ancestor of the cycle
   lineage (merge-base a8823c0996); master carries 34 commits incl.
   the whole prune-assumevalid series. Behavior tests of that
   feature must build from master, not the lineage worktree.
   Remaining queue cell: 101-c2 singleton, then rebuild.
   RE-RANK draw 52 (1 cell, singleton): #101 c3. Queue consumed;
   #101 queue-empty after c3; next draw rebuilds.
   RE-RANK draw 53 (rebuilt 5-cell queue):
   raw=1440765120485782254 -> idx 4 -> #49 c6.
   RE-RANK draw 54 (4 cells): raw=16460576047879496241, masked
   7237204011024720433 -> idx 1 -> #50 c6.
   RE-RANK draw 55 (3 cells): raw=6717978107659154466 -> idx 0 ->
   #49 c5. HARNESS LESSON (reusable): framework sign_ecdsa uses
   python-random nonces by default — seed random.seed() for any
   cross-run txid-deterministic differential.
   RE-RANK draw 56 (2 cells): raw=16574797430435475050, masked
   7351425393580699242 -> idx 0 -> #24 c3. Remaining: 9-c3
   singleton, then rebuild.
   RE-RANK draw 57 (1 cell, singleton): #9 c3. Queue consumed;
   next draw rebuilds from journal queues/URGENT/findings-index.
   RE-RANK draw 58 (rebuilt 5-cell queue):
   raw=15543061168184443242, masked 6319689131329667434 -> idx 4
   -> #49 c7. #49 marked QUEUE-EMPTY (advisories, fork-delta
   validation/net_processing, prevout-pool, flush cadence all
   closed); reopens on new fork commits.
   RE-RANK draw 59 (4 cells): raw=217606016431337542 -> idx 2 ->
   #60 c7. #60 generality cell CLOSED (three seams).
   RE-RANK draw 60 (3 cells): raw=5635536915814007501 -> idx 2 ->
   #9 c4.
   RE-RANK draw 61 (2 cells): raw=6221299261676566590 -> idx 0 ->
   #50 c7.
   RE-RANK draw 62 (1 cell, singleton): #24 c4. Queue consumed;
   #24 queue-empty (write composition, UTXO-growth, pruning,
   flush cadence all closed).
   RE-RANK draw 63 (rebuilt 5-cell queue):
   raw=8574729547574712489 -> idx 4 -> #9 c5.
   RE-RANK draw 64 (4 cells): raw=18394428872774206570, masked
   9171056835919430762 -> idx 2 -> #24 c5. #24 queue-empty.
   RE-RANK draw 65 (3 cells): raw=13781584894610127864, masked
   4558212857755352056 -> idx 1 -> #50 c8.
   RE-RANK draw 66 (2 cells): raw=12027658190395197936, masked
   2804286153540422128 -> idx 0 -> #80 c7.
   RE-RANK draw 67 (1 cell, singleton): #65 c6. Queue consumed;
   next draw rebuilds. TXINDEX SEED: l0rinc's force-updated
   txindex_optimization series (prefixed keys, packed positions,
   bloom-skip) is the top queued assessment.
   RE-RANK draw 68 (rebuilt 4-cell queue):
   raw=6181926257477540249 -> idx 1 -> #80 c8.
   RE-RANK draw 69 (3 cells): raw=2479466053413897937 -> idx 1 ->
   #50 c9.
   RE-RANK draw 70 (2 cells): raw=193349491238106314 -> idx 0 ->
   #49 c8 (txindex migration assessed SOUND).
   RE-RANK draw 71 (1 cell, singleton): #65 c7. Queue consumed;
   next draw rebuilds.
   RE-RANK draw 72 (3 cells): raw=16888814550837975994, masked
   7665442513983200186 -> idx 2 -> #65 c8, DISCARDED as
   degenerate (re-scan minutes after quiet c6/c7 sweeps; zero
   information content). Deferred to next radar interval.
   RE-RANK draw 73 (2 cells): raw=14363064615506178869, masked
   5139692578651403061 -> idx 1 -> #35 c2.
   RE-RANK draw 74 (1 cell, singleton): #80 c9. Queue consumed;
   next draw rebuilds.
   RE-RANK draw 75 (rebuilt 5-cell queue):
   raw=5487700768165387789 -> idx 4 -> #9 c6.
   RE-RANK draw 76 (4 cells): raw=4741416227469711203 -> idx 3 ->
   #35 c3.
   RE-RANK draw 77 (3 cells): raw=5379781919100994291 -> idx 1 ->
   #50 c10.
   RE-RANK draw 78 (2 cells): raw=1875611079037645143 -> idx 1 ->
   #65 c8. Queue consumed; next draw rebuilds. TOP SEED:
   txindex-block-fetch-proxy (4 review questions in #65 journal).
   RE-RANK draw 79 (rebuilt 3-cell queue):
   raw=15220363497701843503, masked 5996991460847067695 -> idx 2
   -> #50 c11.
   RE-RANK draw 80 (2 cells): raw=16381284910993384311, masked
   7157912874138608503 -> idx 1 -> #65 c9 (block-fetch-proxy
   assessed SOUND on all 4 questions).
   RE-RANK draw 81 (1 cell, singleton): #80 c10. All structured
   PSBT field families now seeded+clean (base, input taproot,
   musig2, output taproot). Queue consumed; rebuild next.
   RE-RANK draw 82 (rebuilt 3-cell queue):
   raw=16639617182922783380, masked 7416245146068007572 -> idx 0
   -> #80 c11 (cross-field semantics: informational-field verdict).
   RE-RANK draw 83 (2 cells): raw=15737651929817465930, masked
   6514279892962690122 -> idx 0 -> #65 c10 (knots quiet).
   RE-RANK draw 84 (1 cell, singleton): #60 c8 (watch quiet).
   Queue consumed; next draw rebuilds.
   RE-RANK draw 85 (2 cells): raw=18024658476397289347, masked
   8801286439542513539 -> idx 1 -> #35 c4 (overflow corner
   VACUOUS).
   RE-RANK draw 86 (1 cell, singleton): #50 c12. Queue consumed;
   rebuild next.
   RE-RANK draw 87 (rebuilt 5-cell queue):
   raw=12410924129186828647, masked 3187552092332052839 -> idx 4
   -> #52 c2 (limitclustercount=0 CONFIRMED+FIXED 5e0a80ade5).
   RE-RANK draw 88 (4 cells): raw=14986909651204603959, masked
   5763537614349828151 -> idx 3 -> #108 c2.
   RE-RANK draw 89 (3 cells): raw=14006312474672917596, masked
   4782940437818141788 -> idx 1 -> #89 c2.
   RE-RANK draw 90 (2 cells): raw=5820747569139027543 -> idx 1
   -> #45 c4.
   RE-RANK draw 91 (1 cell, singleton): #69 c2. Queue consumed;
   rebuild next.
   RE-RANK draw 92 (rebuilt 3-cell queue):
   raw=11237199852652658799, masked 2013827815797882991 -> idx 1
   -> #108 c3.
   RE-RANK draw 93 (2 cells): raw=8837950375080321296 -> idx 0
   -> #89 c3.
   RE-RANK draw 94 (1 cell, singleton): #69 c3. Queue consumed;
   rebuild next.
   RE-RANK draw 95 (rebuilt 4-cell queue):
   raw=14085611517425201985, masked 4862239480570426177 -> idx 1
   -> #89 c4 (signed VarInt).
   RE-RANK draw 96 (3 cells): raw=182724797729077762 -> idx 1 ->
   #69 c4.
   RE-RANK draw 97 (2 cells): raw=17777575239433090151, masked
   8554203202578314343 -> idx 1 -> #65 c11.
   RE-RANK draw 98 (1 cell, singleton): #89 c5. Queue consumed;
   rebuild next.
   RE-RANK draw 99 (rebuilt 5-cell queue):
   raw=18090839548819896712, masked 8867467511965120904 -> idx 4
   -> #10 c4.
   RE-RANK draw 100 (4 cells): raw=2928821531056304015 -> idx 3
   -> #74 c3.
   RE-RANK draw 101 (3 cells): raw=18214628344123528261, masked
   8991256307268752453 -> idx 2 -> #13 c2 (dbwrapper leak
   CONFIRMED+FIXED 73a6798206).
   RE-RANK draw 102 (8-cell harvested queue): raw=18209190156073205447,
   masked 8985818119218429639 -> idx 7 -> upstream-watches -> #42 c5
   (watched PRs static; F13/F14 re-verified offerable @ 9611a35603;
   #35753 fork-safe).
   RE-RANK draw 103 (7-cell harvested queue): raw=17579083735087921526,
   masked 8355711698233145718 -> idx 3 -> xswiftec-edge-vectors ->
   #108 c4 (C xswiftec_inv vs CSV 256/256 + Python differential green;
   DISMISSED).
   RE-RANK draw 104a (6-cell queue): raw=9787481964664031861, masked
   564109927809256053 -> idx 3 -> VerifyCryptedKeys — DESCOPED
   (wallet-only, no core reachability; 2026-07-28 scope note).
   RE-RANK draw 104b (5-cell queue): raw=4836711809555485631 -> idx 1
   -> sink-allocs -> #100 c2 (wire-CompactSize alloc sinks bounded
   ~5 MiB, mutation-verified; DISMISSED).
   RE-RANK draw 105 (7-cell queue): raw=10344607599657158645, masked
   1121235562802382837 -> idx 5 -> txgraph-saturation -> #100 c3
   (saturation UNREACHABLE by bounded proof: 68.6x fee / 83.9x size
   margins under 64-cluster cap; DISMISSED).
   RE-RANK draw 106 (5-cell queue): raw=13280760683108707460, masked
   4057388646253931652 -> idx 2 -> RSS-accounting -> #74 c4 (dbcache
   accounting vs RSS tight: 0.94-1.01x over inline/heap scripts,
   safe-direction error; DISMISSED).
   RE-RANK draw 107 (4-cell queue): raw=4478165777402910479 -> idx 3
   -> xswiftec-oracle-delivery -> #108 c5 (C++ edge-vector gate
   delivered in bip324_tests, 98/98 decode checks, mutation-killed;
   oracle O10).
   RE-RANK draw 108 (3-cell queue): raw=2063430805213321808 -> idx 2
   -> dbcache-pressure-critical -> #24 c6 (CRITICAL tier directly
   observed: single-block jump -> 'exceeds total space' + IF_NEEDED
   critical=1 flush same block; DISMISSED; #24 COMPLETE).
   RE-RANK draw 109 (2-cell queue): raw=15860256885964587176, masked
   6636884849109811368 -> idx 0 -> coinsview-flags -> #57 c2
   (flagged-list invariants green over 9.18M per-op checks + both
   guards fire; DISMISSED).
   RE-RANK draw 110 (3-cell queue): raw=13942745621896996524, masked
   4719373585042220716 -> idx 1 -> overlay-prefetch -> #57 c3
   (parallel(8) vs serial(0) differential ALL EQUAL over chain-heavy/
   dup-prevout/reorg stages; DISMISSED; cell closed).
   RE-RANK draw 111 (2-cell queue): raw=1896312681528823955 -> idx 1
   -> estimator-battery -> #57 c4 (mutation sweep: M2 removeTx + M3
   Read set-omissions SURVIVED pre-existing suite -> battery kills
   both; oracle O11).
   RE-RANK draw 112 (5-cell re-harvested queue):
   raw=17127609671097067094, masked 7904237634242291286 -> idx 1 ->
   scan-vs-compaction -> #7 c3 (scan queueing measured: exact N-fold
   staircase 1x/2x/3x on 293k-coin set; DISMISSED, documented
   contract quantified).
   RE-RANK draw 113 (4-cell queue): raw=6880818164717981818 -> idx 2
   -> getblockstats-pruning -> #7 c4 (horizon correct; 70,156
   near-horizon race calls, 0 anomalies; DISMISSED; #7 cells
   closed).
   RE-RANK draw 114a (6-cell re-harvested queue):
   raw=1152682293760198793 -> idx 2 -> int128-split — DISCARDED as
   stale duplicate of #69 c3 (journal-verified closed 2026-07-30).
   RE-RANK draw 114b (same 6-cell queue):
   raw=14671989418026382988, masked 5448617381171607180 -> idx 0 ->
   tx-heavy-import -> #63 c3 (tx-heavy P2P replay profile: ~87%
   ECDSA floor, both perf fixes hold; DISMISSED).
   LEDGER FIX (2026-07-30): cycles-done for #63 said 63(c1); the
   journal has c1,c2. Corrected to 63(c1,c2,c3) with that cycle (later cycles c4,c5 appended normally).
   RE-RANK draw 115 (5-cell queue): raw=6489529727745126125 -> idx 0
   -> comment-code-contract-txgraph -> #1 c4 (106-claim sweep; 4
   stale comments CONFIRMED+FIXED, F15; 0 code defects).
   RE-RANK draw 116 (4-cell queue): raw=12034554563773083305, masked
   2811182526918307497 -> idx 1 -> alt-impl-extrantropy -> #55 c2
   (RFC6979+ndata differential: 25/25 vectors + 200/200 randomized
   across 3 implementations; DISMISSED).
   RE-RANK draw 117 (3-cell queue): raw=12307389541691523835, masked
   3084017504836748027 -> idx 2 -> abi-enum-signedness -> #68 c3
   (enum : char class empty in-tree; explicit underlying types
   arch-invariant; DISMISSED).
   RE-RANK draw 118 (2-cell queue): raw=7779331150053916422 -> idx 0
   -> conformance-aes-cbc -> #107 c2 (Wycheproof AES-CBC-PKCS5:
   24/24 valid + 48/48 invalid rejected + openssl 24/24;
   DISMISSED).
   RE-RANK draw 119 (5-cell re-harvested queue):
   raw=11926377350913041393, masked 2703005314058265585 -> idx 0 ->
   assertion-txgraph-assume -> #2 c3 (dynamic cluster stress with
   trapping build: all 5 phases green; 2 harness-side tripwires
   proven; DISMISSED).
   RE-RANK draw 120 (4-cell queue): raw=8356496777076536508 -> idx 0
   -> critical-point-read-claim -> #49 c9 (txindex v2 prefix
   collisions MEASURED on the real 345,064,792-row DB: uniform,
   1.0402x amplification, pairs +0.014% vs Poisson; DISMISSED).
   RE-RANK draw 121 (3-cell queue): raw=11558546368712918904, masked
   2335174331858143096 -> idx 1 -> network-half-close -> #73 c3
   (v1+v2 SHUT_WR: sub-second disconnect both, 0 CPU, no half-open
   peers; DISMISSED).
   RE-RANK draw 122 (3-cell queue): raw=5478373626480380153 -> idx 2
   -> resume-reorged-position -> #71 c3 (reorged-record resume
   oracle delivered; mock dup-hash fidelity flaw CONFIRMED+FIXED;
   3000 runs clean; artifact BwoANwKKAAA= regression-seeded).
   RE-RANK draw 123 (4-cell queue): raw=5024985370541799207 -> idx 3
   -> missing-inputs-arm -> #50 c13 (programmatic MISSING_INPUTS
   arms: 3 gates + control driven in-process; persistent oracle
   O12, gate-2 mutation killed).
   RE-RANK draw 124 (3-cell queue): raw=8742819288871107508 -> idx 1
   -> history-persistence-artifacts -> #41 c2 (mempool.dat:
   badver rejected pre-loop zero-mutation, v2 full round-trip,
   flip per-entry-fail tolerance, trunc tolerant abort; DISMISSED).
   RE-RANK draw 125 (4-cell queue): raw=17581368771181834455,
   masked 8357996734327058647 -> idx 3 -> xor-dat -> #41 c3
   (obfuscation-key archaeology: 4 corruption classes all fail
   LOUD, designed dbwrapper_error gate for shape corruption;
   DISMISSED).
   RE-RANK draw 126 (5-cell queue): raw=3781142207025897645 -> idx 0
   -> flatfileseq-open-churn -> #63 c4 (FlatFileSeq::Open measured:
   20.7% inclusive = 13.2% genuine page IO + ~7.5% avoidable fopen
   machinery; design cost, not a defect; DISMISSED).
   RE-RANK draw 127 (4-cell queue): raw=1453947281365744417 -> idx 1
   -> anchors-dat -> #41 c4 (anchors.dat archaeology:
   all-or-nothing + read-and-delete everywhere, trailing junk
   tolerated; DISMISSED).
   RE-RANK draw 128 (4-cell queue): raw=9758505963115004795, masked
   535133926260228987 -> idx 3 -> peers-dat -> #41 c5 (three-tier
   archaeology: missing -> recreate, too-new -> .bak+recreate,
   too-old/wrong-magic/corrupt -> FATAL with workaround;
   DISMISSED).
   RE-RANK draw 129 (4-cell queue): raw=5564503156500269060 -> idx 0
   -> generatetoaddress-decay -> #63 c5 (decay NOT reproduced:
   flat curves to 20k both nodes; wallet = constant ~2x factor;
   DISMISSED).
   RE-RANK draw 130 (3-cell queue): raw=15796267858001076908,
   masked 6572895821146301100 -> idx 0 -> abi-layout-battery ->
   #92 c2 (by-value struct layout battery delivered in test_kernel,
   O13; field-swap mutation killed at compile time).
   RE-RANK draw 131 (2-cell queue: analogical-shapes, banlist.dat;
   re-harvested from journal "Limitations / queue" tails):
   raw=13487688468534954634 -> idx 0 -> analogical-shapes -> #104 c2
   (STALE-AUTHORITY-LATCH fails on pindexBestKnownBlock: FindNextBlocks
   fails closed at net_processing.cpp:1531; BLOCK_FAILED_MASK removal
   verified as upstream refactor 29740c06ac with zero FAILED_CHILD
   writers, not a mutation; DISMISSED).
   Queue after draw 131: (banlist.dat), n=1; re-harvest journal
   "Limitations / queue" tails again before the next draw (verify
   each cell against the journal's LAST cycle).
   RE-RANK draw 132 (10-cell pool, re-harvested from live journal
   "Limitations / queue" tails; ledger's 2026-07-28 step-2 queue
   found STALE — all 9 entries since advanced past their queued
   cells): raw=13927279158170128312, masked 4703907121315352504 ->
   idx 4 -> #104 c3 (INTERPRETER-CONFUSION fails on descriptor/
   miniscript: all limit layers fail closed and agree — valid
   nesting cap 200 via ops-201, parse cap 3600, tr braces 128, live
   getdescriptorinfo boundary probe on isolated regtest; campaign
   queue-empty; DISMISSED).
   Queue after draw 132: banlist.dat(#41), #109-c2 compact-block
   matrix, #90 spec-vector cells, #35 CTxUndo semantic differential,
   #55 schnorr vectors, #34 merkle Assume arms, #47 clang UBSan,
   #95 flush-windowed kill, #71 extension-block resume (n=9).
   RE-RANK draw 133 (9-cell queue): raw=2340311064075852148 (already
   63-bit) -> idx 1 -> #109 c2 (live two-node compact-block matrix
   4/4: hb-synced tip = cmpctblock only, zero getblocktxn;
   deterministic mempool-miss = exactly one getblocktxn/blocktxn
   pair; v1 == v2; static map confirmed; DISMISSED).
   Queue after draw 133: banlist.dat(#41), #90 spec-vector cells,
   #35 CTxUndo semantic differential, #55 schnorr vectors, #34 merkle
   Assume arms, #47 clang UBSan, #95 flush-windowed kill, #71
   extension-block resume (n=8).
   RE-RANK draw 134 (8-cell queue): raw=16057275024291464013, masked
   6833902987436688205 -> idx 5 -> clang UBSan cell -> #36 c3 (the
   harvest shorthand mislabeled it #47; #36 = cross-tool-analysis-
   matrix, corrected in journal and ledger). clang-18 UBSan full
   unit suite: 1128 cases, 0 runtime-error reports (positive
   control 117 __ubsan symbols); {gcc,clang} x UBSan consistent;
   DISMISSED. Workflow trap recorded: configure sanitizer build
   dirs AFTER the feature-branch checkout (stale xswiftec rule
   from the all-findings tree failed the first build).
   Queue after draw 134: banlist.dat(#41), #90 spec-vector cells,
   #35 CTxUndo semantic differential, #55 schnorr vectors, #34
   merkle Assume arms, #95 flush-windowed kill, #71 extension-block
   resume (n=7).
   RE-RANK draw 135 (7-cell queue): raw=12699529126198122192, masked
   3476157089343346384 -> idx 4 -> #34 c5 (BitsToBytes padding arms:
   dedicated arm-level test bits_bytes_padding_arms; 0xff-init
   mutant killed by both the new test and c1's roundtrip test —
   c1's <100% branch note was sancov Assume-line granularity, not a
   behavioral gap; in-tree queue CLOSED; no defect).
   Queue after draw 135: banlist.dat(#41), #90 spec-vector cells,
   #35 CTxUndo semantic differential, #55 schnorr vectors, #95
   flush-windowed kill, #71 extension-block resume (n=6).
   RE-RANK draw 136 (6-cell queue): raw=18304798480260019009, masked
   9081426443405243201 -> idx 5 -> #71 c4 (extension-resume oracle
   delivered, forced + fire-proofed; TWO harness-fidelity defects
   found+fixed with preserved crashing artifacts: extension-block
   hash duplicates breaking c3's reorg oracle, c1 extension oracle
   overclaiming (flip fires from pre-scan findBlock callers ->
   legit stop at flip_idx-1); 3k clean; no production defect).
   Queue after draw 136: banlist.dat(#41), #90 spec-vector cells,
   #35 CTxUndo semantic differential, #55 schnorr vectors, #95
   flush-windowed kill (n=5).
   RE-RANK draw 137 (5-cell queue): raw=16972436101960705472, masked
   7749064065105929664 -> idx 4 -> #95 c5 (write-flush-windowed kill:
   temp _Exit hook at CDBWrapper::WriteBatch, kills inside all 4
   batch commits of the reindex lifecycle; every window recovers to
   the byte-identical control tip, 0 corruption lines; hook reverted;
   DISMISSED). Calibration: full reindex+shutdown = exactly 5
   WriteBatch calls (obf key, 2x index, coins, shutdown); mid-run
   flush window needs multi-GB UTXO (disk-queued). gdb batch mode
   unreliable here (silent hides hits); pkill -f self-match trap.
   Queue after draw 137: banlist.dat(#41), #90 spec-vector cells,
   #35 CTxUndo semantic differential, #55 schnorr vectors (n=4).
   RE-RANK draw 138 (4-cell queue): raw=2323887634903536520
   (already 63-bit) -> idx 0 -> banlist.dat -> #41 c6 (CBanDB
   dual-format archaeology: all 6 cells behave as documented —
   round-trip exact, truncated -> warn+recreate-empty, garbage
   schema -> warn+empty, legacy .dat -> 22.x warning with json
   authoritative, expired swept at load, write-failure -> dirty
   retained + retry persists both bans; functional matrix rc=0;
   DISMISSED). Persistence family CLOSED (6/6 artifacts);
   campaign #41 EXHAUSTED. Harness lesson: banlist json key is
   "banned_nets" (list of objects), not "banned".
   Queue after draw 138: #90 spec-vector cells, #35 CTxUndo
   semantic differential, #55 schnorr vectors (n=3).
   RE-RANK draw 139 (3-cell queue): raw=7507051601152807701
   (already 63-bit) -> idx 0 -> spec-vector cells -> #81 c3
   (harvest shorthand mislabeled it #90; #81 = spec-vector-drift,
   corrected). BIP341 both levels byte-exact: C++ script_assets
   141917/141917 (qa-assets canonical set), Python wallet-vector
   port 0 mismatches; no drift; DISMISSED.
   Queue after draw 139: #35 CTxUndo semantic differential, #55
   schnorr vectors (n=2).
   RE-RANK draw 140 (2-cell queue): raw=15293687238298433213, masked
   6070315201443657405 -> idx 1 -> #55 c3 (BIP340 sibling vectors:
   Python 19/19+8/8 incl. 2022-12 variable-message rows; in-tree
   subset byte-exact, zero drift; noble == official rows 0-14;
   DISMISSED). Ledger hygiene: #55 CYCLE-2 row restored on the
   archive (was only on the feature lineage).
   Queue after draw 140: #35 CTxUndo semantic differential (n=1);
   re-harvest journal "Limitations / queue" tails before the next
   draw (this 10-cell pool is now exhausted).
   RE-RANK draw 141 (n=1): raw=8843838926267607625 (already
   63-bit) -> idx 0 -> #35 CTxUndo apply-vs-reject -> campaign
   cycle 5 (decode-reject / apply-reject / trust-boundary
   classification test delivered; compressor range-check mutant
   killed fail-before/pass-after; DISMISSED). DUPLICATE-WORK
   catch: the consumer-side DECODE classification overlapped
   #35 c3/c4 of 2026-07-29 (journaled into property-oracle-
   expansion.md — logging-location quirk); the overflow-wrap
   residual I drafted was already proven VACUOUS by c4
   (bijection + complete range check) and was withdrawn in the
   journal; new content = apply-layer classification + ordered
   mutation kill. Commit messages on the feature/archive say
   "cycle 3"; the correct campaign ordinal is 5.
   Queue after draw 141: EMPTY — re-harvest journal
   "Limitations / queue" tails before the next draw.
   RE-RANK draw 142 (10-cell re-harvested pool, each cell verified
   against its journal's last cycle): raw=3808635104791433633
   (already 63-bit) -> idx 3 -> #35 c6 (SizeComputer sweep +
   NONNEGATIVE_SIGNED battery). CONFIRMED (latent/trivial):
   SizeComputer WriteVarInt overload called GetSizeOfVarInt with a
   missing Mode arg — uncompilable when instantiated, zero callers,
   upstream-inherited; repaired (DEFAULT) and pinned by the new
   boundary battery; mutants M_a/M_b/M_c killed 8/134/15; final
   rebuild green. Pool after draw 142 (9): #55 tx-serialization,
   #81 BIP32/base58/BIP143/Wycheproof, #71 progress-value fuzzing,
   #36 _GLIBCXX_ASSERTIONS, #73 slow-drip ellswift, #74 locked-arena
   mlock-failure, #89 v0.21 relay/downgrade differential, #76
   45-uncacheable itemization, #108 v2 slowloris.
   RE-RANK draw 143 (9-cell queue): raw=606380819244863949 (already
   63-bit) -> idx 0 -> #55 c4 (rust-bitcoin tx-serialization
   fixtures 4/4 agree: legacy_sighash byte-subset 289/289 zero
   drift, bip341_tests byte-identical to official, huge_witness C++
   decode, testnet block hash+merkle+byte-exact round-trip;
   DISMISSED). Campaign #55 EXHAUSTED (c1-c4 all clean).
   Queue after draw 143 (8): #81 BIP32/base58/BIP143/Wycheproof,
   #71 progress-value fuzzing, #36 _GLIBCXX_ASSERTIONS, #73
   slow-drip ellswift, #74 locked-arena mlock-failure, #89 v0.21
   relay/downgrade differential, #76 45-uncacheable itemization,
   #108 v2 slowloris.
   RE-RANK draw 144 (8-cell queue): raw=8265742044986960116
   (already 63-bit) -> idx 4 -> #74 c5 (mlock-failure degraded
   arena: live with setpriv nobody + RLIMIT_MEMLOCK=0 — secure
   allocs proceed unlocked, getmemoryinfo locked=0 vs total,
   node functional, log-silent upstream-identical; DISMISSED).
   Traps recorded: CAP_IPC_LOCK bypasses RLIMIT_MEMLOCK (root
   tests meaningless); pgrep -f self-matches the invoking shell.
   Queue after draw 144 (7): #81 BIP32/base58/BIP143/Wycheproof,
   #71 progress-value fuzzing, #36 _GLIBCXX_ASSERTIONS, #73
   slow-drip ellswift, #89 v0.21 relay/downgrade differential,
   #76 45-uncacheable itemization, #108 v2 slowloris.
   RE-RANK draw 145 (7-cell queue): raw=15010669081987093008,
   masked 5787297045132317200 -> idx 3 -> #73 c4 (slow-drip
   ellswift: 2s/B drip reaped at 64s mid-handshake, "V2 handshake
   timeout"; 60s budget doesn't extend with progress; real peer
   control unaffected; DISMISSED).
   Queue after draw 145 (6): #81 BIP32/base58/BIP143/Wycheproof,
   #71 progress-value fuzzing, #36 _GLIBCXX_ASSERTIONS, #89 v0.21
   relay/downgrade differential, #76 45-uncacheable itemization,
   #108 v2 slowloris.
   RE-RANK draw 146 (6-cell queue): raw=15378291577405012317,
   masked 6154919540550236509 -> idx 1 -> #71 c5 (progress-value
   fuzzing: monotonic/flat/adversarial gVP schedules, [0,1] +
   divide-guard oracles; early-consumption starvation trap found
   via mode-distribution probe 600/0/0 -> ~600/100/100; artifacts
   + 1500 clean; DISMISSED). Campaign #71 EXHAUSTED.
   Queue after draw 146 (5): #81 BIP32/base58/BIP143/Wycheproof,
   #36 _GLIBCXX_ASSERTIONS, #89 v0.21 relay/downgrade
   differential, #76 45-uncacheable itemization, #108 v2 slowloris.
   RE-RANK draw 147 (5-cell queue): raw=12275778598515865734,
   masked 3052406561661089926 -> idx 1 -> #36 c4
   (_GLIBCXX_ASSERTIONS full unit suite: 1128 cases, 0
   checked-container violations; flag control via ninja -t
   commands since non-verbose logs omit command lines; DISMISSED).
   Queue after draw 147 (4): #81 BIP32/base58/BIP143/Wycheproof,
   #89 v0.21 relay/downgrade differential, #76 45-uncacheable
   itemization, #108 v2 slowloris.
   RE-RANK draw 148 (4-cell queue): raw=7620657173068370897
   (already 63-bit) -> idx 1 -> release-version c2 (wtxid-vs-txid
   inventory across the v0.21 boundary: wtxidrelay only with
   v0.21+, inv type 1 (txid) to v0.20.1 vs type 5 (wtx) to
   v0.21.0, getdata MSG_WITNESS_TX vs MSG_WTX; three layers per
   BIP339; DISMISSED). NUMBERING CORRECTION: the queue shorthand
   said #89, but campaign #89 is bitcoin-p2p-accounting; this
   cell is #67 (release-version-differential). Feature/archive
   commit messages say #89 where they mean #67.
   Queue after draw 148 (3): #81 BIP32/base58/BIP143/Wycheproof,
   #76 45-uncacheable itemization, #108 v2 slowloris.
   RE-RANK draw 149 (3-cell queue): raw=6078230012799546660
   (already 63-bit) -> idx 0 -> #81 c4 (BIP32 25/25 xprv/xpub
   match + engine green; base58 + sighash.json byte-identical to
   upstream master; sighash chain master == fork == rust-bitcoin
   289-subset; no drift; DISMISSED).
   Queue after draw 149 (2): #76 45-uncacheable itemization,
   #108 v2 slowloris. #81 remaining: Wycheproof (last cell).
   RE-RANK draw 150 (2-cell queue): raw=14568728326895610543,
   masked 5345356290040834735 -> idx 1 -> #108 c6 (post-handshake
   v2 slowloris: silent v2 peer reaped at exactly +20.0 real-min,
   dual labeled timeouts; control alive; DISMISSED). Two masking
   layers recorded: setmocktime does not move the inactivity path;
   framework default -peertimeout=999999999 suppresses all
   inactivity checks (override on the command line for timeout
   cells). Campaign #108 COMPLETE.
   Queue after draw 150 (1): #76 45-uncacheable itemization;
   then re-harvest journal "Limitations / queue" tails.
   RE-RANK draw 151 (n=1): raw=3249723470637215331 (already
   63-bit) -> idx 0 -> #76 c4 (45-uncacheable itemization: ZERO
   uncacheable in all 6 live families, logfile-verified; lifetime
   83 = deleted-config residue; capnp/IPC guess refuted; policy:
   first-build CCACHE_LOGFILE probe per new family; DISMISSED).
   Queue after draw 151: EMPTY — re-harvest journal
   "Limitations / queue" tails before the next draw.
   RE-RANK draw 152 (10-cell re-harvested pool): raw=
   1749937186513557252 (already 63-bit) -> idx 2 -> #47 c3
   (kernel export-set consumer check: downstream compiles + g++
   links 0-undef + runs via .pc; static lib self-contained; gcc
   driver failure = standard C++-lib property; DISMISSED).
   Pool after draw 152 (9): #81 Wycheproof, #36 TSan subset,
   #74 pruning-mode IO, #35 NONNEGATIVE_SIGNED write semantics,
   #73 node-initiated half-close, #50 Taproot/MuSig2 gates,
   #9 per-seed profiling, #67 downgrade read, #45 secp ctime.
   RE-RANK draw 153 (9-cell pool): raw=6904656846068345248
   (already 63-bit) -> idx 4 -> #73 c5 (node-initiated half-close:
   shape ABSENT — zero shutdown() syscalls in the net layer,
   every disconnect is close()-only; raw v1 peer sees clean EOF
   0.04s after disconnectnode; DISMISSED). Campaign #73 COMPLETE.
   Pool after draw 153 (8): #81 Wycheproof, #36 TSan subset,
   #74 pruning-mode IO, #35 NONNEGATIVE_SIGNED write semantics,
   #50 Taproot/MuSig2 gates, #9 per-seed profiling, #67 downgrade
   read, #45 secp ctime.
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
13, 14, 15, 18(QUEUE-COMPLETE), 19(EXHAUSTED), 108(COMPLETE 2026-08-01), 73(COMPLETE 2026-08-01), 20, 26, 27, 33, 41(EXHAUSTED 2026-07-31), 52, 55(EXHAUSTED 2026-07-31), 71(EXHAUSTED 2026-07-31),
56, 62, 72(deferred), 77(deferred), 82, 83, 84, 85, 86, 87, 88, 89,
96, 97, 98.
Cycles done (random-pool state): 41(c1,c2,c3,c4,c5,c6), 0(c1,c2), 1(c1,c2,c3,c4), 4(c1,c2), 6(c1,c2,c3),7(c1), 2(c1,c2,c3), 9(c1,c2,c3,c4,c5,c6), 10(c1,c2,c3,c4), 7(c1,c2,c3,c4), 13(c1,c2), 16(c1,c2), 17(c1,c2,c3), 21(c1,c2,c3,c4), 22(c1,c2), 89(c1,c2,c3,c4,c5), 108(c1,c2,c3,c4,c5,c6),
28(c1,c2), 29(c1,c2), 30(c1,c2,c3), 31(c1,c2,c3,c4), 34(c1,c2,c3,c4,c5), 35(c1,c2,c3,c4,c5,c6), 36(c1,c2,c3,c4), 23(c1,c2,c3,c4), 25(c1,c2,c3),
37(c1), 39(c1,c2), 40(c1,c2), 42(c1,c2,c3,c4,c5), 44(c1,c2), 46(c1,c2), 54(c1), 51(c1,c2,c3), 52(c1,c2), 21(c1,c2,c3,c4), 43(c1,c2,c3), 45(c1,c2,c3,c4), 47(c1,c2,c3,c4), 48(c1,c2), 49(c1,c2,c3,c4,c5,c6,c7,c8,c9), 50(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13), 53(c1), 55(c1,c2,c3,c4), 57(c1,c2,c3,c4), 58(c1,c2,c3), 59(c1,c2), 60(c1,c2,c3,c4,c5,c6,c7,c8), 24(c1,c2,c3,c4,c5,c6),
61(c1,c2,c3), 63(c1,c2,c3,c4,c5), 64(c1), 65(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11), 66(c1,c2), 67(c1,c2), 68(c1,c2,c3), 69(c1,c2,c3,c4), 70(c1), 71(c1,c2,c3,c4,c5), 73(c1,c2,c3,c4,c5), 74(c1,c2,c3,c4,c5), 75(c1,c2,c3,c4),
76(c1,c2,c3,c4), 80(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11), 81(c1,c2,c3,c4), 90(c1,c2), 91(c1,c2,c3,c4), 92(c1,c2), 93(c1), 94(c1,c2), 95(c1,c2,c3,c4,c5), 99(c1), 100(c1,c2,c3), 101(c1,c2,c3), 102(c1,c2), 103(c1), 104(c1,c2,c3), 105(c1), 106(c1,c2,c3), 107(c1,c2), 108(c1), 109(c1,c2).
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
