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
MERGED into lineage as e15c4025e5 (#66 c2 backport) | bench
compile verified | done.

F2 | merkleblock weak oracles (2 survivors) | p2p/consensus test |
test-oracle gap | low (test-only) | CONFIRMED+FIXED | 50e9d14750
(#28 c2) | MERGED as 84a3913096 (#66 c2) | merkleblock_tests
green in lineage | done.

F3 | AutoFile empty-span fwrite UB | streams | UB (nonnull at size 0)
| trivial severity, fuzz-only trigger | CONFIRMED+FIXED | 22aa75a2eb
(#36 c1) + streams_tests regression | MERGED as 508d9edfca
(#66 c2) | streams_tests green in lineage | done.

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
FIXED | 0a6c377ddb (#94 c1) | MERGED as 75c0616c24 (#66 c2) |
test_kernel green in lineage | done.

F8 | gettxoutsetinfo use_index for arbitrary blocks | RPC/coins |
wrong-data contract (index only valid at tip) | low | CONFIRMED+
FIXED | 9396f0b414 (#31 c3) | in lineage (content verified:
use_index present in rpc/blockchain.cpp) | functional coverage |
done.

F9 | CI script_assets sha256 pin | supply-chain/CI | unpinned
download | low | CONFIRMED+FIXED | 4124803dff (#59 c1) |
MERGED as b73b7c5d39 (#66 c2) | bash -n verified | done.

F10 | mempool hex-tx-array decode-loop exact duplicate | rpc |
duplicated logic (drift risk) | none (code-shape) |
CONFIRMED+FIXED | 4f97fbfe1e (#58 c1) | BACKPORTED as a7067512e8
(#58 c2) | mempool_accept.py + rpc_packages.py green at HEAD |
done.

F11 | base64-PSBT decode-or-throw exact duplicate x6 | rpc |
6 identical blocks (drift risk) | none (code-shape) |
CONFIRMED+FIXED | b1e55802f6 (#58 c2) | in lineage (direct) |
rpc_psbt.py green (all six RPCs, diagnostics asserted) | 7th copy
in wallet/rpc/spend.cpp:1637 queued.

F12 | reorg-repair cross-check ran ungated in production | mempool
perf | fork-added verification loop without abort-only gate |
~32% of ComplexMemPool on reorg-heavy paths | CONFIRMED+FIXED |
83f9989a68 (#23 c1) | BACKPORTED as 93c29aac55 (#23 c2) |
mempool_tests green; ComplexMemPool 268.5 -> 184.7 ms/op at HEAD
(stale-binary 278.5 control exposed + resolved) | done; queue:
EvictionProtection*/ConnectBlockAll profiles.

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

O6 | psbt fuzz hybrid document consumption | PSBT serialization |
harness truncation gate (ConsumeRandomLengthString backslash-escape)
starved whole valid documents | n/a (test infra) | DELIVERED |
d086164661 (#101 c1): 9 starvation-gated functions (483 edges)
covered AFTER, all 9 re-listed UNCOVERED in old-corpus CONTROL,
isolated 136-byte valid seed 528 -> 2857 edges | on
audit/public-characterization, archive pick this cycle | re-pointed
seeds /tmp/btc101_seed/*_whole (0x00 mode byte) | grep other targets
for single-mode document consumers; SigningProvider-bearing target
for SignPSBTInput family.

O7 | txoutproof negative-oracle battery | merkle-proof RPC
composition | thin negative oracle (1 variant + in-tree TODO) | n/a
(test infra) | DELIVERED | 9d1244e6b1 (#6 c2): 8 asserted mutations
(hash-bit flip, header mutation, nTx 0/max/+1, truncation,
unconsumed bits/hashes) all rejected; trailing-garbage acceptance
documented as intentional (shared with submitblock) | BACKPORTED
into lineage 4b8fa7c937 (#6 c3), rpc_txoutproof.py green at HEAD |
scratch probe /tmp/r6_txoutproof_probe.py | BIP37 serving side if
bloom work resumes.

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
std::set per-tx dup check | perf only | RESOLVED-EQUIVALENT | branch
f3cc8fd27d lineage, 1.85x CheckBlockBench claim; equivalence PROVEN
(#40 c1: prover+breaker agents converge, complete case partition,
1-input null arm vacuously safe via IsCoinBase definition) | journal
multi-agent-adjudication.md c1 | fork author's adoption decision.

## Recurrence/dedup notes

- The #66 problem is the governing dedup fact: confirmed fixes live
  on per-campaign side branches and only reach the ledger lineage
  when a later branch forks after them (or via archive copies). F1,
  F2, F3, F7, F9 were MERGED into the ledger lineage on 2026-07-29
  (#66 c2 backport: e15c4025e5, 84a3913096, 508d9edfca, 75c0616c24,
  b73b7c5d39; build+tests verified). All confirmed fixes are now in
  the lineage.
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
1. ~~Merge/re-cherry-pick F1, F2, F3, F7, F9~~ DONE 2026-07-29 (#66 c2).
2. #22 c3: ClearCurrent gating? (verifier said skip; low value).
3. #65 c3: rocksdb/leveldb knob branch batch.
4. #9 c3: qa-assets selective import per target.
5. #24 c2: UTXO-GROWING chain (fan-out) write amplification.

## Replay log
- 2026-07-29: ancestry/content verification above (merge-base +
  grep per item). No code changes this cycle.
