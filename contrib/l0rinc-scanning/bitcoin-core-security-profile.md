# Bitcoin Core Security Knowledge Profile

Snapshot date: 2026-08-15

This profile is a routing and experiment guide for the reusable goal catalog. It is not a replacement for the full audit knowledgebase, current source, issues, PRs, or private disclosure procedure.

## Pinned inputs

- Goal catalog source: `Pasted markdown(20260815-065116).md`
  - SHA-256: `27676efe7ebde63942dda9972156075ff58300cf283908a86a5520687ba67cfe`
- Core/secp audit knowledgebase: `Pasted markdown (2)(1).md`
  - SHA-256: `5ddd768e1577dde408dd8ab97dd5f54fb2bdf172c90e46232f6d927246a88bc4`
- Official advisory index: https://bitcoincore.org/en/security-advisories/
- Bitcoin Core security policy: https://github.com/bitcoin/bitcoin/blob/master/SECURITY.md
- BIP50 post-mortem: https://github.com/bitcoin/bips/blob/master/bip-0050.mediawiki

Always re-fetch or re-read current upstream state before acting. This snapshot records knowledge and ownership, not permanent truth.

## Mandatory knowledge and ownership gate

Before selecting, checking out, reviewing, fixing, or publishing a Core candidate:

1. Search the full supplied knowledgebase by PR number, branch, source commit, patch-id, symbol, feature, bug shape, test name, and related PR family.
2. Apply every warning, split recommendation, coverage gap, supersession, negative result, and ownership note.
3. Verify the claim against current HEAD, current upstream, maintained release branches, and current public/fork ownership.
4. Classify it:
   - `LIVE_UNOWNED`: current behavior is reproduced and no owner exists.
   - `OWNED`: an open public or fork branch owns the same root and intended behavior.
   - `MERGED`: current upstream contains the fix.
   - `SUPERSEDED`: a later change removed the state or replaced the mechanism.
   - `REFUTED`: a concrete caller, format, invariant, or experiment disproved the claim.
   - `ASSURANCE_ONLY`: test/oracle/harness improvement with no production defect.
   - `UNKNOWN`: evidence is stale, incomplete, environment-only, or not replayed.
5. Do not create code for `OWNED`, `MERGED`, `SUPERSEDED`, or `REFUTED` entries unless new evidence changes the classification. Record the saved investigation instead.

## Core priority ladder

1. Consensus inflation, invalid acceptance, permanent chain split, or protocol-level theft.
2. Remote memory corruption, use-after-free, RCE, fatal assertion, or process crash under realistic conditions.
3. Wallet key/funds loss, signing-intent substitution, unauthorized transaction acceptance, or unrecoverable wallet state.
4. Durable chainstate/block/undo/index corruption, false progress, or persistent restart denial.
5. Censorship, block/transaction propagation failure, IBD stall, or peer-state corruption.
6. Practical default-node memory/CPU/disk exhaustion with a demonstrated kill or long stall.
7. Authenticated, opt-in, local-file, component-API, and latent defects, accurately labeled.

The current knowledgebase already contains multiple public/owned memory-amplification families. Pure memory/disk/CPU variants should occupy no more than one of five Core cycles unless they demonstrate a distinct root, a default-node kill, memory corruption, state corruption, consensus effect, or a regression in an existing mitigation.

## Historical advisory root-cause matrix

