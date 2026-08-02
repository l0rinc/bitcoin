# Campaign #18 — exhaustive-algebraic (script compression algebra)

Base: audit/resurrection @ 26e59b8ec8 (rotation ledger commit for #14).
Branch: audit/exhaustive-algebraic. Working state at cycle start: clean except untracked scratch (node_modules/, package*.json, txindex-size.log).

## Cycle 1 (2026-07-27): CompressScript/DecompressScript round-trip oracle gap — CLOSED with mutation-verified test

### Hypothesis
`src/compressor.cpp` defines a bijection between compressible script classes
(0x00 P2PKH, 0x01 P2SH, 0x02/0x03 compressed pubkey, 0x04/0x05 uncompressed
pubkey) and their compact encodings. The UTXO set serializes every coin's
scriptPubKey through `ScriptCompression` (compressor.h:68-99), so the identity
`DecompressScript(nSize, data) == script` whenever `CompressScript(script, out)`
succeeds is a correctness invariant of consensus-critical persistence code.
`src/test/compress_tests.cpp` byte-checked compression output (size, memcmp of
fields, class byte) but never asserted the round-trip identity — an oracle gap.

### Trust boundary
Consensus persistence (UTXO set coins.dat + LevelDB chainstate). A decompressor
regression would corrupt every spend read back from disk.

### Work done
1. Added `compress_script_roundtrip` at end of src/test/compress_tests.cpp
   covering all four classes via a `roundtrip` lambda.
2. First build/run FAILED all 4 checks — investigated: harness bug, not a
   production bug. `DecompressScript(script, nSize, in)` takes the class byte
   as `nSize` and only the DATA portion of the CompressedScript as `in`
   (compressor.cpp:98-146 memcpy's 20/32 bytes from in.data() with no class
   byte offset). Confirmed against the only production caller,
   `ScriptCompression::Unser` (compressor.h:81-88), which reads the class byte
   as VARINT, then reads exactly `GetSpecialScriptSize(nSize)` (20 or 32)
   bytes into a separate buffer. Fixed the lambda to pass
   `{out.begin() + 1, out.end()}`; test green.
3. Mutation-verified the oracle: temporarily changed
   `script[1] = nSize` -> `script[1] = nSize ^ 0x01` (wrong parity byte) in
   the 0x02/0x03 case of DecompressScript; rebuilt;
   `test_bitcoin --run_test=compress_tests/compress_script_roundtrip` reported
   exactly 1 failure (the compressed-pubkey case, as expected); reverted the
   mutant; rebuilt; `compress_tests` suite green
   (`*** No errors detected`). compressor.cpp left byte-identical to HEAD
   (git diff empty).

### Verdict
- CONFIRMED oracle gap (test-only; no production defect). Closed by commit
  8e7513bb1c "test: assert CompressScript/DecompressScript round-trip identity"
  on audit/exhaustive-algebraic.
- Interface-contract note (documented in test comment): DecompressScript's
  `in` excludes the class byte — a natural misuse trap, now pinned by the test.

### Exact commands / key output
- `cmake --build build-before -j4 --target test_bitcoin` -> link ok
- `./build-before/bin/test_bitcoin --run_test=compress_tests` ->
  `*** No errors detected`
- Mutant run: `--log_level=message | grep -c "has failed"` -> `1`

### Limitations
- Random keys per run (existing file style via GenerateRandomKey/m_rng);
  class coverage is deterministic, point values are not.
- Non-compressible scripts still only covered by Ser/Unser paths elsewhere;
  not part of this identity (CompressScript returns false by design).

## Cycle 2 (2026-07-28): Coin composition round-trip + fuzz-gap adjudication

### Fuzz-gap item from cycle-1 queue — DISMISSED (already covered)
The queued "script_compression fuzz gap check" resolved by reading, no new
target needed:
- script identity: src/test/fuzz/script.cpp:94-101 asserts
  `DecompressScript(size, data) == script` (with the correct wire split) and
  `compressed.empty()` on failure.
- amount identity: src/test/fuzz/integer.cpp:74-79 asserts
  `u64 == DecompressAmount(CompressAmount(u64))` for u64 <= MAX_MONEY plus
  `compressed <= CompressAmount(MAX_MONEY - 1)` (top-bound).
Combined with the #28 mutation battery (5/5 mutants killed on
CompressAmount/DecompressAmount vs compress_tests), the primitive round-trips
are covered at unit + fuzz layers.

