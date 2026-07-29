# Overview

This repository is `libsecp256k1`, a high-assurance C library implementing
secp256k1 elliptic-curve operations for Bitcoin and related protocols. Its
primary runtime surface is an in-process library API, not a network service or
standalone daemon. Embedding applications call the exported interfaces in
`include/secp256k1.h` and the optional module headers; the implementation is
compiled from `src/secp256k1.c`, arithmetic and group-operation headers under
`src/`, and selected `src/modules/*` implementations.

The core assets are:

- secret keys, signing and MuSig secret nonces, ECDH shared points/secrets,
  Silent Payments scan keys and spend tweaks, and intermediate secret scalars;
- the unforgeability and correctness of ECDSA, BIP-340 Schnorr, recovery, ECDH,
  ElligatorSwift/BIP-324, MuSig2/BIP-327, and Silent Payments/BIP-352
  computations;
- exact parsing, serialization, point/scalar validation, key aggregation,
  tweaking, nonce derivation, and signature-verification semantics on which
  Bitcoin software may rely;
- constant-time behavior, explicit secret erasure, context blinding, and
  resistance to timing, cache, memory-access, and power-analysis leakage;
- memory safety, ABI stability, and availability of high-value embedding
  processes such as nodes, wallets, signers, and hardware or embedded systems;
- privacy properties such as ElligatorSwift encoding indistinguishability and
  correct Silent Payments output construction and scanning; and
- integrity of release sources, generated precomputation tables, build
  configuration, compiler-selected arithmetic backends, and installed library
  artifacts.

`README.md` makes Bitcoin the best-tested intended environment, describes no
runtime dependencies and almost no runtime heap allocation, and warns that
non-Bitcoin uses may be less analyzed. The library deliberately exposes
higher-level opaque types and narrow APIs. Optional runtime surfaces include
ECDH (`include/secp256k1_ecdh.h`), public-key recovery
(`include/secp256k1_recovery.h`), x-only keys and keypairs
(`include/secp256k1_extrakeys.h`), Schnorr signatures
(`include/secp256k1_schnorrsig.h`), MuSig2 (`include/secp256k1_musig.h`),
ElligatorSwift (`include/secp256k1_ellswift.h`), and Silent Payments
(`include/secp256k1_silentpayments.h`). Build systems choose which modules and
arithmetic/assembly backends are present (`CMakeLists.txt`, `configure.ac`,
`src/CMakeLists.txt`, and `Makefile.am`).

Tests, benchmarks, examples, Sage proof scripts, vector generators, CI, and
precomputation generators are not normal library runtime surfaces. They matter
as verification and software-supply-chain surfaces, but their command-line,
filesystem, subprocess, container, and network privileges must not be confused
with privileges exposed by the installed library.

# Threat Model, Trust Boundaries, and Assumptions

## Runtime trust boundaries

1. **Remote/protocol data to the embedding application.** Network peers,
   transaction data, signatures, public keys, tweaks, messages, ElligatorSwift
   encodings, ECDH peer keys, MuSig participant keys/nonces/partial signatures,
   and Silent Payments transaction inputs and outputs may be adversarial. The
   embedder converts serialized bytes into library calls. Parse and verify
   functions must reject malformed, non-canonical, out-of-range, off-curve, or
   otherwise invalid inputs without memory corruption, secret-dependent
   behavior, incorrect acceptance, or attacker-triggerable internal failure.

2. **Library API contract to C memory and process state.** Callers control
   pointers, lengths, array counts, output capacities, opaque-object lifetimes,
   context instances, and concurrency. Documented non-NULL, size, alignment,
   initialization, object-provenance, non-aliasing, and exclusive-mutation
   requirements are trusted programming contracts, not remotely hostile inputs
   the API promises to recover from. Violations commonly invoke an illegal
   callback whose default implementation aborts (`include/secp256k1.h`,
   `src/secp256k1.c`, `src/util.h`). A binding or application that maps malformed
   network input into a contract violation creates a new availability boundary
   outside the library's intended parser boundary.

