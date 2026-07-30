Mempool acceptance
------------------

Transactions with more than 2,500 BIP54 legacy sigops or a witness-stripped
size of exactly 64 bytes now receive a consensus-failure result from mempool
acceptance on every network, including networks where the deployment is never
active. The 64-byte size was already rejected unconditionally by relay policy.
The sigop limit was previously an input-standardness check that test-network
users could disable with `-acceptnonstdtxn=1`; it now applies despite that
setting. This changes their RPC rejection reasons but does not disconnect the
sending peer.

Regtest
-------

BIP54 consensus cleanup rules are now always active for newly validated
regtest blocks. Existing regtest datadirs created by older versions may contain
blocks which do not satisfy the new rules. Use a fresh datadir or a full
`-reindex` to apply the rules to stored blocks. `-reindex-chainstate` retains
the existing block index and does not repeat the contextual checks.

External regtest `getblocktemplate` clients must include
`"consensuscleanup"` in the request's `rules` array. Templates advertise the
rule as mandatory (`!consensuscleanup`).

To temporarily accept blocks valid under the previous regtest consensus rules,
start the node with
`-vbparams=consensuscleanup:-2:9223372036854775807`.
