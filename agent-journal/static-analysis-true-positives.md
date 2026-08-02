# Journal: static-analysis true-positive campaign (campaign 12)

Uber-goal rotation. Branch: audit/static-analysis-true-positives from
audit/resurrection @ 1ffa7d2747. Tools on host: clang 18 --analyze (CSA);
no clang-tidy installed (CI covers it; no system-package installs per
workspace rules). Prior: campaign 97 verified tidy CI config.

## Cycle 1: clang static analyzer on 4 translation units

- src/netaddress.cpp (rotation's GetNetClass change): CLEAN.
- src/wallet/scriptpubkeyman.cpp (rotation's W4 + TopUpWithDB changes): CLEAN.
- src/net_processing.cpp: CLEAN (zero diagnostics).
- src/txmempool.cpp: 1 report — "Division by zero" at policy/feerate.h:54
  (rhs.size / common_divisor in ExactFeeRateSum).

### Classification of the one report: FALSE POSITIVE (documented pattern)

Guards are explicit: Assume(lhs.size > 0) and Assume(rhs.size > 0)
(feerate.h:49-50), common_divisor = std::gcd of two positives ≥ 1, and
Assume(common_divisor > 0) at line 53 — one line BEFORE the division at
54. CSA does not model std::gcd's range or project Assume semantics.
Pattern recorded so future cycles don't re-litigate: "CSA flags division
after Assume(d > 0) on gcd" → false positive by construction.

## Campaign 12 cycle complete
Zero true positives; one documented false positive. Rotation: uber-ledger
marks #12 DONE, next #13.