| Seed | Historical failure shape | Translate into current audit questions | Preferred proof |
|---|---|---|---|
| BIP50 | A database implementation limit changed which valid block a version accepted, splitting the network | Which consensus path still depends on database, filesystem, architecture, cache, or build behavior? Are failures mapped identically across backends? | Same vectors on old/new versions, backend/config differential, exact chain fork |
| CVE-2018-17144 | An optimization removed full duplicate-input validation; an assertion and UTXO semantics produced crash/inflation | Is any consensus check now only an assertion, `Assume`, caller precondition, cache invariant, or test assumption? Does failed mutation leave coins dirty/spent? | Historical mutant plus UTXO reference model and supply oracle |
| CVE-2024-52911 | Early return destroyed transaction precomputation before background script checks completed | Which queued task borrows caller-owned memory? Do all early returns, exceptions, cancellation, and shutdown drain it before destruction? | Deterministic barrier, ASan first invalid access, construction-order mutation |
| CVE-2024-35202 | A one-shot compact-block object remained reusable after failed reconstruction | Which parsers/reconstruction/session objects can be called twice after partial success/failure? Are counters/output reset transactionally? | Repeated message/state transition with fatal-assert and output oracle |
| CVE-2024-52921 | One peer's mutated block cleared download state owned by other peers | Can peer A mutate peer B's timers, in-flight ownership, reconstruction, request suppression, or eviction protection? | Multi-peer deterministic trace and peer-local state snapshots |
| CVE-2024-52922 | First announcer could withhold a block and delay alternate download | Can ownership/timeouts prevent honest fallback, replacement, or parallel progress? | Mock-time stalling trace with honest-peer progress bound |
| CVE-2024-52914 | Orphan resolution nested a bounded collection scan with expensive validation | Which bounded structures still compose quadratically or restart expensive work after invalid results? | Low-limit operation-count oracle, not wall time alone |
| CVE-2024-52913 | Eviction from a bounded request map reset authority and enabled indefinite censorship | Can queue eviction, retry, disconnect, or cache pressure return exclusive request ownership to an attacker? | Mock-time request model and eventual-honest-request property |
| CVE-2024-52912 | Signed overflow plus `INT64_MIN` absolute-value behavior bypassed time limits | Where do signed sentinels, negation, `abs`, subtraction, or wall-clock jumps bypass a security bound? | Boundary vectors across compilers/widths and network-time scenario |
| CVE-2019-25220 / CVE-2024-52916 | Low-work header chains retained attacker-influenced state | Are limits per message/peer while aggregate or phase-specific state remains unbounded? Can state be replayed across roles? | Aggregate accounting and lifecycle cleanup with fixed low limits |
| CVE-2024-52919 | Long-running addr insertions overflowed a 32-bit identifier; rate limiting only reduced feasibility | Which monotonic IDs/counters can wrap over long horizons? Did a mitigation lower rate without removing the terminal condition? | Accelerated boundary test and exact time/peer-cost calculation |
| CVE-2020-14198 | An attacker-influenced unbounded list was scanned inside another loop | Which global lists/maps interact with per-request scans, sorting, serialization, or RPC output? | Explicit complexity/work-count bound and cleanup proof |
| CVE-2015-3641 | Each incomplete connection could reserve the maximum receive buffer | Which limits are per connection while aggregate process memory escapes accounting? | Multi-peer low-memory replay and exact retained-owner ledger |
| CVE-2024-52915 | A legal huge inventory message caused large per-peer allocation | Does wire size underestimate object/container overhead or transient reallocation overlap? | Wire-to-recursive-memory expansion ratio and constrained replay |
| CVE-2024-52920 | Malformed `GETDATA` could enter an infinite processing loop | Does every parser/queue iteration consume input, change state, or terminate? | Progress measure and mutation that removes consumption |
| CVE-2017-18350 | Signed `char` length became a huge unsigned receive length | Where do byte values cross signedness, width, ABI, syscall, allocation, or shift boundaries? | Architecture/compiler matrix plus sanitizer and exact byte vector |
| CVE-2015-20111 | A non-default dependency exposed local-network memory corruption/RCE | Which optional dependencies, services, plugins, and subprocesses cross a less-reviewed trust boundary? | Reachability with feature enabled, pinned affected version, upstream fix |
| CVE-2025-46597 | A size check overflowed on 32-bit and alternate compact-block ingress made a pathological block conceivable | Do all ingress paths share bounds? Are size/offset calculations width-safe before disk or allocation? | 32-bit build, alternate-ingress fixture, boundary arithmetic proof |
| CVE-2025-46598 | Invalid unconfirmed scripts triggered seconds of validation work without disconnection | Which invalid contexts perform repeated hashing/signature work before cheap rejection? | Operation counters, invalid-vector corpus, honest-progress impact |
| CVE-2025-54604 / 54605 | Attacker-controlled events caused unbounded repeated logging and disk fill | Are logs globally rate-limited, deduplicated, escaped, and bounded per root event? | Log-volume counter over mock time and rotated/restart behavior |
| Inv-to-send sorting DoS | Repeatedly growing sets made periodic sorting dominate communication | Which queue drains re-sort/rebuild all retained state? | Work count per appended item and progress under adversarial schedule |

## Knowledgebase-derived Core families

### 1. Validation and block-index container integrity

Recurring shapes:
- Failed blocks remain in candidate sets.
- Comparator or tie-break fields are mutated while the object remains in an ordered set.
- Pruned/redownloaded blocks are not erased and reinserted.
- Linkable children disappear from `m_blocks_unlinked`.
- Pure disconnect/restart locators are incorrectly considered synced.
- One peer's invalid or mutated block changes another peer's download state.

Required oracles:
- Recompute candidate eligibility and strict ordering after every transition.
- Run `CheckBlockIndex()` and compare all chainstates.
- Mutate each erase-before-key-change and failed-block exclusion.
- Restart after prune, redownload, failed branch, and same-work forks.

