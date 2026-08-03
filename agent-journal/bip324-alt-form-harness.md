# BIP324 alternate-form fixture and peer-harness audit

This seed was added after Cycle 317's BIP324 interoperability matrix found
that Core production parsing handles short and long forms, but the Core test
coverage is not systematic and the Python v2 peer always emits the canonical
short form for every known optimized command. Keep this separate from the
known current rust-bitcoin long-form decoder gap and from Core's intentional
reserved-ID behavior.

Use the deployed BIP324 v1.0.2 table as the application contract: IDs 1--28
have equivalent 12-byte commands; ID 37 is the BIP434 FEATURE extension in
Core's current registry; IDs 29--36 are reserved/undefined in the current
Core compatibility policy. Build exact plaintext fixtures before encryption,
including zero-payload and minimal valid payload forms, then test each form
in both directions through the real transport and parser.

Audit whether a harness can deliberately select short versus long encoding,
whether assertions compare decoded type, payload, error, connection state, and
version gating, and whether a temporary decoder mutation survives the old
tests but is killed by the new fixture. Include malformed padding, embedded
NUL, non-ASCII, unknown IDs, reserved IDs, and commands at the 12-byte limit.
Do not canonicalize both inputs through the same helper before comparison.
Change production code only if an independently verified local behavior
defect appears; otherwise improve the test oracle or leave a report-ready
external finding.
