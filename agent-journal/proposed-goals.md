# Proposed goals — entropy queue

Distinct, evidence-backed candidates for promotion into
agent-journal/campaign-goals.md. Each: slug, profiles, provenance,
risk, semantic fingerprint, parent lineage, first runnable experiment.
Promotion rule: only with runnable evidence backing; merge/retire
overlap without deleting history.

## P1: persistence-write-failure sweep (restart-authoritative files)
- Profiles: Bitcoin Core / storage, validation, init.
- Provenance: F19 (flush marker, f90291ffb9), F26 (xor.dat,
  2110abf119), F27 (snapshot base_blockhash, 3c9090b644) — three
  confirmed instances of ONE bug shape in 24h, across two
  independent audit campaigns.
- Risk: 🟠 Medium — each instance is a local-availability defect
  (unbootable datadir / orphaned state / raw exception), found only
  by fault injection; normal tests never exercise IO-failure paths.
- Semantic fingerprint: AutoFile/WriteBinaryFile producer of a file
  that a later startup treats as authoritative; write or close
  failure leaves truncated content or escapes the designed error
  path.
- Parent lineage: goal93-xor-key, goal38-snapshot-cleanup-fix,
  PR 35714 (flush-failure), #41 persistence archaeology.
- First runnable experiment: enumerate producers — mempool.dat,
  peers.dat, anchors.dat, banlist.dat, fee_estimates.dat,
  settings.json, asmap.dat, i2p/tor keys (goal93-tor/i2p siblings
  already have parallel fixes ae32c111a3/32167c5c58 — assess for
  overlap FIRST). For each: LD_PRELOAD path-targeted one-shot
  short-write interposer (proven harness shape, snap_interpose.c),
  restart, classify: fail-loud-clean / fail-loud-stuck / silent.

## P2: deterministic-chain recipe oracle (assumeutxo hash drift tripwire)
- Profiles: Bitcoin Core / test infrastructure, chainparams.
- Provenance: cycle 296 — reproducing the canonical regtest chain
  failed silently twice (default setup_nodes mines +1 IBD-exit
  block; cache-tip 200 vs START_HEIGHT 199 confusion), costing
  ~40 min; only an exact base_hash == chainparams assertion proved
  recipe correctness. A stale committed assumeutxo hash would
  present identically.
- Risk: 🟡 process/oracle — no direct defect, but high leverage:
  converts a manual archaeology trap into a one-command check.
- Semantic fingerprint: framework cache recipe drift vs committed
  m_assumeutxo_data hashes; setup_network override requirement.
- Parent lineage: goal10-snapshot-basehash, goal38, cycle-296
  fixture work.
- First runnable experiment: /tmp/snap_builder2.py pattern as a
  standing script — build canonical chain per current framework
  recipe, assert dumptxoutset base_hash equals every committed
  regtest m_assumeutxo_data entry; flag drift with the exact
  divergent height (already proven: reproduces 0c552ced... at 299).

## P3: kernel/API ownership on setter-exception paths
- Profiles: Bitcoin Core / kernel API, FFI.
- Provenance: radar-flood triage — goal54-logging-cleanup
  (9b5bdd99fc), goal54-logging-registration (8c5db6e36e),
  goal104-notifications-ownership (309226ff53): three parallel
  commits claiming leaked/mis-owned callback user-data when kernel
  setters throw mid-registration. Unverified by us.
- Risk: 🟡 promising — same exception-escape shape as F27 but in
  the kernel C API; ownership confusion at FFI boundaries is a
  proven class here (F17 btck destroy null deref).
- Semantic fingerprint: C setter allocates/registers user data,
  later step throws, ownership of the registration unclear → leak
  or double-free across the FFI boundary.
- Parent lineage: F17, goal92-abi (dismissed), goal54/goal104
  branches.
- First runnable experiment: apply each commit's claimed failing
  call sequence against test_kernel with LSan/Valgrind (btck
  logging connection failing mid-setup); verdict per commit.

## P4: headers-commitment cap under lagging local clock (goal56-future-mtp)
- Profiles: Bitcoin Core / P2P, headers sync.
- Provenance: radar-flood triage — 7d669fbd94 "net: cap headers
  commitments when clock lags", unverified; touches the same
  headers-sync surface as adopted F22 (empty-headers stall).
- Risk: 🟡 promising — P2P-reachable inputs (peer headers vs local
  clock), but impact unproven.
- Semantic fingerprint: peer-advertised headers with future
  timestamps vs local-clock-gated commitments/requests.
- Parent lineage: F22 empty-headers (297c0f7ca7 stack),
  goal56-future-mtp branch.
- First runnable experiment: read the diff, identify the cap
  condition; drive a regtest peer announcing far-future headers
  (mocktime skew) and measure request/commitment behavior pre/post.

## Resolution log (cycle 306, goal 110 entropy-quality audit)
- P1 persistence-write-failure sweep: EXECUTED as goal 119 cycle 1
  (draw 299). Result: 6-instance family normalized, sweep exhausted
  for src/ producers, i2p+tor adopted (F31/F32, archive 3e5c7b1368).
  RESOLVED — superseded by campaign-119 journal cycle 1.
- P2 deterministic-chain recipe oracle: DELIVERED as standing
  harness agent-journal/artifacts/snap_builder2.py (+ framework
  setup_nodes +1-block trap documented in artifacts/README.md).
  RESOLVED as oracle, not a campaign goal.
- P3 kernel setter-exception ownership: DISMISSED cycle 298 —
  every trigger is bad_alloc-only (leak-under-OOM, no non-
  allocation throw path identified). Resolved as negative
  knowledge; resume only if a non-allocation throw is found.
- P4 future-mtp headers commitment: CONFIRMED + ADOPTED cycle 298
  (F30, 35473f91b4, archive a5a73c53f2). RESOLVED.
Queue state: EMPTY. Session entropy flowed through campaign
cycles directly (write-failure family normalization, clock-margin
boundary table, LevelDB conformance harness shapes, chmod-as-root
injection blind spot). No promotion-ready new proposals — the
promotion bar (evidence-backed, nonduplicate) is not met by the
remaining seeds (bad_alloc sweep = dismissed; methodology oracles
= technique notes, not campaigns).
