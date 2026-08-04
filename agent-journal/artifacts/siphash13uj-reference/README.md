# SipHash-1-3-UJ independent reference (cycle 333, goal #81)

`siphash13uj_ref.py` — Python reference implementation written ONLY from the
documented construction in `src/crypto/siphash.h` (not from the C++ code).

Replay: `python3 siphash13uj_ref.py` from the repo root — checks all pinned
vectors in `src/test/data/siphash.json` (64 with siphash13uj, 146 siphash24),
the jumbo-zero-extension structural invariant (1000 random cases), and the
empty-input edge. Expected tail: `mismatches=0`, invariant OK.

Cycle-333 result: all match -> conformance DISMISSED as a defect source.
Caveat recorded in the journal: parse expected values with `int(x, 16)`
(big-endian JSON hex), not int.from_bytes(..., 'little').
