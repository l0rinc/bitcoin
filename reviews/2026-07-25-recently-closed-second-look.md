# Recently closed/merged PRs — second-look pass (2026-07-25)

37 PRs closed/merged since 2026-07-20. Almost all were already covered by the watches (SipHash series, BIP32 SetSeed assert, submitSolution, global tx rate limit, inbound capacity, vsize RPCs, descriptorprocesspsbt, extra tx count, secp subtree, constexpr, testnet seed). Three needed a fresh look:

- **35769 (zeromq upstream patch, merged)** — 🟢 clean. Vendored libzmq PR #4911 (missing-include fix for llvm-mingw) as `add_new_include.patch`; build-only, no protocol change. Verified the strip level this time: libzmq's tarball keeps `src/` at top, patch paths are `a/src/…`, so `-p1` is correct (the 35796 lesson applied).
- **35727 (blockencodings: fix extra transaction count, merged)** — not a duplicate: this PR carried l0rinc's `be4e64d9e4` + `6aa5d8d948`, the same fix verified in depth earlier (slot-state machine, all edge cases clean).
- **35784 (txdb: Check GetKey() when priming cursor, closed unmerged)** — the FOURTH duplicate of the first-cursor-key fix (issue #35172), after 35191/35248/35654. sedited's "AI treadmill" in action; correctly closed — 35654 remains the canonical fix.
- **35767 (fuzz: dangling prevoutfetch threads after AFL fork, merged)** — fuzz-harness only; production prevout threads unaffected.

Nothing else in the closed set warrants a second look (doc/refactor drive-bys closed unmerged, test/build merges).
