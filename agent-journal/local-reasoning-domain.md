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

### Status

Investigation in progress. No source change has been made in this cycle.
