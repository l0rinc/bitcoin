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

## Cycle 2 (2026-07-29): line-level review-comment mining — R8-R13, held-out 2/3+1 again

### Draw
Random draw over the 13-goal eligible pool (11 pending + 2 CYCLE-1,
#91 excluded as just-cycled): raw=4139937993477073163, index 12 ->
#60 (second cycle; c1 queue cell "review-COMMENTS (line-level)
endpoint — likely richer for code-level rules"). Branch:
audit/reviewer-skill-c2 from 73b2e5cf74 (#91 c2 bookkeeping).
Journal pulled forward from c51d41c8fc (c1).

### Extraction set (same l0rinc seam, line-level endpoint)
pulls/{35714,35744,35754}/comments — 25 line-level comments total
(3 + 4 + 18). Hypothesis: line-level comments carry code-level rules
not visible in issue-level discussion (R1-R7 were discussion-level).

### New rules (trigger + question + evidence + class)

R8. Partial pinning reads as check-list; pin the full attack surface
  or don't pretend (maflcko, 35754). Trigger: integrity pins that
  leave the real surface unpinned (pip deps unpinned while other
  inputs pinned; container image pinned but apt picks latest;
  action-hash not covering transitive actions).
  Evidence: 01_install.sh:35, 00_setup_env_freebsd_cross.sh:10,
  cache/restore action.yml:30 comments. Class: security/CI-specific.
  Line-level sharpening of R6 (trust-model): R6 asks "state what you
  trust"; R8 rejects pins whose coverage pretends more than it is.

R9. Style nits are adjudicated by convention anchors, not taste
  (maflcko, 35714). Trigger: readability nit on code shape
  (&&-chains in conditions). Form: "violates [convention PR 35729]"
  and the author's counter also cites the convention ("makes more
  sense to test them in groups, see 35729#pullrequest...").
  Class: general (process) — no bare-taste comments observed
  anywhere in the 25.

R10. Assert lock contracts at API boundaries (andrewtoth, 35744,
  txdb.cpp:74 "Should we add AssertLockHeld(::cs_main); here?").
  Trigger: function relies on a caller-held lock without asserting.
  Class: concurrency-specific. Sibling of R5 (DEBUG_LOCKORDER
  integration) — R5 is machinery, R10 is per-function assertion.
  Matches this fork's AssertLockHeld/Assume house style.

R11 (extended). Concrete patches over prose: reviewers attach
  commitable diffs (andrewtoth's full diff in c1; willcl-ark's
  uv.lock cherry-pick offer, 35754; davidgumberg's GitHub
  suggestion-block .at() fix, 35670 held-out). Class: general
  (collaboration norm) — and taking/adapting the patch is the
  expected response.

R12. Redundant runtime guards for type-proven invariants get
  pushback — with the declaration cited (held-out 35670).
  darosior perm-links the u32 declaration ("max_extra_txs is a u32,
  so it can't be negative"); davidgumberg: keep the line minimal
  "since otherwise we are relying on this clamp always existing".
  Class: general (code-level). RECORDED TENSION with R2: Assume is
  demanded for LOGIC invariants (flush-failure interrupt state),
  while redundant guards for TYPE-proven facts are declined — the
  line is prove-by-construction for types, machine-check for logic,
  never dead-defend.

R13. Non-blocking nits are self-labeled and declinable with a reason
  (held-out 35670): "non-blocking-nit", "non-blocking-observation-
  feel-free-to-disregard"; author decline accepted ("I prefer
  leaving that as-is to simplify review"). Class: process/general.

### Held-out validation (PR 35670, predictions recorded before fetch)
- P1: >=1 machine-check/oracle demand (R2/R10 family) -> REFUTED as
  stated: no Assume demand at line level; the correctness pressure
  was the INVERSE (R12: remove redundant guards, cite the type).
  Refined: the family is "invariants must be PROVEN, by type or by
  oracle — not guarded blindly".
- P2: >=1 concrete commitable patch from a reviewer -> CONFIRMED
  (suggestion block .at(), davidgumberg).
- P3: no convention-less taste nits -> CONFIRMED with refinement
  (nits are self-labeled non-blocking and declinable, R13).
Score: 2/3 confirmed, 1 refined — same shape as c1's held-out.

### Reviewer map additions (line-level behavior)
- maflcko: nit-with-convention-anchor style; attack-surface
  completeness probes on CI/security PRs.
- andrewtoth: per-function AssertLockHeld requests.
- willcl-ark: offers cherry-pickable tooling patches (uv.lock).
- davidgumberg: self-labels non-blocking nits; bounds-safety
  suggestions (.at()).
- darosior: type-level refutation with perm-linked declarations.

### Exact commands
- api.github.com/repos/bitcoin/bitcoin/pulls/{35714,35744,35754,
  35670,35663}[/comments?per_page=100] (unauthenticated REST)

### Limitations / queue for cycle 3
- Single author-seam still (l0rinc); other high-volume authors
  (maflcko's own PRs, theuni's build PRs) would test generality.
- Maintainer merge-rationale comments not covered (c1 queue, still).
- R1-R13 not yet encoded into reviews/ templates — the reusable
  skill deliverable; now the highest-value next cell (the rule set
  is stable across two held-outs).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 3 (2026-07-29): reusable review skill encoded — reviews/reviewer-rules.md (R1-R13 + reviewer map)

### Draw
Re-rank draw over the 7 remaining CYCLE-2+ open cells:
raw=2554837129443345158, index 6 -> #60 (third cycle; c2 queue cell
"R1-R13 not yet encoded into reviews/ templates — now the
highest-value next cell"). Branch: audit/reviewer-skill-c3 from
0e47dc4623 (#76 c3 bookkeeping).

### Deliverable
reviews/reviewer-rules.md — durable, undated reference: all 13
rules with trigger/question/evidence/class, the R2-vs-R12 tension
line ("Assume for logic invariants, prove-by-construction for type
invariants, never dead-defend"), the reviewer map (8 reviewers with
observed behavior), and application notes (house-style overlap,
pre-submission checklist use, watch items 35744/35818).
Indexed in reviews/README.md per the directory's convention.

### Validation of the encoding
Every rule cites its mined evidence (comment id or PR:line) from
c1/c2; no rule added without evidence, none dropped. The two
held-out validations (35754, 35670; both 2/3+1) are recorded in
the file as the confidence basis.

### Verdict
Deliverable complete (journal + template; no repo code touched).
The campaign's "reusable review skill" artifact now exists and is
indexed; future PR reviews in this seam should cite R-numbers
against it and extend when new evidence lands.

### Queue for cycle 4
- Validate the template against a NEW held-out PR (post-encoding,
  blind application) — the acceptance test of the skill itself.
- Maintainer merge-rationale comments (still unmined).
- Second author seam (maflcko's own PRs) for generality.

## Rotation note
Three cycles; the core deliverable is done. Campaign remains open
for held-out validation of the template itself.

## Cycle 4 (2026-07-29): template held-out validation on PR 35205 — 3/3 blind predictions confirmed; R14 added

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=2955923148731138849, index 4 -> #60 (fourth cycle; c3 queue
cell "validate the template against a NEW held-out PR").
Branch: audit/reviewer-skill-c4 from 1f59a145d6 (#48 c2
bookkeeping).

### Held-out selection
PR 35818 (bloom sizing) was the first candidate — but it has ZERO
human comments (2 DrahtBot posts only), unscoreable; its DrahtBot
CI flag (32-bit ARM, bloom_create_invalid_false_positive_rate
std::fetestexcept) is recorded under L1's watch. Replacement:
PR 35205 (kernel,node dbcache setter + defaults, 9 issue + 32
line-level comments, unmined in c1-c2).

### Blind predictions (recorded before fetching)
- P1: goal-vs-achievement or contract-precision demand (R7/R6).
- P2: >=1 concrete diff/suggestion from a reviewer (R11).
- P3: style feedback only with convention anchors or non-blocking
  labels (R9/R13).

### Result — 3/3 confirmed (2 strong)
- P1: maflcko "I don't understand this pull request? What is the
  point of splitting this header up further?" -> author DROPPED the
  split commit — the R7 dynamic verbatim (unclear-goal elements get
  cut). Plus sedited's contract-precision question (DEFAULT_KERNEL_
  CACHE decoupling) and w0xlt's setter-boundary demand (R14).
- P2: w0xlt's diff suggestion blocks (stale includes; test
  strengthening in <details>), stringintech's iwyu CI links.
- P3: "Nit: alphabetical ordering", "micro-nit", "nit: can be
  ignored or left for a follow-up" (author's decline accepted);
  copyright-year point cited the convention PR 24539 explicitly —
  R9's anchor shape verbatim.

### Rule addition
R14: public setters validate at the boundary, not downstream
(w0xlt: setter accepting 0/1 that abort later in chainstate —
should reject at the setter). Encoded into reviews/reviewer-rules.md
with the held-out record.

### Verdict
The template predicted the KIND of every material demand on a blind
PR; the reusable skill is validated. Campaign's core deliverable
closed (skill + validation).

### Exact commands
- api.github.com repos/bitcoin/bitcoin/{issues,pulls}/{35818,35205}
  /comments; search/issues author:l0rinc created:>2026-03-01

### Limitations / queue
- Maintainer merge-rationale comments still unmined.
- Second author seam for generality (c3 queue) still open.

## Rotation note
Four cycles; core deliverable validated. Rotating.

## Cycle 5 (2026-07-29): maintainer merge-rationale mined — M1-M4 added

### Draw
Re-rank draw over the rebuilt 5-cell queue:
raw=127326143727536095, index 0 -> #60 (fifth cycle; c3/c4 queue
cell "maintainer merge-rationale"). Branch:
audit/reviewer-skill-c5 from 06215e998e (#21 c4 bookkeeping).

### Mining set (8 recently merged l0rinc-seam PRs; last comments
from fanquake/achow101/maflcko)
- 35215 (SipHash coins keys): "light ACK <sha>" — honest depth.
- 35465 (compact chainstate): backport lifecycle notes.
- 35200 (dbcache warnings): process correction on in-place edits +
  self-contained-description demand.
- 35670 (compact block extra tx): expert-routing cc.

### Rules added (M-class, template updated)
M1 merge terseness with honest depth; M2 self-contained
descriptions; M3 maintainer-handled backport lifecycle; M4 no
information-hiding in PR hygiene (+ expert routing).

### Verdict
The reviewer-rules template now covers reviewers AND maintainers;
the merge-rationale gap from c3 is closed. Journal-only.

### Queue for cycle 6
- Second author seam (maflcko's own PRs) for generality — the one
  remaining open cell.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 6 (2026-07-29): second-author-seam generality test (maflcko-authored 35616) — template generalizes (3/4 in-kind, 1 refined); R15 + M5 added

### Draw
Re-rank singleton (last of the 3-cell queue): #60 (sixth cycle; c5
queue cell "second author seam for generality"). Branch:
audit/reviewer-skill-c6 — RECORDED BASE: created from
audit/critical-history-c3 tip 734121c172 (this cycle stacked on the
#49 c3 journal tip; the chain is the ledger lineage).

### Method (blind)
Chose maflcko-authored #35616 ("Use u64 over size_t for all cache
sizes to fix a 32-bit overflow") from the recently-merged list;
recorded 4 blind predictions from the R/M template BEFORE reading
comments; then fetched issue + line-level comments via the public
GitHub API (curl; /tmp/pr_parse.py, /tmp/pr_comments.py).

### Blind predictions (recorded pre-fetch)
- P1: goal-vs-achievement/self-contained-proof challenge (R7/M2).
- P2: type-level prove-by-construction discussion (R12-line).
- P3: convention-anchored nits, declined/adopted with reasons (R9/R13).
- P4: test-to-claim completeness demand (R1/R3).

### Score
- P1 CONFIRMED-in-kind (angle refined): sedited's "might we just
  remove the index caches instead?" is the R7 goal/cost challenge;
  the demanded artifact is a deletion-alternative, not a reproducer.
- P2 CONFIRMED + rule-yielding: std::clamp-vs-max/min precondition
  discussion (min>max ambiguity) -> R15 added; DrahtBot i686
  -Werror=narrowing caught the one real narrowing instance.
- P3 CONFIRMED verbatim: `auto` nit declined via clang-tidy
  modernize-use-auto convention; theStack IWYU `<cstdint>` nit
  adopted immediately (R11-adjacent).
- P4 REFINED-AWAY: no test-completeness demand on a pure type-width
  refactor; the 32-bit arch-CI gate is the substitute oracle.
Score: 3/4 confirmed-in-kind, 1 refined. The template GENERALIZES to
the second author seam.

### Rules added
- R15 (std::clamp only when lo<=hi provable; else explicit max/min —
  construction carries the precondition).
- M5 (conflict choreography: rebase the pull that must force-push
  anyway; outstanding all-reviewer feedback is a named merge
  precondition; terse directional merge rationale).

### Exact commands
- curl api.github.com search/issues?q=repo:bitcoin/bitcoin+type:pr+
  author:maflcko+is:merged (12 candidates; picked 35616 as the most
  substantive non-CI one)
- curl .../issues/35616/comments, .../pulls/35616/comments

### Verdict
CONFIRMED deliverable: generality of the reviewer-rules template is
now evidence-backed on a second author seam; two new durable rules
encoded in reviews/reviewer-rules.md.

### Limitations / queue
- One-PR sample per seam; a third seam (e.g. achow101-authored) would
  tighten the generality claim further — low marginal value, queued.
- DrahtBot LLM-reason hints were treated as hints only; the
  -Werror=narrowing fact was taken from the CI task line, not the
  LLM prose.

## Rotation note
Six cycles; the open generality cell is closed. Not exhausted
(third-seam tightening queued).

## Cycle 7 (2026-07-29): third-seam generality test (achow101-authored 35269) — 4/4 in-kind; generality cell CLOSED

### Draw
Re-rank draw over the remaining 4-cell queue:
raw=217606016431337542, index 2 (of 4) -> #60 (seventh cycle; c6
queue cell "third author seam"). Branch: audit/reviewer-skill-c7
from 2c8722508d (#49 c7 journal tip).

### Method (same blind protocol as c6)
Chose achow101-authored #35269 ("musig: Include pubnonce in session
id") — the most security-relevant of his recent merges. 4 blind
predictions recorded pre-fetch; issue + line-level comments then
fetched via the public API.

### Blind predictions vs observed
- P1 threat-model/goal probe (R6/R7): CONFIRMED-in-kind, refined —
  the seam's security review debates the RESPONSE to failure
  (rkrux on assert-crash vs recoverable error for nonce reuse).
- P2 encoding/type-level nit (R12-line): CONFIRMED — hasher field
  ordering nit; declined with reason, accepted (R13 dynamic).
- P3 test-to-claim (R1): arm already present in the PR; pressure
  went to assertion-style consolidation (assert_equal(a,b,c)).
- P4 convention nits (R9/R13): CONFIRMED verbatim — comment
  placement adopted; "Will leave nits for a followup" accepted.
- R11 verbatim: rkrux attaches commitable diffs throughout.

### Verdict
CONFIRMED: the template generalizes to a THIRD author seam
(l0rinc, maflcko, achow101 — three independent seams). The c6
generality cell is CLOSED; R6 extended with the failure-response
adjudication angle in reviews/reviewer-rules.md.

### Exact commands
- curl api.github.com search author:achow101 is:merged;
  .../issues/35269/comments; .../pulls/35269/comments

### Limitations / queue
- One PR per seam; the template is a prediction aid, not a proof.
  Further cycles only on new-rule signals (a reviewer behavior the
  template fails to predict — that failure itself is the queue).

## Rotation note
Seven cycles; generality cell closed. Not exhausted (template-
failure watch).

## Cycle 8 (2026-07-29): template-failure watch — newest merges show only known dynamics

### Draw
Re-rank singleton (last queue cell): #60 (eighth cycle; the
standing template-failure watch). Branch: audit/reviewer-skill-c8
from cc277a625b (#65 c10 journal tip).

### Sample (2026-07-29 newest merges)
- 35753 (kernel null-mempool chainstate deletion): zero line-level
  comments; single achow101 "ACK <sha>" (M1 terse-with-depth).
- 35787 (empty addnode values): CI-error report (unrelated,
  infra-class), author self-updates title/description — no review
  objections of any novel class.
- Nothing observed that the R1-R15/M1-M5 template fails to
  predict; no new rule mined.

### Verdict
Watch quiet: the template still covers the observed seam
behavior. No update needed.

### Exact commands
- api.github.com search is:merged (8 newest);
  issues/35753|35787/comments; pulls/35753/comments

### Limitations / queue
- Watch continues passively; a template-failure (an objection
  shape the rules don't predict) is itself the next cell.

## Rotation note
Eight cycles; watch quiet.
