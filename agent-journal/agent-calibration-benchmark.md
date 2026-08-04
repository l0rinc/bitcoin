# Journal: security-agent calibration with historical bugs, mutants, and negative controls (goal 117)

Uber-goal rotation. Battery pattern: re-inject the original defect
against the adopted oracle on the CURRENT lineage; kill expected;
restore; re-verify green; tree byte-identical after.

## Battery 1 (2026-07-31, cycle 305, ledger-recorded): F25/F28/F30 — 3/3 KILLED
Re-injected original defects for F25 (txdb cursor), F28
(mempoolexpiry negative), F30 (headers clock clamp); all 3 adopted
oracles killed with failure shapes bit-identical to adoption-day
failing-befores; all mutants restored. (Ledger cycle 305 entry is
the record of this battery.)

## Battery 2 (2026-08-04, cycle-329 draw raw=3384471374599513077 -> idx 117): post-305 adoptions F33/F34/F35 — 3/3 KILLED, plus one adoption-integrity gap found and closed

### F34 (descriptor INT32_MAX, 62e05ae526)
Mutant: rpc/util.cpp int64_t -> int loop counter (static_casts
left as no-ops). Oracle rpc_descriptor_range_max_int32: KILLED
rc=133 (SIGTRAP, aarch64 -O0 -ftrapv; control experiments:
standalone trapv_test and exact loop shape both trap at 133).
Restored: green (rc=0), tree byte-identical.
MEASUREMENT TRAP: `<test> | tail -4; echo exit:$?` reports TAIL's
status, not the test binary's — first reading said exit:0
(oracle-dead scare); PIPESTATUS/direct re-run showed the true
kill. Never read a pipeline's tail status as the subject's.

### F33 (txospenderindex stale tip, 6cd9d75a67)
Mutant: base.cpp FindFork-rewind block removed (restored
"null-next => commit so far"). Oracle feature_txospenderindex.py:
FAILED with the exact adoption-day signature (index returns stale
spendingtxid/spendingtx vs expected bare outpoint). Restored:
"Tests successful", tree byte-identical.

### F35 (blockfilter reorg wedge, 8b9a20b114) — ADOPTION GAP
Mutant: blockfilterindex.cpp restored height-keyed
ReadFilterHeader. FIRST run (stock feature_index_prune.py):
PASSED — oracle scare #2, root-caused NOT to a weak oracle but to
a MISSING oracle: the F35 fix was adopted WITHOUT its regression
test (the author's 26-line kill/restart extension in 6ce88f28f7
never landed; stock test lacks the reorg-kill sequence entirely).
SECOND run (author's extended test staged): mutant KILLED —
node0 exits status 1 during init ('basic block filter index best
block of the index goes beyond pruned data... Failed to start
indexes, shutting down'). Restore + extended test: PASSING-AFTER
in flight; the extension is then committed as the missing F35
regression evidence.
LESSON (calibration integrity): for every adopted fix, verify the
regression test EXISTS IN-TREE — adoption-day evidence run from
the author's branch does not count as in-tree coverage.
