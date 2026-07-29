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
