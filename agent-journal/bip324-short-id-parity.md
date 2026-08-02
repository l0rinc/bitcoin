# BIP324 short-message ID and long-form interoperability audit

This journal is the seed for Goal 107, added after Cycle 307 found a pinned
rust-bitcoin V2 decoder that does not dispatch optimized messages received in
the valid long-command form. Keep this campaign separate from the earlier
Core V2 transport, BIP324 XDH-vector, and DEL-boundary cells.

The governing contract is BIP324 version 1.0.2, especially the application
layer table and the rule that a peer supporting a message with both encodings
must accept the one-byte and 13-byte forms. Compare the authoritative table
with Core, rust-bitcoin, btcd, bitcoinfuzz adapters, Core's Python V2 test
peer, and any newly found implementations. Check both directions, payload
parsing after transport type recognition, reserved and extension IDs,
unknown-message policy, and version-gated messages such as BIP434 FEATURE.

Each cycle must preserve exact byte fixtures, implementation commit hashes,
source line references, supported-message inventories, and independent
short-versus-long verdicts. Distinguish an implementation that intentionally
does not support a message from one that supports it but accepts only one
wire encoding. Do not change Bitcoin Core for an external-only defect;
produce a report-ready external reproducer and extend the queue when a new
table, extension, or parser asymmetry is found.