### Composition item — oracle gap CONFIRMED, closed with mutation-verified test
`ccoins_serialization` (coins_tests.cpp:719) decoded canonical Coin encodings
and checked fields but never re-serialized: the Coin composition
VARINT(height<<1|coinbase) + TxOutCompression (AmountCompression +
ScriptCompression), coins.h:76-90 / compressor.h:123-126 — the exact byte
format of every UTXO set entry on disk — had no round-trip oracle.
Added `ccoins_serialization_roundtrip` (coins_tests.cpp, commit 4c27dad486):
- 3 canonical encodings re-serialize to byte-identical bytes;
- constructed round-trips: 6 script classes (all 4 compressible classes +
  non-compressible + empty) x 5 amounts (0, 1, CENT, COIN, MAX_MONEY) x
  4 heights (0, 1, 203998, 2^31-1) x 2 coinbase flags, asserting nValue,
  scriptPubKey, nHeight, IsCoinBase equality.
Mutation-verified: `nHeight << 1` -> `nHeight << 2` in Coin::Serialize ->
182 failed checks; revert -> `*** No errors detected` on coins_tests.

### Exact commands / key output (cycle 2)
- `cmake --build build-before -j4 --target test_bitcoin` (x3: clean, mutant, reverted)
- clean: `--run_test=coins_tests/ccoins_serialization_roundtrip` -> No errors
- mutant: `--log_level=message | grep -c "has failed"` -> `182`
- reverted: `--run_test=coins_tests` -> No errors; `git diff --stat src/coins.h` empty

### Limitations (cycle 2)
- heights are int-cast from uint32 (0x7fffffff max, matching the 31-bit
  bitfield); larger heights impossible by type.
- Byte-identity vectors limited to the 3 existing canonical encodings
  (non-canonical encodings are intentionally accepted on decode; not
  re-serialization-stable by design).

### Next queue for this campaign (future cycles)
- CTxOutUndo / undo-data compression round-trip (undo.h ApplyTxInUndo
  formats) — same composition shape one level up.
- GetSpecialScriptSize/Unknown-class decode robustness beyond the existing
  not-on-curve tests.

## Rotation note
Two bounded cycles complete; rotating per uber-goal policy. Campaign NOT
exhausted — queue above remains for a future epoch.

## Cycle 3 (2026-07-28): TxInUndoFormatter round-trip closed (d666b70a43); GetSpecialScriptSize dismissed

### Draw
Random draw over the 40-goal eligible pool: raw=5255700095403328486,
index 6 -> #18. Executed the c2 queue's first item (undo-data
composition, one level up from the Coin round-trip).

### Target / gap
TxInUndoFormatter (undo.h:18-45) serializes Coin as
VARINT(height<<1|coinbase) + compat dummy (height>0) +
TxOutCompression — the encoding every rev*.dat undo file uses. The c2
round-trip covered Coin's plain path only; the undo path was untested
(grep: no TxInUndoFormatter use in any test).

### Test (d666b70a43)
coins_tests/ctxinundo_serialization_roundtrip: 6 script classes x 5
amounts x 4 heights x 2 coinbase flags, identity + byte-identical
re-serialization; plus a legacy case: nonzero old-version dummy
accepted on read, canonical zero dummy on write.
Authoring trap recorded in the case comment: HexStr(DataStream)
prints the UNREAD remainder — after `stream >> obj` the stream prints
empty, so canonical byte identity must be compared against captured
pre-read bytes (240 initial "failures" were exactly this, diagnosed
from the asymmetric print [long != empty]).

### Oracle sensitivity (staged clean/mutation/repaired)
- clean: No errors detected.
- mutation: Unser reads nCode >> 2 (was >> 1): 63 check failures.
- repaired (git checkout src/undo.h): full coins_tests suite green.

### Second queue item: GetSpecialScriptSize/Unknown-class decode
Dismissed by reading: the Unknown class stores the full script
(header byte + raw), the 0x06 cap is enforced in
ScriptCompression (compressor.cpp) — decode of Unknown-class scripts
is bounded by the serialized size already; not-on-curve handling for
key classes is covered by existing tests. No falsifiable gap left.

### Verdict
- Undo-composition oracle CLOSED, mutation-verified. No production
  defect found (the formatter is exact over the tested domain).
- Campaign status: script (c1), Coin (c2), undo-data (c3) round-trips
  all closed. Remaining candidates are thinner (VectorFormatter bulk
  bounds are shared with covered paths); campaign marked
  queue-complete pending new shapes.

### Exact commands
- `cmake --build build-before -j4 --target test_bitcoin`
- `build-before/bin/test_bitcoin --run_test=coins_tests[/ctxinundo_serialization_roundtrip]`

## Rotation note
Three bounded cycles complete; queue complete per evidence above
(script, Coin, undo compositions all have mutation-verified oracles).
Reopen when new compression classes or formatter shapes land.
