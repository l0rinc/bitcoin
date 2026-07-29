# Suspicion/artifact index (durable, append-only)

Campaign #102 deliverable. Machine-usable inventory of preserved
artifacts (scripts, seeds, traces, probes) with provenance and
resume points. Schema: id | artifact (path) | origin (campaign/commit)
| trust boundary | status | confidence | resume command/point.

A1 | /tmp/btc50_seed/{psbt_min,psbt_1in} | #50 c1 (RPC-verified valid
  PSBTs; constructors in fuzz-introspector-blockers.md) | PSBT wire
  format | ACTIVE corpus | high | FUZZ=psbt build_fuzz/bin/fuzz
  -runs=N /tmp/btc50_seed
A2 | /tmp/btc101_seed/ (+ psbt_*_whole 0x00-mode-byte re-points) |
  #101 c1 (d086164661) | same | ACTIVE corpus (hybrid harness) |
  high | same, /tmp/btc101_seed
A3 | /tmp/btc101_iso/psbt_1in_whole | #101 c1 isolation seed (528 ->
  2857 edges) | same | REFERENCE | high | FUZZ=psbt .../fuzz -runs=0
  -print_final_stats=1 /tmp/btc101_iso
A4 | /tmp/lw_crash_flags_seed | #10 c2 (fd74c4a7c2) load_wallet
  bring-up crash = harness-oracle bug | wallet-db records |
  PRESERVED negative result | high | FUZZ=load_wallet .../fuzz
  /tmp/lw_crash_flags_seed
A5 | /tmp/btc43_lifecycle.py | #43 c2 | -capturemessages file
  lifecycle | PROVEN reproducer (append + abort rc=-6) | high |
  python3 /tmp/btc43_lifecycle.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc43 (RLIMIT note: none needed)
A6 | /tmp/btc80_diff.py | #80 c1 | PSBT C++/Python parser
  differential | PROVEN differential harness (A=0/400) | high |
  same pattern, --tmpdir=/tmp/btc80 (has 4 GiB RLIMIT_AS guard +
  E>0 positive control)
A7 | /tmp/btc106_probe.py | #106 c1 | hex-decode trailing-garbage
  twins | PROVEN probe | high | --tmpdir=/tmp/btc106
A8 | /tmp/r23.perf (+r23d.perf) | #23 c1 | ComplexMemPool perf
  attribution | SUPERSEDED by HEAD measurement (fix in lineage) |
  medium | regenerate via perf record -F 199 bench_bitcoin
  -filter='^ComplexMemPool$'
A9 | /tmp/r6_txoutproof_probe.py + /tmp/r6_probe_result.txt | #6 c2
  | txoutproof RPC composition | SUPERSEDED (battery in tree,
  rpc_txoutproof.py) | high | battery now in-tree
A10 | L1 CBloomFilter sizing-ctor zero-input UB | #106-index L1 |
  bloom | LATENT — unreachable (replay-verified 2026-07-29:
  production never calls sizing ctor; fuzz clamps >=1) | high |
  watch l0rinc PR 35818 (open, 2026-07-28)
A11 | kernel input_index assert (bitcoinkernel.cpp:717) | #46 c1 |
  kernel C API | NOTE — upstream WIP precondition, assert-guarded |
  medium | revisit when kernel API stabilizes
