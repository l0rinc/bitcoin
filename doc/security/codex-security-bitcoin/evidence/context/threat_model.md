# Bitcoin Core Repository Threat Model

## Overview

Bitcoin Core is security-critical software that connects to the public Bitcoin peer-to-peer network, validates blocks and transactions against consensus rules, maintains the node's chain and mempool state, optionally manages wallets and private keys, and exposes operator interfaces including JSON-RPC, REST, ZeroMQ notifications, a Qt GUI, command-line tools, and experimental multiprocess IPC. The repository also contains build and release automation, bundled libraries, fuzz/unit/functional tests, developer tooling, documentation, and an untracked JavaScript dependency tree used by the scan environment.

The primary security assets are consensus correctness; availability of block and transaction validation; wallet private keys, descriptors, passphrases, and transaction intent; integrity and confidentiality of node configuration and data-directory files; RPC credentials and authorization boundaries; network privacy; reproducible release/build inputs; and the memory-safety and constant-time properties of cryptographic components such as `src/secp256k1`.

The primary product surfaces are the `bitcoind` and `bitcoin-qt` runtimes, P2P message processing in `src/net_processing.cpp` and related protocol parsers, consensus and script validation under `src/consensus`, `src/kernel`, and `src/script`, wallet and wallet RPC code under `src/wallet`, RPC/HTTP/REST code under `src/rpc`, `src/httprpc.cpp`, `src/httpserver.cpp`, and `src/rest.cpp`, networking and proxy integrations including Tor and I2P, local GUI/URI handling, notification interfaces under `src/zmq`, and optional IPC under `src/ipc`. Libraries bundled below `src/leveldb`, `src/secp256k1`, `src/univalue`, `src/minisketch`, and related directories are security-relevant where adversarial network, wallet, or on-disk data reaches them.

## Threat Model, Trust Boundaries, and Assumptions

### Trust boundaries and actors

- Unauthenticated remote peers control P2P messages, transaction and block encodings, service announcements, addresses, timing, ordering, connection churn, and malformed protocol inputs. They must never be able to violate consensus, corrupt durable state, exhaust disproportionate resources, disclose local secrets, or obtain code execution.
- RPC and REST clients cross an operator-configured authentication and authorization boundary. RPC is normally an administrative interface, but individual commands still need correct parameter validation, wallet scoping, capability checks, and safe filesystem/process/network effects. Repository evidence, not an assumption of public exposure, determines severity.
- Wallet users and wallet files cross a high-value boundary. Imported descriptors, keys, PSBTs, transactions, labels, database contents, backup/restore paths, and external signer responses can be malformed or adversarial. Wallet locking, encryption, ownership, signing intent, and fee/amount invariants must hold.
- Local users, desktop handlers, and GUI-originated inputs control command-line arguments, configuration, payment URIs, drag/drop or file inputs, clipboard content, and data-directory selection. These are generally lower exposure than the P2P boundary, but privilege-crossing behavior, unsafe local IPC, credential leakage, and arbitrary file effects remain in scope.
- Operators control configuration, startup flags, RPC allowlists, proxy endpoints, bind/listen addresses, wallet locations, and debug facilities. Purely operator-intentional unsafe configuration is normally not a vulnerability, but unsafe defaults, parsing ambiguity, boundary confusion, and lower-privileged bypasses of operator controls are in scope.
- Build and release systems trust repository scripts, dependency descriptions, downloaded source archives, compiler/toolchain inputs, CI secrets, and signing/reproducibility processes. Malicious pull-request content or compromised dependencies must not gain unintended secret access or alter release artifacts without review and provenance controls.
- Developers control tests, fuzz harnesses, examples, documentation snippets, and one-off tools. These paths are not production surfaces by default. They become reportable when installed, shipped, invoked by privileged automation, or reused by product/runtime code.
- Third-party or generated content, including `node_modules`, generated source maps, translations, test fixtures, and bundled libraries, is accounted for in the scan. Mere presence is not a product reachability claim; a report needs a supported runtime/build path and a meaningful trust boundary.

### Security assumptions

