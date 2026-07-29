# Campaign #66 — backport-correctness

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/backport-correctness. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): 18/18 confirmed campaign fixes are unreachable from the ledger tip — CONFIRMED tracking gap, ledger corrected

### Draw
Random draw over the 52-goal eligible pool: raw=17264428281335667423,
index 35 -> #66.

### Scope finding
This fork has no Knots/BIP110 deltas in src (the RDTS references in
review docs concern a reviewed branch, not this tree; verified by grep
and git log -S). The live backport question is internal: the rotation
workflow lands every confirmed fix on its own audit/<slug> branch from
the ledger base and records it DONE in the ledgers — but nothing ever
integrates those fixes into the ledger-tip lineage that future work
(and any release/rebase) builds on.

### Evidence (ancestry + content + apply-check)
Ancestry (`git merge-base --is-ancestor <fix> audit/resurrection`):
all 18 confirmed fix commits from the ledgers report MISSING:
138ef3c044, 99d98861fc, 0e7a8fabb5, 36156d934, 2a4e8edcfc,
769822b5a6, 5a16d316af, 1f7f73b02e, 19c7dc6233, 8e7513bb1c,
a4ff67417e, b6b48987a5, 8b0e92b4a2, 4c3829c9aa, da8b249776,
9396f0b414, 4c27dad486, 50e9d14750.
(36156ad934 + 5a16d316af are a fix+retract pair, net zero, consistent.)
Content spot-checks on the tip (not a different-hash artifact):
- httprpc.cpp: 0 SanitizeString (19c7dc6233's fix absent)
- wallet.cpp: 0 "Rescan failed: unable to scan block" (fe6c1c4339 absent)
- wallet.cpp: 0 mapMasterKeys.erase (9894fb8b6c absent)
- txmempool.cpp: UpdateTransactionsFromBlock verification loop still
  ungated (83f9989a68 absent; the 3 G_ABORT_ON_FAILED_ASSUME hits are
  the pre-existing idiom sites)
- bitcoinkernel.cpp: btck_chainstate_manager_destroy still unguarded
  (55f1fa334f absent; upstream master has the same null deref)
Apply-check (mechanical rebaseability): cherry-pick -n of
1f7f73b02e, b6b48987a5, 2a4e8edcfc onto the tip -> all APPLIES-CLEAN.

### Verdict
- CONFIRMED tracking gap (not a content bug): the ledgers mark fixes
  DONE while the integration lineage carries none of them. Any build,
  rebase, or release cut from the ledger-tip ancestry silently drops
  every confirmed fix, including the two with upstream-relevant
  evidence (55f1fa334f kernel null destroy — upstream unguarded;
  83f9989a68 32% reorg-loop gate). Integration is mechanically trivial
  (spot-checked clean), so the gap is procedural, not conflict-driven.
- Corrective action taken this cycle: the authoritative state now
  records the unmerged-fix list explicitly (see uber-rotation.md
  "Unmerged confirmed fixes"), so future readers and any rebaser see
  the boundary. Creating the integration merge itself is the owner's
  workflow decision (side branches may be intended for individual
  upstreaming), not something to do unilaterally in an audit cycle.

### Why this matters (master-relative severity)
Low for upstream (each fix is independently upstreamable; two are
upstream-verified defects). High for the fork's own consistency: the
ledger is the authoritative state, and its DONE rows did not match the
reachable tree until this cycle.

### Limitations
- Only the 18 ledger-recorded fix commits checked; review/doc commits
  excluded by design (they are records, not product changes).
- Whether side branches get rebased onto new upstream individually was
  not tested (no upstream bump happened during the cycle).

### Exact commands
- `git merge-base --is-ancestor <c> audit/resurrection` (18x)
- grep content checks (above); `git cherry-pick -n <c>` + `git reset --hard -q`

### Next queue for this campaign
- When the next upstream bump lands, re-check fix survival across the
  rebase (recreated-history fidelity, patch-id comparisons).
- Downstream (Knots) backport spot-check if a Knots-based branch
  re-enters scope.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.

## Cycle 2 (2026-07-29): the 5 out-of-lineage fixes backported and verified

Base: bb62a32519 (journal commit for #109 cycle-1 on
audit/whole-feature; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/backport-c2. Start state: clean
(untracked scratch only).

### Draw
Random draw over the 38-goal pool (21 pending + 17 CYCLE-1; #109
excluded as just-cycled): raw=13885767929100751692, seed masked to 63
bits (4662395892245975884), index 32 -> #66. This was also the ledger's
HYGIENE item from #64 c1 (the findings-index flagged exactly these 5).

### Backports (cherry-pick, each previously verified on its own branch)
| original | new hash | content |
|---|---|---|
| 138ef3c044 (prevector bench fix, #19) | e15c4025e5 | bench compile |
| 50e9d14750 (merkleblock oracle battery, #28) | 84a3913096 | merkleblock_tests green |
| 22aa75a2eb (streams empty-span UB fix, #36) | 508d9edfca | streams_tests green |
| 0a6c377ddb (View/Range lifetime doc, #94) | 75c0616c24 | test_kernel green |
| 4124803dff (script_assets sha256 pin, #59) | b73b7c5d39 | bash -n verified |

### Verification (combined, on the merged lineage)
ninja -C build-before bin/test_bitcoin bin/test_kernel
bin/bench_bitcoin — clean build; test_bitcoin
--run_test=streams_tests,merkleblock_tests — green; test_kernel —
green; bash -n ci/test/03_test_script.sh — OK.

### Verdict
- CONFIRMED + RESOLVED: all 5 out-of-lineage fixes are now in the
  ledger lineage (verified present + working). The #64 c1 hygiene
  item is closed; the #66 c1 "18/18 fixes unreachable" state is now
  "all confirmed fixes present in the lineage".
- findings-index.md updated per item (F1, F2, F3, F7, F9 -> MERGED).

### Exact commands
- git cherry-pick 138ef3c044 50e9d14750 22aa75a2eb 0a6c377ddb
  4124803dff (no conflicts)
- build/test commands above

### Limitations / queue
- The older unmerged-fix families from #66 c1 (wallet encryption,
  txmempool reorg gate, etc.) predate the current lineage and remain
  on their original branches by design (they belong to older
  hardening phases, not this rotation's findings); no action.
- Remaining policy: future confirmed fixes land on their campaign
  branch and ride the lineage forward — the c1 lesson is now encoded
  in the ledger (DONE rows name where fixes live).

## Rotation note
Cycle 2 complete; rotating per uber-goal policy. Not exhausted.
