# Compile-time hardening candidates (2026-07-26)

Survey of what runtime work can move to compile time, following the established
`_hex`/`_hex_u8`, `_MiB`, and printf-hardening patterns. Already done upstream:
bech32 GF tables (constexpr), chainparams genesis script (`_hex`), `uint256{"..."}`
literal, `std::chrono_literals` imported project-wide (util/time.h:23), `_MiB`/`_GiB`.

## Tier 1 — recommended (prod+test, easy to justify, safety + simplification)

### 1. Raw second-arithmetic → chrono literals (6 prod sites + 2 test)
Self-documenting units; kills arithmetic-typo risk; identical values at compile time.

For `std::chrono::seconds`-typed constants — drop-in:
- `src/net.cpp:86` `MAX_UPLOAD_TIMEFRAME{60 * 60 * 24}` → `{24h}`

For int/int64-typed constants — use existing `TicksSeconds` (util/time.h:88, constexpr):
- `src/banman.h:19` `DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24` → `TicksSeconds(24h)` (keep unsigned type, or migrate type to chrono::seconds and adjust ~2 call sites)
- `src/net_processing.cpp:119` `STALE_RELAY_AGE_LIMIT = 30 * 24 * 60 * 60` → `TicksSeconds(30 * 24h)`
- `src/net_processing.cpp:122` `HISTORICAL_BLOCK_AGE = 7 * 24 * 60 * 60` → `TicksSeconds(7 * 24h)`
- `src/validation.cpp:2385` `TWO_WEEKS_IN_SECONDS{60 * 60 * 24 * 7 * 2}` → `{TicksSeconds(14 * 24h)}`
- `src/chain.h:29` `MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60` → `TicksSeconds(2h)`
- tests: `src/test/util_tests.cpp:427` and `src/test/fuzz/parse_iso8601.cpp:115` `SECONDS_PER_DAY{24*60*60}` → `TicksSeconds(24h)`

### 2. Test ParseHex → `_hex`/`_hex_v_u8` literals (18 sites)
Files: `src/test/crypto_tests.cpp`, `src/test/psbt_tests.cpp`, `src/test/util_tests.cpp`.
Every `ParseHex("...")` on a literal becomes a compile-time-validated literal:
typos are compile errors instead of confusing test failures; no runtime parse.
Drop-in mapping: `ParseHex` (returns vector<unsigned char>) → `_hex_v_u8`;
array-compatible consumers → `_hex`/`_hex_u8`. Matches the established migration.

## Tier 2 — worthwhile, trivial

### 3. `src/base58.cpp` `CHARSET_REV[128]` → constexpr
Currently `const int8_t [128]` with a constant-expression initializer (optimizer-
dependent folding). Making it `constexpr` moves the guarantee from optimizer to
language and documents intent. One word.

## Explicitly NOT candidates (documented to avoid re-suggesting)

- `ParseHashV`/`Txid::FromHex` on RPC inputs (transactions.cpp, spend.cpp, backup.cpp) — runtime user input.
- `ParseHDKeypath` test cases (psbt_wallet_tests.cpp) — the parser's runtime behavior is the thing under test.
- bech32 GF tables — already constexpr (`GenerateGFTables()`).
- chainparams genesis script/hash — already `_hex` + `uint256{"..."}`.
- JSON test vectors — data files, not code.
- Subsidy/genesis computation — consensus-relevant; table-izing adds risk, no benefit.
- `bitcoin-tx.cpp:624` `Txid::FromHex` — runtime JSON input.

## Justification summary
Tier 1a: mechanical, prod-visible, mirrors chrono-literal availability already in tree.
Tier 1b: exactly the class the `_hex` work was built for, confined to tests, zero behavior change.
Tier 2: one-word guarantee upgrade. All three keep semantics identical — verifiable by
`static_assert(new == old)` per constant and existing test suites.

## Implementation status (2026-07-26, branch `compile-time-hardening`)

- ✅ Tier 1a committed as `79026584e2` (chrono literals, 6 prod sites). Note: the two test
  chrono sites (util_tests/parse_iso8601 SECONDS_PER_DAY) did not exist on current master —
  scope reduced to the 6 prod sites.
- ✅ Tier 2 committed as `203dc7b58f` (base58 mapBase58 constexpr).
- ⚪ Tier 1b (test ParseHex): dropped — already migrated upstream; only parser-behavior
  tests remain (deliberately excluded).
- Verification: value identities proven via static_asserts; full build green; 10/10
  affected ctest suites pass.