3. **Secret-bearing caller state to library internals.** The caller supplies and
   retains ownership of secret keys, auxiliary randomness, context-randomization
   seeds, MuSig nonce uniqueness state, and output buffers. The library must not
   leak those values through its control flow, memory access, invalid outputs,
   or uncleared temporaries. The caller remains responsible for secure RNG,
   durable key storage, buffer cleansing outside the library, locking, process
   isolation, backups, authorization to sign, and preventing rollback or reuse
   of distributed MuSig counters.

4. **Interactive or peer-controlled protocol state.** A MuSig signer must treat
   other participants and aggregators as malicious. ElligatorSwift/ECDH peer
   encodings and Silent Payments transaction material are also untrusted. The
   library must bind values to the intended key/message/session and produce
   correct success or failure results, while the embedder must preserve protocol
   ordering, participant identities, commitment rules required by the higher
   protocol, and final-result verification. `doc/musig.md` explicitly identifies
   catastrophic misuse risks that no local API can fully prevent.

5. **Trusted extension callbacks and context mutation.** Custom nonce functions,
   ECDH/ElligatorSwift hash callbacks, Silent Payments label lookups, SHA-256
   compression functions, illegal/error callbacks, and their opaque data
   pointers execute inside the caller's process. They are operator/developer
   controlled and trusted for memory safety, cryptographic correctness,
   reentrancy, output size, lifetime, and return-value contracts. Giving an
   attacker control of a function pointer already gives them process execution
   and is not a cryptographic attack on this library. Const context operations
   may be concurrent, but context randomization, destruction, and other mutable
   operations require exclusive access as documented in
   `include/secp256k1.h`.

6. **Source/build/install boundary.** Maintainers, reviewers, release signers,
   packagers, compilers, assemblers, build generators, and configuration are
   trusted to produce the reviewed implementation. Backend-selection macros,
   generated precomputed tables, the experimental 32-bit ARM assembly path, and
   optional modules can change the code that executes. The repository recommends
   verifying signed release tags (`README.md`). CI and developer workflows run
   repository scripts and external toolchains with build-host privileges, so
   malicious source changes or dependencies are a supply-chain concern even
   though the runtime library itself performs no network or file I/O.

## Security invariants

- No supported public or secret input may cause out-of-bounds access, integer
  overflow with security consequences, use of uninitialized state, type/ABI
  confusion, or undefined behavior on a supported platform.
- Verification accepts only signatures valid for the exact public key, message,
  algorithm, and canonicality rules documented by the API. Parsing and recovery
  must not create invalid curve objects or bypass scalar/point range checks.
- Signing, key generation/tweaking, ECDH, and protocol modules must produce
  mathematically correct outputs or fail closed. Failure paths must not expose
  partially valid secret-derived outputs.
- Secret-dependent computations must avoid secret-dependent branches and memory
  accesses on supported, reasonable hardware/toolchains. Variable-time
  algorithms must be limited to public data. Context randomization and blinding
  remain defense in depth, not a substitute for constant-time code.
- Nonces must be unpredictable or deterministically bound to all required
  transcript inputs and never reused in a way that reveals a secret key. MuSig
  secret nonces are single-use and session/key binding must be preserved.
- Domain separation and transcript binding must prevent a signature, nonce, key
  aggregation, or shared secret from being silently reused in another algorithm,
  role, message, public-key ordering, or protocol context.
- Secret intermediates and invalid secret-derived outputs must be explicitly
  cleared where promised. This reduces residual-memory exposure but does not
  defend against a hostile process, debugger, compromised OS, caller-retained
  copies, swapping, crash dumps, or physical acquisition.
- Public array counts and length-driven work must remain memory-safe. Embedders
  must apply application-level resource limits; the library is not a network
  rate limiter.
- Opaque objects are created, parsed, copied only where allowed, and serialized
  through their public APIs. Directly forging or persisting internal
  representations across library versions is outside the contract.
- Silent Payments callers must supply the actual lexicographically smallest
  transaction outpoint, correct eligible inputs, original output order, and all
  generated outputs. The module implements elliptic-curve operations, not the
  full wallet, transaction, address, or script protocol
  (`include/secp256k1_silentpayments.h`).

