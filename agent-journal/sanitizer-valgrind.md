# Journal: sanitizer and Valgrind true-positive sweep (campaign 11)

Uber-goal rotation. Branch: audit/sanitizer-valgrind from
audit/resurrection @ 47974dc490. Prior coverage: campaign 98 audited
suppressions/CI config (1 stale removed, rest justified) — this campaign
runs the actual diagnostics on this host.

## Cycle 1: ASan+UBSan+fuzzer (build_fuzz, SANITIZERS=undefined,address,fuzzer)

7000+ runs across 7 security-critical targets, zero sanitizer reports:
coins_view (3000), p2p_transport_serialization, blocktransactions_deserialize,
cmpctblock, partially_signed_transaction_deserialize, tx_in_deserialize,
script_flags (1000 each, detect_leaks=1 → LSan active: no leaks).
Classification: clean — no report to minimize.

## Valgrind Memcheck: coins_tests on build-before
(with test/sanitizer_suppressions/valgrind.supp, --error-exitcode=42,
--errors-for-leak-kinds=none so only real errors count)
RESULT: ERROR SUMMARY 0 errors. Leak summary: definitely/indirectly lost
0 bytes; possibly lost 368 bytes in 1 block — classified with
--leak-check=full: glibc DTV (dl-tls.c _dl_allocate_tls) for a leveldb
background worker thread (PosixEnv::Schedule → MakeRoomForWrite →
TEST_CompactMemTable). Dependency-level thread-TLS allocation, same
family as the existing leveldb BackgroundCall suppressions — "possibly
lost" is precisely valgrind's category for this noise. Not a project bug.
Verdict: valgrind clean; no suppression change needed (the block is
covered by the still-reachable class, and possibly-lost from pthread TLS
is expected).

## Campaign 11 cycle complete
ASan+UBSan+LSan: zero reports over 7 targets/7000+ runs. Valgrind: zero
errors, one dependency-TLS possibly-lost block classified. Nothing to
minimize, no true positive. Rotation: uber-ledger marks #11 DONE, next #12.