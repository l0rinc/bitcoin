# URGENT — live findings (max 10, severity-ordered)

Updated after every cycle and immediately after any verdict/severity
change. Legend: 🚨 Critical | 🔴 High | 🟠 Medium/correctness-data-loss
risk | 🟡 promising-unverified | ⚪ blocked/inconclusive | ✅ fixed +
independently verified.

## ✅ PSBT fuzz harness truncation gate (fixed d086164661, #101 c1)
- Mechanism: psbt fuzz target fed ConsumeRandomLengthString() into
  DecodeRawPSBT; the backslash-escape convention truncated any document
  containing 0x5c+non-0x5c, so whole valid PSBTs never decoded and the
  per-input/output half of the target was unreachable.
- Evidence: hybrid consumption (ConsumeBool-selected
  RandomLength/RemainingBytes) — 9 starved functions (483 edges)
  covered AFTER, all 9 UNCOVERED again in the old-corpus CONTROL,
  isolated RPC-verified 136-byte seed 528 -> 2857 edges. Inlining
  artifacts separated per #9 c2 discipline.
- Branch/commit: audit/public-characterization @ d086164661 (fix +
  journal public-characterization-fix.md). Archived on
  agent/all-findings @ 6890fc43f0 (+bookkeeping 199fc03af4).
- Next: grep other fuzz targets for the same single-mode document
  pattern; #50 c2 SigningProvider-bearing target for SignPSBTInput.

## ✅ psbt fuzz target missing ECC init — SEGV on first valid key (fixed, #50 c4)
- Mechanism: the #50 c2 signing pass (08590b364d) made the psbt fuzz
  target do ECC work but the target had no .init;
  secp256k1_context_sign stays null and the FIRST valid fuzz key
  crashes in FillableSigningProvider::AddKey -> CKey::GetPubKey ->
  secp256k1_ec_pubkey_create(NULL, ...).
- Evidence: failing-before (pre-fix HEAD target, ASan+UBSan):
  key.cpp:198 null-pointer UBSan + ASan SEGV on 0x0 READ,
  first-invalid frame secp256k1_ecmult_gen_context_is_built <-
  ec_pubkey_create <- CKey::GetPubKey (key.cpp:198) <- AddKey
  (signingprovider.h:323) <- psbt_fuzz_target. Passing-after: fixed
  target clean on both correlated seeds + 500-run corpus (97 s);
  PSBTv2 correlated seed drives SignPSBTInput to OK +
  PSBTInputSignedAndVerified=1 (final_script_sig=106 B) with the
  fuzz-provided key; independent RPC verifier (walletprocesspsbt /
  finalizepsbt complete=True).
- Reachability: decodable doc + >=64 front key bytes — only via the
  hybrid whole-doc mode, hence invisible to truncation-mode corpora.
  Test-infra only, fork-local.
- Branch/commit: audit/introspector-blockers-c4 (this cycle);
  journal fuzz-introspector-blockers.md c4. Archive: agent/all-findings
  pick this cycle.
- Next: multi-input multi-key correlated docs; taproot/witness
  variants; qa-assets-style corpus-dir import.

## ✅ Rolling-bloom reset-per-tip-change CPU storm (fixed c8f53e58d9)
- Mechanism: TxDownloadManagerImpl::ActiveTipChange reset two ~863 KiB
  rolling bloom filters per accepted block once the regtest IBD latch
  (minwork=0 + fresh timestamps) flipped is_ibd=false; 40.6% of P2P
  IBD CPU in memset.
- Strongest evidence: perf attribution (42.05% __memset_zva64 from
  CRollingBloomFilter::reset), before/after user CPU 3.2s -> 1.35s
  (-58%), identical final tip hash, bloom_tests green.
- Branch/commit: audit/full-sync-ibd-c2 @ c8f53e58d9; journal
  full-sync-ibd-profile.md c2. Integration: in ledger lineage
  (ancestor of current tip).
- Next: none locally; optionally upstreamable (trivial, provably
  identical post-state, re-salt preserved).

## ⚪ qrencode depends primary URL dead (404), fallback covers
- Mechanism: https://fukuchi.org/works/qrencode/ is gone; depends
  .mk pins its tarball sha256 (upstream-master-identical file).