- Consensus rules and cryptographic verification are security invariants even when an attack only causes a minority implementation to disagree with the network.
- Network input is hostile regardless of peer reputation, claimed services, transport, or prior protocol success.
- RPC authentication does not make command injection, path traversal, secret disclosure, wallet-boundary mistakes, or unintended cross-wallet effects harmless; it changes attacker preconditions and severity.
- The local data directory and configuration are trusted at process start unless the issue is a lower-privileged overwrite/read, unsafe restore/import, symlink race, or boundary-crossing parser bug.
- Memory exhaustion, CPU amplification, disk growth, deadlock, assertion abort, and process termination are security-relevant when a realistic remote or lower-privileged actor can trigger them at materially favorable cost.
- `src/secp256k1` must preserve constant-time behavior where documented, reject invalid inputs, avoid secret-dependent leakage, and maintain cryptographic correctness. Its nested `SECURITY.md` changes reporting contacts but not these invariants.
- Untracked scan-environment files are in inventory because the user requested the entire directory. They are not assumed to be part of Bitcoin Core releases without build or packaging evidence.

## Attack Surface, Mitigations, and Attacker Stories

### Public P2P and consensus processing

The highest-exposure surface is inbound and outbound P2P processing: message framing and deserialization, handshake and feature negotiation, address relay, transaction relay, block download, compact-block reconstruction, headers, bloom or compact filters where enabled, and peer-state accounting. Relevant failure classes include memory corruption, integer overflow, unchecked allocation, parser differentials, consensus divergence, use-after-free, deadlock, remote assertion/exception termination, algorithmic complexity, resource-accounting bypass, eclipse/privacy weaknesses, and state-machine confusion.

Mitigations include bounded serialization types, message-size checks, per-peer state and discouragement/banning, rate and resource accounting, validation caches, chain-work rules, extensive unit/functional/fuzz coverage, and separation between policy and consensus. A safe neighboring message handler does not prove another handler safe.

Realistic attacker stories include a remote peer sending malformed or strategically ordered messages to crash or stall a node; causing memory, CPU, disk, connection-slot, or validation-work amplification; exploiting a discrepancy between prevalidation and later consumption; influencing peer selection or transaction-origin privacy; or triggering a consensus-validation discrepancy. An attacker already able to modify the node binary, trusted data directory, or operator configuration is generally out of scope unless the issue grants a further privilege or secret.

### RPC, HTTP, REST, notifications, and local IPC

The HTTP server, JSON-RPC dispatch, REST endpoints, cookie/basic authentication, RPC allowlist and bind configuration, wallet routing, ZeroMQ publishers, Qt payment/URI handling, and optional Cap'n Proto-based multiprocess IPC expose structured inputs and privileged actions. Relevant failures include authentication or allowlist bypass, cross-wallet/object authorization errors, unsafe argument conversion, secret leakage, path traversal, unintended file/network/process effects, request smuggling or parsing ambiguity, local IPC peer confusion, and denial of service.

Mitigations include authentication cookies and credentials, default loopback binding for administrative interfaces, explicit method registration, argument schemas and type checks, request/body limits, wallet endpoint selection, and operator-controlled enablement. Missing public-ingress evidence lowers likelihood but does not disprove a code-level authorization or parser defect.

### Wallet, signing, and sensitive local state

Wallet databases, descriptors, imported keys, PSBTs, transaction construction, coin selection, external signer integration, backups, migration, and encryption hold or act on high-value secrets. Relevant failures include private-key disclosure, signing the wrong transaction or script, bypassing wallet locking, passphrase mishandling, cross-wallet confusion, malicious descriptor or PSBT parsing, unsafe backup/restore paths, symlink or permission errors, and persistent corruption.

Mitigations include wallet encryption and unlock state, script and descriptor validation, database transactions, explicit wallet contexts, ownership and solvability checks, secure memory practices, amount/fee checks, and test coverage. A report must distinguish intentional operator capabilities from behavior reachable by an untrusted RPC client, malicious file, external signer, or lower-privileged local actor.

### Filesystem, configuration, databases, and startup

Configuration parsing, include files, settings persistence, data-directory locks, block/undo files, chainstate databases, wallet databases, snapshots, caches, PID/socket/cookie files, log paths, and import/export utilities cross filesystem trust boundaries. Relevant failures include traversal, symlink/hardlink races, unsafe permissions, arbitrary overwrite/read, decompression or parsing bombs, rollback or corruption, inconsistent validation after restart, and path confusion between wallets or networks.

