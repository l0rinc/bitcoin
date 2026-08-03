# Compressed Script Replacement and Malformed-UTXO State Audit

## Seeded from Cycle 315

- Goal index: 115
- Slug: `compressed-script-replacement-invariant`
- Seed finding commit: `8d3a058c57` (`compressor: replace oversized scripts during decode`)
- Seed surface: `ScriptCompression::Unser`, `TxOutCompression`, `Coin`, undo
  records, database cursors, and UTXO snapshot export.

Cycle 315 found that an oversized compressed script appended `OP_RETURN` to a
pre-populated destination even though the source comment says to replace the
script. The fix clears the destination before the replacement. The regression
compares fresh and reused `CTxOut` destinations from identical bytes.

This journal intentionally expands the next campaign beyond that repaired
line. Check all malformed and boundary encodings for the same state-dependent
shape, including truncated payloads and exceptions, while preserving the
historical bounded `ignore` behavior that prevents invalid UTXO script lengths
from causing unbounded allocation.

### Initial queue

1. Compare `ScriptCompression::Unser` on fresh and reused `CScript` objects for
   special scripts, ordinary scripts, oversized lengths, CompactSize edges,
   noncanonical lengths, and truncated payloads.
2. Trace `CDBWrapper::MakeDeserializeTarget` and commit behavior through every
   `Coin` and `TxOutCompression` caller, especially cursor loops that reuse an
   output object.
3. Build a scratch database or deterministic cursor fixture containing
   repeated malformed entries and compare direct decode, cursor decode, and
   snapshot output against an independent expected-state model.
4. Exercise undo and restart/recovery paths with malformed-but-bounded data;
   classify safe rejection, replacement, partial output, and state mutation
   according to each caller's contract.
5. Add minimized recurrence fixtures for any distinct caller or failure mode;
   do not repeat the clear-before-replace fix without new evidence.

### Known evidence

- Historical OOM guard: `5d0434d13d`.
- Fixed regression logs: `/data/my_storage/tmp/cycle315-compress-before.log`
  and `/data/my_storage/tmp/cycle315-compress-after.log`.
- Current known contract: a successful oversized decode consumes exactly the
  encoded payload and yields the short invalid `OP_RETURN` script independent
  of destination state.

Next run must record the exact catalog hash, branch/base, dirty state, and
whether each queue cell is confirmed, dismissed, or inconclusive before
changing code.
