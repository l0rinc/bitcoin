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

## Cycle 317 investigation

Selection: Goal 107, `bip324-short-id-parity`, on branch
`uber-cycle-317-bip324-short-id-parity-20260802`; selection commit
`22ce16206e92a110fe37f632d401ad507017b7dd`; starting source base was
`23be1ca3a470a0b511a4c87e8e3ff43bfa043239`. The BIP324 v1.0.2 application
contract used for comparison is the deployed specification: IDs 1--28 have
the listed long command equivalents, ID 0 carries the 12-byte command, IDs
29 and above are undefined, and a supported message with both encodings must
be accepted in either form.

Evidence ledger:

* Bitcoin Core `src/net.cpp:935-969` has the complete IDs 1--28 table, reserved
  empty slots 29--36, and BIP434 `feature` at 37. `GetMessageType` accepts
  both short IDs in the table and valid long ASCII forms; the peer layer then
  ignores unknown message names for extensibility. `V2MessageMap` also inserts
  the repeated empty slots, so an internally constructed empty message name
  maps to the first reserved ID (29), an edge contract to carry into the next
  harness campaign rather than a confirmed network defect.
* The current Core transport test (`src/test/net_tests.cpp:1967-2185`) checks
  short IDs for several messages, a long `tx`, malformed long padding, and
  unknown types, but has no systematic short/long equivalence table for all
  supported IDs. The Python test peer's `build_message` at
  `test/functional/test_framework/p2p.py:420-432` always chooses the short
  form whenever a command is in `SHORTID`; it can receive a long form at
  `:342-352` but exposes no per-message override to emit one. This is a
  test-oracle and harness-realism gap, not evidence that Core's production
  transport mishandles the long form.
* Current btcd snapshot `05585e037ba0690572208dbc46d121a49cc0c4c9` maps only
  IDs for message structs it implements. Its table omits 3, 4, 10, and 20,
  while `makeEmptyMessage` also has no compact-block message types; its
  omission is an unsupported-feature boundary, not a supported-message
  parity failure. Supported entries use long fallback and short decoding.
* Current rust-bitcoin snapshot `19436dde9ae7f56b9b999560120a66ad08958810`
  still maps all 1--28 in `v2_command_byte` and its short decoder, but its
  `payload_decoder_from_command` recognizes only handshake and a few legacy
  commands. A valid long zero-payload `mempool` fixture
  `00 6d 65 6d 70 6f 6f 6c 00 00 00 00 00` therefore reaches `Unknown`,
  while short `0f` reaches `MemPool`. This reproduces the Cycle 307 external
  report at a newer upstream commit and is deliberately not counted again as
  a Core finding. The current `p2p/src/message.rs` hash is
  `ec2a53253da6164e2a456aac33787851741858544e25bcb6fda1d0cddede3fee`.
* The rust-bitcoin/bip324 transport-only snapshot is
  `7c8894452929e1f1019e27a84ef8d6834facaa47`; no application ID registry was
  found there. Rust, Go, and Core net test execution were unavailable: the
  host has no rustc/cargo/go and the existing Core test binary aborted during
  chain test fixture setup because both `/` and `/data` reported no free
  space. Source comparison and exact fixtures remain reproducible.

Verdict: no independently proven local production defect. The supported
surface that remains highest value is the alternate-form test harness and
fixture contract: force both encodings, cover ID 37/version gating and
undefined IDs, and make the Core/Python oracle sensitive to a decoder that
handles only the canonical short form. This produced Goal 117,
`bip324-alt-form-harness`, as the next queue entry. Do not repeat the exact
rust-bitcoin `mempool` long-form report or the intentional btcd compact-block
omission without a changed commit, caller, or behavior.
