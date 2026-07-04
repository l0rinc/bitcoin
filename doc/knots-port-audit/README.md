# Knots Port Audit

This note records the main audit findings from replaying Bitcoin Knots changes
onto current Bitcoin Core master.

Branch: `codex/knots-current-master`
Source baseline: Bitcoin Knots `29.x`, `v29.3.knots20260508`
Integration commit: `e8c2b257eeb953dbab930bb38632cc7d9aae733a`
Audit date: 2026-07-04

## Scope

The port was reviewed as an integration and security-audit pass, not as a final
consensus compatibility sign-off. The main focus was on Knots deltas that do not
cleanly replay onto current Core, areas touched by the rebase, and suspicious
changes that looked like possible security or robustness fixes.

## Replay Status

The integration commit reconciles Knots wallet, RPC, net, init, validation,
build, and test-framework changes with current Core APIs. Earlier commits on
this branch already replayed many Knots changes directly; the integration commit
contains the manual conflict resolution and additional tests for changed areas.

Important replayed areas include legacy-wallet import/ownership behavior,
watch-only coin selection, `-paytxfee`, private transaction relay handling, RPC
argument help updates, SQLite feature detection, `bitcoin-wallet` build fixes,
and functional-test framework argument compatibility.

## Core-Missing Hardenings

The following hardening changes appear to be present in Knots or this port but
absent from current Bitcoin Core as inspected in this pass. They are not proven
consensus-critical, but they are security-relevant enough to track.

1. RPC auth-cookie deletion/replacement hardening

   Knots avoids deleting or replacing an authentication cookie that no longer
   matches the cookie the process created. This reduces stale-file and race
   hazards around local RPC authentication material.

2. Fee-estimator corrupt-file overflow guard

   Knots guards fee-estimator deserialization before using corrupt dimensions in
   arithmetic or allocation-sensitive paths. The relevant risk is a malformed
   local fee-estimator file causing overflow or excessive allocation behavior.

3. Subprocess file-descriptor closing before exec

   Knots closes inherited file descriptors before executing subprocesses. This
   prevents unintended descriptor inheritance into helper processes such as
   signers or command hooks.

4. Windows exclusive-open handling for `wbx`

   Knots carries a Windows-specific exclusive-open workaround for MSVCRT mode
   handling. Without it, code that intends an exclusive create can fail open on
   Windows in cases where POSIX-like mode strings are assumed.

5. External signer fingerprint validation

   Knots validates external signer fingerprints as exactly eight hex characters
   and sanitizes invalid values in error output. This was missing from the
   partially replayed branch and was ported here with functional coverage in
   `rpc_signer.py`.

## Original Knots Defect Found

`addnode ... onetry connection_type=inbound` is a Knots-only RPC surface and was
not introduced by this port. Original Knots accepts `inbound` and then reaches
the outbound connection path with `ConnectionType::INBOUND`, which conflicts
with Core's outbound-open invariant and can hit an assertion. This branch now
rejects that input with `RPC_INVALID_PARAMETER`, and `rpc_net.py` covers it.

## Port-Introduced Defects Fixed

The following issues were introduced or exposed by the rebase and fixed before
this audit note was committed.

1. `BroadcastTransaction` submitted private broadcasts to the mempool before
   switching on the requested relay mode. Private relay now avoids the public
   mempool path as intended.

2. `CTxMemPool::check()` failed to update `check_total_adjusted_weight` while
   reconciling Knots and Core mempool-accounting changes. The accounting check
   is restored.

3. Several stale RPC help strings retained leading newlines after Core's newer
   `RPCMethod::ToString()` assertions. The affected help descriptions were
   normalized.

4. Functional-test framework state was stale around binary tracking and wallet
   option parsing. The framework now uses the current binary map and exposes
   the wallet mode options expected by the ported tests.

5. Disabled-private-key descriptor wallets were treated as spendable in wallet
   selection paths. Available-coin logic now treats those descriptor matches as
   watch-only for selection.

6. `setfeerate` used rounded `CFeeRate` values too early. It now parses the full
   user precision first, rejects positive values that round to zero, and still
   accepts values that round to the represented sat/vB rate.

## Consensus And BIP-110 Notes

No clearly consensus-critical covert Knots fix missing from Core was identified
in this pass. The miner package unsigned-overflow fix and secp256k1 EllSwift
overflow-flag fix both appear to already be present in current Core and are not
new Core-missing findings here.

BIP-110 and other Knots consensus or policy deltas still require a dedicated
review before this branch should be treated as consensus-equivalent to either
upstream project. This audit only records that no additional consensus-security
fix missing from Core was proven during the port.

## Test Coverage

Build targets:

- `cmake --build build --target bitcoind bitcoin-cli bitcoin-wallet test_bitcoin -j2`

Unit tests:

- `build/bin/test_bitcoin --run_test=util_tests,streams_tests,transaction_tests,txpackage_tests,policyestimator_tests,system_ram_tests,miniscript_tests`
- `build/bin/test_bitcoin --run_test=wallet_tests,scriptpubkeyman_tests,spend_tests,coinselector_tests,coinselection_tests,group_outputs_tests,feebumper_tests,wallet_rpc_tests,ismine_tests,psbt_wallet_tests,wallet_transaction_tests,walletdb_tests,walletload_tests,init_tests --log_level=message`

Functional tests:

- `test/functional/rpc_net.py --configfile build/test/config.ini`
- `test/functional/rpc_signer.py --configfile build/test/config.ini`
- `test/functional/wallet_fundrawtransaction.py --configfile build/test/config.ini`
- `test/functional/wallet_create_tx.py --configfile build/test/config.ini`
- `test/functional/wallet_bumpfee.py --configfile build/test/config.ini`
- `test/functional/wallet_send.py --configfile build/test/config.ini`
- `test/functional/wallet_sendall.py --configfile build/test/config.ini`
- `test/functional/rpc_psbt.py --configfile build/test/config.ini`

Each functional test was run with a fresh temporary cachedir.

## Open Risks

This branch still deserves focused review of P2P policy, mining, consensus-
adjacent behavior, and any Knots-specific RPC surfaces not exercised by the
tests above. The findings here are the issues confirmed during this porting
pass, not a complete security audit of all Knots deltas.
