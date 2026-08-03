# Differential and varint formatter input-boundary audit

## Seeded from Cycle 312

Cycle 312 Goal 7 confirmed that `BlockTransactionsRequest` used the generic vector allocator before `DifferenceFormatter` could enforce its `uint16_t` index domain. A 37-byte `GETBLOCKTXN` payload declaring 2,500,000 indexes produced a 2,500,000-element capacity and one partially constructed element before EOF. The fix uses `LimitedVectorFormatter<65536, DifferenceFormatter>` and rejects the count before allocation.

This goal generalizes the learned shape without reopening the repaired caller. Inventory every custom count, CompactSize, varint, difference, and delta formatter, then map each caller's trust boundary, valid cardinality, element type, arithmetic domain, allocation path, output-on-failure contract, and malformed-input accounting. Include P2P, RPC/REST, file, database, WAL, index, and generated-vector consumers where they deserialize attacker-controlled or corrupt data.

## Initial questions

- Does each formatter reject impossible counts before reserve, resize, construction, or expensive per-element work?
- Is the count bound derived from the actual element domain and caller contract, rather than a transport maximum or an assumed block/file size?
- Can delta accumulation, signedness, narrowing, `count * sizeof(T)`, or CompactSize conversion overflow before the intended check?
- After failure, are output containers empty/unchanged as documented, and are network misbehavior, retry, disconnect, or corruption classifications preserved?
- Do serialization and deserialization agree at zero, one, maximum, maximum-plus-one, truncated, duplicate, and non-canonical values?

## Required evidence

For each candidate, keep the exact serialized bytes, declared count, valid domain table, old/new capacity and size, first failing operation, source/history links, and independent verifier. Prefer a count-only fixture plus a complete valid-boundary round trip. Use an existing build or a small standalone probe under `/data/my_storage/tmp`; do not create large builds while `/data` and `/` are full. A finding needs a reproducible allocation, arithmetic, output-state, or accounting mismatch and a focused regression. Otherwise classify the path, record why it is safe or unreachable, and queue the next distinct formatter/caller.
