# Campaign #60 — reviewer-preference-skill

Base: 6e1dc71fbd (journal commit for #0 cycle-2 on audit/bug-mining-c2;
ledger-lineage anchor audit/resurrection @ 5d0155254c).
Branch: audit/reviewer-preference. Start state: clean (untracked
scratch only).

## Cycle 1 (2026-07-28): l0rinc upstream PR seam — 7 rules extracted, held-out validation 2/3 + 1 refinement

### Draw
Random draw over the 28-goal eligible pool (29-pool minus #0, now
CYCLE-2): raw=10304638172543270608, seed masked to 63 bits
(1081266135688494800), index 16 -> #60.

### Seam selection
The fork author (l0rinc) has 217 upstream PRs; his recent hardening PRs
are the population this repo's own commits most resemble, so reviewer
demands on them are the operative review bar for the fork's work.
Mining set (extraction): PR 35714 (validation flush-error), PR 35744
(coins cursor/resize). Held-out validation set: PR 35754 (CI pin/
verify). API: unauthenticated GitHub REST (issue search + 3 comment
endpoints, 4 requests total).

### Extracted rules (trigger + review question + evidence + class)

R1. Test-to-claim completeness (arejula27, PR 35714)
  Trigger: fault-injection/regression test covers fewer failure arms
  than the commit message or error strings name.
  Question: "the commit message mentions block OR UNDO files, but the
  test only injects failure on blk*.dat — add the undo arm?"
  Evidence: issuecomment-4962258493. Class: general (testing).

R2. Invariant comments must be machine-checked (arejula27, PR 35714)
  Trigger: a comment states an invariant nothing enforces.
  Question: "would it be correct adding an Assume to verify this
  behaviour is kept in the future?" (Assume(m_chainman.m_interrupt)
  after FlushChainstateBlockFile failure).
  Evidence: issuecomment-4983411974. Class: general — and exactly the
  fork's Assume-oracle theme (#61 c1-c3, #1 c1 claim audits).

R3. Behavior changes need test + caller audit up front (history via
  PR 35714 body quoting #27866 review)
  Trigger: validation-behavior change, even small.
  Demand: "review requested a test and caller audit before changing
  behavior" — deferred to a TODO until both existed (the TODO this
  rotation verified at validation.cpp:2821 in #0 c2; PR 35714 resolves
  it). Class: validation-specific, durable (12 years of practice).

R4. Characterization-first commit stacking (andrewtoth, PR 35744)
  Trigger: fix for long-standing behavior.
  Demand: series split "into a characterization test, a commit that
  makes ResizeCache() wait for live cursors, and a follow-up that lets
  cursors remain live during compaction". Evidence: l0rinc's rework
  reply issuecomment-5077126658. Class: general — matches this
  rotation's staged clean/mutation/repaired-controls protocol.

R5. New locking must join DEBUG_LOCKORDER (andrewtoth, PR 35744)
  Trigger: PR introduces a new mutex/lock discipline (shared locks).
  Demand: integrate with lock-order checking machinery, incl. non-LIFO
  pop for out-of-scope lock lifetimes (full diff supplied by
  reviewer). Evidence: issuecomment-5084823870. Class:
  concurrency-specific. Corollary observed: sanitizer CI is a hard
  gate (TSan coins_tests race on the first rework, DrahtBot
  issuecomment-5099186608).

R6. State what you check AND what you trust; no circular trust
  (Sjors, MEMBER, PR 35754)
  Trigger: supply-chain/verification PR pinning external inputs.
  Question: "can you elaborate what you're actually checking and
  trusting? ... the trust assumption seems to be circular ... there
  might be a subset of checks that ARE useful."
  Evidence: issuecomment-5020566874. Class: security-specific.

R7. Each change must achieve its stated goal at its maintenance cost
  (maflcko MEMBER + sedited, PR 35754)
  Trigger: broad hardening PR with mixed-value elements.
  Feedback: "for other stuff, it is unclear what the goal is, or
  rather the stated goal is not achieved and will also lead to other
  problems" (maflcko, issuecomment-5030976083); "seems to mostly be
  adding to the maintenance burden and I'm also not sure what exactly
  it is trying to achieve" (sedited, issuecomment-5069559687).
  Counterexample to naive "split big PRs": maflcko explicitly liked
  ONE large PR "to cover all topics here and explain what is changed
  and what is not changed" — the demand is goal-achievement per
  element, not smallness. Class: general.

### Held-out validation (PR 35754, predictions made before fetching)
- P1: reviewers will re-run/demand the verification evidence in the
  body -> CONFIRMED-STRONGER: the actual demand was deeper than
  reproducibility — an explicit trust model (R6).
- P2: behavior-change proof demand -> CONFIRMED (R6/R7 are that
  demand, shaped for CI).
- P3: scope-split pushback -> REFUTED as stated; refined into R7
  (goal-achievement per element; large survey PR explicitly welcomed).
Score: 2/3 confirmed, 1 refined. The rule set predicts the KIND of
demand (justification/proof, zero style nits in 15 human comments)
even where the specific angle differed.

### Reviewer map (this seam, n=5 humans)
- arejula27: test-completeness + Assume-oracle requests; reviews by
  building/running own fault-injection tests; opens follow-up PRs
  (#35731) instead of blocking.
- andrewtoth: mechanism-level redesign suggestions with full diffs;
  cares about debug/checking machinery (DEBUG_LOCKORDER).
- Sjors (MEMBER): trust-model rigor; skeptical of circular security
  claims; accepts useful subsets.
- maflcko (MEMBER): goal-vs-achievement precision; maintenance-cost
  weighing; tolerates large PRs when they serve as a complete survey.
- sedited: maintenance-burden first; probes implied claims ("what is
  this meant to imply?").

### Cross-validation with this rotation
- R2/R4 are already this fork's house style (Assume oracles,
  staged-control commits) — independent upstream confirmation.
- #0 c2's validation.cpp:2821 TODO cell is exactly R3's deferred
  artifact, and PR 35714 (open upstream) is its resolution — the
  rotation's risk-map cell and upstream work agree.

### Exact commands / sources
- api.github.com/search/issues?q=repo:bitcoin/bitcoin+author:l0rinc
  +type:pr (217 total; 15 fetched)
- api.github.com/repos/bitcoin/bitcoin/issues/{35714,35744,35754}
  /comments

### Limitations / queue for cycle 2
- One author-seam (l0rinc), 3 PRs deep-mined; review-COMMENTS
  (line-level review endpoint) not yet mined — likely richer for
  code-level rules; queued.
- Closed-unmerged PRs (NACK rationale) underrepresented: 35663 closed,
  35670 merged — their comment counts were low; a dedicated
  contentious-PR pass (e.g., cluster-mempool, BIP54) queued.
- Maintainer-merge rationale (fanquake/achow101 merge comments) not
  covered.
- Rules not yet encoded into reviews/ templates; that is the
  "reusable review skill" deliverable for a later cycle.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
