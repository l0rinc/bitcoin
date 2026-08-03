# BIP32 seed-length contract and caller parity audit

## Seeded from Cycle 313

Cycle 313 Goal 67 compared the v31.1 release with current CExtKey::SetSeed behavior. v31.1 accepted a 15-byte seed and generated a valid CKey. Current enforces the BIP32 16--64 byte domain with Assert, and the cycle added a permanent regression for 15- and 65-byte inputs. The production fix is already present upstream; this goal must follow the contract into its callers and wrappers rather than duplicate that change.

## Initial questions

- Do wallet seed-generation, import, migration, descriptor, fuzz, and test callers always produce 16--64 bytes, and do they document that precondition?
- Which callers can receive external or persisted seed bytes, and do they translate invalid length into a controlled error rather than an assertion or process abort?
- Do C++, C, Rust, Python, or other bindings preserve the exact length, ownership, error, and output-on-failure contract?
- Are empty, 15, 16, 32, 64, and 65-byte values rejected, accepted, normalized, or declassified consistently across current and maintained releases?
- Could a too-short value produce a valid-looking key, survive backup/migration, or be logged or retained after rejection?

## Required evidence

Keep source and release commit IDs, caller paths, exact input bytes or lengths, key-validity/output state, exception/status/log results, and independent verifier commands. Re-run the current seed-length regression and use source-matched release tests where possible. Distinguish the CExtKey API contract from wallet-generated entropy and from BIP32 serialization. Treat missing toolchains or bindings as limitations. A finding needs a caller-specific mismatch with a minimal test or source/dataflow proof; otherwise classify the caller and record the next unchecked boundary.
