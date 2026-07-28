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

## ⚪ Fee-estimator UpdateMovingAverages per-block cost (unquantified)
- Mechanism: next visible bottleneck after the bloom fix — 21.7% of
  post-fix regtest IBD CPU on b-scheduler; runs per connected block.
- Evidence: perf report #22 c2 (pre/post fix profiles).
- Branch: journal-only, full-sync-ibd-profile.md c2 queue.
- Next: check IBD/latch gating for fee-estimator updates (same
  is_ibd-latch family as the bloom storm) in a future #22 cycle.

---
Recently removed from this list (dismissed/closed): clang-18
differential (green, #36 c2); net_processing sancov 0/23 alarm
(inlining artifact, #9 c2); TODO sweep (0/56 defects, #0 c2);
BIP173/350 vectors (no drift, #81 c2); install manifest parity
(exact, #47 c2); BIP324/RFC8439 vectors (byte-exact, #81 c1).
