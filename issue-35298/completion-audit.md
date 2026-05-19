# Completion Audit

Date: 2026-05-18 UTC

## Objective Checklist

| Requirement | Evidence | Status |
| --- | --- | --- |
| Investigate #35298 and relevant Core PRs #30611/#30039 | `issue-35298/report.md` sources section lists issue/PR links and prior art. Local code inspection covered `validation.cpp`, `dbwrapper.cpp`, LevelDB compaction code, and cache sizing. | Covered |
| Use local datadir without deleting/reindexing/corrupting | Run used `/mnt/my_storage/BitcoinData`; no reindex or destructive operation was run. Node was stopped cleanly after reproduction. | Covered |
| Reporter-like config | `issue-35298/runs/20260518T182043Z/start-command.sh` records `server=1`, `-nosettings`, `-txindex=0`, `-dbcache=800`, `-maxmempool=100`, `debug=leveldb`, `debug=bench`, `debug=coindb`. | Covered |
| Document commit/version/OS/filesystem | `run.env`, `git-head.txt`, `bitcoind-version.txt`, `findmnt.txt`, and report metadata. | Covered |
| Ensure fully synced before steady-state measurement | `summarize_io.py --steady-state --fully-synced` found `2026-05-18T19:14:04Z`, `blocks=headers=949977`, `initialblockdownload=false`; report excludes startup/catch-up wave. | Covered |
| Reproduce high chainstate writes after fully synced | Wave `2026-05-18T21:25:41Z` to `21:40:34Z`, 8.445 GiB generated, process write 8.452 GiB over the wave window. | Covered |
| Determine txindex relation | Reproduction command used `-txindex=0`; issue comments include both txindex and no-txindex reports. | Covered |
| Parse LevelDB waves with levels/tables/bytes/timing | `parse_compactions.py` output in report and rerun during audit. | Covered |
| Track chainstate file age/size | `chainstate_age.py` output in report and audit, showing 8.448 GiB / 79.43% touched within 1 hour after reproduction. | Covered |
| Measure process/device I/O | `proc_io.tsv`, `diskstats.tsv`, and `summarize_io.py`; report emphasizes process I/O because device I/O was noisy from concurrent build activity. | Covered |
| Capture UpdateTip cache and BatchWrite/FlushState logs | `parse_compactions.py` extracts bench/flush context and UpdateTip cache range. Report quotes the last periodic flush before the large wave. | Covered |
| Analyze periodic flush vs compaction behavior | Report distinguishes small immediate periodic compactions from delayed no-flush large wave. | Covered |
| Analyze 2 MiB to 32 MiB migration | Report notes chainstate was mostly 32 MiB-ish files and reporter `-forcecompactdb=1` did not stop recurrence. | Covered |
| Analyze dbcache/write-buffer interaction | Report notes `coins_db` cache cap is 8 MiB and chainstate `write_buffer_size` is about 2 MiB. | Covered |
| Determine whether waves settle after 1-2 days | Local 1-2 day run was intentionally stopped after reproduction to avoid continued SSD writes; issue comments include multi-day persistence evidence (about 12 days at high write rate). | Covered with external evidence and documented limitation |
| Evaluate mitigations and side effects | Report includes mitigations, tradeoffs, expected side effects, and why `forcecompactdb`, interval changes, and write-buffer changes are incomplete fixes. | Covered |
| Attempt fix if warranted | Local patch disables LevelDB seek-triggered compaction globally by making `Version::UpdateStats()` a no-op while leaving size/manual compactions enabled. | Covered |
| Run tests | Build completed; `dbwrapper_tests`, `validation_flush_tests`, and `coins_tests` passed. | Covered |
| Leave durable final report | `issue-35298/report.md`. | Covered |

## Verification Commands Rerun

```sh
git diff --check
python3 issue-35298/parse_compactions.py --since 2026-05-18T19:19:39Z --until 2026-05-18T21:42:00Z issue-35298/runs/20260518T182043Z/debug.log
python3 issue-35298/summarize_io.py --since 2026-05-18T21:25:00Z --until 2026-05-18T21:42:00Z issue-35298/runs/20260518T182043Z
python3 issue-35298/chainstate_age.py /mnt/my_storage/BitcoinData/chainstate
build-35298-seekfix/bin/test_bitcoin --run_test=dbwrapper_tests
build-35298-seekfix/bin/test_bitcoin --run_test=validation_flush_tests
build-35298-seekfix/bin/test_bitcoin --run_test=coins_tests
TEST_TMPDIR=/data/my_storage/tmp/leveldb-tests /data/my_storage/tmp/leveldb-manual-tests/db_test
TEST_TMPDIR=/data/my_storage/tmp/leveldb-tests /data/my_storage/tmp/leveldb-manual-tests/autocompact_test
pgrep -af '[b]itcoind|[m]onitor\.sh|[c]make --build build-35298-seekfix|[n]inja.*build-35298-seekfix' || true
```

All checks passed. No `bitcoind`, monitor, or build process remained running at audit time.
