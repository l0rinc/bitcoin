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
