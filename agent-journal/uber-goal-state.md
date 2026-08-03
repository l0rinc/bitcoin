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
| 4 | public-interface-contracts | CYCLE-3 | 2026-08-02 | LINEAGE REPAIR: journal restored to archive; c1 retraction re-verified at HEAD; bookkeeping CONFIRMED |
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
| 28 | weak-test-oracles | CYCLE-3 | 2026-08-02 | sighash guard mutant KILLED 409x by script corpus; densely covered; DISMISSED |
| 28 | weak-test-oracles | CYCLE-4 | 2026-08-02 | mask-arm mutant killed 3x; both guard arms proven; DISMISSED |
| 16 | api-misuse-resistance | CYCLE-2 | 2026-07-28 | 5+7 kernel C API @pre doc fixes (b6b48987a5, 8b0e92b4a2); #33943 precedent journaled |
| 16 | api-misuse-resistance | CYCLE-4 | 2026-08-02 | lineage repair x2: journal restored + null-destroy fix (55f1fa334f) re-landed with regression test; upstream still vulnerable; CONFIRMED+REPAIRED |
| 61 | stateful-contract-fuzzing | CYCLE-3 | 2026-07-28 | sigcache oracle (a4ff67417e); c2/c3 assessments all already-strong; triage guidance updated |
| 61 | stateful-contract-fuzzing | CYCLE-4 | 2026-08-02 | undo-data cell absorbed by #35-c5 battery (green); DISMISSED |
| 30 | security-logging | CYCLE-3 | 2026-07-28 | 2 injection fixes; amplification dismissed by design (0 bytes at default, 14.6KB/round at -debug) |
| 30 | security-logging | CYCLE-5 | 2026-08-02 | URI-on-exception safe by design (no secret-bearing URIs, body/auth never logged); DISMISSED |
| 30 | security-logging | CYCLE-6 | 2026-08-02 | PR 35833 both arms CONFIRMED+ADOPTED (forged-consensus-error failing-before); F20 |
| 31 | cross-layer-contracts | CYCLE-3 | 2026-07-28 | +gettxoutsetinfo use_index restriction (9396f0b414); 3 doc/RPC layer fixes total |
| 29 | dead-stale-code | CYCLE-2 | 2026-07-28 | util/common + node/wallet/script scans clean; SAFE_CHARS_FILENAME parked |
| 29 | dead-stale-code | CYCLE-4 | 2026-08-02 | reverse-dead-code sample: 9/9 helpers live; DISMISSED |
| 17 | build-matrix-modules | CYCLE-3 | 2026-07-28 | c1-c3 all clean (fbe821c003, own branch) |
| 17 | build-matrix-modules | CYCLE-4 | 2026-08-02 | DISABLE_OPTIMIZED_SHA256 end-to-end: 'standard' backend, 3 functional tests green; CONFIRMED |
| 18 | exhaustive-algebraic | QUEUE-COMPLETE | 2026-07-28 | c1-c3 round-trips all mutation-verified (c382122eeb, own branch) |
| 31 | cross-layer-contracts | CYCLE-4 | 2026-07-28 | testnet-key examples labeled (b2a0c38154, own branch) |
| 31 | cross-layer-contracts | CYCLE-5 | 2026-08-02 | external-signer.md extraction: zero falsely-parseable examples; DISMISSED |
| 36 | cross-tool-analysis-matrix | CYCLE-1 | 2026-07-28 | UBSan unit suite: 1 UB found+fixed (22aa75a2eb, own branch) |
| 36 | cross-tool-analysis-matrix | COMPLETE | 2026-08-01 | c6: clang-18 functional subset 7/7 == gcc baseline; Werror CI gap dismissed (BITCOIN_CONFIG_ALL); campaign complete |
| 37 | build-dead-zones | CYCLE-1 | 2026-07-28 | parity/empty-arm/#if 0/DEBUG_LOCK clean (442db1aa87, own branch) |
| 39 | generated-artifact-determinism | CYCLE-1 | 2026-07-28 | raw+json regen byte-identical (4bdf490240, own branch) |
| 43 | option-api-lifecycle | CYCLE-1 | 2026-07-28 | -prevoutfetchthreads lifecycle clean (331048ba1e, own branch) |
| 45 | constant-time-declassification | CYCLE-1 | 2026-07-28 | AES ctaes + CBC padding constant-time; declassifications intended-only |
| 45 | constant-time-declassification | CYCLE-5 | 2026-08-02 | PR 35688 empty-HMAC UB CONFIRMED+ADOPTED (UBSan pair); F21 |
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
| 22 | full-sync-ibd-profile | CYCLE-4 | 2026-08-02 | churn profile: accounting returns to 0, RSS retains ~3.3MB converging; #65-🟡 baseline |
| 1 | comment-code-contract | CYCLE-1 | 2026-07-28 | LockPoints bound comment wrong (b1c267c9f1); AddCoin/lock/nullptr-parent claims verified true |
| 0 | continuous-bug-mining | CYCLE-2 | 2026-07-28 | TODO evidence sweep: 56 production items, 0 defects, 7 verified risk-map cells |
| 0 | continuous-bug-mining | CYCLE-3 | 2026-08-02 | TODO re-sweep: 56/56 identical, zero session debt; DISMISSED |
| 60 | reviewer-preference-skill | CYCLE-1 | 2026-07-28 | l0rinc seam: 7 rules (R1-R7), held-out 2/3+1 refined; reviewer map n=5 |
| 36 | cross-tool-analysis-matrix | CYCLE-2 | 2026-07-28 | clang-18 differential green; 4 clang-only warnings triaged (3 fuzz-only-helper, 1 test-annotation) |
| 36 | cross-tool-analysis-matrix | CYCLE-3 | 2026-07-31 | clang-18 UBSan full suite: 1128 cases 0 reports (117 __ubsan syms control); {gcc,clang}xUBSan consistent; DISMISSED |
| 36 | cross-tool-analysis-matrix | CYCLE-4 | 2026-07-31 | _GLIBCXX_ASSERTIONS full suite: 1128 cases 0 violations (ninja -t commands flag control); DISMISSED |
| 36 | cross-tool-analysis-matrix | CYCLE-5 | 2026-08-01 | TSan concurrency subset green; 2 warnings = intentional sync_tests inversions; 0 races; DISMISSED |
| 76 | reproducible-builds | CYCLE-1 | 2026-07-28 | A/B rebuild: code bit-identical; 1-byte secp comp_dir DWARF delta attributed; Guix packaging sound |
| 9 | hit-frequency-coverage | CYCLE-2 | 2026-07-28 | net_processing sancov: all gaps harness-scope; 0/23 alarm resolved as inlining artifact via per-line PC check |
| 9 | hit-frequency-coverage | CYCLE-7 | 2026-08-02 | upstream clusterlin corpus x12: 3,436 seeds clean through hardened build; DISMISSED |
| 47 | build-ci-parity | CYCLE-2 | 2026-07-28 | install manifest declarative-single-source; 17-file install set exact match |
| 47 | build-ci-parity | CYCLE-3 | 2026-08-01 | export-set consumer check: downstream compiles+links(g++, 0 undef)+runs via .pc; static lib self-contained; DISMISSED |
| 47 | build-ci-parity | COMPLETE | 2026-08-01 | c4: shared .so consumer full closure (0 undef, 134 exports), runs CONSUMER-OK; DISMISSED; campaign complete |
| 81 | spec-vector-drift | CYCLE-1 (retro) | 2026-07-28 | BIP324+RFC8439 vectors byte-exact (0f6c2640b7, own branch; row restored) |
| 81 | spec-vector-drift | CYCLE-2 | 2026-07-28 | BIP173/350 bech32(m): no drift across unit/key_io/functional layers |
| 81 | spec-vector-drift | CYCLE-3 | 2026-07-31 | BIP341 both levels byte-exact: C++ script_assets 141917/141917, Python wallet-vector port 0 mismatches; no drift |
| 81 | spec-vector-drift | CYCLE-4 | 2026-07-31 | BIP32 25/25 xprv/xpub match + engine green; base58+sighash byte-identical to upstream master; no drift |
| 81 | spec-vector-drift | CYCLE-5 (EXHAUSTED) | 2026-08-01 | Wycheproof AES-CBC-PKCS5 72/72 (independent confirm of #107 c2, cross-journal dup caught pre-archive) |
| 76 | reproducible-builds | CYCLE-2 | 2026-07-28 | depends pins exact (qrencode primary 404, fallback serves pinned bytes); secp ccache = absolute-I key divergence, not uncacheable |
| 21 | rebuild-recovery-profile | CYCLE-2 | 2026-07-28 | tx-heavy reindex-chainstate: 6.8s user, 85% secp256k1 EC math; checks negligible at 610 blocks; harness lessons logged |
| 2 | assertion-invariant-audit | CYCLE-1 | 2026-07-28 | fork production Assumes all construction-tautologies; cache-overflow fix verified; no input-validation-by-assert |
| 2 | assertion-invariant-audit | CYCLE-4 | 2026-08-02 | upstream assert sweep: 209 sampled, all construction-tautologies; DISMISSED |
| 65 | contributor-branch-radar | CYCLE-2 | 2026-07-28 | perf seam: CheckBlock dup-check 1.85x branch locally absent (equivalence plausible), prevector-36 + serialization seeds recorded |
| 79 | fuzz-corpus-stewardship | CYCLE-1 | 2026-07-28 | cross-seed transfer +39-55% on 3 P2P siblings; merge-minimize -36% size zero-loss |
| 10 | fuzz-target-gaps | CYCLE-2 | 2026-07-28 | load_wallet harness delivered (fd74c4a7c2); bring-up crash = harness-oracle bug, production clean |
| 48 | property-oracle-expansion | CYCLE-1 | 2026-07-28 | CompactSize exhaustive battery delivered (8b7d8ac878), 1/1 boundary mutant killed |
| 67 | release-version-differential | CYCLE-1 | 2026-07-28 | v28.2/v0.20.1 ↔ HEAD sync+handshake clean both directions; BIP324 fallback correct |
| 67 | release-version-differential | CYCLE-2 | 2026-07-31 | wtxid/txid inventory across v0.21 boundary: negotiation/inv/getdata all per BIP339; DISMISSED |
| 67 | release-version-differential | CYCLE-3 | 2026-08-01 | downgrade read: v28.2 clean; v0.20.1 loud abort on blocksxor-obfuscated blk files (upstream #28052); its mutations forward-safe |
| 67 | release-version-differential | COMPLETE | 2026-08-01 | c4: mempool.dat v0.20.1<->HEAD bidirectional + coinstatsindex v28.2/HEAD suites PASS with exact binaries; feature_backwards_compatibility absent in fork |
| 67 | release-version-differential | CYCLE-5 | 2026-08-03 | PR 33324 reobf: feature, boundary already verified; NO adoption |
| 24 | disk-io-amplification | CYCLE-1 | 2026-07-28 | regtest IBD: ~2x byte amplification (undo+index), bounded 16MB prealloc; no pathological growth |
| 63 | loupe-style-pipeline | CYCLE-1 | 2026-07-28 | fee-estimator zero-state waste confirmed+fixed (675011ba86): 20.4%->0 samples, -34% IBD user |
| 63 | loupe-style-pipeline | CYCLE-6 | 2026-08-02 | banlist.dat archaeology: 3 distinct loud paths (legacy/corrupt/missing), corrupt arm fault-injected; DISMISSED |
| 35 | mutation-testing | CYCLE-1 | 2026-07-28 | ReadVarInt overflow guards test-blind (M2/M3 survived); oracle delivered+verified (083afedbf1) |
| 92 | abi-alignment-aliasing | CYCLE-1 | 2026-07-28 | kernel enum name-maps value-independent; by-value structs static-linkage contract; aliasing sweep clean |
| 92 | abi-alignment-aliasing | CYCLE-3 | 2026-08-02 | user_data_destroy lifetime proven executable vs shared .so (once-at-context-destroy / NULL-user-owned); DISMISSED |
| 78 | translation-validation | CYCLE-1 | 2026-07-28 | Assume-erasure contract validated at binary: fork hardening zero-cost in release |
| 78 | translation-validation | COMPLETE | 2026-08-02 | c2: g++{-O0,-O2,-O3}+clang{-O0,-O2} outputs md5-identical on 742-vector feefrac/Amount corpus; DISMISSED |
| 100 | sink-reverse-reachability | CYCLE-1 | 2026-07-28 | bloom ctor div-by-zero test-only latent; empty-filter guarded (CVE-2013-5700 present) |
| 100 | sink-reverse-reachability | CYCLE-4 (COMPLETE) | 2026-08-01 | feefrac backends exact: Mul/Div vs fallback 0 diffs (3200 cases) + UBSan clean (shift corners C++20-defined) |
| 7 | resource-exhaustion-variants | CYCLE-2 | 2026-07-29 | UTXO-scan/resize race: fixed in-tree (e049f064e1 unique-lock cursor); upstream master verified still racy |
| 7 | resource-exhaustion-variants | EXHAUSTED | 2026-08-02 | bound census: 16/16 constants enforced+measured |
| 94 | bindings-ffi-parity | CYCLE-2 | 2026-07-29 | enum mapping static_assert tables (073d543f26), reorder tripwire fires at :268 |
| 94 | bindings-ffi-parity | CYCLE-3 | 2026-08-02 | wrapper ownership sound; copy family documented-nonnull (SIGSEGV=annotated misuse); DISMISSED |
| 94 | bindings-ffi-parity | CYCLE-4 | 2026-08-03 | PR 35662: lineage already protected (67239a4a19 flag-reset); upstream asserts |
| 64 | finding-dedup-recurrence | CYCLE-1 | 2026-07-29 | findings-index.md built; 5 fixes confirmed NOT in lineage (F1,F2,F3,F7,F9) |
| 64 | finding-dedup-recurrence | CYCLE-4 | 2026-08-02 | index updated: F17 null-destroy, F18 qa-assets pin (F9-sibling), L3 txgraph retention open item |
| 104 | analogical-vulnerability-translation | CYCLE-3 (queue-empty) | 2026-07-31 | INTERPRETER-CONFUSION: descriptor/miniscript limits agree, all fail closed (valid nest cap 200 via ops 201; parse cap 3600; tr braces 128); DISMISSED |
| 104 | analogical-vulnerability-translation | CYCLE-4 (queue-empty) | 2026-08-02 | Coldcard RNG-fallback/32-bit-reseed shape translated: abort-on-failure + 256-bit seeds throughout; NEGATIVE |
| 104 | analogical-vulnerability-translation | CYCLE-5 | 2026-08-03 | strong-random-contracts adopted: #104 contracts pinned behaviorally+statically |
| 90 | historical-knowledge-recipes | CYCLE-2 | 2026-07-29 | R15-R22 added (sancov inlining, reindex gating, MiniWallet API, io sampling, mutant-first, pool mechanics, dict fuzzing, flag persistence) |
| 90 | historical-knowledge-recipes | CYCLE-3 | 2026-08-02 | R23 lineage sweep: 28 pre-rotation journals restored (2a147cfb08); REPAIR COMPLETE |
| 71 | deterministic-simulation | CYCLE-2 | 2026-07-29 | crash-resume durability invariant delivered (6c6e7d9f87), 3k scans clean |
| 71 | deterministic-simulation | CYCLE-3 | 2026-07-30 | reorged-record resume oracle + mock duplicate-hash fidelity fix (7e88645b92); 3k clean (row restored 2026-07-31) |
| 71 | deterministic-simulation | CYCLE-4 | 2026-07-31 | extension-resume oracle (forced, fire-proofed) + ext-hash uniqueness fix + c1 oracle correction (flip can fire pre-scan); 3k clean |
| 71 | deterministic-simulation | CYCLE-5 (EXHAUSTED) | 2026-07-31 | progress-value fuzzing: monotonic/flat/adversarial gVP schedules, [0,1] + divide-guard oracles; early-consumption trap fixed |
| 32 | whole-history-leftovers | CYCLE-1 | 2026-07-29 | multiply-first percentage shape: no exploitable survivor (range/memory-bounded) |
| 32 | whole-history-leftovers | CYCLE-2 (queue-empty) | 2026-08-02 | all 3 seed families absorbed (#105 c1, scope rule, #49 c10); queue EMPTY |
| 69 | backend-differential | CYCLE-1 | 2026-07-29 | secp wide-multiply int128-vs-int64 differential: 4/4 suites pass, no divergence |
| 69 | backend-differential | EXHAUSTED | 2026-08-02 | census: all host-executable pairs green; x86/4way absent, SHA512/ctaes single |
| 103 | finding-composition | CYCLE-1 | 2026-07-29 | capability graph: no realizable chain; 3 edges tested broken (L2 fixed, F4+F5 fixed, F8 bounded) |
| 103 | finding-composition | CYCLE-2 | 2026-08-02 | F10-F16 indexed, #95-c7 excluded (not in HEAD); all edges broken-by-repair; no realizable chain |
| 53 | timing-side-channel | CYCLE-1 | 2026-07-29 | AES-CBC padding dudect: Welch t 1.53/1.69/-1.14 (no leak), confirms #45 code-read |
| 53 | timing-side-channel | CYCLE-2 | 2026-08-02 | secp ctime_tests under valgrind: 0/0 errors; library constant-time CONFIRMED |
| 74 | memory-pressure-allocator | CYCLE-1 (retro) | 2026-07-28 | mempool accounting honest (1.13x RSS/usage @8k); glibc drain retention not a leak (2ef390de05, row restored) |
| 74 | memory-pressure-allocator | CYCLE-2 | 2026-07-29 | LockedPool oversize alloc -> graceful bad_alloc via RPC (fault-injected); no residue |
| 74 | memory-pressure-allocator | CYCLE-5 | 2026-07-31 | mlock-failure path live: degraded arena works unlocked (locked=0/total=262144), log-silent (upstream-identical); DISMISSED |
| 74 | memory-pressure-allocator | CYCLE-6 (COMPLETE) | 2026-08-01 | pruning IO: disk freed exact (-129KB), RSS flat +0 (index retained by design); boundary exact |
| 105 | project-bug-autopsy-recurrence | CYCLE-1 | 2026-07-29 | txgraph saturation family autopsy: recurrence mapped, no uncovered sibling |
| 105 | project-bug-autopsy-recurrence | CYCLE-2 | 2026-08-01 | dbwrapper failed-construction family: fixed in HEAD, 0 siblings, upstream STILL live (fetched today); offerable |
| 105 | project-bug-autopsy-recurrence | CYCLE-3 | 2026-08-02 | serialize dead-template family: forced-instantiation sweep 0 siblings, contract guard proven both ways; DISMISSED |
| 45 | constant-time-declassification | CYCLE-2 | 2026-07-29 | walletpassphrase: KDF-only throttle 0.10s/attempt measured, no lockout by design |
| 109 | whole-feature-public-path | COMPLETE | 2026-08-01 | c3: shortid collision ~2^48/target (grinder validated, MISS as predicted); tampered-prefilled drive -> IsBlockMutated bad-txnmrklroot -> full-block fallback accepted tip; DISMISSED |
| 66 | backport-correctness | CYCLE-2 | 2026-07-29 | 5 out-of-lineage fixes backported+verified (e15c4025e5, 84a3913096, 508d9edfca, 75c0616c24, b73b7c5d39) |
| 66 | backport-correctness | CYCLE-3 | 2026-08-02 | post-c2 fix reachability: all present (3 as cherry-picked copies); content-level check proceduralized; CONFIRMED+RESOLVED |
| 39 | generated-artifact-determinism | CYCLE-2 | 2026-07-29 | full JSON sweep 60/60 byte-identical (9 tree + 51 univalue) |
| 39 | generated-artifact-determinism | CYCLE-3 | 2026-08-02 | blocked cells re-checked: manpages are intentional placeholders, sage absent; DISMISSED |
| 37 | build-dead-zones | CYCLE-2 | 2026-07-29 | config-dead zones clean: ZMQ/USDT/chainstate-util all properly gated |
| 37 | build-dead-zones | CYCLE-3 | 2026-08-02 | runtime-dead feature opts: accepted-but-inert by design (hidden reg + value validation); DISMISSED |
| 95 | database-semantics-differential | CYCLE-2 | 2026-07-29 | dbwrapper contracts hold: WriteBatch atomicity + HEAD_BLOCKS crash protocol, scan-only iterators |
| 108 | adversarial-artifact-generation | CYCLE-1 | 2026-07-29 | hostile V1 peer artifact: 4/4 classes classified correctly (magic/checksum/oversize/valid) |
| 108 | adversarial-artifact-generation | CYCLE-6 (COMPLETE) | 2026-08-01 | post-handshake v2 slowloris reaped at +20.0min (ping+send timeout dual lines); mocktime + framework peertimeout masks recorded |
| 41 | history-seed-archaeology | CYCLE-1 | 2026-07-29 | fee_estimates.dat version gate fails closed both directions (v28.2 rejects 309900 non-fatally) |
| 41 | history-seed-archaeology | CYCLE-6 | 2026-07-31 | banlist.dat/.json: all 6 cells as documented (round-trip/corrupt/garbage/legacy-dat/expired/write-fail-retry); persistence family CLOSED; campaign EXHAUSTED |
| 50 | fuzz-introspector-blockers | CYCLE-1 | 2026-07-29 | PSBT blocker = harness truncation (ConsumeRandomLengthString); ~500 serialize edges unreachable |
| 57 | local-reasoning-contracts | CYCLE-1 | 2026-07-29 | m_all_zero contracts observable+guarded; broken-discipline mutation caught at bloom_tests:535 |
| 57 | local-reasoning-contracts | EXHAUSTED | 2026-08-02 | successor cells (FRESH/spent, m_dirty_count) already battery-covered; reopen on new flag domain |
| 68 | architecture-abi-parity | CYCLE-2 | 2026-07-29 | char-signedness sweep clean: no sign-sensitive plain-char use; serialize forbid guards the class |
| 68 | architecture-abi-parity | CYCLE-4 | 2026-08-02 | unaligned sweep: memcpy-only wire layer, zero casts; DISMISSED |
| 93 | system-fault-injection | CYCLE-1 | 2026-07-29 | mid-flush crash injection: recovery rolls forward identically (3 crashes, 415 blocks, no corruption) |
| 93 | system-fault-injection | CYCLE-2 (COMPLETE) | 2026-08-01 | fs/permission faults: unreadable blk = loud attributed abort; unwritable blocksdir = loud write-fail; full recovery both |
| 59 | supply-chain-security-gates | CYCLE-2 | 2026-07-29 | workflow byte-identical to upstream (mutable tags, no permissions block); posture upstream-accepted |
| 59 | supply-chain-security-gates | CYCLE-3 | 2026-08-03 | PR 35754 CI pinning adopted: pip hashes, action commits, image digests, tool sha256; F23 |
| 75 | build-throughput-cacheability | CYCLE-2 | 2026-07-29 | header-cost via -ftime-trace: validation.h 6.1s (20%), no anomaly |
| 73 | network-state-machine | CYCLE-2 | 2026-07-29 | handshake EOF sweep: 7/7 v2 offsets + v1 partial close clean, zero half-open peers |
| 73 | network-state-machine | CYCLE-4 | 2026-07-31 | slow-drip ellswift reaped at 64s mid-handshake (V2 handshake timeout); real peer unaffected; DISMISSED |
| 73 | network-state-machine | CYCLE-5 (COMPLETE) | 2026-08-01 | node-initiated half-close: shape absent (zero shutdown() syscalls); peer view = full close, clean EOF 0.04s |
| 99 | clean-room-reimplementation | CYCLE-1 | 2026-07-29 | CompactSize clean-room differential: 804 cases, 0 mismatches |
| 99 | clean-room-reimplementation | CYCLE-6 | 2026-08-02 | merkle root+mutation clean-room: 400/400 exact incl. CVE-2012-2459 anchor; DISMISSED |
| 99 | clean-room-reimplementation | CYCLE-7 | 2026-08-03 | PR 35161 doc+test ADOPTED: merkle two-output contract pinned in-tree (CVE anchor) |
| 38 | failure-cleanup-crash-safety | CYCLE-1 (retro) | 2026-07-28 | EncryptWallet mkey rollback fix (9894fb8b6c, row restored) |
| 38 | failure-cleanup-crash-safety | CYCLE-2 | 2026-07-29 | txindex interrupted-build: empty-block build uninterruptible (~3s); resume mechanics present |
| 38 | failure-cleanup-crash-safety | CYCLE-3 | 2026-08-02 | mid-build kill attempt: warm rebuild sub-second, 6/6 post-build resumes clean; premise confirmed; DISMISSED at scale |
| 55 | alternative-implementation-diff | CYCLE-1 | 2026-07-29 | noble-secp256k1 ECDSA differential: 2019/2019+5/5 RFC6979 vectors match |
| 55 | alternative-implementation-diff | CYCLE-2 | 2026-07-30 | RFC6979+ndata extraEntropy differential: 25/25 + 200/200 byte-identical, three implementations (row restored 2026-07-31) |
| 55 | alternative-implementation-diff | CYCLE-3 | 2026-07-31 | BIP340 sibling vectors: Python 19/19+8/8, in-tree subset byte-exact, noble==official rows 0-14; no drift |
| 55 | alternative-implementation-diff | CYCLE-4 (EXHAUSTED) | 2026-07-31 | rust-bitcoin fixtures 4/4: sighash subset 289/289, BIP341 identical, huge-witness decode, block round-trip byte-exact |
| 107 | conformance-test-transplant | CYCLE-1 | 2026-07-29 | RFC 4231 case 5 transplanted (0d36c6cd80); all 7 cases both hashes |
| 107 | conformance-test-transplant | CYCLE-3 | 2026-08-02 | Wycheproof chacha20_poly1305 316/316 (256 valid byte-exact, 60 invalid rejected), OpenSSL 2nd verifier 0 mismatch |
| 101 | public-characterization-fix | CYCLE-1 | 2026-07-29 | PSBT fuzz hybrid consumption fix (d086164661): 9 starvation-gated functions covered, control re-lists all 9, iso seed 528->2857 edges |
| 1 | comment-code-contract | CYCLE-2 | 2026-07-29 | net_processing strong claims: 9/9 verified TRUE (prune-read, SetupAddressRelay, diff-encoding, tx-inventory-empty); no defect |
| 43 | option-api-lifecycle | CYCLE-2 | 2026-07-29 | -capturemessages: append-across-restart proven (488->976 prefix-intact); capture IO failure aborts node (rc=-6), upstream-identical |
| 101 | public-characterization-fix | CYCLE-2 | 2026-07-29 | truncation-gate sibling sweep: class clean, psbt sole member (13 uses classified; script/banman coverage-confirmed) |
| 101 | public-characterization-fix | EXHAUSTED | 2026-08-02 | queue-empty (truncation-gate class closed, complete table); no fresh signal; review-watch rule stands |
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
| 70 | compiler-optimization-differential | CYCLE-2 | 2026-08-02 | clang thin-LTO HOST-BLOCKED at link (no LLVMgold.so/lld); compile proven; ⚪ resume condition recorded |
| 44 | secret-copy-optimization | CYCLE-1 | 2026-07-29 | key paths secure-container-clean; memory_cleanse elision-resistant at -O3 -flto (2 disassembly probes) |
| 25 | performance-regression-bisect | CYCLE-1 (retro) | 2026-07-28 | +2.4% MemPoolAddTransactions bisected to 3ae78dbd25 (row restored) |
| 25 | performance-regression-bisect | CYCLE-2 | 2026-07-29 | regression persists at HEAD (250.75 ms/op); consumer split caps feerate-only fix upside (2/4 need bitsets); journal-only |
| 46 | api-output-on-failure | CYCLE-1 | 2026-07-29 | kernel C API: 5 surfaces output-on-failure-defined (verify/block_read/process_block/_header/to_bytes); input_index assert = upstream WIP note |
| 102 | durable-suspicion-replay | CYCLE-1 | 2026-07-29 | suspicion-index A1-A11 delivered; L1 blind replay confirms unreachable (sharper UB split: int 0/0 at bloom.cpp:40, inf-cast at :34) |
| 54 | raii-resource-leaks | CYCLE-1 | 2026-07-29 | fopen/raw-new sweeps clean: RAII pervasive; readwritefile/dbwrapper manual paths upstream-matching, bad_alloc-theoretical only |
| 54 | raii-resource-leaks | CYCLE-3 | 2026-08-02 | subprocess: Linux clean; Windows szCmdline leak vendored+upstream-identical; CONFIRMED-LATENT external |
| 51 | differential-metamorphic | CYCLE-1 (retro) | 2026-07-28 | full-UTXO undo oracle (47e5bf2f95, row restored); backported 4807d408fe |
| 51 | differential-metamorphic | CYCLE-2 | 2026-07-29 | multi-block undo composition oracle added (negative control fired [101!=102], reverted green); suite green |
| 76 | reproducible-builds | CYCLE-3 | 2026-07-29 | full depends sweep: 22 URL-verified + 28 cached sources pin-clean + make download green; qrencode primary still dead (fallback exact); xorg.freedesktop.org unreachable=environmental |
| 60 | reviewer-preference-skill | CYCLE-3 | 2026-07-29 | reusable skill encoded: reviews/reviewer-rules.md (R1-R13 + reviewer map, indexed) |
| 21 | rebuild-recovery-profile | CYCLE-3 | 2026-07-29 | Taproot-OP_TRUE mix profiled: tweak math = validation floor (~86% EC), 11.2k vs 9.0k tx/s vs P2PK; no defect |
| 65 | contributor-branch-radar | CYCLE-3 | 2026-07-29 | leveldb knob batch assessed: block-cache-bypass premise VERIFIED (format.cc:106 + table.cc:182); others = bench ladders/diagnostic/vendored bump |
| 1 | comment-code-contract | CYCLE-3 | 2026-07-29 | txmempool+txgraph strong claims 8/8 TRUE; chunk-connectedness assert matches IsAcceptable guard exactly; core claim surface covered |
| 1 | comment-code-contract | CYCLE-5 | 2026-08-02 | validation.cpp leftovers: 60 claims/8 families, deepest 4 TRUE; DISMISSED |
| 45 | constant-time-declassification | CYCLE-3 | 2026-07-29 | RPC auth sweep: comparisons constant-time, single-401; HMAC-on-match username oracle = de minimis at local RPC boundary |
| 75 | build-throughput-cacheability | CYCLE-3 | 2026-07-29 | clean-build wall: cold 1418.2s vs warm 59.6s (23.8x ccache lever); CI cache keys sound (content-key is the anchor) |
| 50 | fuzz-introspector-blockers | CYCLE-2 | 2026-07-29 | signing section covers SignPSBTInput/UpdatePSBTOutput/PSBTInputSignedAndVerified; iso seed 528->2857->3048 edges |
| 25 | performance-regression-bisect | CYCLE-3 | 2026-07-29 | txindex baseline: 3.7s catch-up for 41.4k txs (~validation-path speed); 35531 lineage = branch decision |
| 25 | performance-regression-bisect | CYCLE-4 | 2026-08-02 | txindex lookup profile: pos/neg symmetric ~0.42ms p50, no collision-walk at 41k txs; DISMISSED |
| 23 | perf-flamegraph-investigation | CYCLE-3 | 2026-07-29 | EvictionProtection (sort, 34us micro) + ConnectBlockAll (~98% EC verify) profiles clean; no fix candidate |
| 34 | uncovered-code-classification | CYCLE-3 | 2026-07-29 | ScriptCompression 3 malformed arms closed + mutation-verified (guard-drop kills arm 1) |
| 35 | mutation-testing | CYCLE-2 | 2026-07-29 | WriteVarInt per-line sweep: M2/M3/M4 all killed (34/137/137 failures); CTxUndo VARINT fuzz-covered |
| 48 | property-oracle-expansion | CYCLE-2 | 2026-07-29 | CompactSize 254-class exhaustive (130k+ cases); guard-weakening mutant killed 61440x |
| 48 | property-oracle-expansion | CYCLE-5 | 2026-08-02 | ByRatio ordering laws: 55,201 checks 0 mismatches incl. cpp_int reference; DISMISSED |
| 60 | reviewer-preference-skill | CYCLE-4 | 2026-07-29 | template held-out on 35205: 3/3 blind predictions confirmed; R14 added (setter boundary validation) |
| 106 | semantic-twin-inconsistency | CYCLE-2 | 2026-07-29 | merkle-root twins: 6/6 multi-shape blocks agree; mutation flag C++-only (intentional); Python empty-vector unreachable |
| 40 | multi-agent-adjudication | CYCLE-2 | 2026-07-29 | A11 adjudicated: kernel input_index assert = defect-classified (policy + author's fix branch 6f23568be8); severity none today; parked |
| 80 | fuzz-engine-differential | CYCLE-2 | 2026-07-29 | raw-tx parser differential: A=0/300, B=0 round-trip-exact, C=240 all Python-lax; production never over-accepts |
| 49 | critical-history-sweep | CYCLE-2 | 2026-07-29 | remaining advisory cells: 54604 fork-interaction clean (PRIVBROADCAST rate-limited), 46597 32-bit cap present, 52922/21/13/14 markers verified |
| 49 | critical-history-sweep | CYCLE-10 | 2026-08-02 | pre-2020 advisory batch: CVE-2018-17144 executable guarded (bad-txns-inputs-duplicate at testmempoolaccept) + 8 marker cells; DISMISSED |
| 91 | compiler-binary-hardening | CYCLE-3 | 2026-07-29 | shared kernel lib measured: 134 btck_ + std weak only, zero internal/secp exports; full RELRO+NX; no BTI note (c1's toolchain finding) |
| 106 | semantic-twin-inconsistency | CYCLE-3 | 2026-07-29 | vsize twins agree (formula + 6/6 functional); sighash numerics agree; PSBT DEFAULT\|ALL restriction = intentional policy |
| 106 | semantic-twin-inconsistency | CYCLE-4 | 2026-08-02 | amount-parse twin 26/26: exponent accepted, tzero compression, 21M exact; DISMISSED |
| 43 | option-api-lifecycle | CYCLE-3 | 2026-07-29 | -v2transport settings.json lifecycle proven: honored, persisted (write-back intact), CLI-overridable |
| 43 | option-api-lifecycle | CYCLE-4 | 2026-08-02 | deprecated ancestor opts: wallet-report-only, acceptance cluster-based; docs accurate; DISMISSED |
| 34 | uncovered-code-classification | CYCLE-4 | 2026-07-29 | dbwrapper boundary layers safe; Cursor warmup asymmetry = author's PR 35654 pending (corrupt-only, parked) |
| 34 | uncovered-code-classification | CYCLE-5 | 2026-07-31 | BitsToBytes padding arms: dedicated test, 0xff-mutant killed (was sancov-granularity, not behavioral gap); in-tree queue CLOSED |
| 34 | uncovered-code-classification | EXHAUSTED | 2026-08-02 | in-tree cells closed; only external watch remains (PR 35654 open @2026-08-02); watch rides #42 |
| 23 | perf-flamegraph-investigation | CYCLE-4 | 2026-07-29 | CompareMainTransactions = O(1) early-exit comparator; memcmp/atomic shares inherent-by-design; no fix candidate |
| 23 | perf-flamegraph-investigation | CYCLE-5 | 2026-08-02 | retention attribution: author test run at HEAD, pre-fix semantics confirmed; chain closed |
| 95 | database-semantics-differential | CYCLE-3 | 2026-07-29 | RocksDB swap builds+reindexes correctly; CPU parity (validation-bound), wall -43% (write parallelism, directional) |
| 95 | database-semantics-differential | CYCLE-6 | 2026-08-02 | kill-during-pressure-flush at -dbcache=4 scale: 3/3 zero-corruption recoveries; durability surface closed; DISMISSED |
| 95 | database-semantics-differential | COMPLETE | 2026-08-02 | c7: rocksdb-brute MultiRead autopsy — ASan UAF + 2N/4N misalignment CONFIRMED branch-local; HEAD clean; campaign complete |
| 42 | ci-review-bot-followup | CYCLE-2 | 2026-07-29 | wider DrahtBot sweep (15 PRs, 7 flagged): all failures PR-owned; 35793 shows in-tree assert working as designed |
| 44 | secret-copy-optimization | CYCLE-2 | 2026-07-29 | clang cross-check: barrier un-elidable in IR (calloc+barrier+free); cleanse idiom confirmed both compilers |
| 44 | secret-copy-optimization | CYCLE-3 | 2026-08-02 | crossing map: all crossings secure/cleansed/deliberate-export; DISMISSED |
| 80 | fuzz-engine-differential | CYCLE-3 | 2026-07-29 | consensus acceptance differential: A=0/300 (no structural over-acceptance); E=53/D=99 agreement, C=148 parse/policy |
| 80 | fuzz-engine-differential | COMPLETE | 2026-08-02 | c12: musig2 nonce/sig vs participant membership unchecked (parser maps, decodepsbt-only consumer); foreign-K3 doc accepted live; DISMISSED |
| 80 | fuzz-engine-differential | CYCLE-13 | 2026-08-03 | PR 35797: lineage already protected (53506a51e9); upstream still vulnerable |
| 75 | build-throughput-cacheability | CYCLE-4 | 2026-07-29 | 45-uncacheable itemized: 57/58 = failed compilations (rotation's own mutants); IPC/capnp hypothesis refuted |
| 75 | build-throughput-cacheability | CYCLE-5 | 2026-08-02 | posture re-check: 85 uncacheable=own residue, 6 modified-during-compile=own edits; hit 58% steady; DISMISSED |
| 95 | database-semantics-differential | CYCLE-4 | 2026-07-29 | durability differential: kill -9 mid-reindex, both engines recover identical tip, zero corruption |
| 102 | durable-suspicion-replay | CYCLE-2 | 2026-07-29 | A5 capturemessages abort replayed on v28.2 binary: same msghand exception abort (behavior-verified, second verifier form) |
| 102 | durable-suspicion-replay | CYCLE-3 | 2026-08-02 | A11 replay: upstream-identical @556988790a, zero in-tree callers; CONFIRMED-LATENT; replay queue empty |
| 102 | durable-suspicion-replay | CYCLE-4 | 2026-08-02 | pre-existing crash-* analysis: 33 targets, zero reproduction; stale-harness verdict |
| 102 | durable-suspicion-replay | CYCLE-5 | 2026-08-02 | full census 242/242 targets zero reproduction; closed |
| 91 | compiler-binary-hardening | CYCLE-4 | 2026-07-29 | ELF census 7/7 binaries: PIE+NX+full RELRO+canary uniform, zero divergence; BTI note absent (c1 toolchain finding) |
| 91 | compiler-binary-hardening | CYCLE-5 | 2026-08-02 | _FORTIFY_SOURCE=3 configured+visible in all 7 binaries; no gap except c1 BTI |
| 80 | fuzz-engine-differential | CYCLE-4 | 2026-07-29 | PSBTv2 differential: v2 paths clean (A=0/400 mixed); E=107 round-trip-exact, C=124 Python-lax |
| 50 | fuzz-introspector-blockers | CYCLE-3 | 2026-07-29 | correlated PSBT signing seed: layout replay byte-exact + walletprocesspsbt complete=True; complete-arm driver |
| 50 | fuzz-introspector-blockers | CYCLE-14 (COMPLETE) | 2026-08-01 | taproot sighash size-class gates 6/6 (DEFAULT 64B, non-DEFAULT 65B+trailing, musig2 psig parse-reject); RPC-parameter confounder recorded |
| 51 | differential-metamorphic | CYCLE-3 | 2026-07-29 | fee-diagram incremental-vs-recompute: hook exists (txgraph fuzz sim+CompareChunks), 1000 runs green |
| 51 | differential-metamorphic | CYCLE-4 | 2026-08-02 | BIP30 dup unconstructable PROVEN LIVE (bad-cb-height + bad-version); queue empty |
| 51 | differential-metamorphic | CYCLE-5 | 2026-08-03 | dup disconnect-pool txids: Assert-fires confirmed, skip-fix adopted (unit green); feature_block.py reverted in archive (BIP30 divergence) |
| 51 | differential-metamorphic | CYCLE-6 | 2026-08-03 | divergence settled: BIP30 identical to upstream; PR test is author's open iteration; not a fork issue |
| 42 | ci-review-bot-followup | CYCLE-3 | 2026-07-29 | corecheck endpoint = real oracle; flags 35744 bench regressions (ComplexMemPool +15.6%, OrphanageEraseForPeer +33%) upstream-side |
| 46 | api-output-on-failure | CYCLE-2 | 2026-07-29 | import_blocks clean; callback reentrancy constraint real (cs_main held, deadlock on reentry) but undocumented (upstream-identical) |
| 46 | api-output-on-failure | CYCLE-3 | 2026-08-02 | per-callback lock map: block_tip cs_main-held (unique), header_tip asserted lock-free, progress init-thread; same upstream doc gap |
| 21 | rebuild-recovery-profile | CYCLE-4 | 2026-07-29 | dbcache sensitivity: wall +60% at dbcache=4, user-CPU neutral (validation-bound); fork default sane |
| 21 | rebuild-recovery-profile | CYCLE-5 | 2026-08-02 | forcecompactdb: sub-second, chainstate 2MB->1MB, scope=2 validation DBs; fact |
| 60 | reviewer-preference-skill | CYCLE-5 | 2026-07-29 | maintainer merge-rationale mined: M1-M4 (terse depth-honest ACKs, self-contained descriptions, lifecycle, no info-hiding) |
| 60 | reviewer-preference-skill | CYCLE-9 | 2026-08-02 | watch: 35592/35838 template-conformant; ordering caveat recorded; quiet |
| 60 | reviewer-preference-skill | CYCLE-10 | 2026-08-02 | watch: master static; 35744 rework exchange, 35859 tested-ACK + CI flags; quiet |
| 60 | reviewer-preference-skill | CYCLE-11 | 2026-08-02 | watch: 35865 premise-refutation close; template holds; quiet |
| 60 | reviewer-preference-skill | CYCLE-12 | 2026-08-02 | watch: zero delta; quiet |
| 60 | reviewer-preference-skill | CYCLE-13 | 2026-08-02 | zero-delta x3; cadence -> merge-event-triggered |
| 58 | helper-reuse | CYCLE-3 | 2026-07-29 | 7th PSBT copy deduplicated: helper moved to rpc/rawtransaction_util (existing shared header); rpc_psbt green |
| 58 | helper-reuse | CYCLE-4 (queue-empty) | 2026-08-02 | census: DecodeHexTx shared; near-twins binary-separated by design; queue EMPTY |
| 65 | contributor-branch-radar | CYCLE-4 | 2026-07-29 | rocksdb-brute assessed: bulk-fetch class subsumed by shipped -prevoutfetchthreads; stale WIP, nothing actionable |
| 65 | contributor-branch-radar | CYCLE-12 | 2026-08-02 | new author branch txgraph-retained-entry-usage: memory-accounting gap REAL+bounded at HEAD, fix in flight; URGENT 🟡 |
| 65 | contributor-branch-radar | CYCLE-13 | 2026-08-02 | package-weight-accumulator: narrowing real, wrap unreachable (25x4M WU cap); hygiene only |
| 65 | contributor-branch-radar | CYCLE-14 | 2026-08-02 | rpc-deduplicate-scan-objects: perf nicety, semantics-preserving; no URGENT change |
| 65 | contributor-branch-radar | CYCLE-15 | 2026-08-02 | radar: zero delta; quiet |
| 65 | contributor-branch-radar | CYCLE-16 | 2026-08-02 | retained-capacity fix ADOPTED (28ba79168b), flipped test green; 🟡 -> ✅ verified |
| 65 | contributor-branch-radar | CYCLE-17 | 2026-08-03 | PR 35839 empty-headers stall CONFIRMED+ADOPTED (5-commit stack); F22 |
| 65 | contributor-branch-radar | CYCLE-18 | 2026-08-03 | PR 35195 tradeoff premise measured (-1.0% time); deliberate, NO adoption |
| 65 | contributor-branch-radar | CYCLE-19 | 2026-08-03 | PR 34864 large refactor of verified code; no defect; NO adoption |
| 65 | contributor-branch-radar | CYCLE-20 | 2026-08-03 | PR 33637 comparator perf refactor: not hot on our profiles, NO adoption |
| 65 | contributor-branch-radar | CYCLE-21 | 2026-08-03 | PR 31868 author-draft, not adoptable; watch 32043 |
| 65 | contributor-branch-radar | CYCLE-22 | 2026-08-03 | PR 35205 kernel dbcache setter: API extension, no defect; NO adoption |
| 65 | contributor-branch-radar | CYCLE-23 | 2026-08-03 | PR 34320: presence==unspent by discipline; NO adoption |
| 65 | contributor-branch-radar | CYCLE-24 | 2026-08-03 | PR 34132 catcher-removal refactor; fatal handling verified; NO adoption |
| 65 | contributor-branch-radar | CYCLE-25 | 2026-08-03 | PR 35820 typed durations: hardening, no live defect; NO adoption |
| 65 | contributor-branch-radar | CYCLE-26 | 2026-08-03 | PR 32729 sigop coverage: test refactor, no defect; NO adoption |
| 65 | contributor-branch-radar | CYCLE-27 | 2026-08-03 | 26-PR sweep COMPLETE: 6 adopted, 2 covered-ahead, 17 assess-only, 5 watch |
| 80 | fuzz-engine-differential | CYCLE-5 | 2026-07-29 | rich PSBTv2 differential: A=0/400, E=115, R=4 guard-contained; BIP371 value-layout lesson (6-seed iterations) |
| 42 | ci-review-bot-followup | CYCLE-4 | 2026-07-29 | corecheck sweep: bench deltas noise-shaped (same +10-33% on unrelated bloom PR); regression reading corrected |
| 42 | ci-review-bot-followup | CYCLE-6 | 2026-08-02 | watch @556988790a: 5 commits (2 out-of-scope merges), PRs static, F13/F14/F16 offerability re-confirmed |
| 42 | ci-review-bot-followup | CYCLE-7 | 2026-08-02 | watch: static; F13/F14/F17 offerable, accumulate hygiene offer noted; PRs open |
| 42 | ci-review-bot-followup | CYCLE-8 | 2026-08-02 | watch: static; quiet |
| 42 | ci-review-bot-followup | CYCLE-9 | 2026-08-02 | variant: 26 author PRs open; radar branches pre-PR; static |
| 42 | ci-review-bot-followup | CYCLE-10 | 2026-08-02 | event-trigger sweep: all quiet |
| 42 | ci-review-bot-followup | CYCLE-11 | 2026-08-02 | trigger re-check: all quiet |
| 42 | ci-review-bot-followup | CYCLE-12 | 2026-08-02 | PR 35714: flush-failure write-through CONFIRMED+ADOPTED (f90291ffb9), F19 |
| 42 | ci-review-bot-followup | CYCLE-13 | 2026-08-03 | master static; dup-txid CI progressing, 0 failures so far |
| 42 | ci-review-bot-followup | CYCLE-14 | 2026-08-03 | flush characterize = pre-fix anti-test; dropped, fix oracle kept; S18 |
| 10 | fuzz-target-gaps | CYCLE-3 | 2026-07-29 | load_wallet widened (crypted/ACTIVE*SPK/BESTBLOCK); LoadActiveScriptPubKeyMan assert on corrupt DB documented (upstream-matching) |
| 80 | fuzz-engine-differential | CYCLE-6 | 2026-07-29 | MuSig2 PSBT seeding: differential clean (A=0, E=100); format-from-source worked first try |
| 95 | database-semantics-differential | CYCLE-5 | 2026-07-31 | write-flush-windowed kill: _Exit inside all 4 batch commits (idx/coins/shutdown); identical tip recovery, 0 corruption; DISMISSED |
| 35 | mutation-testing | CYCLE-5 | 2026-07-31 | CTxUndo hostile-field layers classified (decode-reject/apply-reject/trust-boundary); range-check mutant killed fail-before/pass-after |
| 35 | mutation-testing | CYCLE-6 | 2026-07-31 | latent uncompilable SizeComputer overload repaired (DEFAULT mode, upstream-inherited); boundary battery; M_a/M_b/M_c killed 8/134/15 |
| 76 | reproducible-builds | CYCLE-4 | 2026-08-01 | 45-uncacheable itemization: 0 uncacheable in all 6 live families (logfile-verified); lifetime 83 = deleted-config residue; DISMISSED |
| 76 | reproducible-builds | CYCLE-5 | 2026-08-02 | qrencode fallback byte-exact today, upstream .mk identical; URGENT pruned; DISMISSED |
| 45 | constant-time-declassification | CYCLE-4 (COMPLETE) | 2026-08-01 | secp ctime_tests under valgrind memcheck: full suite, 0 errors (production backend params, aarch64) |
| 79 | fuzz-corpus-stewardship | CYCLE-2 | 2026-08-01 | per-seed profile FLAT (max/median 1.05x); ~99.5% per-invocation cost = startup; oversized-seed hypothesis refuted |
| 79 | fuzz-corpus-stewardship | CYCLE-3 | 2026-08-01 | qa-assets sparse import @918cdd3 (==CI pin): 7,846 seeds/3 corpora single-pass clean, zero artifacts; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-4 | 2026-08-02 | coins/UTXO/storage batch: 15 targets 20,760 seeds all clean; cumulative 32,042 green; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-5 | 2026-08-02 | crypto batch: 11 targets 6,634 seeds clean; cumulative 38,676; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-6 | 2026-08-02 | script/sighash batch: 14 targets 22,177 seeds clean; cumulative 60,853; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-7 | 2026-08-02 | net/P2P batch: 15 targets 19,470 seeds clean; cumulative 80,323; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-8 | 2026-08-02 | block/merkle batch: 18 targets 7,405 seeds clean; IMPORT COMPLETE 87,728 seeds green; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-9 | 2026-08-02 | feefrac/miner batch: 15 targets 12,077 seeds clean; cumulative 99,805; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-10 | 2026-08-02 | utility sweep: 25 targets 12,577 seeds clean; PROGRAM COMPLETE 112,382 green; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-11 | 2026-08-02 | 10-min mutation campaign on ephemeral_package_eval: fresh coverage, zero crashes; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-12 | 2026-08-02 | txgraph scratch campaign: 420,878 runs 0 crashes, corpus 0->4,586; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-13 | 2026-08-02 | txorphanage_sim campaign: 67,241 runs 0 crashes, corpus +205; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-14 | 2026-08-02 | txgraph corpus minimized 3,468 units cov 12,054/ft 73,114; qa-assets PR candidate |
| 79 | fuzz-corpus-stewardship | CYCLE-15 | 2026-08-02 | coinscache_sim campaign: 2,380 runs 0 crashes, corpus +152; DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-16 | 2026-08-02 | mini_miner campaign: 4,400 runs 0 crashes (saturation target); DISMISSED |
| 79 | fuzz-corpus-stewardship | CYCLE-17 | 2026-08-02 | txrequest scratch campaign: 235,767 runs 0 crashes; mutation sweep 6/6 clean |
| 35 | mutation-testing | CYCLE-7 (COMPLETE) | 2026-08-01 | NONNEGATIVE_SIGNED negative-write mangle pinned (-1->0x7f->127); unreachable from all call sites; upstream-identical |
| 59 | supply-chain-security-gates | CYCLE-3 (COMPLETE) | 2026-08-01 | qa-assets corpus clone commit-pinned (weakening arm was silent; c1 reasoning covered injection only); live-verified |
| 40 | multi-agent-adjudication | CYCLE-3 (COMPLETE) | 2026-08-01 | L4 executable confirmation: HEAD vs branch dup-check differential fuzz, 300k cases 0 divergences |

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
   RE-RANK draw 154 (8-cell pool): raw=9940712460200409047,
   masked 717340423345633239 -> idx 7 -> #45 c4 (secp ctime_tests
   under valgrind memcheck: full suite 0 errors at production
   backend params; DISMISSED). Campaign #45 COMPLETE.
   Pool after draw 154 (7): #81 Wycheproof, #36 TSan subset,
   #74 pruning-mode IO, #35 NONNEGATIVE_SIGNED write semantics,
   #50 Taproot/MuSig2 gates, #9 per-seed profiling, #67 downgrade
   read.
   RE-RANK draw 155 (7-cell pool): raw=7423517245362505699
   (already 63-bit) -> idx 6 -> #67 c3 (downgrade read: v28.2
   clean; v0.20.1 loud deterministic abort on blocksxor-
   obfuscated blk files (upstream #28052, default-on since
   v28.0); its mutations forward-safe; DISMISSED).
   Pool after draw 155 (6): #81 Wycheproof, #36 TSan subset,
   #74 pruning-mode IO, #35 NONNEGATIVE_SIGNED write semantics,
   #50 Taproot/MuSig2 gates, #9 per-seed profiling.
   RE-RANK draw 157 (5-cell pool): raw=6943923678126847234
   (already 63-bit) -> idx 4 -> #79 c2 (per-seed profile FLAT
   max/median 1.05x; ~99.5% per-invocation cost = startup;
   oversized-seed hypothesis REFUTED).
   RE-RANK draw 159 (3-cell pool): PREEMPTED by confirmed in-tree
   defect (user-reported upstream PR bitcoin#35859): KDF iteration
   count overflow — rounds > INT_MAX narrow into the KDF's signed
   count, ~2^31-round unlock hang. FIXED on audit/kdf-rounds-overflow
   (55788c9a76, mechanism probe + boundary test; F16 added; URGENT
   updated). Draw resumes next cycle.
   RE-RANK draw 162 (n=1): raw=2610058608719210074 (already
   63-bit) -> idx 0 -> #35 c7 (NONNEGATIVE_SIGNED negative-write
   mangle pinned (-1->0x7f->127, INT64_MIN->0x00->0); unreachable
   from all call sites; upstream-identical; DISMISSED). Campaign
   #35 COMPLETE. Queue after draw 162: EMPTY — re-harvest journal
   "Limitations / queue" tails before the next draw.
   RE-RANK draw 166 (7-cell pool): raw=14119870030197581011,
   masked 4896497993342805203 -> idx 4 -> #40 c3 (L4 executable
   confirmation: dup-check differential fuzz 300k cases 0
   divergences, verdict+diagnosis; CONFIRMED). Campaign #40
   COMPLETE.
   Pool after draw 166 (6): #47 shared-lib consumer, #36
   functional-under-clang, #10 VerifyCryptedKeys, #109 short-id
   collision, #105 capability autopsy, #67 backwards-compat.
   RE-RANK draw 167 (6-cell pool): raw=1164948412836237405
   (already 63-bit) -> idx 3 -> #109 c3 (short-id collision cell:
   collision infeasible ~2^48 preimage/target, grinder HIT-validated
   then MISS as predicted; tampered-prefilled solicited drive PROVED
   the IsBlockMutated bad-txnmrklroot gate + MSG_WITNESS_BLOCK
   fallback live, tip accepted; DISMISSED). Campaign #109 COMPLETE.
   Pool after draw 167 (5): #47 shared-lib consumer, #36
   functional-under-clang, #10 VerifyCryptedKeys, #105 capability
   autopsy, #67 backwards-compat.
   RE-RANK draw 168 (5-cell pool): raw=2943418269569493264
   (already 63-bit) -> idx 4 -> #67 c4 (cross-version functional
   suites with exact archived binaries: mempool.dat v0.20.1<->HEAD
   bidirectional PASS, coinstatsindex v28.2/HEAD PASS; queued
   feature_backwards_compatibility test absent in fork, in-scope
   equivalents run; DISMISSED). Campaign #67 COMPLETE.
   Pool after draw 168 (4): #47 shared-lib consumer, #36
   functional-under-clang, #10 VerifyCryptedKeys, #105 capability
   autopsy.
   RE-RANK draw 169 (4-cell pool): raw=13595585722515773213,
   masked 4372213685660997405 -> idx 1 -> #36 c6 (functional-under-
   clang: 6-test consensus/P2P subset 7/7 identical to gcc
   baseline; Werror CI gap DISMISSED via BITCOIN_CONFIG_ALL).
   Campaign #36 COMPLETE.
   Pool after draw 169 (3): #47 shared-lib consumer, #10
   VerifyCryptedKeys, #105 capability autopsy.
   RE-RANK draw 250: raw=5687290599673046444 -> consolidated
   regression sweep (test_bitcoin + test_kernel + functional
   subset ALL GREEN on the integrated lineage).
   Cycle 256 (census completion): 242/242 targets zero
   reproduction; artifact question closed.
   Cycle 255 (crash-artifact analysis, no draw — unexplored
   artifact preempt): 33-target scan, zero reproduction; stale
   artifacts, left untouched.
   RE-RANK draw 254 (3-watch pool): raw=6000597539641640036
   (63-bit) -> idx 2 -> #60 c13 (zero-delta x3; cadence ->
   merge-event-triggered).
   Pool: #42, #65 on same event-triggered rule.
   Cycle 318 (watch): upstream master advanced 30f6b05857 ->
   e27c179db2 (merge #35869 lint: re-add guix Python linting;
   ci/lint/requirements.txt +1 dep, guix security/symbol-check,
   lint-python.py). F23 note: the ci/lint pin lockfiles will need
   regeneration on next rebase (new requirement). No adoption
   target (lint tooling). Author remote static (1363, 0 new).
   Cycle 317 (fuzz transplants + flood close-out): txospenderindex
   crash ROOT-CAUSED — harness-contract violation (parallel target
   seeds RNG, never SetMockTime; BaseIndex::Sync reads NodeClock;
   CheckGlobalsImpl teardown abort at check_globals.cpp:54; NOT
   index logic; 4-byte seed 76 00 43 00 preserved in artifacts/);
   repaired with SetMockTime(1231006505), 20k clean (70f5b19656).
   blockmanager malformed-disk native test green (cf33694d3c).
   dbwrapper_scheduled_pair transplanted with full-value Oracle
   adaptation (their fork's key→size vs ours key→value; 2 compile
   errors fixed; 20k running). private_broadcast goal61-stateful
   COVERED-AHEAD — our target has the complete independent state
   model since dbef68896c (2026-07-02, fork-only): AssertMatches
   Model + AssertSameBroadcastInfo superset theirs. FLOOD CLOSED:
   52/52 src-touching branches have recorded verdicts.
   Cycle 316 (verification + goal48 closure): REGRESSION #8 GREEN
   (full test_bitcoin on the cycle-312 lineage with F34/F35 +
   blockfilter). goal48 oracle series all 4 VERIFIED GREEN +
   adopted as coverage: mtp boundary property (30bb38cdc7),
   AccessByTxid sparse-output, variable-tip proof relation,
   BitSet Fill boundary (310051e236); archive b2573677da.
   Tooling note: fuzz runs need build_fuzz + FUZZ=<target> env
   (build-after has BUILD_FOR_FUZZING=OFF). Flood queue fully
   closed: every one of the 52 src-touching branches has a
   recorded verdict; remaining flood branches are wallet-scoped
   or journal/meta per the goal-111 manifest.
   Cycle 315 (watch): upstream master advanced again dcc2ed52b8
   -> 30f6b05857 (merge #35860 rpc fuzz rework, fuzz-only 112
   lines; second fuzz advance). Both advances merge-tree CLEAN
   against our modified rpc.cpp/tx_pool.cpp (0 conflicts) — no
   adoption (upstream fuzz coverage lands on next rebase), no
   rebase hazard. Author remote static (1363 heads, 0 new).
   Cycle 314 (goal 111 tail — goal87 series disposition):
   STRUCTURAL MISMATCH — their 4 test-only branches extend their
   fork's divergent persistence cases (MempoolV1DependencyOrdering
   family, 0 matches in-tree); extension impossible. Gap found:
   no GetTotalFee/GetTotalTxSize roundtrip coverage in-tree.
   Native test MempoolTotalsPreservedAcrossDumpLoad written
   (matured-coinbase spends via ProcessTransaction, dump, wipe,
   reload, identical totals); first run caught
   bad-txns-premature-spend-of-coinbase (mineBlocks(3) maturity
   pattern); full mempool_tests GREEN. audit/transplant-goal87-
   tests 8146b1fd8a, archive 9ee781a1bd. Flood manifest now fully
   closed: every src-touching branch has a recorded verdict.
   Cycle 313 (watch + verification): upstream master ADVANCED
   556988790a -> dcc2ed52b8 (single merge #35856, fuzz-only 48
   lines in tx_pool.cpp; no src changes, no adopted-area overlap;
   merge-tree clean vs our modified tx_pool.cpp — no rebase
   conflict). dup-txid CI: 34 success/2 NetBSD failures (author's
   own test flow — settlement holds). Watch PRs 35744/35859/
   35818/35620/35654 all open, unmerged. qa-assets == pin.
   Regression #8b test_kernel GREEN on final lineage. Regression
   #8 full suite in flight.
   Cycle 312 (goal 111 cell closures, end): goal86-prune-restart
   (blockfilter) CONFIRMED + ADOPTED — height-only ReadFilterHeader
   rejects own index after reorg + unclean kill; failing-before
   init exit 1 with exact 'unexpected block ... Cannot read last
   block filter header'; passing-after clean restart, full
   feature_index_prune green (--timeout-factor=8 for ASan speed);
   audit/adopt-blockfilter-reorg-recovery 8b9a20b114, archive
   fe8d015755, F35 🟠 in URGENT (txgraph-retained evicted to
   history). Repair: feature_txospenderindex registered in
   test_runner (7b46cbcc5d). Flood queue now EXHAUSTED per the
   goal-111 manifest: all 52 src-touching branches have recorded
   dispositions (8 adopted F25-F35 span + goal10/goal6 tests,
   2 dismissed-with-evidence, rest covered/deferred/test-only).
   Catalog pool returns to watches-only (116 tool-blocked,
   120-124 repo-blocked, 0-109 cycled, 110-119/125-127 done).
   Cycle 311 (goal 111 cell closures, continued): goal7-descriptor-
   range CONFIRMED + ADOPTED — int loop counter overflows after the
   valid INT32_MAX endpoint; failing-before test process KILLED BY
   SIGNAL (exit 133, -ftrapv build); passing-after full rpc_tests
   green; audit/adopt-descriptor-range-overflow 62e05ae526,
   archive 759755b6c5, F34. goal43-reindex-interrupt COVERED
   (upstream c1313b199f in master + in-tree feature_init boundary
   test; parallel 6d29ab1a1e duplicates). goal86-prune-restart
   (blockfilter) failing-before run in flight (first attempt died
   to ASan-speed RPC timeout, re-run at --timeout-factor=8).
   Cycle 310 (goal 111 coverage manifest + cell closures): flood
   manifest — 106 i9/codex branches -> 52 with unique src/ commits:
   17 assessed, 9 wallet-deferred, 2 release-branch, 12 test-only,
   2 minor, 6 unreviewed-core ranked, 4 misc. Cell 1 goal7-
   getblocktxn DISMISSED (VectorFormatter 5MiB batching +
   DifferenceFormatter uint16 overflow + 4MB cap = 3 existing
   bounds; failing-before unobtainable). Cell 2 goal6-merkle
   COVERED-AHEAD (fork 1e7ca53c1b 2026-06-27: vnIndex.clear() +
   Assume invariants; their reuse test adopted, green; archive
   c949f83780). Cell 3 goal72 settings durability CONFIRMED +
   ADOPTED (strace syscall pair: 0 fsync/fdatasync pre-fix,
   present post-fix; argsman suite green; f194e4482e, archive
   e3fc515b7d). Cell 4 goal86-txospender CONFIRMED + ADOPTED
   (public-RPC wrong spend status post-crash; functional failing-
   before with invalidated spender returned; 6cd9d75a67, archive
   902d84a97f, F33 🟠 in URGENT, F26 evicted).
   Cycle 309 (goal 118 sandbox/isolation audit, draw
   raw=3903574178988509641 n=2 idx=1): 0 stray processes; /tmp
   scratch swept (foreign ldb_oracle_merger.cc untouched);
   CONTINUITY REPAIR — all harness sources canonical in
   agent-journal/artifacts/ (12 files + replay README) incl.
   rescued xor_tool.cpp (#41 c3, /tmp-only since 2026-07-29);
   credentials/dirty-state compliant. Pool after: {111} workable
   + 116 tool-blocked + Sparrow repo-blocked.
   Cycle 308 (goal 125 WAL/MANIFEST/VersionSet recovery; draw
   raw=139903139087397213 idx=1 -> 116 tool-blocked, recorded
   125-preempt): MANIFEST corruption fails loud even with
   create_if_missing (CONFORM); WAL mid-record corruption —
   paranoid=true loud / default keeps intact prefix 1,755/2,000
   0 torn (CONFORM); CURRENT-loss + create_if_missing opens EMPTY
   with live tables silently orphaned — VERIFIED HAZARD NOTE
   (documented client-choice contract; atomic temp+rename writes
   make the window external-only; mitigation seed recorded).
   Regression #7 GREEN. LevelDB trio (125/126/127) all covered.
   Cycle 307 (goal 113 risk ranking, draw
   raw=12828903349424140929 masked=3605531312569365121 n=5 idx=1):
   live-queue risk table (severity x reachability x confidence x
   proof-value / cost) — 125 WAL/MANIFEST recovery is the only
   non-trivial marginal yield (proof-val 4, low cost, reopen-time
   blast radius, harness pattern proven); 116 tool-blocked (no
   clang-tidy); 111/118 bookkeeping; standing watches zero local
   marginal yield. Stop-depth audit: 126/127 dismissals correctly
   bounded. Decision recorded: 125 preempts next cycle if not
   drawn (urgent-preempt rule).
   Cycle 306 (goal 110 catalog evolution + entropy audit, draw
   raw=6547846130219125632 n=6 idx=0): proposed-goals resolutions
   logged — P1 EXECUTED (goal 119 c1), P2 DELIVERED (snap_builder2
   artifact), P3 DISMISSED (bad_alloc-only), P4 ADOPTED (F30);
   queue EMPTY. Entropy-quality verdict: genuine shape/boundary
   returns each cycle (write-failure family, clock-margin table,
   LevelDB conformance harnesses, harness-preservation fix,
   chmod-as-root blind spot); no promotion-ready new seeds (bar =
   evidence-backed nonduplicate). Catalog file hash unchanged
   (ba7b1dd0a2ab7203); regeneration needs user's generator
   (prompt-chars metadata is machine-written) — documented.
   Cycle 305 (goal 117 calibration, draw
   raw=17929826660396438331 masked=8706454623541662523 n=7 idx=4):
   mutation-catch battery on the final lineage — 3/3 adopted
   oracles kill re-injected original defects (F25 txdb cursor,
   F28 mempoolexpiry, F30 headers clamp; failure shapes bit-
   identical to adoption-day failing-befores; all mutants
   restored). Oracles live, evidence chain end-to-end.
   Cycle 304 (goal 127 LevelDB corruption/checksums/bg-errors,
   draw raw=11133441295003937015 masked=1910069258149161207 n=8
   idx=7): client assumptions code-verified (paranoid_checks +
   verify_checksums read+iter, HandleError throw on non-NotFound);
   corruption arm — 1-byte table corruption -> 86/5000 keys
   Corruption-surfaced, 0 silent-wrong (CONFORM); bg-error arm —
   dir-rename fault -> next Put fails NotFound immediately
   (CONFORM); harness lesson: chmod faults invalid as root
   (CAP_DAC_OVERRIDE), use dir-rename. Both arms DISMISSED;
   MANIFEST/descriptor recovery = goal 125 queue.
   Cycle 303 (goal 114 threat-model->oracle, draw
   raw=17746324290832141658 masked=8522952253977365850 n=9 idx=3):
   clock-skew security-gate sweep (35 NodeClock/GetTime sites)
   — every remaining gate has a design margin exceeding the
   +/-70min adjusted-time attacker bound (24h staleness on system
   clock, 3.3h tip recency, 2h block-future on adjusted time,
   hours-level bans/file ages); the only failable class is
   sub-minute-margin + remote-input gates = the two proven
   instances F22/F30. NEGATIVE RESULT narrowing the threat model;
   validity condition for future findings recorded.
   Cycle 302 (goal 126 LevelDB semantics, draw
   raw=7175895203802760358 n=10 idx=8): comparator N/A (default
   Bytewise everywhere, no custom in-tree); bloom standard
   10 bits/key; iterator-vs-concurrent-writes conformance
   harness 20 rounds 0 violations (torn/post-snapshot/status,
   ASan+UBSan silent); pinned-iterator + overwrite + delete +
   full CompactRange keeps complete original snapshot (20,000
   keys) — both arms DISMISSED as defects; real iterator hazard
   (DB reset mid-scan) already covered-ahead (35744 family).
   Harnesses in agent-journal/artifacts/. Regression #7 running.
   Cycle 301 (goal 112 replay/continuity/FP-revalidation, draw
   raw=8554301223849903997 n=11 idx=2): CONTINUITY GAP FIXED —
   fault-injection harnesses lived only in /tmp (ephemeral);
   preserved into agent-journal/artifacts/ (xor_interpose.c,
   snap_interpose.c, xor_experiment.sh, snap_builder2.py +
   replay README; commit b99892faab). FP revalidation: goal92-abi
   dismissal RE-CONFIRMED with 3rd/4th independent verifiers
   (gcc -fsanitize=undefined no-recover exit 0; gcc -O3 -flto
   -fstrict-aliasing exit 0). Regression #6 GREEN (full
   test_bitcoin on 8996d8c1e8). Regression #7 running on final tip.
   Cycle 300 (goal 115 committed-diff/working-tree audit, draw
   raw=12596182104518000101 masked=3372810067663224293 n=12
   idx=5): reviewer-pass over session adoptions — F26 catch-all
   remove could delete a foreign-created xor.dat only in a
   datadir-lock-impossible race: ACCEPTED RISK documented;
   F27/F30/F28/F29 CLEAN (typed catches, per-attempt recovery,
   no valid-config rejections). Working-tree: tracked clean,
   untracked = user files + 20 stale corpus artifacts (untouched
   per protocol). Verdicts journaled in bulk-ecosystem-recurrence
   cycle 2.
   Cycle 299 (goal 119 bulk-ecosystem-recurrence, draw
   raw=6652536966674966662 n=13 idx=9): write-failure bug shape
   NORMALIZED (restart-authoritative file, direct write, failure
   leaves truncated artifact or escapes designed error path) and
   swept across all in-tree persistence producers: mempool.dat/
   peers.dat/anchors.dat atomic (.new/temp+RenameOver); banlist
   #41-c6 covered; fee_estimates.dat non-atomic but read-discard
   + hourly rewrite = benign; settings.json parse-tolerant =
   benign; i2p + tor persistent keys LIVE — CONFIRMED + ADOPTED
   (i2p e976e68fc9 failing-before truncated-key-persists+retry-
   fails / passing-after suite green; tor 5cf00e1380 key-path-as-
   directory portable write failure); archive 3e5c7b1368; F31/F32.
   Family total now 6 confirmed instances (F19/F26/F27/F31/F32 +
   goal38), 5 adopted this session. Sweep EXHAUSTED for src/.
   Cycle 298 (radar-flood tail): P3 kernel ownership trio
   (9b5bdd99fc/8c5db6e36e/309226ff53) DISMISSED — every trigger is
   a bad_alloc on the OOM path (leak-under-OOM, no non-allocation
   throw reachable); goal56-future-mtp CONFIRMED + ADOPTED —
   signed elapsed -> uint64 m_max_commitments wrap kills the
   presync memory cap under >2h backward clock skew
   (failing-before PRESYNC vs FINAL; passing-after full headers
   suite green); audit/adopt-headers-clock-lag 35473f91b4,
   archive a5a73c53f2, F30. URGENT swap: F30 in (remote-gated),
   F25 out to index. Radar-flood adoption tally now F25-F30
   (6 adopted) + goal10 test; covered-ahead: goal26/goal98/goal33;
   dismissed: goal92-abi, P3 trio; cross-confirmed: dup-txid.
   Cycle 297b (same cycle): goal43-cluster-size — count arm
   COVERED-AHEAD (in-tree both-bounds :115-120), size arm
   CONFIRMED + ADOPTED (failing-before 2 arms accepted; passing-
   after mempool_tests green; minimal pre-ingestion guard variant
   vs their helper refactor — avoids the post-multiply overflow
   window; audit/adopt-clustersize-validation a3253e6396, archive
   e9cb3e3f02, F29).
   Cycle 297 (radar-flood continuation): goal26-null-mempool
   COVERED-AHEAD (fork 0a2deeea1d 2026-07-19 with kernel-API
   reproducer + test; parallel 1d05b4ac4f semantic-duplicate);
   goal43-mempoolexpiry CONFIRMED + ADOPTED — failing-before
   (ApplyArgsManOptions(-1) accepted) / passing-after (mempool_tests
   green) + parallel daemon-eviction second verifier;
   audit/adopt-mempoolexpiry-negative 0e0b3d6576, archive
   b095724b20, F28. goal43-cluster-size count arm COVERED-AHEAD
   (both-bounds in-tree :110-115); clustersize negative/overflow
   arm still live (queued). NOTE: clearing a bad cherry-pick with
   reset --hard restored the user's unstaged deletion of
   agent-journal/campaign-goals-99.md (content == HEAD, no loss;
   their catalog-migration deletion may be re-applied by them).
   Cycle 296 (radar-flood continuation + catalog migration to
   128-goal mutable, hash ba7b1dd0a2ab7203): goal10-snapshot-
   basehash COVERED-AHEAD (fork a146380c8e 2026-07-02 guards the
   read arm; their regression test adopted, passes: archive
   297175a4aa); goal38-snapshot-write CONFIRMED + ADOPTED —
   deterministic LD_PRELOAD path-targeted short-write on the
   base_blockhash stream: pre-fix RPC generic -1 raw exception +
   orphaned chainstate_snapshot with 1-byte marker; post-fix
   designed -32603 'could not write base blockhash' + orphan
   removed + clean retry (audit/adopt-snapshot-write-cleanup
   3c9090b644, archive 86533108ab; F27). Canonical regtest
   height-299 snapshot reproduced bit-exact (0c552ced ==
   chainparams) via framework cache + feature_assumeutxo recipe;
   framework trap recorded: default setup_nodes mines +1 block
   (IBD-exit), override setup_network for recipe-exact chains.
   goal33 parse-oracles COVERED (all parsers have in-tree fuzz
   targets + 112,382-seed corpus). Consolidated regression #4
   GREEN (full test_bitcoin on 297175a4aa). URGENT at 10 (F27 in,
   dbwrapper-leak pruned to history). Unrelated worktree deletion
   of agent-journal/campaign-goals-99.md (user-side catalog
   migration) left untouched, never staged.
   Cycle 295b (same radar-flood cycle): goal93-xor-key
   (d7d3559a30) CONFIRMED + ADOPTED — deterministic LD_PRELOAD
   short-write pair: pre-fix 1-byte xor.dat left + restart
   'AutoFile::read: end of file' (unbootable); post-fix file
   removed, restart regenerates 8-byte key, getblockcount=0,
   clean stop. audit/adopt-xor-key-shortwrite 2110abf119 +
   journal 8334dcfa8e. Harness preserved in radar journal
   (one-shot fwrite interposer + driver script).
   Cycle 295 (RESUME TRIGGER FIRED — radar flood): 498 new l0rinc
   branches (865->1363), a parallel audit campaign's working set.
   Triage + verdicts: goal56-disconnect-duplicate (4061d3763d)
   CROSS-CONFIRMED == adopted a9a78f2907 semantics (independent
   second verifier for F24); goal56-txdb-cursor (062a1a02ad = open
   PR 35654) CONFIRMED + ADOPTED — failing-before (cursor Valid()
   true over one-byte 'C' malformed key) / passing-after (focused +
   coins_tests green) on audit/adopt-txdb-cursor-firstkey
   8481b1f27f; goal92-abi enum aliasing DISMISSED (uint8_t ==
   unsigned char byte-alias permitted by [basic.lval]; gcc
   -Wstrict-aliasing=2 + clang UBSan clean; zero behavior delta);
   goal98 fee-estimator pair COVERED-AHEAD (fork 62f15c4ab9 +
   0cf655f1d6 already reject non-finite/negative vectors + bad
   bucket boundaries, stricter than parallel isfinite-only).
   Queued: goal93-xor-key (HEAD vulnerable; LD_PRELOAD interposer
   staged at /tmp/xor_interpose.so). Full test_bitcoin verify in
   flight at cycle close.
   Cycle 294 (watch, zero-delta x3 after resume — BLOCKED audit
   threshold met): origin/master static 556988790a; l0rinc fetch
   zero ref updates (865 heads); qa-assets HEAD 918cdd3 == CI pin;
   dup-txid CI 44 queued/9 in_progress/3 success/1 skipped/0
   failures — no verdict; watch PRs 35744/35859/35818/35620/35654
   all open, unmerged. All internal work complete (catalog cycled,
   corpus 112,382 seeds green, 6/6 mutation clean, regression #3
   green on 01abf72d0b lineage). Goal marked BLOCKED on external-
   signal impasse; resume on: any upstream advance, new/force-
   updated author branch, qa-assets past pin, dup-txid CI verdict,
   watch-PR merge, or host tooling (lld, clang-tidy, disk, 2nd host).
   Cycle 293 (watch, zero-delta x2 after resume): origin/master
   static 556988790a; l0rinc fetch zero ref updates (865 heads);
   qa-assets HEAD 918cdd3 == CI pin; dup-txid CI 44 queued/9
   in_progress/3 success/1 skipped/0 failures — still no verdict;
   watch PRs 35744/35859/35818/35620/35654 all open, unmerged.
   No resume trigger fired.
   Cycle 292 (watch, zero-delta x1 after resume): origin/master
   static 556988790a; l0rinc fetch zero ref updates (865 heads);
   qa-assets HEAD 918cdd3 == CI pin; dup-txid CI (l0rinc 8dfa501356)
   45 queued/8 in_progress/3 success/1 skipped/0 failures — no
   verdict; watch PRs 35744/35859/35818/35620/35654 all open,
   unmerged; recently-touched author PRs (32729/34864/34132/35205,
   35754 head 49cc4e8cabdd == adopted tip) all assessed at current
   heads. No resume trigger fired.
   Cycle 291 (verification): consolidated regression #3 — full
   suite + kernel + 8 functional instances ALL GREEN.
   Cycle 290: flush-failure characterize adoption REDIRECTED
   (pre-fix anti-test dropped; fix oracle kept; S18).
   Cycle 289 (watch): master static; dup-txid CI progressing,
   0 failures so far (settlement holds).
   Cycle 288: divergence settled — BIP30 identical to
   upstream; PR's test flow is the author's open iteration.
   Cycle 287 (radar hit): disconnect-pool dup txids: skip-fix
   ADOPTED (unit green; functional fork-scenario BIP30
   divergence recorded, upstream-CI oracle).
   Cycle 285 (verification): consolidated regression #2 — full
   suite + kernel + 7 functional tests ALL GREEN on the final
   integrated lineage.
   Cycle 284 (radar hit): strong-random-contracts ADOPTED
   (#104 RNG contracts pinned; tidy check CI-side).
   Cycle 283 (suspicion-mined): PR 35260 + 32189 closure;
   26-PR sweep COMPLETE (6 adopted, 2 covered-ahead, 17
   assess-only, 5 standing-watch).
   Cycle 282 (suspicion-mined): PR 32554 bench block gen:
   bench modernization; A/B methodology unaffected; no
   adoption.
   Cycle 281 (suspicion-mined): PR 32729 sigop coverage: test
   refactor, no defect; no adoption.
   Cycle 280 (suspicion-mined): PR 35161 merkle doc+test
   ADOPTED (pins #99-c6's proven contract in-tree).
   Cycle 279 (suspicion-mined): PR 35820 typed durations:
   hardening refactor; no adoption.
   Cycle 278 (suspicion-mined): PR 33324 reobfuscation:
   feature, boundary verified; no adoption.
   Cycle 277 (suspicion-mined): PR 34132 error-catcher
   refactor; fatal handling verified; no adoption.
   Cycle 276 (suspicion-mined): PR 34320 HaveCoin/Exists split:
   no reachable divergence; no adoption.
   Cycle 275 (suspicion-mined): PR 35662 txdata reuse: lineage
   already protected (67239a4a19); upstream asserts-on-reuse.
   Cycle 274 (suspicion-mined): PR 35754 CI pinning CONFIRMED
   gap + ADOPTED (full 7-commit stack: lint binaries, uv.lock,
   test pip hashes, Git-source pins, image digests, SDK digests,
   action commits; F23).
   Cycle 273 (suspicion-mined): PR 35205 kernel dbcache
   setter: API extension, no defect; no adoption.
   Cycle 272 (suspicion-mined): PR 35797 PSBT abort: lineage
   already protected (53506a51e9); upstream still vulnerable.
   Cycle 271 (suspicion-mined): PR 31868 author-draft; no
   adoption; watch 32043.
   Cycle 270 (suspicion-mined): PR 33637 comparator perf
   refactor: not hot on our profiles, semantic-sensitive; no
   adoption.
   Cycle 269 (suspicion-mined): PR 34864 cache-state refactor
   assessed: battery-verified working code, no defect; no
   adoption.
   Cycle 268 (suspicion-mined): PR 35195 hash-cache tradeoff
   premise measured on host (-1.0% time); deliberate tradeoff,
   no adoption.
   Cycle 267 (suspicion-mined): PR 35839 empty-headers stall
   CONFIRMED (PR's own test fails at HEAD) + ADOPTED (5-commit
   stack; F22).
   Cycle 266 (suspicion-mined): PR 35688 empty-HMAC UB
   CONFIRMED (first-invalid UBSan trace) + ADOPTED (std::copy;
   probe silent; F21); archive 27fda24406.
   Cycle 265 (suspicion-mined): PR 35833 log injection both
   arms CONFIRMED+ADOPTED (forged consensus-error failing-before
   proof); F20; archive 0c7d19aec8.
   Cycle 264 (suspicion-mined from 26-PR sweep): PR 35714
   flush-failure write-stop CONFIRMED at HEAD + ADOPTED with
   failing-before/passing-after pair; F19 added; URGENT entry.
   Cycle 263 (goal resumed with suspicion-mining protocol;
   adopt-the-🟡 preempt): 475ab49da6 adopted onto lineage
   (28ba79168b), flipped test green, profile rerun unchanged
   within noise; 🟡 L3 -> ✅ adopted+verified.
   RE-RANK draw 262: raw=4778902433768003794 -> trigger
   re-check (all quiet).
   RE-RANK draw 261: raw=3682198685844657455 -> #79 c17
   (txrequest scratch campaign: zero crashes; sweep 6/6).
   RE-RANK draw 260: raw=10150652524316948765 -> #79 c16
   (mini_miner campaign: zero crashes).
   RE-RANK draw 259: raw=12284359011998066296 -> #79 c15
   (coinscache_sim campaign: zero crashes).
   RE-RANK draw 258: raw=12724671350429090044 -> event-trigger
   sweep (master/radar/qa-assets: all quiet).
   RE-RANK draw 257: raw=12184613341022450052 -> #79 c14
   (txgraph corpus stewardship: 3,468 units, cov measured).
   RE-RANK draw 253 (1-target pool): raw=16849002992585107402
   -> #79 c13 (txorphanage_sim campaign: zero crashes).
   Pool: watches/signals only.
   RE-RANK draw 252 (2-target pool): raw=450380974070410478
   (63-bit) -> idx 0 -> #79 c12 (txgraph scratch campaign:
   420,878 runs, zero crashes).
   Pool: txorphanage_sim mutation run, watches.
   RE-RANK draw 251 (3-target mutation pool): raw=
   5490570177768737669 (63-bit) -> idx 2 -> #79 c11 (10-min
   mutation campaign: fresh coverage, zero crashes).
   Pool: txgraph / txorphanage_sim mutation runs, watches.
   RE-RANK draw 249 (1-watch pool): raw=173496863651005843 ->
   #65 c15 (radar: zero delta; quiet). Pool: watches on
   interval / new signals only.
   RE-RANK draw 248 (2-watch pool): raw=13484172206762621838,
   masked 4260800169907846030 -> idx 0 -> #42 c9 (variant
   watch: 26 author PRs; radar branches pre-PR).
   Pool: #65 radar (next).
   RE-RANK draw 247 (3-watch pool): raw=3324739333356707657
   (63-bit) -> idx 2 -> #60 c12 (watch: zero delta; quiet).
   Pool: #42, #65 (next).
   RE-RANK draw 246 (1-cell pool): raw=3671710469452730917 ->
   #79 c10 (utility sweep: 12,577 seeds; PROGRAM COMPLETE
   112,382 green; DISMISSED). Pool: watches/signals only.
   RE-RANK draw 245 (corpus-remainder pool): #79 c9 (feefrac/
   miner batch: 12,077 seeds clean; cumulative 99,805;
   DISMISSED). Pool: utility-encoding sweep (last corpus cell).
   RE-RANK draw 244 (1-watch pool): raw=8935267519944944495
   -> #65 c14 (radar: rpc dedup nicety; no URGENT change).
   Pool: watches/signals only.
   RE-RANK draw 243 (2-watch pool): raw=17911194993721673680,
   masked 8687822956866897872 -> idx 0 -> #42 c8 (watch: static;
   quiet).
   Pool: #65 radar (next).
   RE-RANK draw 242 (3-watch pool): raw=13148362549488644065,
   masked 3924990512633868257 -> idx 2 -> #60 c11 (watch:
   35865 premise-refutation; quiet).
   Pool: #42, #65 (next).
   RE-RANK draw 241 (1-family pool): raw=7943594722486301467
   -> #79 c8 (block/merkle batch: 7,405 seeds; IMPORT PROGRAM
   COMPLETE 87,728 seeds green; DISMISSED).
   Pool: watches/signals only.
   RE-RANK draw 240 (2-family pool): raw=15578937025157429834,
   masked 6355564988302654026 -> idx 0 -> #79 c7 (net/P2P
   batch: 19,470 seeds clean; DISMISSED).
   Pool: block/merkle family (last corpus family).
   RE-RANK draw 239 (3-family pool): raw=13612649842643089771,
   masked 4389277805788313963 -> idx 2 -> #79 c6 (script/
   sighash batch: 22,177 seeds clean; DISMISSED).
   Pool: net/P2P-ser, block/merkle families.
   RE-RANK draw 238 (4-corpus-family pool): raw=
   17902753180228641946, masked 8679381143373866138 -> idx 2 ->
   #79 c5 (crypto corpus batch: 6,634 seeds clean; DISMISSED).
   Pool: net/P2P-ser, block/merkle, tx/sighash families.
   RE-RANK draw 237 (1-watch pool): raw=4535641261288490616
   -> #42 c7 (watch: static; F17 newly offerable).
   Pool: all watches cycled; next deeper harvest or new
   signals.
   RE-RANK draw 236 (2-watch pool): raw=17935444686604899945,
   masked 8712072649750124137 -> idx 1 -> #65 c13 (radar:
   package-weight wrap unreachable; hygiene note).
   Pool: #42 upstream watch (next).
   RE-RANK draw 235 (3-watch pool): raw=4428187949155325621
   (63-bit) -> idx 2 -> #60 c10 (watch: template holds; CI
   flags recorded).
   Pool: #42 upstream watch, #65 branch radar (next draws).
   RE-RANK draw 234 (1-cell pool): raw=2425925052785082083
   -> v22 real banlist migration: 3-version chain CONFIRMED
   (v0.21 dat -> v22 silent json migration, timestamps
   preserved -> HEAD warning + intact state). Pool EMPTY —
   deeper harvest next.
   RE-RANK draw 233-redraw (2-cell pool): raw=
   3871160351176686871 (63-bit) -> idx 1 -> lineage re-sweep:
   112/112 present, zero missing; CONFIRMED.
   Pool after 233r (1): v22 real banlist migration; then
   deeper harvest.
   RE-RANK draw 233 (3-cell pool): raw=17388002019312348372,
   masked 8164629982457572564 -> idx 1 -> #100 >int64 tail —
   ABSORBED by #48 c5 (postscript recorded).
   Pool after 233 (2): v22 real banlist migration, lineage
   re-sweep.
   RE-RANK draw 232-redraw (4-cell pool): raw=
   1432272597518399293 (63-bit) -> idx 1 -> #28 c4 (mask-arm
   mutant killed 3x; DISMISSED).
   Pool after 232r (3): v22 real banlist migration, >int64
   absorption, lineage re-sweep.
   RE-RANK draw 232 (5-cell pool): raw=15445030354089246842,
   masked 6221658317234471034 -> idx 4 -> network-state tail
   parse — VACUOUS (self-classified irrelevant; null
   experiment, #73 postscript recorded).
   Pool after 232 (4): v22 real banlist migration, mask-arm
   mutation, >int64 absorption, lineage re-sweep.
   RE-RANK draw 231 (6-cell re-harvested pool): raw=
   16378166984725965126, masked 7154794947871189318 -> idx 4 ->
   #40 c4 (corpus-extension tail: VACUOUS, null experiment).
   Pool after 231 (5): v22 real banlist migration, mask-arm
   mutation, >int64 absorption, lineage re-sweep, tail parse.
   RE-RANK draw 230 (1-campaign pool): raw=418748291906749644
   (63-bit) -> #1 c5 (validation leftovers: deepest TRUE;
   DISMISSED). Pool EMPTY — re-harvest before draw 231.
   RE-RANK draw 229 (2-campaign eligible set): raw=
   8430095800060831913 (63-bit) -> idx 1 -> #68 c4 (unaligned
   sweep: clean; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64, #2, #7,
   #37, #29, #68}.
   RE-RANK draw 228 (3-campaign eligible set): raw=
   4931726890564104754 (63-bit) -> idx 1 -> #29 c4 (reverse
   dead-code sample: 9/9 live; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64, #2, #7,
   #37, #29}.
   RE-RANK draw 227 (4-campaign eligible set): raw=
   12394660665079399478, masked 3171288628224623670 -> idx 2 ->
   #37 c3 (runtime-dead opts: by design; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64, #2, #7,
   #37}.
   RE-RANK draw 226 (5-campaign eligible set): raw=
   17325288802186375634, masked 8101916765331599826 -> idx 1 ->
   #7 c5 (bound census: EXHAUSTED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64, #2, #7}.
   RE-RANK draw 225 (6-campaign eligible set): raw=
   16331761350244753227, masked 7108389313389977419 -> idx 1 ->
   #2 c4 (upstream assert sweep: tautologies; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64, #2}.
   RE-RANK draw 224 (7-campaign eligible set): raw=
   11143129023243121708, masked 1919756986388345900 -> idx 5 ->
   #64 c4 (index sweep: F17/F18/L3 added with dedup notes).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39, #64}.
   RE-RANK draw 223 (8-campaign eligible set): raw=
   8820920191778943197 (63-bit) -> idx 5 -> #39 c3 (blocked
   cells re-checked: placeholders; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23, #39}.
   RE-RANK draw 222 (9-campaign eligible set): raw=
   6987695114313435315 (63-bit) -> idx 3 -> #23 c5 (retention
   attribution via author test at HEAD; chain closed).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22, #23}.
   RE-RANK draw 221 (10-campaign eligible set): raw=
   521034545174603853 (63-bit) -> idx 3 -> #22 c4 (churn
   memory profile: #65-🟡 empirical baseline).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61, #22}.
   RE-RANK draw 220 (11-campaign eligible set): raw=
   12572157352548436285, masked 3348785315693660477 -> idx 8 ->
   #61 c4 (undo-data absorbed; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69, #61}.
   RE-RANK draw 219 (12-campaign eligible set): raw=
   16824079916139056791, masked 7600707879284280983 -> idx 11 ->
   #69 c5 (backend census: EXHAUSTED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28, #69}.
   RE-RANK draw 218 (13-campaign eligible set): raw=
   8871967550067354166 (63-bit) -> idx 5 -> #28 c3 (sighash
   guard mutant killed 409x; DISMISSED). Side-repair noted:
   weak-test-oracles.md was written-to-disk by the 2a147cfb08
   restore but missed from its staging list; the complete file
   (c1-c3) is now tracked via the c3 commit.
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21, #28}.
   RE-RANK draw 217 (14-campaign eligible set): raw=
   7034722257148739485 (63-bit) -> idx 3 -> #21 c5
   (forcecompactdb profile; fact).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58, #21}.
   RE-RANK draw 216 (15-campaign eligible set): raw=
   6114129767934136255 (63-bit) -> idx 10 -> #58 c4 (duplicate
   census: queue EMPTY).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38, #58}.
   RE-RANK draw 215 (16-campaign eligible set): raw=
   18054278019244708601, masked 8830905982389932793 -> idx 9 ->
   #38 c3 (mid-build kill: sub-second rebuild, premise
   confirmed; DISMISSED at scale).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57,
   #38}.
   RE-RANK draw 214 (17-campaign eligible set): raw=
   8524466033060714935 (63-bit) -> idx 11 -> #57 c5 (successor
   survey: both covered; EXHAUSTED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51, #57}.
   RE-RANK draw 213 (18-campaign eligible set): raw=
   10746796534152436801, masked 1523424497297660993 -> idx 11 ->
   #51 c4 (BIP30 unconstructability proven live; queue empty).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54, #51}.
   RE-RANK draw 212 (19-campaign eligible set): raw=
   6892152841030680972 (63-bit) -> idx 12 -> #54 c3 (subprocess
   sweep: Windows vendored leak, Linux clean; CONFIRMED-LATENT).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48, #54}.
   RE-RANK draw 211 (20-campaign eligible set): raw=
   15898866038248710399, masked 6675494001393934591 -> idx 11 ->
   #48 c5 (ByRatio ordering laws oracle: 0/55,201; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76, #48}.
   RE-RANK draw 210-redraw2 (21-campaign pool): raw=
   1130504839277943065 (63-bit) -> idx 20 -> #76 c5 (qrencode
   fallback verified; URGENT pruned; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32, #101, #34, #76}.
   RE-RANK draw 210-redraw (22-campaign pool): raw=
   2652496819051359750 (63-bit) -> idx 8 -> #34 — EXHAUSTED
   (in-tree cells closed; PR 35654 still open, watch rides
   #42). Redraw pending.
   RE-RANK draw 210 (23-campaign eligible set): raw=
   3799289345000871924 (63-bit) -> idx 22 -> #101 — EXHAUSTED
   (queue-empty, no fresh signal; recorded). Redraw pending.
   RE-RANK draw 209 (24-campaign eligible set): raw=
   10310213087096134840, masked 1086841050241359032 -> idx 8 ->
   #32 c2 (seed absorption audit: queue EMPTY).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60, #32}.
   RE-RANK draw 208 (25-campaign eligible set): raw=
   15795260889241617676, masked 6571888852386841868 -> idx 18 ->
   #60 c9 (reviewer watch: template holds; quiet).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30, #60}.
   RE-RANK draw 207 (26-campaign eligible set): raw=
   4599885279918200254 (63-bit) -> idx 8 -> #30 c5 (URI-on-
   exception: safe by construction; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31, #30}.
   RE-RANK draw 206 (27-campaign eligible set): raw=
   13014474695973678266, masked 3791102659118902458 -> idx 9 ->
   #31 c5 (docs extraction: zero falsely-parseable; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53,
   #31}.
   RE-RANK draw 205 (28-campaign eligible set): raw=
   3235443014574466965 (63-bit) -> idx 17 -> #53 c2 (secp
   ctime_tests valgrind green; CONFIRMED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79, #53}.
   RE-RANK draw 204 (29-campaign eligible set): raw=
   664956795418753102 (63-bit) -> idx 27 -> #79 c4 (coins/
   storage corpus batch: 20,760 seeds clean; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43, #79}.
   RE-RANK draw 203 (30-campaign eligible set): raw=
   5711495130052899885 (63-bit) -> idx 15 -> #43 c4 (deprecated
   ancestor opts lifecycle: docs accurate; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9, #43}.
   RE-RANK draw 202 (31-campaign eligible set): raw=
   4615193276133132286 (63-bit) -> idx 3 -> #9 c7 (clusterlin
   corpus family validated: 3,436 seeds clean; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0, #9}.
   RE-RANK draw 201 (32-campaign eligible set): raw=
   7541771897718507552 (63-bit) -> idx 0 -> #0 c3 (TODO
   re-sweep: 56/56 identical; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17, #0}.
   RE-RANK draw 200 (33-campaign eligible set): raw=
   15353805843689760592, masked 6130433806834984784 -> idx 5 ->
   #17 c4 (scalar SHA256 end-to-end green; CONFIRMED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70, #17}.
   RE-RANK draw 199 (34-campaign eligible set): raw=
   13766814895528030814, masked 4543442858673255006 -> idx 30 ->
   #70 c2 (clang thin-LTO: host-blocked at link, ⚪ resume
   condition recorded).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65, #70}.
   RE-RANK draw 198 (35-campaign eligible set): raw=
   3654393603145900293 (63-bit) -> idx 28 -> #65 c12 (radar:
   txgraph retained-capacity gap CONFIRMED REAL+bounded, author
   fix in flight; URGENT 🟡 added).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44, #65}.
   RE-RANK draw 197 (36-campaign eligible set): raw=
   13023617683058369127, masked 3800245646203593319 -> idx 19 ->
   #44 c3 (secret crossing map: clean; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75, #44}.
   RE-RANK draw 196 (37-campaign eligible set): raw=
   1891301078475215477 (63-bit) -> idx 33 -> #75 c5 (cache
   posture re-check: own-residue only; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63, #75}.
   RE-RANK draw 195 (38-campaign eligible set): raw=
   13100020088567363976, masked 3876648051712588168 -> idx 28 ->
   #63 c6 (banlist.dat archaeology: 3 loud paths, corrupt arm
   live; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90, #63}.
   RE-RANK draw 194 (39-campaign eligible set): raw=
   16675846116259616253, masked 7452474079404840445 -> idx 37 ->
   #90 c3 (R23 lineage sweep: 28 journals restored in 2a147cfb08;
   REPAIR COMPLETE).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94,
   #90}.
   RE-RANK draw 193 (40-campaign eligible set): raw=
   14784019335709398726, masked 5560647298854622918 -> idx 38 ->
   #94 c3 (wrapper/copy-null contract: documented nonnull;
   DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16, #94}.
   RE-RANK draw 192 (41-campaign eligible set): raw=
   4334000722194619450 (63-bit) -> idx 5 -> #16 c4 (LINEAGE
   REPAIR x2: journal + null-destroy fix restored, test_kernel
   green; upstream @556988790a vulnerable; CONFIRMED+REPAIRED;
   URGENT entry added).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106, #16}.
   RE-RANK draw 191 (42-campaign eligible set): raw=
   6989687031219001031 (63-bit) -> idx 41 -> #106 c4 (amount-
   parse twin 26/26; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92, #106}.
   RE-RANK draw 190 (43-campaign eligible set): raw=
   16880636655586222475, masked 7657264618731446667 -> idx 39 ->
   #92 c3 (user_data_destroy lifetime executable proof;
   DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4, #92}.
   RE-RANK draw 189 (44-campaign eligible set): raw=
   16823264503710230643, masked 7599892466855454835 -> idx 3 ->
   #4 c3 (lineage repair + retraction re-verified; CONFIRMED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105, #4}.
   RE-RANK draw 188 (45-campaign eligible set): raw=
   12902246674943953776, masked 3678874638089177968 -> idx 43 ->
   #105 c3 (serialize SizeComputer family: 0 siblings, guard
   proven; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91, #105}.
   RE-RANK draw 187 (46-campaign eligible set): raw=
   17580883877144746278, masked 8357511840289970470 -> idx 40 ->
   #91 c5 (_FORTIFY_SOURCE=3 verified in binaries; fact).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102, #91}.
   RE-RANK draw 186 (47-campaign eligible set): raw=
   8926299595551660088 (63-bit) -> idx 44 -> #102 c3 (A11
   replay: CONFIRMED-LATENT, upstream-identical, zero callers;
   replay queue empty).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104, #102}.
   RE-RANK draw 185 (48-campaign eligible set): raw=
   5590414441187505117 (63-bit) -> idx 45 -> #104 c4 (Coldcard
   RNG shape translation: NEGATIVE; abort-on-failure design).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42, #104}.
   RE-RANK draw 184-redraw (49-campaign pool): raw=
   3428502195126078629 (63-bit) -> idx 20 -> #42 c6 (upstream
   watch @556988790a: quiet; PRs static; F13/F14 offerable).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99, #49,
   #24, #42}.
   RE-RANK draw 184 (50-campaign eligible set): raw=
   6415546748680638711 (63-bit) -> idx 11 -> #24 — STALE
   (journal: 6 cycles, COMPLETE 2026-07-30; handoff entry added).
   Redraw pending.
   RE-RANK draw 183 (51-campaign eligible set): raw=
   2201154799395572746 (63-bit) -> idx 25 -> #49 c10 (pre-2020
   advisory batch: CVE-2018-17144 executable guarded + 8 marker
   cells; DISMISSED). Pool: eligible minus {#103, #107, #66,
   #46, #25, #99, #49}.
   RE-RANK draw 182 (52-campaign eligible set): raw=
   4137810895725815666 (63-bit) -> idx 46 -> #99 c6 (merkle
   clean-room differential 400/400; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46, #25, #99}.
   RE-RANK draw 181 (53-campaign eligible set): raw=
   13891094590669444291, masked 4667722553814668483 -> idx 12 ->
   #25 c4 (txindex lookup profile: symmetric, no cliff;
   DISMISSED). Pool: eligible minus {#103, #107, #66, #46, #25}.
   RE-RANK draw 180 (54-campaign eligible set): raw=
   676234014466335055 (63-bit) -> idx 25 -> #46 c3 (per-callback
   lock-state map; block_tip uniquely cs_main-held; DISMISSED).
   Pool: eligible minus {#103, #107, #66, #46}.
   RE-RANK draw 179 (55-campaign eligible set): raw=
   7791267694587749993 (63-bit) -> idx 38 -> #66 c3 (post-c2
   fix-lineage reachability: all present; CONFIRMED+RESOLVED).
   Pool: eligible minus {#103, #107, #66}.
   RE-RANK draw 178 (56-campaign eligible set): raw=
   11916844877793972431, masked 2693472840939196623 -> idx 55 ->
   #107 c3 (Wycheproof chacha20_poly1305 transplant 316/316;
   CONFORMANCE CONFIRMED). Pool: eligible minus {#103, #107}.
   RE-RANK draw 177 (57-campaign eligible set rebuilt from the
   handoff DONE list): raw=5480729779092233995 (63-bit) -> idx 52
   -> #103 c2 (cross-session capability indexing F10-F16; all
   edges broken; DISMISSED). Pool: eligible set minus #103.
   RE-RANK draw 176 (1-cell pool): raw=2446543444696896762
   (63-bit) -> idx 0 -> #80 c12 (musig2 cross-field: unchecked
   membership, informational, foreign-K3 accepted; DISMISSED).
   Campaign #80 COMPLETE. Pool EMPTY — re-harvest before 177.
   RE-RANK draw 175 (2-cell pool): raw=17804226048192678763,
   masked 8580854011337902955 -> idx 1 -> #95 c7 (rocksdb-brute
   MultiRead autopsy: ASan UAF + 2N/4N CONFIRMED branch-local,
   HEAD clean). Campaign #95 COMPLETE.
   Pool after draw 175 (1): #80 fuzz-engine-diff musig2
   cross-field; then re-harvest.
   RE-RANK draw 174 (3-cell pool): raw=3650315433786879391
   (63-bit) -> idx 1 -> #78 c2 (opt-level arithmetic differential:
   5 builds md5-identical; DISMISSED). Campaign #78 COMPLETE.
   Pool after draw 174 (2): #80 fuzz-engine-diff musig2
   cross-field, #95 rocksdb-brute bulk-ops.
   RE-RANK draw 173 (4-cell pool): raw=17651119340599297244,
   masked 8427747303744521436 -> idx 0 -> STALE (#55 exhausted;
   pool repair). Redraw (3-cell): raw=2606098433264139421
   (63-bit) -> idx 1 -> #95 c6 (kill-during-pressure-flush at
   -dbcache=4 scale: 3/3 zero-corruption; DISMISSED).
   Pool after draw 173 (2): #80 fuzz-engine-diff musig2
   cross-field, #78 translation-validation single-TU; #95
   rocksdb-brute bulk-ops harvested as next-cycle candidate.
   RE-RANK draw 172 (6-cell re-harvested pool): raw=
   184429387142081413 (63-bit) -> idx 1 -> STALE (#108 complete;
   pool repair). Redraw (5-cell): raw=14500292587252405485, masked
   5276920550397629677 -> idx 2 -> #79 c3 (qa-assets selective
   import @918cdd3: 7,846 seeds single-pass clean; DISMISSED).
   Pool after draw 172 (4): alt-impl-diff btcd/rust-bitcoin
   tx-ser, fuzz-engine-diff musig2 cross-field, database-semantics
   large-UTXO, translation-validation single-TU.
   RE-RANK draw 171 (1-cell pool): raw=3220867768142961582
   (63-bit) -> idx 0 -> #105 c2 (dbwrapper failed-construction
   leak family: fixed in HEAD, sibling sweep 0, upstream master
   re-fetched still missing dtor; DISMISSED-for-siblings).
   Pool EMPTY — re-harvest journal "Limitations / queue" tails
   before draw 172.
   RE-RANK draw 170 (3-cell pool): raw=16461226049798957217,
   masked 7237854012944181409 -> idx 1 -> #10 VerifyCryptedKeys —
   DESCOPED re-confirmed (zero non-wallet refs, draw-104a ruling
   stands). Redraw (2-cell): raw=6266662857617590820 (63-bit) ->
   idx 0 -> #47 c4 (shared-kernel-lib consumer: .so install via
   component, full ldd -r closure, CONSUMER-OK; DISMISSED).
   Campaign #47 COMPLETE. #10 REMOVED from pool (descoped).
   Pool after draw 170 (1): #105 capability autopsy; then
   re-harvest journal "Limitations / queue" tails.
   RE-RANK draw 165 (8-cell pool): raw=10771425690861073957,
   masked 1548053654006298149 -> idx 5 -> #100 c4 (feefrac
   backends exact: 0 mul diffs, 0/3200 div diffs + UBSan clean;
   DISMISSED). Campaign #100 COMPLETE.
   Pool after draw 165 (7): #47 shared-lib consumer, #36
   functional-under-clang, #10 VerifyCryptedKeys, #109 short-id
   collision, #40 CheckTransaction differential fuzz, #105
   capability autopsy, #67 backwards-compat.
   RE-RANK draw 164 (9-cell pool): raw=10692509565713273262,
   masked 1469137528858497454 -> idx 1 -> #93 c2 (fs/permission
   fault family: unreadable blk = loud abort; unwritable blocksdir
   = loud write-fail; full recovery; CAP_DAC_OVERRIDE trap;
   DISMISSED). Campaign #93 COMPLETE.
   Pool after draw 164 (8): #47 shared-lib consumer, #36
   functional-under-clang, #10 VerifyCryptedKeys, #109 short-id
   collision, #40 CheckTransaction differential fuzz, #100 feefrac
   sinks, #105 capability autopsy, #67 backwards-compat.
   RE-RANK draw 163 (10-cell re-harvested pool): raw=
   15618775271313699753, masked 6395403234458923945 -> idx 5 ->
   #59 c3 (qa-assets corpus clone commit-pinned: c1's loud-failure
   reasoning covered injection only, weakening was silent; FIXED
   and live-verified; CONFIRMED). Campaign #59 COMPLETE.
   Pool after draw 163 (9): #47 shared-lib consumer, #93 fault
   hooks, #36 functional-under-clang, #10 VerifyCryptedKeys, #109
   short-id collision, #40 CheckTransaction differential fuzz,
   #100 feefrac sinks, #105 capability autopsy, #67
   backwards-compat.
   RE-RANK draw 161 (2-cell pool): raw=8672416563413024783
   (already 63-bit) -> idx 1 -> #50 c14 (taproot sighash
   size-class gates 6/6 + musig2 psig parse-reject; RPC sighash-
   parameter confounder recorded; DISMISSED). Campaign #50
   COMPLETE. Pool after draw 161 (1): #35 NONNEGATIVE_SIGNED
   write semantics; then re-harvest.
   RE-RANK draw 160 (3-cell pool): raw=11628138699865233707,
   masked 2404766663010457899 -> idx 0 -> #74 c6 (pruning-mode IO:
   disk freed byte-exact (-129KB, files 7->4), RSS flat +0,
   boundary exact; DISMISSED). Campaign #74 COMPLETE.
   Pool after draw 160 (2): #35 NONNEGATIVE_SIGNED write
   semantics, #50 Taproot/MuSig2 gates.
   RE-RANK draw 158 (4-cell pool): raw=15981968078687135964,
   masked 6758596041832360156 -> idx 0 -> #81 c5 (Wycheproof
   AES-CBC-PKCS5: 72/72 after harness-contract fixes; INDEPENDENT
   CONFIRM of #107 c2 — cross-journal duplicate caught pre-archive
   (harvest note "Wycheproof unclaimed" was wrong); DISMISSED).
   Campaign #81 EXHAUSTED. Pool after draw 158 (3): #74 pruning-
   mode IO, #35 NONNEGATIVE_SIGNED write semantics, #50 Taproot/
   MuSig2 gates.
   RE-RANK draw 156 (6-cell pool): raw=15079885369332575445,
   masked 5856513332477799637 -> idx 1 -> #36 c5 (TSan
   concurrency subset: 6 suites green, 0 data races; 2 warnings =
   sync_tests' intentional inversions; capnp/kj TSan link failure
   noted, IPC off; DISMISSED).
   Pool after draw 156 (5): #81 Wycheproof, #74 pruning-mode IO,
   #35 NONNEGATIVE_SIGNED write semantics, #50 Taproot/MuSig2
   gates, #9 per-seed profiling.
   RE-RANK draw 157 (5-cell pool): raw=6943923678126847234
   (already 63-bit) -> idx 4 -> #79 c2 (per-seed profile: FLAT
   max/median 1.05x; ~99.5% per-invocation cost = startup,
   ~4ms/seed in-process; oversized-seed hypothesis REFUTED;
   shorthand said #9, campaign is #79). Pool after draw 157 (4):
   #81 Wycheproof, #74 pruning-mode IO, #35 NONNEGATIVE_SIGNED
   write semantics, #50 Taproot/MuSig2 gates.
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
Updated after every rotation. PROGRAM STATE 2026-08-03 v2
(post-flood): the 2026-08-03 branch flood (498 new l0rinc
branches, 865->1363) is FULLY TRIAGED — goal-111 manifest: 52
src-touching branches, all dispositions recorded. Session totals:
9 adoptions with failing-before/passing-after pairs (F25 txdb
cursor, F26 xor.dat, F27 snapshot write, F28 mempoolexpiry, F29
clustersize, F30 headers clock-lag, F31 i2p key, F32 tor key,
F33 txospender stale tip 🟠, F34 descriptor INT32_MAX, F35
blockfilter reorg 🟠 — F25-F35 span), 2 test adoptions (goal10,
goal6-merkle covered-ahead), 3 dismissals with evidence
(goal92-abi, P3 bad_alloc trio, goal7-getblocktxn), 3 covered-
ahead (goal26, goal98, goal43-reindex), dup-txid settlement
externally corroborated (author CI red on own test flow; our fix
identical). LevelDB trio (125/126/127) CONFORM-dismissed with
harnesses preserved in agent-journal/artifacts/ (12 files +
replay README). URGENT at 10 (F33+F35 🟠, F30, rest ✅; 🔴
UTXO-scan upstream-only watch). Open external: upstream master
dcc2ed52b8 (fuzz-only advance), dup-txid CI (2 NetBSD failures =
author's own test), watch PRs 35744/35859/35818/35620/35654
(open), qa-assets pin 918cdd3, host tooling (lld, clang-tidy,
Sparrow repo). Regression #8 full suite in flight; #8b kernel
GREEN.
PROGRAM STATE 2026-08-03 FINAL
(blocked 3x zero-delta after resume, cycles 292-294):
all internal work complete and re-verified — 26-PR sweep fully
covered (6 adopted incl. dup-txid skip fix, 2 covered-ahead, 17
assess-only, 5 standing-watch); strong-random guards adopted;
consolidated regression #3 all-green (test_bitcoin + test_kernel
+ 8 functional instances on the final lineage). Open external
items only: dup-txid branch CI (queued, 0 failures), upstream
master 556988790a (static), qa-assets pin 918cdd3 (current).
Resume on: any upstream advance, new/force-updated author
branch, qa-assets moving past the pin, or host tooling (lld,
clang-tidy, more disk, second host).
PROGRAM STATE 2026-08-03 (resumed
suspicion-mining protocol): the 26-PR author sweep is FULLY COVERED —
6 adopted with failing-before/passing-after pairs (F19 flush-failure,
F20 log injection, F21 empty-HMAC UB, F22 empty-headers stall, F23 CI
pinning, merkle doc+test), 2 covered-ahead (35797 PSBT via 53506a51e9,
35662 txdata via 67239a4a19), 17 assess-only (no defect/design/feature/
draft), 5 standing-watch (35744/35859/35818/35620/35654). The strong-
random-contracts branch was adopted on arrival (#104 RNG contracts
pinned behaviorally + statically). Catalog/corpus programs complete
(112,382 seeds green, 6/6 mutation clean, consolidated regression green
on the integrated lineage incl. all adopted fixes).
PROGRAM STATE 2026-08-02 (session
summary): the full 110-goal catalog has been cycled; the corpus
import program is COMPLETE (112,382 upstream qa-assets seeds
validated through the fork's hardened build across 8 families,
zero crashes/Assume aborts, plus 3 mutation campaigns clean:
ephemeral_package_eval ft 110,667, txgraph 420,878 runs
scratch->4,586-unit corpus, txorphanage_sim 67,241 runs);
consolidated regression sweep green (test_bitcoin + test_kernel
+ 7-instance functional subset on the integrated lineage);
watches (#42/#65/#60) moved to merge-event-triggered cadence
after 3 consecutive zero-deltas. Open items: URGENT.md (10
entries: 1 🔴 UTXO-scan race upstream-only, 1 🟡 txgraph
retained capacity with author fix in flight, rest ✅/fixed);
offerable-upstream set F13/F14/F17 + hygiene accumulate;
adoption watch on l0rinc/txgraph-retained-entry-usage.
## Handoff
Updated after every rotation. Campaign-DONE/QC/EXHAUSTED/deferred
(mechanically rebuilt from table rows 2026-07-28): 3, 5, 6(EXHAUSTED), 8, 11, 12,
13, 14, 15, 18(QUEUE-COMPLETE), 19(EXHAUSTED), 108(COMPLETE 2026-08-01), 73(COMPLETE 2026-08-01), 45(COMPLETE 2026-08-01), 81(EXHAUSTED 2026-08-01), 74(COMPLETE 2026-08-01), 50(COMPLETE 2026-08-01), 35(COMPLETE 2026-08-01), 59(COMPLETE 2026-08-01), 93(COMPLETE 2026-08-01), 100(COMPLETE 2026-08-01), 40(COMPLETE 2026-08-01), 109(COMPLETE 2026-08-01), 67(COMPLETE 2026-08-01), 36(COMPLETE 2026-08-01), 47(COMPLETE 2026-08-01), 78(COMPLETE 2026-08-02), 95(COMPLETE 2026-08-02), 80(COMPLETE 2026-08-02), 24(COMPLETE per its own journal: 6 cycles, 2026-07-30), 101(EXHAUSTED 2026-08-02), 34(EXHAUSTED 2026-08-02), 57(EXHAUSTED 2026-08-02), 69(EXHAUSTED 2026-08-02), 7(EXHAUSTED 2026-08-02), 20, 26, 27, 33, 41(EXHAUSTED 2026-07-31), 52, 55(EXHAUSTED 2026-07-31), 71(EXHAUSTED 2026-07-31),
56, 62, 72(deferred), 77(deferred), 82, 83, 84, 85, 86, 87, 88, 89,
96, 97, 98.
Cycles done (random-pool state): 41(c1,c2,c3,c4,c5,c6), 0(c1,c2), 1(c1,c2,c3,c4), 4(c1,c2), 6(c1,c2,c3),7(c1), 2(c1,c2,c3), 9(c1,c2,c3,c4,c5,c6), 10(c1,c2,c3,c4), 7(c1,c2,c3,c4), 13(c1,c2), 16(c1,c2), 17(c1,c2,c3), 21(c1,c2,c3,c4), 22(c1,c2), 89(c1,c2,c3,c4,c5), 108(c1,c2,c3,c4,c5,c6),
28(c1,c2), 29(c1,c2), 30(c1,c2,c3), 31(c1,c2,c3,c4), 34(c1,c2,c3,c4,c5), 35(c1,c2,c3,c4,c5,c6,c7), 36(c1,c2,c3,c4,c5,c6), 23(c1,c2,c3,c4), 25(c1,c2,c3),
37(c1), 39(c1,c2), 40(c1,c2,c3), 42(c1,c2,c3,c4,c5), 44(c1,c2), 46(c1,c2), 54(c1), 51(c1,c2,c3), 52(c1,c2), 21(c1,c2,c3,c4), 43(c1,c2,c3), 45(c1,c2,c3,c4), 47(c1,c2,c3,c4), 48(c1,c2), 49(c1,c2,c3,c4,c5,c6,c7,c8,c9), 50(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14), 53(c1), 55(c1,c2,c3,c4), 57(c1,c2,c3,c4), 58(c1,c2,c3), 59(c1,c2,c3), 60(c1,c2,c3,c4,c5,c6,c7,c8), 24(c1,c2,c3,c4,c5,c6),
61(c1,c2,c3), 63(c1,c2,c3,c4,c5), 64(c1), 65(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11), 66(c1,c2), 67(c1,c2,c3,c4), 68(c1,c2,c3), 69(c1,c2,c3,c4), 70(c1), 71(c1,c2,c3,c4,c5), 73(c1,c2,c3,c4,c5), 74(c1,c2,c3,c4,c5,c6), 75(c1,c2,c3,c4),
76(c1,c2,c3,c4), 80(c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11), 81(c1,c2,c3,c4,c5), 90(c1,c2), 91(c1,c2,c3,c4), 92(c1,c2), 93(c1,c2), 94(c1,c2), 95(c1,c2,c3,c4,c5), 99(c1), 100(c1,c2,c3,c4), 101(c1,c2,c3), 102(c1,c2), 103(c1), 104(c1,c2,c3), 105(c1,c2), 106(c1,c2,c3), 107(c1,c2), 108(c1), 109(c1,c2,c3).
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
