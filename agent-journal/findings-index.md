# Findings index (durable)

Campaign #64 deliverable. Durable dedup/recurrence index of confirmed
findings, oracles, harnesses, and unresolved leads from the rotation.
Schema per item: id | title | trust boundary | bug shape | severity
(master-relative) | verdict | strongest evidence | branch@commit |
in-lineage? | regression artifact | next/resume.

Lineage note (measured 2026-07-29, HEAD 31c8af0f6b): the main ledger
lineage and agent/all-findings interleave (campaign branches were
forked from archive cherry-picks), so some side-branch commits are
present under COPY hashes. "in-lineage" below = content present in
HEAD's tree, with the carrying hash where known.

## Confirmed fixes (defects, with regression evidence)

F1 | prevector zero-fill read | core container | OOB/UB read in
zero-filled resize | low | CONFIRMED+FIXED | 138ef3c044 (#19 c1) |
audit/benchmark-integrity branch | NOT in lineage | unit test in
commit | merge or re-cherry-pick into main lineage.

F2 | merkleblock weak oracles (2 survivors) | p2p/consensus test |
test-oracle gap | low (test-only) | CONFIRMED+FIXED | 50e9d14750
(#28 c2) | audit/weak-test-oracles | NOT in lineage | mutation
battery in commit | same.

F3 | AutoFile empty-span fwrite UB | streams | UB (nonnull at size 0)
| trivial severity, fuzz-only trigger | CONFIRMED+FIXED | 22aa75a2eb
(#36 c1) + streams_tests regression | audit/cross-tool | NOT in
lineage | UBSan re-run zero reports | same.

F4 | rolling-bloom reset-per-tip CPU storm | p2p/net-processing |
perf (reset per block post-latch) | low, test-net IBD | CONFIRMED+
FIXED | c8f53e58d9 (#22 c2), -58% user, bloom_tests green | in
lineage (direct) | bloom_tests + profile table | done.

F5 | fee-estimator zero-state decay sweep | fees/validation-adjacent |
perf (per-block bucket sweep with zero state) | low, IBD | CONFIRMED+
FIXED | 675011ba86 (#63 c1), 20.4%->0 samples, -34% user,
feature_fee_estimation.py green | in lineage via archive copy
2e3c671150 | profile + functional | done.

F6 | LockPoints max_input_height bound comment | validation docs |
wrong stated invariant (< vs <=) | doc-only | CONFIRMED+FIXED |
b1c267c9f1 (#1 c1) | in lineage (direct) | comment-only | done.

F7 | View/Range lifetime doc gap (kernel wrapper) | FFI docs |
documented-C-contract weakened by omission | doc-only | CONFIRMED+
FIXED | 0a6c377ddb (#94 c1) | audit/bindings-ffi | NOT in lineage |
test_kernel green | merge or re-cherry-pick.

F8 | gettxoutsetinfo use_index for arbitrary blocks | RPC/coins |
wrong-data contract (index only valid at tip) | low | CONFIRMED+
FIXED | 9396f0b414 (#31 c3) | in lineage (content verified:
use_index present in rpc/blockchain.cpp) | functional coverage |
done.

F9 | CI script_assets sha256 pin | supply-chain/CI | unpinned
download | low | CONFIRMED+FIXED | 4124803dff (#59 c1) |
audit/supply-chain-gates | NOT in lineage | pin diff | merge or
re-cherry-pick.

## Oracles/harnesses delivered (test infrastructure, mutation-verified)

O1 | CompactSize exhaustive boundary + non-canonical battery |
serialization consensus boundary | oracle gap | n/a (test infra) |
DELIVERED | 8b7d8ac878 (#48 c1), 1/1 boundary mutant killed | in
lineage via archive copy 66917d5efc | battery itself | widen
254/255-form sampling.

O2 | ReadVarInt overflow-rejection battery | undo-format consensus
boundary | 2 test-blind guards (M2/M3 survivors) | n/a | DELIVERED |
083afedbf1 (#35 c1), both mutants then killed | in lineage (direct) |
battery itself | WriteVarInt per-line sweep.

O3 | kernel enum mapping static_assert tables | FFI/ABI | unguarded
pairing + numeric identity | n/a | DELIVERED | 073d543f26 (#94 c2),
reorder tripwire fires at :268 | in lineage (direct) | compile-time
table | ScriptVerify families if switches appear.

O4 | wallet_rescan mock-chain harness + tip-extension | wallet/rescan
| unreachable failure branches | n/a | DELIVERED | 537e819eb0 +
04254c1da7 (#10 c1 + #71 c1) | in lineage via archive 0d74e960f8 +
6942d0d269 | 5k-run clean | multi-reorg/progress values.

O5 | load_wallet record-application harness | wallet/persistence |
unfuzzed record-application seam | n/a | DELIVERED | fd74c4a7c2
(#10 c2), bring-up crash = harness-oracle bug (SetWalletFlag
persists over seeded FLAGS), 5k runs clean | in lineage via archive
a8f5e2b503 | crash seed /tmp/lw_crash_flags_seed preserved | widen
record classes.

## Latent / upstream-context items (not local defects)

L1 | CBloomFilter ctor div-by-zero/log(0) | bloom | math UB at
nElements=0/nFPRate=0 | test-only reachability | LATENT |
grep-verified: production uses copy ctor only; FILTERLOAD uses raw
deserialization + IsWithinSizeConstraints | journal
sink-reverse-reachability.md | watch l0rinc PR 35818 landing.

L2 | UTXO-scan/resize race | coins/leveldb | iterator
use-after-free on ResizeCache | availability (local/authorized RPC +
rebalance) | FIXED IN-TREE ahead of upstream | e049f064e1
(clean-master reproducer aborted in LevelDB) | upstream master
VERIFIED still racy today (raw fetch); tree has unique-lock cursor |
journal resource-exhaustion-variants.md c2 | watch upstream 35744.

L3 | qrencode depends primary URL 404 | supply-chain | dead primary
| none (fallback serves pinned bytes) | WATCH | journal
reproducible-builds.md c2 | upstream .mk identical; take upstream's
fix when it lands.

L4 | l0rinc CheckBlock dup-check optimization | consensus/perf |
std::set per-tx dup check | perf only | UNADOPTED SEED | branch
f3cc8fd27d lineage, 1.85x CheckBlockBench claim; equivalence
plausible (1-input null arm unverified) | journal
contributor-branch-radar.md c2 | fork author's decision.

## Recurrence/dedup notes

- The #66 problem is the governing dedup fact: confirmed fixes live
  on per-campaign side branches and only reach the ledger lineage
  when a later branch forks after them (or via archive copies). F1,
  F2, F3, F7, F9 are currently NOT in the ledger lineage's tree
  (verified by merge-base + content grep 2026-07-29); everything else
  is present. Merging or re-cherry-picking those five is the single
  highest-value hygiene action; do not re-fix them (they exist and
  are tested).
- Semantic duplicates deliberately not re-reported: bloom-empty-filter
  (CVE-2013-5700, guarded), CheckBlockIndex cost (#21/#22 — profiled,
  documented in code at validation.cpp:6280), bloom reset storm (F4
  vs #61 c1's txdownload fuzz oracles — same family, already covered).
- Negative results with replay value: net_processing sancov "0/23"
  (inlining artifact — replay must use per-line UNCOVERED_PC, #9 c2);
  clang -Wunneeded on G_ABORT_ON_FAILED_ASSUME helpers (by design,
  #36 c2); bloom ccache "uncacheable" (absolute -I key divergence,
  #76 c2).

## Resume points (highest first)
1. Merge/re-cherry-pick F1, F2, F3, F7, F9 into the main lineage.
2. #22 c3: ClearCurrent gating? (verifier said skip; low value).
3. #65 c3: rocksdb/leveldb knob branch batch.
4. #9 c3: qa-assets selective import per target.
5. #24 c2: UTXO-GROWING chain (fan-out) write amplification.

## Replay log
- 2026-07-29: ancestry/content verification above (merge-base +
  grep per item). No code changes this cycle.
