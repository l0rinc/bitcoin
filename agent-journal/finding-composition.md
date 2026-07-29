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