### 2. UTXO, undo, and block-storage transactionality

Recurring shapes:
- Duplicate-input validation and cache flags can interact catastrophically.
- Failed `ConnectBlock` or cache operations risk partial state.
- Dirty undo files or block files may be published before durable sync.
- Retryable block-index writes can forget dirty entries.
- Metadata, prune locks, and index locators can advance ahead of durable data.
- 32-bit file positions or undo sizes can wrap.
- Failed seek followed by truncate can damage earlier data.

Required oracles:
- Independent UTXO map and supply equation after every transaction/block.
- Connect/disconnect and full-sync/reindex/snapshot convergence.
- Failure before/after write, sync, rename, metadata, and in-memory adoption.
- Last-good-copy and retry-union assertions.

### 3. Compact-block lifecycle and work ownership

Recurring shapes:
- One-shot reconstruction reused after failure.
- Partial counters/output not reset.
- Collision or invalid responses recreate request state.
- Mutated blocks clear another peer's state.
- Repeated optimistic reconstruction rescans the mempool under global locks.
- Alternate compact ingress reaches platform/size paths full block ingress cannot.

Required oracles:
- Per-peer ownership snapshots.
- One-shot/reset lifecycle state machine.
- Full versus compact validation/result convergence.
- Work counters under repeated wrong, late, or replayed `blocktxn`.

### 4. Concurrency, lifetime, and callback teardown

Recurring shapes:
- Background script checks borrow destroyed precomputation.
- DB iterators outlive wrapper replacement or cache resize.
- Index readers observe restart-time chainstate publication.
- Wallet/GUI callbacks retain destroyed owners.
- IPC, external-signer, or kernel callbacks reenter or outlive their context.

Required oracles:
- Deterministic barriers at every destruction edge.
- ASan and TSan in separate builds.
- Explicit start/stop/unregister/flush/join linearization.
- Construction/destruction order mutation.

### 5. Wallet authority and durability

Recurring shapes:
- Encryption or passphrase changes report success after failed writes.
- Descriptor/keypool/cache memory advances ahead of database state.
- Failed writes leave mixed encrypted/plaintext records or unusable recovery.
- Counterparty PSBT metadata reaches assertions.
- An external signer can redefine the transaction instead of only signing intent.
- Authenticated RPC iterator/pagination arithmetic can crash the process.

Required oracles:
- Fault injection at every database operation and restart.
- Intent digest before/after external signing.
- PSBT role-order and output-on-failure state.
- Reservation/keypool/address reuse rollback.

### 6. Exact arithmetic and representable-domain contracts

Recurring shapes:
- Modified-fee, RBF, TxGraph, and MiniMiner sums/negations leave `int64_t`.
- Saturating arithmetic can violate associativity, reversibility, and exact fee-ratio rules.
- Long-horizon IDs, positions, heights, times, and counters narrow silently.
- Configuration/RPC values overflow before validation.

Required oracles:
- State mathematical domains before changing code.
- Use checked reference arithmetic and exact boundary vectors.
- Compare every incremental aggregate with recomputation.
- Do not use saturation as a generic repair without proving downstream algebra.

### 7. Snapshot, cache provenance, and context reuse

Recurring shapes:
- Duplicate snapshot records can preserve a UTXO hash while falsifying metadata.
- A snapshot can be accepted initially but fail later background validation.
- Snapshot cleanup/restart can become permanently stuck.
- Reused kernel/mining block objects carry trusted caches across network contexts or mutation.
- Script/signature caches may omit a semantic input.

Required oracles:
- Fresh versus reused object/context comparison.
- Semantically valid snapshot later invalidated by background validation.
- Crash at every takeover/cleanup rename.
- Temporary omission of each cache-key/provenance input.

### 8. P2P liveness, censorship, and state isolation

Recurring shapes:
- Empty headers retain the only initial-sync slot.
- Low-work/future-time arithmetic bypasses commitment bounds or triggers fatal policy.
- Transaction request ownership can be monopolized or reset through eviction.
- Orphan and request state may cross product across peers/parents.
- Peer role/permission flags disagree with accepted behavior.
- Mutated/stalling blocks hinder propagation.

Required oracles:
- Mock-time eventual-progress properties.
- Per-peer versus aggregate state accounting.
- Disconnect/reconnect cleanup.
- Honest-peer progress while one peer withholds, replays, or sends valid empty responses.

### 9. Release and maintainer trust

Recurring shapes:
- Inactive, revoked, or untrusted signatures count toward a verification quorum.
- Git ancestry verification can accept a revision outside trusted history.
- Security fixes or fix-the-fix commits can miss release branches.
- CI/tool downloads or generated artifacts are not pinned or authenticated.

