# Campaign #103 — finding-composition

Base: 0dff151836 (journal commit for #69 cycle-1 on
audit/backend-diff; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/finding-composition. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): capability graph over the findings index — no realizable chain; three edges tested and recorded broken

### Draw
Random draw over the 44-goal pool (27 pending + 17 CYCLE-1; #69
excluded as just-cycled): raw=6640084940197297139, index 23 -> #103.

### Capability nodes (from findings-index.md, this session's set)
| node | preconditions | attacker control | capability gained |
|---|---|---|---|
| F1 prevector fix | local code path | none | (fixed) OOB read |
| F3 streams UB | fuzz target only | none | (fixed) UB |
| F4 bloom storm | block download | low (peer paces) | (fixed) CPU |
| F5 estimator waste | block download | low | (fixed) CPU |
| F8 index restriction | authenticated RPC | auth | wrong-data (fixed) |
| F9 CI pin | CI infra | maintainer | (fixed) unpinned input |
| L1 bloom ctor | tests only | none | latent |
| L2 resize race | authorized RPC + assumeutxo rebalance | auth + timing | process abort (FIXED in-tree) |
| O1-O5 oracles/harnesses | n/a | n/a | detection capability |

### Edge tests (ranked by end-state severity)
1. L2 -> process abort -> validation stall chain: L2's abort needs
   (a) a live UTXO-scan RPC and (b) an assumeutxo cache rebalance in
   the same window. BROKEN EDGE in this tree: e049f064e1's unique-
   lock cursor makes (b) wait for (a); the composition exists only on
   upstream master (verified racy there). Chain unrealizable locally;
   the upstream instance is a single-step availability kill, no
   escalation (no corruption, no privilege change).
2. F4+F5 CPU waste -> sustained remote DoS: both were per-block CPU
   costs triggerable by serving blocks. BROKEN EDGE: both fixed
   (c8f53e58d9, 675011ba86 with before/after profiles); even combined
   they were ~40% of regtest-test-IBD CPU, self-limiting (per-block
   cost ends at tip), and needed a serving-peer relationship.
3. F8 wrong-data -> downstream consumer corruption (a caller trusting
   gettxoutsetinfo for a non-tip block): BROKEN EDGE: the 9396f0b414
   restriction rejects the misuse at the boundary; the wrong-data
   state never materializes.

### Verdict
- DISMISSED (composition): no realizable end-to-end chain from the
  current finding set. The capability inventory is perf/test/doc-
  dominated; the one availability capability (L2) is fixed locally
  and single-step upstream.
- Hygiene fact reaffirmed: chains involving F1/F2/F3/F7/F9 must be
  evaluated AFTER the #64-c1 out-of-lineage fixes are merged —
  recorded as the graph's open condition.

