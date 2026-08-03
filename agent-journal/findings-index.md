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

F13 | -limitclustercount=0 accepted at startup validation |
config/mempool | lower bound unvalidated -> TxGraph Assume abort
(assert builds) or silent all-tx mempool rejection (release) |
low (DEBUG_TEST option) | CONFIRMED+FIXED | 5e0a80ade5 (#52 c2):
failing-before (node starts with 0) / passing-after ('must be at
least 1' InitError); boundary cases 0 and 65 in
mempool_cluster.py, suite green; upstream-identical gap
(7dea464d6b), offerable upstream | audit/assertion-invariant-c2,
archive pick this cycle | done; upstream offer.

F14 | dbwrapper failed-construction leak | storage/leveldb |
CDBWrapper ctor throws after allocating options-owned members;
LevelDBContext had no destructor -> per-failed-open leak of
block_cache/filter_policy/info_log | low (needs repeated failed
DB opens; no consensus/remote impact) | CONFIRMED+FIXED |
73a6798206 (#13 c2): failing-before LSan probe 79,800 B / 361
allocs over 19 failed opens (stacks NewLRUCache dbwrapper.cpp:142,
filter_policy :144, info_log :146); passing-after 20 failed opens
LSan-silent, dbwrapper_tests green; upstream master has the same
missing destructor (offerable) | audit/raii-resource-leaks-c2 @
4d8ae03172, archive agent/all-findings @ d87da3929e (fix
461c21cbfa) | done; upstream offer.

F15 | txgraph.cpp stale comment set (4 sites) | docs/comment-code
contract | :706 named nonexistent types (MiningOrder/EvictionOrder);
:775/:786 referenced deleted Ref move assignment; :2261-2262
unconditional chunk-connectedness claim dropped the saturation
condition proven necessary by 3ae78dbd25 | trivial (documentation;
no behavioral impact) | CONFIRMED+FIXED | independent re-verification
of scout flags (tree-wide greps, sole-caller checks :3792/:3783,
txgraph.h:247, GetChunking collapse :1142-1153); 106-claim sweep:
48 sanity-enforced (70,024-run fuzz enforcement clean), 52
prose-verified, T-block resolved by Trim caller audit
(txmempool.cpp:110-126 single-direction deps) | #1 c4 on
audit/comment-code-contract-c4 (this cycle) | done; same
comment-drift family as F1 (LockPoints).

## F17: btck_chainstate_manager_destroy null deref (FFI misuse) — FIXED 2026-08-02
- Mechanism: 22 btck_*_destroy functions, 21 delete-only (null-safe
  free() convention); btck_chainstate_manager_destroy flushed
  chainstates through btck_ChainstateManager::get(chainman) BEFORE the
  delete — destroy(nullptr) = null dereference. FFI error-path cleanup
  after a failed create hits it directly.
- Evidence: #16 c4 — HEAD had the unguarded deref (bitcoinkernel.cpp:
  1192-1202); cherry-picked fix (union conflict + 2 labeled repairs);
  test_kernel btck_destroy_null_tests green (crashes pre-fix), full
  suite green. Upstream master @556988790a still unguarded (:1126-1130).
- Fix: 55f1fa334f -> in-lineage as 81b13d9e9f (+ repairs d004ac04d2,
  6e9753d608); journal #16 c4. Offerable upstream.
- Dedup note: distinct from F14 (dbwrapper ctor leak) — same null/RAII
  family, different layer (FFI destroy vs DB construction).

## F18: qa-assets fuzz-corpus clone unpinned (supply chain, F9 sibling) — FIXED 2026-08-01
- Mechanism: ci/test/03_test_script.sh cloned bitcoin-core/qa-assets
  with --depth=1 at floating HEAD — a compromised/weakened upstream
  corpus would silently reduce fuzz coverage (c1's 'fails loudly'
  covered only corruption injection, not weakening).
- Evidence: #59 c3 — fixed by fetching+detaching a reviewed commit
  (QA_ASSETS_COMMIT=918cdd36fec3...); a coverage-weakening change now
  requires a second reviewed commit touching the pin. Live-verified
  this session: the pin == upstream HEAD at import time (#79 c3).
- Fix: 3afdc24bc7 (in lineage). Dedup note: F9 family (supply-chain
  pin) but a DIFFERENT input (qa-assets vs script_assets) — tracked
  separately so a future third pin gap stays visible.

## F19: flush-failure write-through (durability marker advance) — FIXED 2026-08-02
- Mechanism: FlushChainstateBlockFile failure only logged (TODO at
  validation.cpp:2821-2822); block-index + coins writes proceeded and
  m_last_flushed_block advanced — a block recorded flushed while its
  block/undo data may not be durable; restart/wallet trust it.
- Evidence: PR-35714 boundary test FAILS 2 assertions pre-fix (marker
  advanced past injected file-open failure), green post-fix (exit
  EXIT_FAILURE, marker unchanged). Author's PR 35714 (open upstream).
- Fix: e1a337ee96 adopted as f90291ffb9 (return the flush error before
  the writes/marker advance); union-resolved test conflict.
- Dedup note: distinct from F14 (leak) and #93 c2 (loud-failure
  behavior) — this one was a SILENT marker-advance, now loud.

## F20: RPC method-name log injection + wallet-name control chars — FIXED 2026-08-02
- Mechanism: whitelist-rejection warnings logged the method name
  UNSANITIZED (httprpc.cpp:118/:147) — newline injection forges
  node-looking log lines; wallet names accepted control chars
  (paths/UIs/logs).
- Evidence: FAILING-BEFORE — characterize test fails expecting the
  injected payload 'getblock\nERROR: ConnectTip: ConnectBlock
  0000...deadbeef failed' (a forged consensus-error line);
  PASSING-AFTER — rpc_whitelist.py green, createwallet bad\nname
  rejected (-8), goodname created. Author PR 35833 (open).
- Fix: 9d5fb22f1d + 6ed8e2af39 + ed4eb51e9f adopted (sanitize method
  names in both rejection paths; reject control chars in new wallet
  names).
- Dedup note: extends the #30 family; the debug method= log was
  already sanitized (rpc/request.cpp:245-249) — the rejection paths
  were the missed twins.

## F21: empty-HMAC-key null memcpy (F3 empty-span family, crypto arm) — FIXED 2026-08-02
- Mechanism: CHMAC_SHA256/512 constructors memcpy'd key bytes
  unconditionally; empty key vector -> memcpy(nullptr, 0), UB by the
  nonnull contract (UBSan reports even at length 0). The in-tree fuzz
  target force-resized empty inputs to avoid it.
- Evidence: FAILING-BEFORE — UBSan first-invalid trace:
  hmac_sha256.cpp:16 + hmac_sha512.cpp:16 'null pointer passed as
  argument 2, which is declared to never be null'; PASSING-AFTER —
  std::copy version, probe silent, crypto_tests green (empty-key
  vectors cohabiting with our RFC-4231 case-5 docs).
- Fix: b80907909c adopted (std::copy for empty ranges + empty-key
  vectors; fuzz guards dropped) + a6b1b82f0d (eval_script same-family
  guard); conflicts union-resolved (case-5 comment kept, fork chunking
  calls kept unconditionally).
- Dedup note: same class as F3 (AutoFile empty-span fwrite UB) —
  null-at-size-0 in a C API; this is the crypto arm.

## F22: empty-headers sync-slot stall (IBD liveness) — FIXED 2026-08-03
- Mechanism: a sync peer answering valid empty headers keeps its
  initial-sync slot: empty input fails IsContinuationOfLowWorkHeadersSync
  (headersync.cpp:76-77), the timestamp is not cleared, and the slot
  release at net_processing.cpp:6300 fires only when a replacement
  exists — IBD stalls on a slot the peer never fills.
- Evidence: FAILING-BEFORE — the PR's new test fails at unfixed HEAD
  ('Test empty headers response during initial sync', AssertionError);
  PASSING-AFTER — full p2p_initial_headers_sync.py green (timeout +
  empty-slot + eviction cases). Author PR 35839 (open).
- Fix: 5-commit stack adopted (297c0f7ca7, 7de4fba5b8, 79aca0b97f,
  92a98ffb30, 6d10e8e193); ReleaseHeadersSyncSlot kept after stack-order
  analysis (S9 rule: verify helper survival to branch tip).
- Dedup note: liveness sibling of the CVE-2024-52922 stalling family
  (#49 c10 markers) — that was withholding; this is truthful-empty.

## F23: CI external inputs unpinned (pip/actions/images/tool binaries) — FIXED 2026-08-03
- Mechanism: CI installed test pip deps unpinned, pulled actions by
  floating tags, ran OCI images by tag, and downloaded lint tool
  binaries unverified — all mutable between runs without a
  repository change.
- Evidence: gap greps at HEAD (01_base_install.sh:61 unpinned pip,
  @v6/@v5 floating tags, FROM ...ubuntu:26.04 tag); adoption verified
  in-tree: --require-hashes + 190-hash lockfiles, actions@full-commits,
  images @sha256 digests, shellcheck/mlc sha256sum blocks; bash -n OK.
- Fix: PR 35754's 4 commits (5671b32614, 9bf68d70bc, 0f0eb35c8b,
  49cc4e8cab) adopted with one union resolution.
- Dedup note: completes the F9 (script_assets) / F18 (qa-assets)
  supply-chain family across all four remaining input classes.

## F24: disconnect-pool duplicate-txid abort (BIP30 reorg) — FIXED 2026-08-03
- Mechanism: DisconnectedBlockTransactions asserted txid uniqueness,
  but a BIP30 historical duplicate coinbase can be disconnected
  together on a stronger-genesis-fork reorg -> Assert abort.
- Evidence: adopted from the author's PR branch (a9a78f2907) +
  test-repair 89b150976d; archive a3ba4aff55; unit green in-tree.
  CROSS-CONFIRMED 2026-08-03 by the parallel campaign's independent
  implementation 4061d3763d — identical skip semantics (try_emplace,
  no queue append, no memory charge, first reverse-order occurrence).
- Severity: reorg-with-BIP30-duplicates availability (assert-only
  path in release? Assert aborts in all builds here); remote trigger
  requires crafting a stronger genesis fork containing both
  duplicates — narrow but real.
- Upstream: PR branch CI (l0rinc 8dfa501356) still queued, 0
  failures at last check; fork-scenario feature_block.py divergence
  settled as the author's open iteration (upstream-CI oracle).

## F25: txdb cursor valid over malformed first key (PR 35654) — FIXED 2026-08-03
- Mechanism: CCoinsViewDB::Cursor() cached the default DB_COIN tag
  when first-key decode failed -> Valid()/GetKey() reported a usable
  cursor over an undecodable key (Next() already invalidated it).
- Evidence: failing-before (cursor Valid/GetKey true over a
  one-byte 'C' malformed key) / passing-after (focused +
  coins_tests green); audit/adopt-txdb-cursor-firstkey 8481b1f27f;
  archive 35548eb2ba.
- Reachability: corrupt/foreign-written chainstate keyspace only;
  not network reachable. Severity Medium-low robustness.
- Upstream: PR 35654 open; lineage covered-ahead.

## F26: xor.dat short-write leaves unbootable datadir (goal93) — FIXED 2026-08-03
- Mechanism: InitBlocksdirXorKey created blocksdir/xor.dat and a
  short write threw before the fclose check, leaving a truncated
  key file; next startup read it as authoritative -> AutoFile::read:
  end of file -> unbootable until manual deletion.
- Evidence: deterministic one-shot LD_PRELOAD fwrite interposer:
  pre-fix 1-byte xor.dat left + restart EOF failure; post-fix file
  removed, restart regenerates 8-byte key, getblockcount=0, clean
  stop. Harness preserved in contributor-branch-radar journal.
  audit/adopt-xor-key-shortwrite 2110abf119; archive ebd42ea45e.
- Reachability: local IO fault on first key creation (disk
  full/quota/transient); loud failure, availability only. Same
  write-failure family as F19. Upstream master vulnerable.

## F27: snapshot base-blockhash write escapes as raw exception (goal38) — FIXED 2026-08-03
- Mechanism: WriteSnapshotBaseBlockhash left the 32-byte marker
  write unguarded; a short write threw ios_base::failure out of
  ActivateSnapshot past cleanup_bad_snapshot -> RPC generic -1 +
  orphaned chainstate_snapshot dir with a truncated marker.
- Evidence: LD_PRELOAD path-targeted one-shot short-write on the
  base_blockhash stream; canonical height-299 regtest snapshot
  (base 0c552ced == committed hash); failing-before RPC -1 +
  orphan; passing-after designed -32603 'could not write base
  blockhash' + orphan removed + clean retry. Harness + fixture
  recipe preserved in contributor-branch-radar journal.
  audit/adopt-snapshot-write-cleanup 3c9090b644; archive 86533108ab.
- Reachability: local IO fault during loadtxoutset; loud,
  availability only. Third arm of the F19/F26 write-failure family.
  Upstream vulnerable on write AND read arms (read arm covered
  in-tree by a146380c8e + goal10 test 07c8ce5392).

## F28: negative -mempoolexpiry empties mempool (goal43) — FIXED 2026-08-03
- Mechanism: -mempoolexpiry=-1 accepted at startup; negative
  std::chrono::hours makes LimitMempoolSize compare entry_times
  against now minus a negative duration -> every entry instantly
  expired on any trim.
- Evidence: failing-before MempoolExpiryOptionTest negative arm
  (ApplyArgsManOptions(-1) accepted); passing-after full
  mempool_tests green; second verifier = parallel daemon reproducer
  (tx evicted under -1, retained under 336h).
  audit/adopt-mempoolexpiry-negative 0e0b3d6576; archive b095724b20.
- Reachability: local config error only; policy/availability, no
  consensus impact. Same family as -limitclustercount=0 (5e0a80ade5).
  Upstream master vulnerable.

## F29: negative/overflowing -limitclustersize accepted (goal43 size arm) — FIXED 2026-08-03
- Mechanism: -limitclustersize KiB value ingested unvalidated;
  negative -> negative TxGraph cluster limit; astronomical ->
  int64 overflow at the *1'000 ingestion or *40 graph-limit
  multiply.
- Evidence: failing-before MempoolClusterLimitOptions (-1 and
  max_kvb+1 accepted, 2 BOOST failures); passing-after full
  mempool_tests green. Minimal pre-ingestion guard variant
  (parallel 108e3a118b refactors the helper; same behavior).
  audit/adopt-clustersize-validation a3253e6396; archive e9cb3e3f02.
- Reachability: local config error only; no consensus impact.
  Count arm covered in-tree already (mempool_args.cpp:115-120).

## F30: headers commitment cap wraps under lagging clock (goal56-future-mtp) — FIXED 2026-08-03
- Mechanism: HeadersSyncState assigned signed (possibly negative)
  elapsed seconds into uint64_t m_max_commitments; clock skew >2h
  backward (NTP correction, VM resume) wraps the presync memory
  cap to ~2^64; a syncing peer can stream header commitments past
  the intended per-peer bound (memory DoS, no consensus impact).
- Evidence: failing-before future_chain_start_mtp_bounds_commitments
  (FakeNodeClock lagging genesis-MTP: PRESYNC, expected FINAL);
  passing-after clamp-to-0 fast-fail, full headers suite green.
  audit/adopt-headers-clock-lag 35473f91b4; archive a5a73c53f2.
- Reachability: remote peer + local clock skew >2h; upstream
  556988790a vulnerable. Same surface as F22.

## F31: truncated I2P key never regenerated (goal93-i2p) — FIXED 2026-08-03
- Mechanism: WriteBinaryFile short-write leaves a truncated
  persistent I2P private key; the session treats it as an existing
  key and never retries generation — I2P unavailable until manual
  removal.
- Evidence: failing-before persistent_key_write_failure_is_not_reused
  (file persists + retry Connect fails); passing-after full
  i2p_tests green. audit/adopt-i2p-key-removal e976e68fc9;
  archive 3e5c7b1368. Write-failure family instance #5.

## F32: truncated onion key blocks service restart (goal93-tor) — FIXED 2026-08-03
- Mechanism: ADD_ONION succeeds but caching the private key fails;
  WriteBinaryFile leaves a truncated onion_v3_private_key that a
  later startup reuses, keeping the service down.
- Evidence: failing-before tor_private_key_write_failure_removes_path
  (key path as directory, portable write failure); passing-after
  full torcontrol_tests green. audit/adopt-tor-key-removal
  5cf00e1380; archive 3e5c7b1368. Write-failure family instance #6.

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

O8 | psbt fuzz signing section | PSBT signing machinery |
SignPSBTInput/UpdatePSBTOutput/PSBTInputSignedAndVerified
unreachable (no SigningProvider in target) | n/a (test infra) |
DELIVERED | psbt target signing pass: FillableSigningProvider +
PrecomputePSBTData + per-input sign/verify + per-output update;
iso seed chain 528 -> 2857 -> 3048 edges; 0 UNCOVERED_FUNC matches
for all three at 3000 runs, no crash | #50 c2 on
audit/introspector-blockers-c2 | /tmp/btc101_seed,
/tmp/btc101_iso | key-script-correlated seeds for complete=true
sign arms; PSBTv2 signing seeds.

O9 | psbt fuzz ECC init + v2 correlated seed | PSBT signing machinery
/ fuzz harness | missing .init -> null secp256k1_context_sign -> SEGV
on first valid key in AddKey->GetPubKey; reachable only via hybrid
whole-doc mode + >=64 key bytes | n/a (test infra) | CONFIRMED+FIXED |
failing-before: UBSan key.cpp:198 null arg + ASan SEGV 0x0 READ
(pre-fix target, /tmp/psbt_corr_seed); passing-after: fixed target
clean on v0+v2 seeds + 500-run corpus; v2 seed drives SignPSBTInput
OK + PSBTInputSignedAndVerified=1 (final_ss=106B) + independent RPC
verifier complete=True | #50 c4 on audit/introspector-blockers-c4,
archive pick this cycle | seeds /tmp/psbt_v2_corr_seed,
/tmp/psbt_corr_seed (+/tmp/psbt_c4_artifacts), WIF /tmp/corr_wif.txt;
c3 v0 seed layout corrected (single-terminator whole-mode merge ate
the keys) | multi-input multi-key docs; taproot/witness variants.

## Latent / upstream-context items (not local defects)
- L3 (2026-08-02): txgraph GetMainMemoryUsage under-charges retained
  Entry vector capacity after churn (Compact pop_back, no shrink):
  allocated memory sits outside DynamicMemoryUsage/-maxmempool after
  fill-then-drain. Mechanism verified (#65 c12), empirical ~3.3MB
  converging over 5x1600-tx rounds (#22 c4), pre-fix semantics run at
  HEAD (#23 c5). Author fix in flight (l0rinc/txgraph-retained-
  entry-usage, 475ab49da6); adopt-then-reprofile is the close-out.


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
- Negative results with replay value: 2024 remote-P2P advisory batch
  (52915 inv-buffer / 52916 low-diff headers / 52919 addrman nIdCount /
  52920 getdata spin) all mitigation-present on HEAD incl.
  fork-interaction diff analysis; BIP70/SOCKS/UPnP cells dispositioned
  absent/depends-only/far-past (#49 c3); net_processing sancov "0/23"
  (inlining artifact — replay must use per-line UNCOVERED_PC, #9 c2);
  clang -Wunneeded on G_ABORT_ON_FAILED_ASSUME helpers (by design,
  #36 c2); bloom ccache "uncacheable" (absolute -I key divergence,
  #76 c2).

## Resume points (highest first)
1. ~~Merge/re-cherry-pick F1, F2, F3, F7, F9~~ DONE 2026-07-29 (#66 c2).
2. #22 c3: ClearCurrent gating? (verifier said skip; low value).
3. #65 c3: rocksdb/leveldb knob branch batch.
4. ~~#9 c3: qa-assets selective import per target~~ DONE 2026-07-29
   (#9 c3: psbt corpus covers all but the key-requiring
   sign-complete arm; correlated seeds remain necessary).
5. ~~#24 c2: UTXO-GROWING chain (fan-out) write amplification~~ DONE
   2026-07-29 (#24 c2: linear ~1.8x ex-logs; no superlinear growth).
   Next cell if redrawn: pruning-mode undo retention/blk rotation.

## Replay log
- 2026-07-29: ancestry/content verification above (merge-base +
  grep per item). No code changes this cycle.
O10 | xswiftec_inv edge-vector C++ gate | BIP324 ellswift decode /
secp256k1 | oracle gap: CSV consumed in-tree only by the Python
framework self-test; production C never saw the edge vectors | n/a
(test infra) | DELIVERED, mutation-verified | bip324_tests
xswiftec_inv_edge_vectors: all 32 rows / 98 ok-case encodings
decode to the annotated X coordinate; exact-count guards (32 rows,
98 encodings) catch silent truncation; mutation (row1 case2 t-tail
flip) killed at the x-comparison, restore green; CSV byte-identical
to the framework copy (cmp-verified) | #108 c5 on
audit/adversarial-artifact-c5 (this cycle) | bad-case rejection
classes stay Python-side only (they assert xswiftec_inv None, not
expressible through the public decode path).
O11 | feestats m_all_zero dirty-decay battery | fee estimation /
TxConfirmStats | oracle gap: failAvg-only and Read-restored dirty
states had no behavioral guard (M2/M3 set-omission mutants survived
the pre-existing suite) | n/a (test infra) | DELIVERED,
mutation-verified | feestats_dirty_decay_contracts: failAvg-only
dirty state asserted (other avgs exactly zero) + decay^3 oracle via
Write/parse-back; restored-vs-live bit-identity after 3 shared
blocks; M2 -> 12 failures, M3 -> 6 failures, M1 already killed by
BlockPolicyEstimates, M4 (skip-site) intentionally unobserved
(perf-only) | #57 c4 on audit/local-reasoning-c4 (this cycle) |
skip-site perf regression would need the profile harness, not a
unit test.
O12 | SignPSBTInput programmatic MISSING_INPUTS arms | PSBT signing
machinery | oracle gap: gates 1-2 unreachable from byte-parsed
PSBTs (decoder pre-rejects, psbt.h:1583-1595 both versions); only
in-process construction reaches them | n/a (test infra) |
DELIVERED, mutation-verified | psbt_tests
signpsbtinput_missing_inputs_arms: 3 gates + control (106 B
final_script_sig, byte-exact vs c4 fuzz-side); gate-2-check
deletion turns case red at psbt_tests.cpp:439; restore green |
#50 c13 on audit/introspector-blockers-c13 (this cycle) |
taproot/MuSig2 sighash-class variants as the next cell.
O13 | btck by-value struct layout battery | kernel C ABI | oracle
gap: layout drift in the public by-value C structs would silently
break downstream ABI with no tripwire | n/a (test infra) |
DELIVERED, compile-time mutation-verified | test_kernel
btck_abi_layout_battery: sizeof + per-field offsetof static_asserts
for ValidationInterfaceCallbacks (48 B), NotificationInterfaceCallbacks
(72 B), LoggingOptions (20 B) + runtime echoes; field swap ->
static assertion failed at test_kernel.cpp:1498-1499; restore
green | #92 c2 on audit/abi-alignment-c2 (this cycle) | 32-bit row
only if such a target ships.

## F16: KDF iteration-count overflow (wallet unlock hang) — FIXED 2026-08-01
- Mechanism: CMasterKey::nDeriveIterations (unsigned int, unbounded on
  deserialize) narrows into BytesToKeySHA512AES's signed `int count`;
  rounds > INT_MAX -> negative count -> guard `!count` passes, loop
  `for(i=0; i != count-1; i++)` runs ~2^31 SHA-512 rounds (~2.4h at
  the 0.10s/25k-rounds rate) + signed-overflow UB. Crafted/corrupted
  wallet file -> every unlock attempt hangs (wallet-scope DoS, no
  key/consensus impact). Identical to upstream PR bitcoin#35859 (open).
- Evidence: mechanism probe /tmp/btc159_probe (rounds=0x80000000:
  guard_accepts=1, narrowed_count=-2147483648 — rc=0 defect present);
  passing-after wallet_crypto_tests/passphrase_rounds_limit (0,
  INT_MAX+1, UINT_MAX rejected; DEFAULT accepted; suite completes in
  ms, proving rejection-before-derivation). E2E failing-before is a
  ~2.4h hang by construction — mechanism-probe substitute recorded.
- Fix: audit/kdf-rounds-overflow (crypter.cpp: reject rounds > INT_MAX
  in SetKeyFromPassphrase + harden KDF guard !count -> count < 1;
  mirrors PR #35859 exactly).
- Severity: Low-Medium (wallet-scope availability via crafted wallet
  file; local/supply-chain delivery only).
