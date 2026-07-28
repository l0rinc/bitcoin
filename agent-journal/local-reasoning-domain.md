# Local Reasoning Domain and Relationship Audit

## Cycle 65: AddrMan network classification relationship

### Cycle identity and gate

- Draw command: `shuf -i 0-98 -n 1`
- Draw: `57`
- Selected goal: `local-reasoning-domain`
- Worktree: `/data/my_storage/bitcoin`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- HEAD at cycle start: `d7109ee6a310bbfeac419e3f0833910ee2454570`
- `origin/master`: `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Merge base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`
- Divergence: `origin/master...HEAD = 2 901`
- Catalog/protocol/TSV hashes matched the authoritative values.
- Tracked and staged state was clean; only known agent-owned untracked artifacts were present. No relevant process was running.

### Scope and hypothesis

Audit local reasoning domains where related APIs use different address relationships. The selected candidate is AddrMan's network-filter path: `m_network_counts` and `Select_` use `CNetAddr::GetNetwork()`, while `GetAddr_` and the public `Select` contract use `CNetAddr::GetNetClass()`.

The falsifiable hypothesis is that a valid, routable linked-IPv4 address, such as RFC3964 6to4 or RFC6052 IPv4-embedded IPv6, can be counted as `NET_IPV6` but returned as `NET_IPV4`. If so, `Size`, `Select`, `GetAddr`, and their public contracts disagree for a reachable address domain. The trust boundary is a caller requesting peers by network; no unsupported internal object state is needed.

### Evidence plan

1. Confirm the classification difference for each supported linked-IPv4 representation and trace the history that introduced network-filtering and per-network counts.
2. Add a disposable focused unit assertion for a valid linked-IPv4 address in AddrMan, covering all relevant table states and both network filters.
3. Run the old-source control and a temporary candidate repair independently. Require a failing-before contract assertion and a passing-after result, then run the full AddrMan suite and sanitizer/fuzz smoke where available.
4. Check all callers and review precedent before deciding whether the correct repair is to use `GetNetClass()` consistently or to document an intentional distinction. Remove disposable scaffolding unless the regression is justified for retention.

### Initial history evidence

- `CNetAddr::GetNetClass()` intentionally maps routable IPv4, RFC6145, RFC6052, RFC3964, and RFC4380 forms to `NET_IPV4` through `HasLinkedIPv4()`.
- `AddrMan::GetAddr()` already filters with `GetNetClass()` and its public postcondition asserts the same classification.
- The per-network-count change (`d35595a78a`) and network-selected `Select` change (`6b229284fd`) count/filter with `GetNetwork()`.
- The multi-network change (`829becd990`) preserved that `GetNetwork()` filter, while later contract checks added a `GetNetClass()` postcondition. This is the primary suspected cross-layer drift.

### Contract result

The suspected product-wide classifier mismatch is intentional and must remain split:

- `Size(network)` and `Select(network)` use `GetNetwork()`, the transport-level network used by reachable-network and `-onlynet` logic.
- `GetAddr(network)` uses `GetNetClass()`, the legacy public address-list classification. A linked IPv4 address is therefore included by `GetAddr(NET_IPV4)` even though its transport network is `NET_IPV6`.

The current-branch defect was the postcondition added by the earlier AddrMan contract campaign: `AddrManImpl::Select()` asserted `GetNetClass()` against a selection performed with `GetNetwork()`. The same wrong relationship was duplicated in the AddrMan deterministic test oracle and fuzzer oracle.

### Before and after evidence

The disposable `addrman_linked_ipv4_network_contracts` test exercised four valid, routable forms: RFC6145, RFC6052, RFC3964, and RFC4380. Each had `GetNetwork()==NET_IPV6` and `GetNetClass()==NET_IPV4`.

- Clean pre-fix build: the test first showed `Size(NET_IPV4)==0`, `Size(NET_IPV6)==4`, and `Select({NET_IPV4})` empty while `GetAddr(NET_IPV4)` returned all four. Calling `Select({NET_IPV6})` then aborted at `src/addrman.cpp:1208` because the postcondition required `GetNetClass()==NET_IPV6`.
- Repair: changed only the `Select` postcondition and the matching deterministic/fuzzer oracles from `GetNetClass()` to `GetNetwork()`. The focused test passed 29 assertions; the full AddrMan suite passed 28 cases and 14,346 assertions.
- Mutation: temporarily restored the old `GetNetClass()` postcondition. The focused test again aborted at `src/addrman.cpp:1208` with exit 134. The mutation was restored.

The normal libFuzzer `addrman` corpus replay was independently attempted with `-runs=1000 -seed=6501`; it stopped at execution 117 on an existing `AssertSerializationRoundTrip` assertion at `src/test/fuzz/addrman.cpp:206`, before this classifier path was established. That result is preserved as a separate fuzz-oracle limitation, not attributed to this fix. A companion `addrman_serdeser` replay with `-runs=500 -seed=6502` reached 732 executions at about one execution per second without a diagnostic, then was interrupted after roughly ten minutes at the execution boundary; libFuzzer reported 1,837 MiB peak RSS. An empty-corpus `addrman` smoke with `-runs=100 -seed=6503` completed cleanly, adding three units at about 50 executions per second with 1,850 MiB peak RSS. The interrupted corpus replay is inconclusive only for that large corpus, while the focused unit, full AddrMan suite, mutation, and empty-corpus smoke provide the selected-path evidence.

### Status

Confirmed and repaired in the current branch by `b2d858ae4e` (`addrman: match Select contract to transport network`). The production behavior remains transport-network based; only the invalid postcondition and duplicated test/fuzzer relationship were corrected. The focused and full unit builds, fuzz-target rebuild, mutation control, and clean empty-corpus smoke passed. The large corpus replay was stopped at a documented resource boundary, and its unrelated serialization assertion is retained as a limitation.
