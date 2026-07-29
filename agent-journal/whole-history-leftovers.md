# Campaign #32 — whole-history-leftovers

Base: 1943091503 (journal commit for #71 cycle-2 on
audit/deterministic-sim-c2; ledger-lineage anchor audit/resurrection @
5d0155254c). Branch: audit/history-leftovers. Start state: clean
(untracked scratch only).

## Cycle 1 (2026-07-29): multiply-first percentage shape (seed 21a97c2721) — no exploitable survivor in HEAD

### Draw
Random draw over the 46-goal pool (29 pending + 17 CYCLE-1; #71
excluded as just-cycled): raw=15264671492345946424, seed masked to 63
bits (6041299455491170616), index 2 -> #32.

### Seed and shape
Fork fix 21a97c2721 (caches.cpp): `total_cache * 10 / 100` wrapped
uint64 at -dbcache=2^43 MiB, yielding a zero index cache before
std::min could cap; fixed to division-first budgets with unit +
fuzz boundary oracles (its commit message carries the full mutation
proof). Repository-wide rule extracted: any `x * K / D` percentage/
proportion computation where x can be large must be checked for
multiply-first overflow (or narrowing after the multiply).

### HEAD sweep (production src, subtrees/tests excluded)
1. mapport.cpp:114 `actual_lifetime * 5 / 8` — int64 chrono seconds;
   realistic leases ≤ 2^32 s (overflow needs ~73-billion-year
   lifetimes). SAFE BY RANGE.
2. base58.cpp:54 `strlen(psz) * 733 / 1000 + 1` (into int) and :99
   `input.size() * 138 / 100 + 1` (into int) — the multiply is
   size_t (64-bit); truncation to int needs input > ~2.9 GB, which is
   memory-bound before it is arithmetic-bound. BOUNDED BY MEMORY —
   recorded, not a defect.
3. addrman.cpp:797 `Assume(max_pct <= 100)` — pinned.
4. mempool_persist.cpp:114 `100.0 * txns_tried / total` — double
   math, no integer wrap.
5. kernel_notifications progress_percent — pass-through int, no
   arithmetic.

### Verdict
- DISMISSED: no surviving multiply-first overflow site reachable from
  untrusted input in HEAD. The fork's seed fix plus its boundary
  tests/fuzzer cover the original site; siblings are range- or
  memory-bounded.
- Blame discipline honored: the base58 sites are pre-fork upstream
  code; their bound was computed, not assumed (2^31/733 ≈ 2.9M chars
  WOULD be reachable IF the multiply were 32-bit — it is size_t;
  verified from the actual expression, not the comment).

### Exact commands
- git show 21a97c2721 --format=%B
- grep -rnE '\* *[0-9]+ */ *[0-9]+' and '\) *\* *[0-9]+ */ *[0-9]+' over
  production src; per-site range analysis above

### Limitations / queue
- One seed family this cycle. Queued seeds for c2: the saturation
  arithmetic family (3ae78dbd25 lineage), the fork's BDB overflow
  chain bound (71cf0ba593), and upstream-era CVE fix shapes whose
  siblings may survive (e.g., the 2018 inventory-dos family).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
