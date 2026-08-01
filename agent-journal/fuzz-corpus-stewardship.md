# Fuzz-corpus stewardship

## Cycle 242 - 2026-07-31

### Selection and handoff

- Selected goal: 79, `fuzz-corpus-stewardship`.
- Branch: `uber-cycle-242-fuzz-corpus-stewardship-20260731`.
- Start and end source: `d0f9abef7f55e797d06bea2d08055ebaff16ca2a` (`uber-goal: close cycle 241 deterministic artifacts`).
- Required author for a finding commit: `Lőrinc <pap.lorinc@gmail.com>`.
- No production source change was justified. This cycle is a journal-only handoff.
- Cycle scratch root: `/data/my_storage/tmp/cycle242-dbwrapper`.
- Protected long-running test processes were left running. No default datadir, wallet, key, or production database was used.

### Scope ledger

This cycle audited the `dbwrapper` fuzz corpus and its structurally related
`dbwrapper_threaded` and `dbwrapper_concurrent_reads` targets. The exact target
source is `src/test/fuzz/dbwrapper.cpp`; the last path-changing commit is
`5b32d8965f8752b4bd0688d822668d5909e0fb25` (`dbwrapper: reject batches from
another database`). The prior Cycle 187 target-specific corpus was reused only
as an input seed, not treated as a new finding. Earlier journals already close
the exact transport, descriptor, process-message, and other engine-comparison
cells, so those were not repeated.

Corpus source copied without modification:
`/data/my_storage/tmp/cycle187-fuzz-corpus-dbwrapper` to
`/data/my_storage/tmp/cycle242-dbwrapper/source`.

The source directory contains 267 files, 21,463 bytes, sizes 0..128 bytes,
mean 80.39 bytes, and one empty file. LibFuzzer loaded 266 non-empty files.
All 267 content hashes are unique; there are no duplicate groups to remove.
The sorted source manifest hash is
`95bd514bdc0b7abe37cf7a761e2185f201d836db31274692c9c167f76c7c3bb8`.

### Build identity

The fuzz binary was rebuilt from the cycle HEAD with:

```text
TMPDIR=/data/my_storage/tmp/cycle242-build-tmp cmake --build /data/my_storage/tmp/cycle131-build-libfuzzer --target fuzz -j2
```

The resulting binary is
`/data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz`, SHA-256
`761ae5858cbf841db541e1a4ad3a726c912ebe944448f2b85667aea8f9b1ad85`.
The CMake cache reports `BUILD_FOR_FUZZING=ON`, `CMAKE_BUILD_TYPE=Debug`, and
`-O0 -ftrapv -g3`; no ASan, UBSan, TSan, MSan, or LSan runtime is linked.
These are Debug/fuzz-harness results, not sanitizer results.

### Commands and evidence

The source replay used the current binary and immutable source corpus:

```text
FUZZ=dbwrapper TMPDIR=/data/my_storage/tmp/cycle242-dbwrapper/runs \
  /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz \
  /data/my_storage/tmp/cycle242-dbwrapper/source \
  -runs=1 -rss_limit_mb=8192 -timeout=2 -print_final_stats=1 \
  -artifact_prefix=/data/my_storage/tmp/cycle242-dbwrapper/artifacts/replay-run1-
```

It completed 267 runs, with no artifact, 16,049 coverage counters, 56,017
feature units, and 331 MiB peak RSS. The source directory was not modified.

Coverage merge used a separate output directory and fixed seed 242:

```text
FUZZ=<target> TMPDIR=/data/my_storage/tmp/cycle242-dbwrapper/runs \
  /data/my_storage/tmp/cycle131-build-libfuzzer/bin/fuzz \
  -merge=1 <output-corpus> <input-corpus> -seed=242 \
  -rss_limit_mb=8192 -timeout=2 -print_final_stats=1
```

Fixed-seed full-source merges produced:

| target | files | bytes | coverage edges | features |
| --- | ---: | ---: | ---: | ---: |
| `dbwrapper` | 191 | 14,942 | 16,222 | 57,466 |
| `dbwrapper_threaded` | 175 | 12,919 | 18,253 | 59,960 |
| `dbwrapper_concurrent_reads` | 231 | 17,483 | 18,585 | 42,706 |

The `dbwrapper` reduced set was then transferred and re-merged independently
for each related target. The target-specific results were:

| target | files | bytes | coverage edges | features |
| --- | ---: | ---: | ---: | ---: |
| `dbwrapper` | 187 | 14,439 | 16,228 | 57,510 |
| `dbwrapper_threaded` | 168 | 12,193 | 18,105 | 59,759 |
| `dbwrapper_concurrent_reads` | 173 | 13,013 | 18,618 | 41,536 |

