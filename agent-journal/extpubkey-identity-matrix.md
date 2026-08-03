# Extended public-key identity across descriptor and wallet containers

## Lineage

- Added after Cycle 310 Goal 15. The cycle fixed `CExtPubKey::operator<`, which previously collapsed objects that `operator==` considered distinct by depth, parent fingerprint, or child number.
- Starting evidence: `7bc2afcf91` and `agent-journal/public-object-validation.md` Cycle 310. The regression uses two valid xpub objects with identical derivation material but distinct serialized provenance and exercises descriptor extraction plus the `gethdkeys` xpub/xprv map pattern.

## Campaign protocol

For every context, write down the intended identity before testing: derivation material, complete BIP32 metadata, complete wire serialization including version bytes, key origin, or some explicit composite. Trace the identity through parsing, normalization, descriptor storage, set/map aggregation, RPC output, backup/import, migration, PSBT merge, and restart. Use valid deterministic xpub/xprv pairs that differ one field at a time, plus same-material collisions, wrong-network versions, and origin paths.

Require an executable contract test for every claimed distinction. Check that all emitted xpub/xprv pairs agree after neutering, descriptor origin information is retained, maps do not merge objects under an unintended comparator, and intentional cryptographic deduplication remains explicit and stable. Re-run the Cycle 310 regression after changes. Separate version-byte identity from the generic `CExtPubKey` equality contract and from PSBT's complete serialized-key rule.

Search history and prior findings before each cell. Preserve minimized descriptor, RPC, PSBT, backup, and migration fixtures. Treat a reduced result or unchanged count as evidence only after identifying the caller's documented contract. Report external or alternate-implementation differences separately from local fixes.

## Initial queue

1. Re-run the repaired descriptor and `gethdkeys` paths with collisions in depth, fingerprint, child, chain code, public key, and network/version bytes.
2. Audit `createwalletdescriptor` active-key selection and backup/migration sets for arbitrary representatives or false uniqueness.
3. Compare PSBT full-wire identity with wallet/descriptor identity and test merge, round-trip, and cross-network cases.
4. Search for new ordered containers or recurrence after descriptor and wallet refactors.