### Exact commands
- findings-index.md capability extraction (18 nodes);
  per-edge precondition walk (journal history: #7 c2, #22 c2, #63 c1,
  #31 c3)

### Limitations / queue
- One composition pass. Rerun after the out-of-lineage merges and
  after any new confirmed finding (standing rule: composition passes
  evaluate chains over CONFIRMED capabilities only).
- Cross-session findings (fork's pre-rotation fix family) not yet
  indexed as capability nodes — queued for a c2 autopsy pass (#105
  is the natural home).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-08-02, draw 177, raw=5480729779092233995 (63-bit), idx 52/57 over the rebuilt eligible set): cross-session capability indexing — F10-F16 added as nodes, #95-c7 branch-only EXCLUDED, all edges broken-by-repair; no realizable chain; DISMISSED

### Draw/provenance
Pool rebuilt from the authoritative handoff DONE list (57 eligible
campaigns, enumerated in the ledger); c1's queue cell "cross-session
findings not yet indexed" executed here. Index source: findings-
index.md F1-F16 (grep-verified), fix commits re-checked against
HEAD (e.g. 55788c9a76 for F16).

### New capability nodes (c1 set was F1-F9, L1, L2, O1-O5)
| node | trust boundary | pre-fix end-state | edge status |
| F10 mempool hex-tx decode dup | rpc | none (code-shape) | fixed 4f97fbfe1e |
| F11 base64-PSBT decode dup x6 | rpc | none (code-shape) | fixed b1e55802f6 |
| F12 reorg-repair ungated | mempool perf | CPU on reorg paths | fixed 83f9989a68 |
| F13 -limitclustercount=0 | local config | release: SILENT all-tx mempool rejection (liveness); assert: abort | fixed 5e0a80ade5 |
| F14 dbwrapper failed-open leak | local DB state | per-restart-loop memory growth | fixed 461c21cbfa |
| F15 txgraph stale comments | docs | none | fixed a9d7be8c11 |
| F16 KDF rounds overflow | crafted wallet file | ~2.4h unlock hang (wallet DoS) | fixed 55788c9a76 |
EXCLUDED: #95-c7 MultiRead (rocksdb-brute) — never in HEAD; a
capability must exist on an evaluated tree; recorded as exclusion
with its ASan proof pointer.

### Edge tests over the enlarged graph (ranked)
1. F13 silent-rejection -> liveness kill: pre-fix, a config mistake
   made the node silently reject ALL mempool traffic while staying
   "synced" — composable with nothing remote (local config only),
   single-step, self-inflicted. BROKEN by startup rejection.
2. F16 hang -> credential-access stall: needs a crafted wallet file
   delivered to the victim (supply-chain/backup-restore boundary);
   chain to key exposure: NONE (hang is pre-KDF, keys never
   touched). BROKEN by the INT_MAX guard.
3. F14 leak -> OOM under restart loop: needs persistent DB
   corruption + an operator/systemd restart loop; growth is
   ~4.2 KB/failed open — gigabyte-scale OOM needs ~250k restarts;
   BROKEN by RAII context, and bounded even pre-fix.
4. F12 + F4/F5 CPU family: all three fixed with before/after
   profiles; combined pre-fix ceiling was still self-limiting
   (per-block/per-reorg cost, ends at tip/quiescence).

### Verdict
DISMISSED (composition): with F10-F16 indexed and #95-c7 excluded,
the confirmed-capability graph still has no realizable end-to-end
chain. Every availability-touching node (L2, F13, F14, F16) is
fixed in-lineage with regression evidence; all pre-fix chains were
single-step, local-boundary, and self-limiting. Hygiene rule from
c1 stands: re-run on any new confirmed finding or after out-of-
lineage merges.

### Exact commands
- findings-index.md F10-F16 extraction; git cat-file/log on
  55788c9a76; per-edge precondition walk above.

### Limitations / queue
- Severity assessments of pre-fix chains are judgment (bounded by
  the recorded mechanisms), not re-derived empirically this cycle.
- Standing rule unchanged: composition passes evaluate CONFIRMED
  capabilities only.

## Cycle 3 (2026-08-03, cycle-324 draw r6, raw=8142385977759109223 -> idx 103): F25-F35 flood findings as nodes — all new edges broken; no realizable chain; DISMISSED

### Reopen trigger
Standing hygiene rule fired: 11 new confirmed findings (F25-F35,
2026-08-03 flood adoptions) not yet composed. All FIXED in-lineage
with regression evidence; edges evaluated on PRE-fix capabilities
per c2 rule.

### Edge walk (new-node pairs and cross-generation pairs)
- F26 (xor.dat short-write -> unbootable datadir) x F27 (snapshot
  base-blockhash write abort): mutually exclusive orderings, not a
  chain — F27's snapshot path needs a booted node, F26 prevents
  boot. BROKEN.
- F33 (txospenderindex false-synced) x F35 (blockfilter wedge):
  different indexes, no shared state; F33's false report can MASK
  F35's symptom at the RPC client — diagnostic ambiguity, not a
  capability grant. Indexes are read-only observers; no validation
  influence. BROKEN.
- F34 (descriptor INT32_MAX RPC thread kill) x F28/F29 (negative
  config acceptance): disjoint preconditions (live authenticated
  RPC session vs startup flags). BROKEN.
- F30 (headers cap wrap under lagging clock) x F33/F35: indexes
  consume connected blocks post-validation; commitment-cap wrap
  affects header acceptance, not index state. BROKEN.
- F31/F32 (i2p/tor key persistence) x F30: service-restart failure
  degrades peer diversity environmentally; does not grant the
  lagging-clock precondition. BROKEN.
- F25 (txdb cursor over malformed key) x F33/F35: corrupt-keyspace
  precondition foreign to index findings (no coins-DB cursor use).
  BROKEN.

### Verdict
DISMISSED: no realizable multi-step chain. Every new capability is
local-boundary (config/filesystem/RPC-session/observer-index),
pairwise-independent in preconditions, and repair-masked. The two
Medium items (F33, F35) are observer faults; neither composes into
a validation or remote primitive.

### Limitations / queue
- Severity composition is judgment bounded by recorded mechanisms
  (same caveat as c2).
- Standing rule unchanged: re-run on any new CONFIRMED finding or
  out-of-lineage merge.