The same fixed-seed `dbwrapper` source merge was repeated into a fresh output
directory. It again selected 191 files and produced manifest hash
`4ea955c094885d144470b66a722d0fd976680ae29a9e09759421ca2bb878a111`.
This is evidence that the merge file set is reproducible for this exact build,
target, input, and seed. The selected set is 75 files and 6,521 bytes smaller
than the 266 non-empty source inputs, a 28.2% file reduction and 30.4% byte
reduction.

The transferred reduced sets were replayed with `-runs=1 -seed=242`:

| target | input files | executed units | final coverage | final features | peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dbwrapper` | 187 | 188 | 16,045 | 56,009 | 312 MiB |
| `dbwrapper_threaded` | 168 | 170 | 17,923 | 58,053 | 313 MiB |
| `dbwrapper_concurrent_reads` | 173 | 175 | 17,769 | 39,830 | 626 MiB |

All three replays exited successfully with zero new units and no crash or
artifact. The exact logs and merge control files are under the cycle scratch
root, including `artifacts/fixed-*-merge.log` and
`artifacts/fixed-*-replay.log`.

### Findings and verdicts

1. **Confirmed stewardship result: content deduplication is not the current
   reduction opportunity.** Every source input has a unique content hash. The
   fixed-seed libFuzzer merge, rather than hash deduplication, reduced the
   `dbwrapper` corpus by 75 inputs while preserving the selected merge
   coverage for this build.
2. **Confirmed transfer result: the `dbwrapper` reduced set is executable on
   both related targets.** Threaded and concurrent-read re-merges completed
   without crashes. Target-specific minimization differs: the threaded target
   retained 168 shared inputs, while its full-source merge retained 175; the
   concurrent-read target retained 173 shared inputs, while its full-source
   merge retained 231. Coverage edges are not directly comparable across
   targets, so the numbers are retained as per-target evidence only.
3. **Inconclusive flakiness lead: final replay coverage is not an exact stable
   metric.** Two replays of each fixed reduced corpus used the same binary,
   corpus, `-seed=242`, and limits. Their final coverage was:

   ```text
   dbwrapper:                 16033 vs 16037
   dbwrapper_threaded:        17953 vs 17923
   dbwrapper_concurrent_reads:17750 vs 17764
   ```

   All repeats were crash-free with the same executed-unit counts. The target
   code intentionally includes background work and concurrent reads, while
   `-runs=1` executes the loaded corpus plus a final fuzz iteration. The small
   drift may therefore be scheduling or LibFuzzer final-iteration accounting,
   but the sequential `dbwrapper` drift means this must not be declared solved
   without a narrower replay experiment. Use `-merge=1` file-set and edge
   results for corpus selection until this is classified.

4. **Dismissed source-defect hypothesis:** no crash, assertion, sanitizer
   report, invalid oracle result, or production-code regression was reproduced
   by the old corpus or either transferred corpus. No source/test change is
   justified by these measurements alone.

### Unrelated leads and limitations

- The repository has no checked-out `qa-assets` corpus in this workspace. No
  external corpus was imported in this cycle; the existing `/data` sparse
  checkout was left untouched.
- The selected binary is a Debug `-O0 -ftrapv` fuzz build, not a sanitized or
  release/high-throughput build. A sanitizer matrix remains untested here.
- The source corpus has historical provenance only through the Cycle 187
  scratch directory; individual input-generation commits are not encoded in
  filenames or content. No online PR was used as evidence this cycle.
- The merge output is build- and target-specific. It must not be promoted to a
  shared project corpus without recording target, source HEAD, binary hash,
  seed, and validation command.
- Existing earlier journals cover the exact prior `dbwrapper` algebraic and
  engine-comparison cells. This cycle intentionally measured corpus transfer
  and minimization instead of reopening those findings.

### Next queue

1. Isolate the replay coverage drift with a pure corpus replay mode or a
   harness that prevents the extra `-runs=1` mutation, then compare repeated
   per-input traces for `dbwrapper` before attributing drift to the target.
2. Repeat fixed-seed merge/replay under the project's sanitized libFuzzer build
   and, when available, a high-throughput build; record whether the selected
   set and crash/regression behavior change.
3. If the qa-assets corpus becomes available, inventory provenance and perform
   a structurally related transfer against this target family without mixing
   its files into the current scratch set.

### Close condition

The selected goal completed one evidence-backed corpus cycle. The strongest
result is a reproducible fixed-seed minimization and cross-target transfer
ledger, with no production defect confirmed. Continue from the next queue;
do not claim corpus exhaustion.
