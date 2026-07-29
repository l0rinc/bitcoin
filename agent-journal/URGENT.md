# URGENT — live findings (max 10, severity-ordered)

Updated after every cycle and immediately after any verdict/severity
change. Legend: 🚨 Critical | 🔴 High | 🟠 Medium/correctness-data-loss
risk | 🟡 promising-unverified | ⚪ blocked/inconclusive | ✅ fixed +
independently verified.

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

## 🟡 l0rinc CheckBlock dup-input optimization (1.85x claim) unadopted
- Mechanism: CVE-2018-17144 dup-prevout check via std::set for every
  tx incl. coinbases; branch restructures to skip coinbase/1-input,
  direct-compare 2-input, sorted-vector 3+.
- Evidence: branch commits f3cc8fd27d/c379975b5a/532176cd27 (bench
  335.9->181.9us); local tx_check.cpp:41-45 still set-based.
  Equivalence PLAUSIBLE (1-input null-check arm unverified).
- Branch: journal-only, contributor-branch-radar.md c2.
- Next: fork author's adoption decision (his own upstream work);
  verify the 1-input null-check placement before any adoption.

## ✅ LockPoints max_input_height bound comment (fixed b1c267c9f1)
- Mechanism: comment claimed "always less than tip height"; CPFP
  1-conf parents yield equality. GetAncestor safe either way.
- Evidence: code read (validation.cpp:247-259, chain.cpp:100 GetAncestor
  nullptr condition), CPFP reachability argument; comment-only fix.
- Branch/commit: audit/comment-contract @ b1c267c9f1; in lineage.
- Next: none.

## ✅ CompactSize exhaustive boundary battery (oracle delivered 8b7d8ac878)
- Mechanism: consensus wire-length primitive canonicality; existing
  tests sampled 8 non-canonical strings — no exhaustive or
  length-class coverage.
- Evidence: boundary round-trips all four width classes + exhaustive
  253-form rejection (253/253) + MAX_SIZE both sides; injected
  mutation (serialize.h:345 <253 -> <252) killed; restore green.
- Branch/commits: audit/property-oracle @ 8b7d8ac878, journal
  e9020948c7; archive: next cycle's pick onto agent/all-findings.
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

## ✅ Fee-estimator zero-state per-block waste (fixed 675011ba86)
- Mechanism: processBlock swept all estimator buckets every connected
  block with no IBD gate; with zero tracked state (all of IBD for a
  fresh node) decay-of-zeros is a bit-identical no-op — 20.4-21.7% of
  regtest IBD CPU.
- Evidence: independent verifier reproduction (20.43%/1279 samples);
  post-fix 0 samples in the same 5000-block IBD; user CPU 1.35s ->
  0.89s (-34%); client tip == server tip; feature_fee_estimation.py
  green. Skip keyed by explicit m_all_zero dirty flag (Record /
  removeTx-failAvg / Read) — the naive emptiness predicate was proven
  unsafe by the verifier (failAvg-without-firstRecordedHeight and
  Read() restore cases).
- Branch/commits: audit/loupe-pipeline @ 675011ba86; journal #63;
  archive: next pick onto agent/all-findings.
- Next: nothing local; candidate upstream perf note.

---
Recently removed from this list (dismissed/closed): clang-18
differential (green, #36 c2); net_processing sancov 0/23 alarm
(inlining artifact, #9 c2); TODO sweep (0/56 defects, #0 c2);
BIP173/350 vectors (no drift, #81 c2); install manifest parity
(exact, #47 c2); BIP324/RFC8439 vectors (byte-exact, #81 c1);
v28.2/v0.20.1 ↔ HEAD P2P differential (no divergence, #67 c1);
write-amplification ~2x bounded by design (#24 c1).