Required oracles:
- Disposable keys/repositories and explicit trust sets.
- Semantic backport replay per release.
- Reproducible artifact/hash/signature verification.
- Distinguish cryptographic validity from authorization.

## Current ownership and saturation notes

As of the attached 2026-08-14 knowledgebase snapshot, the following resource families already have public/fork owners and should not be rediscovered as novel work without a distinct root or regression:

- transient witness decoder expansion;
- orphan witness dynamic-memory undercount;
- aggregate large-response and receive memory;
- `relay=0` request/orphan cross-product;
- aggregate `GETDATA` queues;
- compact reconstruction transaction memory;
- several transaction/count vector bounds.

Higher-value non-resource seeds in the same snapshot include external-signer transaction substitution, Signet authorization parsing, reusable kernel/mining validation-cache provenance, an AssumeUTXO undisconnectable-fork path, MuSig2 PSBT assertion handling, release-key trust/quorum, wallet RPC pagination overflow, Git ancestry verification, compact-block repeated-work/replay, wallet/database transactionality, and block/undo durability.

These are routing facts only. Search current ownership and revalidate before work.

## Fuzzer-boundary rules from the knowledgebase

A removed fuzzer restriction is not automatically a production bug.

- Malformed IPC JSON, full-range fees, snapshot edge cases, transport command lengths, pool-resource alignment, and protocol bounds require an independent expected-state model before widening.
- Keep cost bounds that prevent quadratic harness work unless the production path itself admits that work.
- Preserve unusual catches, clamps, magic values, and special seeds as suspicious evidence; temporarily remove them on a branch, minimize any failure, and prove the production precondition.
- Separate four verdicts: production defect, harness defect, invalid oracle, and valid-domain coverage only.
- Hundreds of local oracle commits are assurance material; do not count them as separate vulnerabilities.

## Negative knowledge and hard controls

Do not revive these without evidence that invalidates the stated reason:

- The old null compact-extra-transaction condition was removed by the `reserve`/`emplace_back` design merged in #35670.
- PSBT trailing bytes are rejected by production outer decoders; direct inner deserializer construction is not the public framing contract.
- Scientific-notation RPC amounts are intentional and tested through `ParseFixedPoint`.
- Earlier `rotr` UB and UniValue assertion claims were retracted after audit.
- Duplicate oracle batches with repeated subjects/dates are not new roots.
- A component API hazard with no affected in-tree/public caller must be labeled latent, not remote or consensus.
- Local data corruption alone is not a peer vulnerability unless a peer can cause the write or exploit the later state.
- External scan/dossier candidates remain leads until clean-current-master reproduction and ownership checks.
- A passing sanitizer, timing test, or differential is supporting evidence, not exhaustive proof.

## High-value experiment recipes

### Historical mutant replay

1. Pin a vulnerable and fixed revision.
2. Extract the smallest semantic change, not the whole patch.
3. Apply the vulnerable shape as a temporary mutant to current HEAD.
4. Run the historical vector plus a current public path.
5. If the mutant survives, add an independent oracle before searching variants.

### Cross-ingress block convergence

Deliver identical blocks through full P2P, compact P2P, RPC, disk import, kernel, and mining interfaces. Compare validation result, cache provenance, block-index state, UTXO digest, peer ownership, notifications, disk/undo state, and restart behavior.

### Durable-state schedule

Inject one failure before and after every write, flush, fsync, rename, metadata update, dirty-set clear, in-memory adoption, and callback. Retry and restart. Compare with an uninterrupted control and preserve the last good copy.

### Peer ownership schedule

Use at least two attacker peers and one honest peer. Record every request owner, timer, in-flight entry, compact reconstruction, orphan/request pair, and eviction protection after each message, disconnect, and timeout. One peer must not mutate another's state.

### Cache-provenance schedule

Validate an object, mutate one semantic dimension or reuse it under another context/network, then compare cached and fresh execution. Temporarily omit each key/provenance input to prove the oracle.

### Valid-work adversarial block

Separate chain preparation, trigger block, proof-of-work cost, victim configuration, and affected architecture. Test full/compact ingress, inline/queued script checks, block/undo write, restart, and reorg.

## Goal mapping

The enriched catalog amends goals 0, 2, 8, 26, 32, 33, 38, 49, 61, 66, 72, 85-89, 117, and 128. It adds goals 142-160 for advisory variants, UTXO/undo conservation, invalid-block containment, compact-block lifecycle, block-index containers, durable retries, AssumeUTXO, cache provenance, async script checks, headers/time, transaction censorship, external signers, wallet DB transactionality, file-width boundaries, release backports, release verification, critical APIs, valid-work blocks, and negative controls.