- Evidence: curl 404 on primary; bitcoincore.org/depends-sources
  fallback serves byte-exact pinned tarball (da448ed4f5...71e8e).
  expat 2.7.3 primary verified matching too (#76 c2).
- Branch: journal-only, reproducible-builds.md c2.
- Next: upstream-watch — if bitcoin/bitcoin updates qrencode.mk, take
  theirs. No local divergence warranted (hash is the trust anchor).

## ✅ -limitclustercount=0 accepted at startup (fixed 5e0a80ade5, #52 c2)
- Mechanism: only the upper bound of -limitclustercount was
  validated; 0 slipped to TxGraph's Assume(max_cluster_count >= 1)
  — startup abort in assert builds, or silent rejection of EVERY
  mempool transaction in release builds (total_count >= 1 > 0).
- Evidence: failing-before (regtest node starts and runs with 0,
  Release) + code chain (mempool_args.cpp:35/:110, txmempool.cpp
  :222, txgraph.cpp:699-700/:2143); passing-after: startup fails
  with 'limitclustercount must be at least 1', boundary cases
  (0 and 65) in mempool_cluster.py, full suite green.
- Severity: small (DEBUG_TEST option, config-triggered) but
  byte-identical gap present in upstream master (7dea464d6b).
- Branch/commit: audit/assertion-invariant-c2 @ 5e0a80ade5;
  journal assertion-invariant-audit.md c2. Archive pick this cycle.
- Next: offer upstream (one-line validation, mirrors the existing
  upper-bound error).

## ✅ CompactSize exhaustive boundary battery (oracle delivered 8b7d8ac878)
- Mechanism: consensus wire-length primitive canonicality; existing
  tests sampled 8 non-canonical strings — no exhaustive or
  length-class coverage.
- Evidence: boundary round-trips all four width classes + exhaustive
  253-form rejection (253/253) + MAX_SIZE both sides; injected
  mutation (serialize.h:345 <253 -> <252) killed; restore green.
- Branch/commits: audit/property-oracle @ 8b7d8ac878, journal
  e9020948c7; archived on agent/all-findings (content-verified
  2026-07-29).
- Next: 254/255-form sampling widened; independent-deserializer
  differential (functional framework) in a c2.

## ✅ load_wallet fuzz harness delivered; bring-up crash = harness bug (fd74c4a7c2)
- Mechanism: WalletBatch::LoadWallet (wallet-db record application)
  had zero fuzz coverage; new harness pre-seeds in-memory SQLite with
  fuzzed FLAGS/VERSION/NAME/descriptor/TX/unknown records and asserts
  DBErrors classification + FLAGS/NAME round-trips.
- Evidence: bring-up crash traced (seed /tmp/lw_crash_flags_seed) to
  the harness's own SetWalletFlag persisting over the seeded FLAGS
  record — production correct at every step; 5000 runs clean (~51/s).
- Branch/commits: audit/fuzz-gaps-c2 @ fd74c4a7c2 (+ rescan harness
  537e819eb0, 04254c1da7); archive agent/all-findings @ a8f5e2b503,
  journal 90416bd242.
- Next: widen record classes (crypted keys, ACTIVE*SPK, BESTBLOCK);
  descriptor/TX semantic apply-vs-reject oracle.

## ✅ ReadVarInt overflow-rejection oracle (delivered 083afedbf1)
- Mechanism: both ReadVarInt overflow guards were test-blind —
  deletion mutants survived the full suite (undo-data VARINT parsing
  is consensus-adjacent via CTxUndo).
- Evidence: mutation sweep M1 killed / M2 M3 survived; battery added
  (both guards x uint8/16/32/64 + legal-max controls + signed mode);
  M2/M3 then killed; repaired green.
- Branch/commits: audit/mutation-testing @ 083afedbf1, journal
  a2c5cf8935; archive c0bd287a31 + 726959bae1.
- Next: WriteVarInt per-line sweep; CTxUndo consumer-side fuzzed
  VARINT fields.

## 🔴 UTXO-scan/resize race — upstream master (fixed in-tree e049f064e1)
- Mechanism: gettxoutsetinfo/scantxoutset create a LevelDB cursor
  under cs_main then scan unlocked; assumeutxo ResizeCache resets
  m_db mid-scan → iterator use-after-free → LevelDB abort
  (VersionSet dtor assert).
- Public trigger: authorized/local RPC during snapshot activation or
  cache rebalance — availability kill, not consensus.
- Evidence: upstream master raw fetch today has no cursor lock; the
  fork's clean-master reproducer aborted (commit message e049f064e1);
  tree holds a UniqueLock for cursor lifetime — verified in txdb.cpp
  :231-262.
- Branch/commits: fix in lineage (e049f064e1 + unit test + resize
  fuzz target); journal resource-exhaustion-variants.md c2;
  archived on agent/all-findings (hash-present e049f064e1).
- Next: watch upstream 35744 (open, head 38b84769608a; DrahtBot
  flagged a TSan pthread_cond_destroy race in its shared-lock
  rework 2026-07-28 — that shape cannot exist in-tree: zero
  condvar/shared_mutex in txdb, resize-cursor test green at HEAD
  per #42 c1). Nothing to do locally.

## ✅ dbwrapper failed-construction leak (fixed 73a6798206, #13 c2)
- Mechanism: CDBWrapper ctor can throw (LevelDB open failure) after
  options members block_cache/filter_policy/info_log were allocated;
  LevelDBContext had no destructor, leaking them per failed open.
- Evidence: failing-before LSan probe — 79,800 B / 361 allocs over
  19 failed opens (stacks NewLRUCache dbwrapper.cpp:142, filter_policy
  :144, info_log :146); passing-after 20 failed opens LSan-silent;
  dbwrapper_tests green. Upstream master has the same missing
  destructor (offerable).
- Branch/commit: audit/raii-resource-leaks-c2 @ 4d8ae03172 (fix
  73a6798206); journal raii-resource-leaks.md c2. Archived on
  agent/all-findings @ d87da3929e (fix 461c21cbfa).
- Next: offer upstream (small RAII fix, mirrors existing option
  ownership comments); audit other throwing ctors holding raw
  members.

---
Recently removed from this list (dismissed/closed): Fee-estimator
zero-state per-block waste (fixed 675011ba86, verified, no local
follow-ups); LockPoints
bound comment (fixed b1c267c9f1, no follow-ups); l0rinc CheckBlock
dup-input equivalence (PROVEN, #40 c1 — resolved, no local action);
clang-18
differential (green, #36 c2); net_processing sancov 0/23 alarm
(inlining artifact, #9 c2); TODO sweep (0/56 defects, #0 c2);
BIP173/350 vectors (no drift, #81 c2); install manifest parity
(exact, #47 c2); BIP324/RFC8439 vectors (byte-exact, #81 c1);
v28.2/v0.20.1 ↔ HEAD P2P differential (no divergence, #67 c1);
write-amplification ~2x bounded by design (#24 c1).
