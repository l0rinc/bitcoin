# Harness artifacts — replay index

Preserved fault-injection harnesses with provenance. Each maps to a
finding in findings-index.md. Build/run from the repo root.

## F26 xor.dat short-write (fix 2110abf119)
- `xor_interpose.c` — one-shot LD_PRELOAD fwrite interposer; the
  FIRST 8-byte fwrite writes 1 byte and reports short.
- `xor_experiment.sh` — driver: `xor_experiment.sh prefix|postfix`
  (expects BIN=/mnt/my_storage/bitcoin/build-after/bin; scratch
  datadirs under /tmp/xor-test-*).
- Build: `gcc -shared -fPIC -o /tmp/xor_interpose.so agent-journal/artifacts/xor_interpose.c -ldl`
- Replay: pre-fix bitcoind: `LD_PRELOAD=/tmp/xor_interpose.so` +
  fresh regtest datadir → init error + 1-byte blocks/xor.dat;
  restart without preload → `AutoFile::read: end of file`.
  Post-fix: file removed; restart regenerates 8-byte key, boots.

## F27 snapshot base_blockhash write (fix 3c9090b644)
- `snap_interpose.c` — LD_PRELOAD fopen/fopen64 tracker targeting
  streams whose basename is `base_blockhash`; first fwrite on it
  writes 1 byte and reports short.
- Build: `gcc -shared -fPIC -o /tmp/snap_interpose.so agent-journal/artifacts/snap_interpose.c -ldl`
- `snap_builder2.py` — canonical regtest height-299 chain +
  snapshot builder (framework cache recipe; asserts
  base_hash == 0c552ced... == committed chainparams hash).
  Run: `python3 agent-journal/artifacts/snap_builder2.py
  --configfile=test/config.ini --cachedir=/tmp/snapcache --tmpdir=/tmp/snaptmp`
  (writes /tmp/snapA/utxos.dat).
- Node prep: copy the cache node datadir (199-chain) for node B;
  `submitheader` headers 200..299 from a 299-tip donor; then
  `loadtxoutset /tmp/snapA/utxos.dat` under LD_PRELOAD.

## Framework trap (recorded cycle 296)
Default `setup_nodes` mines +1 IBD-exit block after the cache copy;
recipe-exact chains must override `setup_network` with
`add_nodes` + `start_nodes` only (see snap_builder2.py).

## Goal 126 LevelDB conformance harnesses (cycle 302, verdicts CONFORM)
- `ldb_iter_conformance.cpp` — iterator snapshot isolation under
  concurrent writes/deletes (20 rounds, half-range mutations).
- `ldb_compact_conf.cpp` — pinned iterator survives
  overwrite+delete+full CompactRange with full snapshot view.
- Build (repo root): `g++ -O1 -fsanitize=address,undefined
  -fno-sanitize-recover=all -I src/leveldb/include <file>
  build-after/src/libleveldb.a build-after/src/libcrc32c.a -lpthread
  -o /tmp/<name>` (build-after's libleveldb is ASan-instrumented;
  the harness must link the same sanitizer runtime).

## Goal 127 LevelDB corruption/bg-error harnesses (cycle 304, CONFORM)
- `ldb_corrupt_conf.cpp` — single-byte table corruption ->
  reads surface Status::Corruption, never silent wrong data.
- `ldb_bgerr_conf.cpp` — db-dir rename fault -> background
  compaction error surfaces on the very next Put.
  LESSON: chmod-based fault injection is invalid when running as
  root (CAP_DAC_OVERRIDE bypasses permission checks) — first
  attempt false-negatived 2,400 writes; use dir-rename instead.

## Goal 125 LevelDB crash-recovery harnesses (cycle 308)
- `ldb_manifest_conf.cpp` — MANIFEST byte corruption ->
  DB::Open fails Corruption even with create_if_missing (CONFORM).
- `ldb_wal_conf.cpp` — mid-WAL corruption -> paranoid=true fails
  loud; default keeps intact prefix (kept=1755, torn=0) (CONFORM).
- `ldb_current_conf.cpp` — CURRENT deleted + create_if_missing ->
  opens EMPTY, live tables silently orphaned (VERIFIED HAZARD,
  client-choice contract — not a LevelDB defect).

## xor_tool.cpp (campaign #41 c3, obfuscation-key archaeology)
Modes: dump | corruptkey | delkey | flipcoin — LevelDB-internal
obfuscation-record tooling for scratch chainstates. Build:
`g++ -O1 -I src/leveldb/include agent-journal/artifacts/xor_tool.cpp
build-after/src/libleveldb.a build-after/src/libcrc32c.a -lpthread
-o /tmp/xor_tool` (see #41 c3 journal for usage).