## Platform and scope assumptions

The implementation assumes 8-bit bytes, supported integer widths, two's
complement-like conversion and signed-shift behavior, correct alignment, and a
conforming C toolchain; `src/assumptions.h` checks material compile-time
properties. Basic runtime self-tests check the SHA-256 compression function and
serious configuration errors (`src/selftest.h`, `src/secp256k1.c`). Compiler,
microarchitecture, fault-injection, and physical side channels cannot be
eliminated completely; the README qualifies constant-time and blinding claims
to reasonable hardware/toolchains.

Web-specific authorization, sessions, CSRF, XSS, SSRF, SQL/template injection,
tenant isolation, HTTP rate limiting, and TLS termination are not repository
runtime concerns because the library has no web or network server. Authorization
to use a key, transaction-policy validation, consensus integration, wallet
database protection, entropy collection, and transport authentication belong to
the embedding system. Deliberate API misuse by code already able to read process
memory, malicious replacement of the source/toolchain, and extraction from a
fully compromised host are outside the library's cryptographic boundary, though
defense-in-depth behavior should avoid making their consequences worse.

# Attack Surface, Mitigations, and Attacker Stories

## Primary runtime attack surfaces

- **Core parsing and verification.** Variable-length public-key and DER
  signature parsing, fixed-size compact signatures, public-key serialization,
  sorting/combining, ECDSA verification, recovery, x-only key parsing, and
  Schnorr verification consume bytes commonly originating with remote peers.
  Relevant classes are parser memory safety, scalar/field overflow,
  canonicalization disagreement, invalid-curve or infinity handling,
  malleability-policy mismatch, and false acceptance.

- **Secret operations.** ECDSA/Schnorr signing, public-key creation, secret and
  keypair tweaking, ECDH, ElligatorSwift creation/XDH, Silent Payments sender
  and scanner computations, and MuSig nonce/signing routines handle high-value
  secrets. Relevant classes are nonce bias or reuse, transcript omission,
  secret-dependent control or memory access, invalid-input fault behavior,
  residual secrets, incorrect output masking, and unsafe custom callbacks.

- **Interactive MuSig2.** Public-key aggregation and ordering, key tweaks,
  public/aggregate nonce parsing and aggregation, session construction, partial
  signing, partial verification, and final aggregation cross a malicious
  participant boundary. Reusing or copying `secp256k1_musig_secnonce` can reveal
  the signing key. The API invalidates caller randomness after successful nonce
  generation, consumes and zeroes a secnonce during signing, binds generated
  nonces to a public key, and documents optional binding to the known message
  and aggregate key. Callers must keep randomness unique/secret (or counters
  globally non-repeating per key), preserve session associations, and verify
  partial or final signatures where their threat model requires it.

- **ElligatorSwift/ECDH and Silent Payments privacy.** Peer public data can be
  chosen adversarially. ElligatorSwift encoding randomness must not be a
  deterministic function of the public key when encoding an existing public
  key, and XDH hashes both encoded keys with the shared x-coordinate to bind
  roles/transcripts. Silent Payments processes attacker-visible recipients,
  inputs, outputs, labels, and callback-backed label caches. Incorrect grouping,
  outpoint selection, ordering, scan bounds, label association, or derived
  tweaks can lose funds or privacy even when the group arithmetic is locally
  correct.

- **C ABI, counts, allocation, callbacks, and contexts.** The preallocated API,
  context creation/cloning/destruction, user-supplied buffers, pointer arrays,
  counts, custom callbacks, and mutable context state are exposed to bindings
  and applications. Memory corruption, integer wraparound, incorrect buffer
  sizing, reentrant mutation, use-after-destroy, and default-abort amplification
  matter. These are most realistic when a foreign-function binding or embedder
  fails to enforce the documented C contract.

