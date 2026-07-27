# Review: PR 35818 "bloom: support zero-element filters" + 35819, 35814 (2026-07-27)

## PR 35818 (l0rinc, bloom zero-element) — 🟢 correct with one leftover 🟡

The two defects are real and the guards fix them:
1. `nHashFuncs` formula `vData.size() * 8 / nElements * LN2` — integer division by zero at nElements=0 (UB/trap).
2. `vData` formula `nElements * log(nFPRate)` — `0 * log(0)` = NaN → unsigned conversion (UB).

The `nElements == 0 ? 0 : …` guards produce the correct empty-filter state (empty vData, 0 hash funcs), and `contains()` with 0 hash functions vacuously matches everything — the mathematically consistent empty-product result, now documented by the test comment. The unit test's feclearexcept check pins the NaN (FE_INVALID) case, and the fuzz change maps nElements==0 to rate 0 so UBSan reproduces each defect with the guards removed.

**Leftover 🟡: `nFPRate == 0` with `nElements > 0` is still unguarded.** With rate exactly 0: `log(0) = -inf`, `nElements * -inf = -inf`, `-1/LN2SQUARED * -inf = +inf`, and `(unsigned int)(+inf)` is UB — reachable via the same 4-arg constructor. The PR's own description scopes to the zero-element NaN case, and the fuzz mapping (`n_elements == 0 ? 0.0 : rate`) deliberately avoids rate=0-with-elements, so the residual case is neither guarded nor exercised. Suggest either extending the guard (e.g. `nFPRate <= 0 → clamp vData to MAX_BLOOM_FILTER_SIZE` or reject rate ≤ 0 explicitly) or asserting rate > 0 in the ctor. One-line fix; without it the UB class isn't fully closed.

Minor: the fetestexcept check only pins the FE_INVALID (NaN) defect; the int div-by-zero isn't an FP exception (that's what the UBSan path covers). Fine as-is.

## PR 35819 (test: descriptor parse error paths, azuchi) — 🟢 test-only, skim clean.
## PR 35814 (contrib: autogenerate bitcoin-cli completion from getopenrpcinfo) — 🟢 contrib+test only; generated file + sync test, sensible.

Marker → 35819. Known issues unchanged (32387/26022/27409/35387/30437 still open).
