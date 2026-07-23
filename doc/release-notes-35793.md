Mempool policy
--------------

- Transactions with more than 2,500 legacy (non-Segwit) signature operations in
  their inputs, or with a witness-stripped size of exactly 64 bytes, are now
  rejected by mempool acceptance as consensus failures on every network,
  including those where the BIP54 deployment is never active. The 64-byte case
  was already rejected by relay policy, and the sigop limit was previously an
  input standardness check which could be disabled on test networks with
  `-acceptnonstdtxn=1`. The rejection reasons are `bad-txns-legacy-sigops` and
  `txn-size-64`, and the sending peer is not disconnected. (#35793)

Regtest
-------

- The BIP54 consensus cleanup rules are now always enforced on regtest. An
  existing regtest datadir may contain blocks which do not satisfy them, which a
  full `-reindex` would reject. Clients of `getblocktemplate` must include
  `"consensuscleanup"` in the request's `rules`, and templates advertise the rule
  as mandatory (`!consensuscleanup`). The deployment can be disabled with
  `-vbparams=consensuscleanup:-2:0`. (#35793)
