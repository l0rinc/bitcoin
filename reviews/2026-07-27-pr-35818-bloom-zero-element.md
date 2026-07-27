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

## Reachability of the leftover (follow-up, 2026-07-27)

The residual `nFPRate==0 && nElements>0` UB is NOT remotely accessible: the 4-arg
constructor's only call sites are the unit test and fuzz target; BIP37 `filterload`
uses the default constructor + Unserialize (wire data, no formula evaluation);
net_processing.cpp:5073 is the copy ctor. Not a vulnerability — latent UB for local
library callers only; fix remains worthwhile for UB-hygiene and fuzz-domain coverage.

## Correction (2026-07-27, prompted by Codex review)

My "one-line guard closes the class" was WRONG. The defect class is wider:
rate <= 0 (NaN), rate > 1 (negative size), non-finite (NaN/+-inf), AND finite
pre-clamp overflow — UINT_MAX elements at rate 0.01 yields ~4.12e10 before
conversion (verified numerically), which is out of unsigned range even inside the
"valid" (0,1] domain because std::min clamps AFTER the conversion, not before.
A complete fix is a real parameter-validation change: define the accepted rate
domain (0 < rate <= 1), handle non-finite values deliberately, and clamp in
floating-point space before narrowing. That is separate scope from #35818 and
should not ride along on it.

Codex nuances: (a) their "assert disappears in release" is wrong for THIS project
(Core strips -DNDEBUG; asserts stay live) — but a bare assert(nFPRate > 0) is
still insufficient since it says nothing about rate > 1, non-finite, or the
finite overflow; (b) 133020edfd/d6472bc4aa are review-doc commits on the local
audit branch, not proposed patches for the PR; (c) their internal-target point
(bitcoin_common static lib, no public API) further narrows the leftover's reach.