- **Supplementary and build-time code.** `contrib/lax_der_parsing.*` and
  `contrib/lax_der_privatekey_parsing.*` intentionally provide compatibility
  parsing separate from the strict primary API and should be assessed in the
  consuming application's context. `examples/`, `tools/`, `sage/`,
  precomputation generators, CI scripts, CMake/Autotools logic, and Docker
  recipes consume developer-controlled files, arguments, environment, source,
  and external packages. Their security impact is build/release integrity or
  developer-host compromise, not direct remote runtime reachability unless a
  downstream system separately deploys them.

## Existing mitigations and robustness measures

- The implementation has no runtime dependencies and avoids runtime heap
  allocation except explicit context/scratch creation; opaque public types and
  narrow high-level APIs reduce exposed internal state (`README.md`).
- Public parse paths validate encodings, scalar ranges, curve points, and
  subgroup membership and commonly zero outputs before or after failure.
  Secret operations use conditional moves and mask invalid outputs.
- Secret scalar, field/group, hash, RNG, nonce, key-data, and other temporary
  buffers are cleared with explicit anti-optimization helpers in `src/util.h`
  and throughout `src/secp256k1.c`, `src/ecmult_gen_impl.h`, and module
  implementations.
- Secret-key multiplication uses fixed access patterns, conditional table
  selection, constant-time scalar arithmetic/inversion, unknown-logarithm
  precomputation offsets, and optional context blinding
  (`src/ecmult_gen_impl.h`, `src/ecmult_const_impl.h`, `src/modinv32_impl.h`,
  `src/modinv64_impl.h`). `secp256k1_context_randomize` adds runtime blinding.
- `src/ctime_tests.c` marks secret memory undefined and uses Valgrind/MSan-style
  checking to detect secret-dependent control or memory access. The repository
  also has broad unit and exhaustive small-group tests, Wycheproof vectors,
  module-specific vectors, sanitizer/Valgrind configurations, multiple
  arithmetic backends, architectures, compilers, and endianness in CI
  (`src/tests.c`, `src/tests_exhaustive.c`, `src/wycheproof/`, `.github/workflows/ci.yml`).
- Sage scripts and design documentation provide independent reasoning about
  group formulas, constants, and safegcd inversion (`sage/`,
  `doc/safegcd_implementation.md`, `doc/ellswift.md`).
- RFC6979 ECDSA nonce derivation, BIP-340 tagged hashes and hardened nonce
  inputs, key-prefixed challenges, transcript-bound ElligatorSwift hashing, and
  MuSig key/session binding address protocol-specific misuse. APIs document
  cases where callers should verify a newly produced Schnorr or partial
  signature rather than assuming self-verification.
- Illegal-argument and internal-error callbacks separate caller contract errors
  from ordinary invalid serialized inputs. Self-tests and platform assumptions
  fail fast on serious build/runtime inconsistencies.

## Realistic attacker stories

- A remote peer sends malformed, boundary-value, or non-canonical keys and
  signatures through a node's normal parse/verify path, seeking memory
  corruption, false acceptance, consensus divergence, or a reliable process
  abort.
- A malicious transaction or wallet counterparty supplies many or adversarially
  structured public keys, tweaks, ElligatorSwift encodings, Silent Payments
  inputs/outputs, or labels, seeking invalid derived keys, missed or
  misdirected payments, privacy loss, excessive work, or a crash.
- A malicious MuSig participant or aggregator reorders keys, substitutes
  nonces, replays sessions, sends invalid partial signatures, or pressures a
  signer into nonce reuse, seeking the victim's secret key or a signature under
  an unintended aggregate key/message.
- A local co-tenant, compromised peripheral, or observer with timing, cache,
  branch, memory-access, power, or fault information repeatedly observes signing
  or key agreement, seeking secret-dependent leakage. Feasibility and severity
  depend strongly on deployment hardware, compiler, isolation, blinding, and
  observation capability.
- A downstream binding supplies incorrect object sizes, counts, capacities, or
  lifetimes based on untrusted input, turning a safe serialized-input rejection
  into an API contract abort or memory-safety issue.