Mitigations include path normalization and allowed-root checks, file locks, atomic writes/renames, fsync and database transactions, checksums, magic/version validation, network-specific directories, and restrictive credential-file permissions. Operator-selected arbitrary paths are not inherently findings; lower-trust control or a broken containment invariant is required.

### Cryptography and privacy

Cryptographic code covers ECDSA/Schnorr operations, hashes, key derivation, random number generation, signature verification, and secure erasure. Privacy-sensitive behavior includes peer selection, address relay, transaction broadcast, proxy DNS behavior, Tor/I2P integration, wallet metadata, and logs. Relevant failures include nonce or randomness weakness, secret-dependent timing, invalid-curve or encoding acceptance, signature/verification mismatch, sensitive logging, proxy bypass, address leakage, and transaction-origin deanonymization.

Mitigations include mature specialized libraries, exhaustive and vector-based tests, constant-time implementations and declassification annotations, strong randomness plumbing, explicit proxy/network controls, and privacy-aware relay logic. Pure theoretical cryptographic concerns require concrete violation evidence; cryptographic correctness or key-exposure failures can be severe even without a conventional remote service boundary.

### Build, dependencies, tests, and developer tooling

CI workflows, `depends`, CMake, packaging, release scripts, linters, generators, fuzz corpora, functional-test helpers, examples, and the untracked JavaScript toolchain can execute code in developer or automation contexts. Relevant failures include pull-request command injection, unsafe archive extraction, dependency-source substitution, checksum/signature bypass, secret exposure in privileged workflows, and release artifact tampering.

Mitigations include pinned hashes/commits, reproducible `depends` builds, restricted CI event contexts, review, sandboxing, and separation of test-only code from installed binaries. Deliberately unsafe fixtures, mock credentials, test-only RPC servers, examples, and source-map copies are not production vulnerabilities without an execution or packaging path across a meaningful boundary.

## Severity Calibration (Critical, High, Medium, Low)

### Critical

- Remotely reachable unauthenticated code execution or exploitable memory corruption in default P2P processing.
- A practical consensus-validation flaw that can cause acceptance of invalid blocks/transactions, network split, or theft at scale.
- Remote or cross-boundary compromise of wallet private keys, signing authority, release-signing credentials, or broad authentication state.
- A realistic supply-chain path that silently produces or distributes attacker-controlled trusted Bitcoin Core binaries.

Critical requires a clear, low-friction path and immediate, compromise-equivalent impact; a dangerous primitive or speculative chain is insufficient.

### High

- Remotely triggerable memory corruption, sensitive file/key disclosure, or durable high-impact denial of service with clear reachability but meaningful exploitation constraints.
- Authentication/authorization bypass over an enabled administrative boundary that exposes sensitive wallet/node data or privileged operations.
- Practical signing-intent, wallet-locking, descriptor/PSBT, cryptographic, update, or build-integrity failure with major but bounded impact.
- Unsafe archive, filesystem, or dependency handling that gives a lower-trust actor meaningful code or release-artifact influence.

### Medium

- Real P2P resource amplification, crash, privacy leak, or state corruption with material constraints, limited persistence, or non-default conditions.
- Same-operator or local-network RPC/IPC boundary violations with meaningful data or integrity impact.
- Lower-privileged local file/IPC issues, narrower wallet/object authorization errors, or CI weaknesses requiring non-secret-capable workflow access.
- Cryptographic or protocol robustness defects whose concrete impact is bounded and does not enable forgery, key recovery, or consensus failure.

### Low

- Narrow, transient, or difficult-to-trigger availability effects; low-sensitivity information disclosure; local-only weaknesses without privilege escalation; or defense-in-depth gaps with a concrete but limited attacker benefit.
- Developer/test/example issues that cross a small real boundary but are not shipped or privileged by default.
- Missing hardening where existing controls substantially limit exploitability and a meaningful security consequence remains.

Correctness bugs with no realistic lower-trust attacker path, intentionally privileged operator behavior, benign test fixtures, generated translations/source maps, and dependency presence without a reachable affected path are not reportable security findings, though they remain accounted for in coverage.

Repository: target_sha256_288a08a59bfebd85ea5bae25fab12b655512a9644b2a14fcfdaf7f5e29ff6ffc
Version: e69dbe47927ee94d3c640c4c55cd29ecb6be907f
