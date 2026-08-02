# X-only Conversion Precondition and Invalid CPubKey Boundary Audit

## Seed from Cycle 303

Cycle 303 Goal 15 rejected two public-object hypotheses: direct extended-public-key
decoding already requires `IsFullyValid()`, and off-curve compressed P2PK scripts
are intentionally preserved as syntactic raw-script destinations. The remaining
learned boundary is the constructor in `src/pubkey.h`:

```cpp
explicit XOnlyPubKey(const CPubKey& pubkey) : XOnlyPubKey(std::span{pubkey}.subspan(1, 32)) {}
```

This has no error channel. A default or short `CPubKey` violates the span
precondition; a syntactically valid but off-curve key produces a non-fully-valid
x-only value. Current callers include Taproot descriptor providers, script
signing, benchmarks, tests, and fuzz support. Most production paths appear to
validate first, but that must be proved per caller rather than inferred from the
constructor's name.

## Initial scope and exclusions

Map every `XOnlyPubKey(CPubKey)` construction and its immediate dataflow. For
each caller, classify the source as fully validated, syntax-only, empty/short,
fuzz-generated, test-only, or an intentional raw script boundary. Check
descriptor serialization and private-key lookup, MuSig/signing assertions,
Taproot destination construction, error paths, wrappers, and release/debug
behavior. Use deterministic malformed objects and a temporary precondition
mutation where useful, then preserve any minimized reproducer.

Do not make raw script or consensus validation stricter merely to satisfy this
audit. A fix needs a concrete reachable failure, a caller-contract violation,
or a precise regression oracle. Consider a checked conversion API or an
explicit caller assertion only after reviewing project idioms and history.

The adjacent wallet observation is separate: `wallet/walletdb.cpp` may pass
`fSkipCheck=true` after validating a stored pubkey/private-key hash, and
`FillableSigningProvider::AddKeyPubKey()` stores the supplied key identity.
History indicates this is an intentional load-time optimization; do not merge
it into this goal unless a supported corruption or untrusted-input path is
independently demonstrated.

## First queue

1. Inspect `src/script/descriptor.cpp` `ConstPubkeyProvider` validation and
   `GetPrivKey()` x-only lookup, including clone and failure paths.
2. Trace `src/script/sign.cpp`, `src/key.cpp`, `src/addresstype.h`, and all
   public/binding entry points for valid-key guarantees.
3. Exercise fuzz and test-only arbitrary `CPubKey` construction under UBSan,
   debug assertions, and a minimized invalid-input corpus.
4. Compare behavior across normal, VERIFY, sanitizer, and supported platform
   configurations before deciding whether a source change is justified.
