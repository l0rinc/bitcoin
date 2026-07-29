# Reviewer rules — l0rinc-seam reviewer map (durable)

Source: campaign #60 cycles 1-2 (agent-journal/reviewer-preference-skill.md),
mined from upstream PRs 35714, 35744, 35754 (issue + line-level
comments) with held-out validation on 35754 (2/3+1) and 35670 (2/3+1).
Use as a review checklist for PRs in this seam; each rule lists
trigger, the question a reviewer will ask, evidence (comment id or
file:line), and class (general / subsystem / person-specific /
contextual).

## Rules

R1. Test-to-claim completeness (general)
  Trigger: test covers fewer failure arms than the commit message or
  error strings name.
  Ask: "message mentions block OR UNDO files, but the test injects
  only blk*.dat — add the undo arm?" (issuecomment-4962258493)

R2. Logic invariants must be machine-checked (general)
  Trigger: a comment states an invariant nothing enforces.
  Ask: "would it be correct adding an Assume to verify this
  behaviour is kept in the future?" (issuecomment-4983411974)
  TENSION with R12 — see R12 for the line.

R3. Behavior changes need test + caller audit up front (validation)
  Trigger: any validation-behavior change, even small.
  Demand: test and caller audit BEFORE the change lands; otherwise
  defer to a TODO (12 years of practice; PR 35714 body quoting
  #27866; resolved as PR 35714 itself).

R4. Characterization-first commit stacking (general)
  Trigger: fix for long-standing behavior.
  Demand: series = characterization test, then the minimal fix, then
  optional follow-ups (andrewtoth on PR 35744,
  issuecomment-5077126658). Matches this repo's staged
  clean/mutation/repaired protocol.

R5. New locking joins DEBUG_LOCKORDER (concurrency)
  Trigger: new mutex/lock discipline.
  Demand: lock-order machinery integration incl. non-LIFO pop for
  out-of-scope lifetimes (andrewtoth, issuecomment-5084823870).
  Corollary: sanitizer CI is a hard gate (DrahtBot TSan on the first
  rework, issuecomment-5099186608).

R6. State what you check AND what you trust; no circular trust
  (security)
  Trigger: verification/supply-chain PR.
  Ask: "what are you actually checking and trusting?" — circular
  trust assumptions get rejected; useful subsets accepted (Sjors,
  issuecomment-5020566874).

R7. Each change must achieve its stated goal at its maintenance cost
  (general)
  Trigger: broad hardening PR with mixed-value elements.
  Feedback: "unclear what the goal is, or the stated goal is not
  achieved" (maflcko, issuecomment-5030976083); "adding to the
  maintenance burden" (sedited, issuecomment-5069559687).
  Counterexample: ONE large survey PR explicitly welcomed when it
  covers all topics — the demand is goal-achievement per element,
  not smallness.

R8. Partial pinning reads as check-list (security/CI)
  Trigger: integrity pins that leave the real surface unpinned (pip
  deps unpinned while other inputs pinned; image pinned but apt
  latest; action hash not covering transitives).
  Reality: rejected or challenged every time (35754 line-level:
  01_install.sh:35, 00_setup_env_freebsd_cross.sh:10,
  cache/restore action.yml:30).

R9. Style nits are adjudicated by convention anchors (process)
  Trigger: readability nit on code shape.
  Form: "violates [convention PR]" and counter-arguments cite the
  same convention (35714: &&-chains vs PR 35729). No bare-taste
  comments observed in 25 line-level comments.

R10. Assert lock contracts at API boundaries (concurrency)
  Trigger: function relies on a caller-held lock without asserting.
  Ask: "Should we add AssertLockHeld(::cs_main); here?"
  (andrewtoth, 35744 txdb.cpp:74). Sibling of R5: R5 is machinery,
  R10 is per-function assertion.

R11. Concrete patches over prose (collaboration norm)
  Reviewers attach commitable diffs/suggestion blocks/cherry-pick
  offers (andrewtoth full diff; willcl-ark uv.lock patch, 35754;
  davidgumberg .at() suggestion, 35670). Expected author response:
  take or adapt.

R12. Redundant guards for type-proven invariants get pushback
  (code-level)
  Trigger: defensive check whose invariant the type system already
  proves (u32 can't be negative).
  Form: perm-link to the declaration (darosior, 35670); "otherwise
  we are relying on this clamp always existing" (davidgumberg).
  THE LINE (R2 vs R12): Assume/machine-check for LOGIC invariants;
  prove-by-construction for TYPE invariants; never dead-defend.

R13. Non-blocking nits are self-labeled and declinable (process)
  Form: "non-blocking-nit", "feel-free-to-disregard"; author decline
  with a reason is accepted ("prefer leaving as-is to simplify
  review", 35670).

## Reviewer map (observed behavior)

- arejula27: test-completeness + Assume-oracle requests; verifies by
  building own fault-injection tests; opens follow-up PRs.
- andrewtoth: mechanism-level redesigns with full diffs; lock
  machinery (DEBUG_LOCKORDER) + AssertLockHeld.
- Sjors (MEMBER): trust-model rigor; skeptical of circular security
  claims; accepts useful subsets.
- maflcko (MEMBER): goal-vs-achievement precision; maintenance-cost
  weighing; convention-anchored nits; attack-surface completeness.
- sedited: maintenance-burden first; probes implied claims.
- willcl-ark: offers cherry-pickable tooling patches.
- davidgumberg: self-labels non-blocking nits; bounds-safety (.at()).
- darosior: type-level refutation with perm-linked declarations.

## Application notes

- R2/R4 are already this repo's house style (Assume oracles, staged
  controls) — independent upstream confirmation.
- For a new PR in the seam, walk R1-R13 as a pre-submission
  checklist; the held-out validations (35754, 35670) show the set
  predicts the KIND of demand, not always the specific angle.
- Watch items with open upstream PRs: 35744 (TSan flagged on first
  rework — see campaign #42 c1), 35818 (bloom sizing UB — see L1).

R14. Public setters validate at the boundary, not downstream
  Trigger: a public API accepts a value that later aborts or
  misbehaves.
  Ask: "accepts 0 and 1, but these produce an empty cache and abort
  in node/chainstate.cpp — should the setter reject these?"
  (w0xlt, 35205 test_kernel.cpp:804). Class: API-robustness.

R15. std::clamp only when lo<=hi is provable; else explicit max/min
  Trigger: clamp-style bounds on values whose bounds can cross.
  Form: "max/min was chosen exactly to avoid the potential ambiguity
  of clamp for cases when the min isn't smaller than the max"
  (l0rinc, 35616 caches.cpp:56; maflcko probing std::clamp and a
  safe Clamp<MIN,MAX> template, both declined as not-worth-it).
  Class: code-level, mechanism. Sibling of R12: the CONSTRUCTION
  must carry the precondition, not a comment.

## Held-out validation record

- 35754 (pre-encoding): 2/3 confirmed, 1 refined (R7).
- 35670 (pre-encoding): 2/3 confirmed, 1 refined (R12/R13).
- 35205 (post-encoding, blind): 3/3 confirmed — maflcko's
  goal-clarity challenge cut the header split (R7 verbatim
  dynamic); diff/suggestion blocks from w0xlt + iwyu links from
  stringintech (R11); nits self-labeled and convention-anchored
  (R9/R13; copyright-year point cited PR 24539's convention).
  R14 added from this PR's setter-validation demand.
- 35818 (attempted): no human review yet — unscoreable; carries a
  DrahtBot CI flag on 32-bit ARM (bloom_create_invalid_false_
  positive_rate std::fetestexcept) — recorded under L1's watch.
- 35616 (post-encoding, blind, SECOND AUTHOR SEAM — maflcko as
  author): 3/4 confirmed-in-kind, 1 refined-away.
  * goal-vs-cost challenge confirmed (sedited: "might we just remove
    the index caches instead?" — R7 dynamic; angle refined: the
    challenge proposes DELETION of the target, not a reproducer).
  * type-level construction confirmed (R12-line; clamp-vs-max/min
    precondition discussion -> R15 added; DrahtBot i686
    -Werror=narrowing gate caught the one real instance — the
    32-bit CI gate substitutes for a test-demand on pure refactors).
  * convention-anchored nits confirmed verbatim (R9/R13: `auto`
    nit declined citing clang-tidy modernize-use-auto; theStack
    IWYU-nit `<cstdint>` adopted immediately, R11-adjacent).
  * test-to-claim demand NOT observed (P4 refined): pure type-width
    refactors draw no test-completeness demand in this seam; arch CI
    is the substitute oracle.
  Conclusion: the rule set generalizes to a second author seam.

## Maintainer merge-rationale class (mined 2026-07-29, #60 c5)

M1. Merges are terse but honest about review depth: "light ACK
  <sha>" (achow101, 35215) — the merge message carries the actual
  confidence level; "light" is a signal, not a formality.
M2. Pull descriptions must be self-contained: referencing another
  PR's graph/data is not enough — "a pull description should bring
  context" (maflcko, 35200, objecting to a graph copied from a
  different PR).
M3. Post-merge lifecycle is maintainer-handled: backports to
  stable branches tracked by the merger (fanquake: "Backported to
  30.x in #35452 / 29.x in #35450", 35465).
M4. Process hygiene: don't hide useful information (fanquake, 35200:
  on in-place edits shrinking discussion content — "All it does is
  hide useful information"). Review routing: maintainers cc relevant
  experts (fanquake -> davidgumberg, 35670).
M5. Conflict choreography + outstanding-feedback precondition
  (mined 2026-07-29, #60 c6, maflcko-author seam 35616)
  When two pulls conflict, rebase the one that has to be force-pushed
  anyway (has outstanding feedback), not the clean one — "it seems
  less churn to rebase the one that has to be force pushed anyway,
  than to force push both" (maflcko). Precondition before merging
  anything: "it would be kind to address the feedback if all
  reviewers are asking for it" — outstanding all-reviewer feedback
  blocks, and the blocker is named explicitly (perm-link to the
  outstanding comment). Merge rationale stays terse and directional
  (sedited: "I think it is preferable to fix these types first, so
  putting this in now").
