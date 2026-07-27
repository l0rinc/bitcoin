# Review: upstream PR 35820 — refactor: use chrono literals for durations

Date: 2026-07-27. Head reviewed: 7bb44a4b75 (base: origin/master).
Scope: 16 files, +48/-40. Mechanical conversion of raw-second constants to
chrono literals via TicksSeconds/Ticks helpers.

## Method
Line-by-line value/type equivalence check of every converted constant, plus
destination-type check (int vs int64_t vs unsigned) at each use site.
TicksSeconds is `constexpr int64_t` (src/util/time.h:88).

## Verdict: 🟢 clean — every conversion value- and type-equivalent

| site | old | new | check |
|---|---|---|---|
| banman.h DEFAULT_MISBEHAVING_BANTIME | 60*60*24 | TicksSeconds(24h) | 86400 ✓ unsigned int |
| chain.h MAX_FUTURE_BLOCK_TIME | 2*60*60 | TicksSeconds(2h) | 7200 ✓ int64_t |
| chainparams nPowTargetTimespan (×4 nets) | 14*24*60*60 | TicksSeconds(2*7*24h) | 1209600 ✓ |
| chainparams nPowTargetSpacing | 10*60 | TicksSeconds(10min) | 600 ✓ |
| chainparams regtest timespan | 24*60*60 | TicksSeconds(24h) | 86400 ✓ |
| net.cpp MAX_UPLOAD_TIMEFRAME | seconds{60*60*24} | seconds{24h} | ✓ same type |
| net_processing STALE_RELAY_AGE_LIMIT | 30*24*60*60 | TicksSeconds(30*24h) | 2592000 ✓ int |
| net_processing HISTORICAL_BLOCK_AGE | 7*24*60*60 | TicksSeconds(7*24h) | 604800 ✓ |
| net_processing prune window | 3600 | TicksSeconds(1h) | 3600 ✓ |
| qt/bitcoingui MAX_BLOCK_TIME_GAP | 90*60 | TicksSeconds(90min) | 5400 ✓ |
| qt/guiutil formatNiceTimeOffset | 3600/86400/604800/31556952 | 1h/24h/7*24h/years{1} | chrono::years IS 31556952 s (Gregorian avg) exactly ✓ |
| qt/intro prune estimate | 600, 86400 | 10min, 24h | ✓ arithmetic order preserved |
| qt/modaloverlay | 300 ms, 500000 ms, 3600000 ms | 300ms, 500s, 1h | ✓ |
| qt/rpcconsole ban menu | 60*60(*24*7*365) | 1h/24h/7*24h/365*24h | ✓ (int64→int narrowing harmless: max 31.5M) |
| rpc/blockchain getchaintxstats | 30*24*60*60 | TicksSeconds(30*24h) | ✓ |
| rpc/node mockscheduler | 3600 | TicksSeconds(1h) | ✓ |
| txmempool.h ROLLING_FEE_HALFLIFE | 60*60*12 | TicksSeconds(12h) | 43200 ✓ |
| validation.cpp assumevalid two weeks | 60*60*24*7*2 | TicksSeconds(2*7*24h) | 1209600 ✓ (consensus-relevant: value unchanged) |
| wallet/spend anti-fee-sniping | 8*60*60 | TicksSeconds(8h) | 28800 ✓ |
| wallet rebroadcast | 5*60 | TicksSeconds(5min) | 300 ✓ |

## Notes (nits, not defects)
- The dropped "Average length of year in Gregorian calendar" comment is
  self-documented by std::chrono::years — fine.
- The assumevalid comparison (validation.cpp) is consensus-relevant
  policy-adjacent code; the value is provably unchanged (1209600 s both ways),
  so no behavior delta.
- Not built locally; conversions are constexpr and CI-covered.

## Other checks this run
- (B) origin/master: no new l0rinc-authored commits since 2026-07-24. 🟢
- (A) new fork branches (all l0rinc-authored, other sessions): detached621
  (wallet/fuzz coverage series), detached622 (compressor/base58 constexpr
  tables), l0rinc/compile-time-chrono-hardening (= PR 35820),
  l0rinc/compile-time-hardening (draft, has TODO commit),
  l0rinc/bloom-zero-elements (= PR 35818 branch). No overlap with active
  campaigns (consensus mutation / bug shapes). Reverted sigops commit
  2b6f78006f absent from all new branches. 🟢
- (C) head-unchanged (comments-only updates): PR 35744 (coins: prevent DB
  resize from invalidating cursors, head 1154f96c5f), PR 35818 (bloom:
  support zero-element filters, head b55a50b67e), PR 35753 (kernel: handle
  null mempool on chainstate deletion, head a99b27f192). 🟢