- A malicious contributor, compromised CI dependency, packager, compiler, or
  release channel alters arithmetic, precomputed tables, configuration, or
  installed artifacts. This can subvert every downstream operation, but requires
  crossing the trusted source/build boundary rather than the public runtime API.

Less realistic or out-of-scope stories include an attacker already able to
replace callback pointers or read arbitrary process memory, direct calls with
deliberately invalid C pointers by trusted application code, web attacks against
interfaces absent from this repository, and compromise of higher-level wallet
authorization or key databases without a library defect. Developer-only tool
argument injection is not remotely reachable merely because the library is
linked into a networked application.

# Severity Calibration (Critical, High, Medium, Low)

## Critical

A flaw is critical when ordinary supported use permits theft-scale key
compromise, universal or practical signature forgery, or equally broad
subversion with little or no special deployment assumption. Examples include:

- remotely supplied signatures or keys being accepted as valid in a way that
  permits unauthorized Bitcoin spending across common deployments;
- deterministic nonce reuse/bias or a practical constant-time failure that
  reveals signing keys under normal repeated signing, including MuSig nonce
  handling that lets another participant extract a victim's key;
- attacker-controlled serialized input causing practical code execution in
  typical high-value embedding processes; or
- systematic Silent Payments or key-tweaking arithmetic that redirects funds to
  attacker-controlled keys without detectable higher-level failure.

## High

High severity covers direct compromise of a major security property with
realistic but narrower preconditions. Examples include:

- false signature acceptance, invalid-curve behavior, wrong ECDH secrets, or
  fund-affecting output derivation limited to one optional module or common
  protocol configuration;
- remotely reachable memory corruption or reliable process termination through
  valid API use with attacker-controlled serialized data;
- a practical timing/cache/power attack requiring local or co-resident access
  but recovering long-lived secret keys from a realistic signer deployment;
- MuSig session/key/transcript confusion enabling malicious participants to
  cause unauthorized signing or key loss; or
- a reproducible supported-platform arithmetic/backend error that silently
  creates invalid or attacker-beneficial cryptographic results.

## Medium

Medium severity includes meaningful but bounded availability, privacy,
integrity, or misuse-resistance failures. Examples include:

- algorithmic resource exhaustion or a crash in a commonly exposed parse/scan
  path that affects a service but does not corrupt memory or cryptographic
  decisions;
- ElligatorSwift distinguishability or Silent Payments scanning/label behavior
  that leaks material privacy or causes recoverable missed detections under
  realistic inputs;
- a protocol-state or domain-separation issue requiring substantial malicious
  participant interaction, non-default application choices, or an additional
  caller mistake;
- incorrect results on a supported but uncommon platform/configuration without
  a demonstrated theft or key-recovery path; or
- a significant foreign-binding footgun where documented valid inputs can still
  violate memory safety or confidentiality.

## Low

Low severity is appropriate for limited defense-in-depth or developer-only
impact. Examples include:

- an abort that requires documented API contract violations, forged opaque
  objects, or invalid pointers rather than ordinary hostile serialized input;
- incomplete temporary clearing without evidence that secrets become observable
  across a realistic boundary;
- minor timing variation involving only public data, non-sensitive
  canonicalization differences rejected by higher layers, or limited diagnostic
  leakage;
- issues confined to benchmarks, examples, vector generators, proof scripts,
  non-default experimental assembly, or CI tooling without a credible path to
  shipped artifacts or developer-host compromise; or
- build hardening, misuse documentation, or self-test improvements that do not
  correct a currently exploitable cryptographic or memory-safety failure.

Severity must be lowered when the attacker control required by a story does not
exist in real deployment. In particular, arbitrary function-pointer control,
direct process-memory access, source replacement, or caller-supplied invalid C
pointers already crosses a trusted boundary. Conversely, impact must not be
downgraded merely because the library has no network stack: serialized
signatures, keys, transactions, and protocol messages routinely reach its API
through network-facing embedders.

Repository: target_sha256_14d9ed724692e982f67454f0268d054160720e522563938cbb2cd1b07616ff16
Version: 7151e3b843cbf912cf8313fd49ab7c228f23b1a7
