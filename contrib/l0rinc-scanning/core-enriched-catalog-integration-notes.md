# Core-Enriched Goal Catalog: Integration Notes

## Problem addressed

The supplied 142-goal catalog already has strong evidence, journaling, deduplication, and self-evolution rules. The main weakness was not breadth. It was that Bitcoin Core campaigns still depended on each agent independently rediscovering three things:

1. which historical bug shapes matter most;
2. which local findings are already owned, merged, superseded, refuted, or only assurance work;
3. which high-risk Core invariants deserve a dedicated executable campaign instead of being one paragraph in a generic goal.

The attached knowledgebase explicitly requires searching by PR, branch, commit, symbol, and PR family before acting, applying supersession and coverage notes, and replaying external reports locally. The revised catalog turns that into a mandatory Core gate.

## Result

- Base prompts retained: 142
- Existing prompts materially amended: 18
- New Bitcoin Core prompts added: 19
- Total prompts: 161
- Longest prompt: goal 92 at 3987 UTF-8 bytes
- Uber-goal: 3998 UTF-8 bytes
- Canonical goal JSON SHA-256: `d00cc061c7dbc9911898a148386820ea40f2caa3550b247a5130297db71b986a`

## Core selection changes

The catalog now ranks Core work in this order:

1. consensus inflation, invalid acceptance, permanent split, or protocol-level theft;
2. realistic remote crash, memory corruption, use-after-free, or RCE;
3. funds/key loss and signing-intent or authorization failures;
4. durable corruption, false progress, and persistent restart denial;
5. censorship, block/transaction propagation, and IBD liveness;
6. practical default-node resource exhaustion;
7. accurately labeled authenticated, opt-in, local-file, component-API, and latent defects.

The knowledgebase shows that several memory-amplification families are already public or owned. The new rules keep those findings eligible but prevent them from consuming most Core cycles unless a scan demonstrates a new root, a default-node kill, memory/state corruption, or a broken mitigation.

## Existing goals amended

- **0. Continuous evidence-first bug mining**: adds the Core knowledge/ownership gate, critical-first ranking, and a limit on repeated resource-only rediscovery.
- **2. Assertion, Assume, and invariant reachability audit**: turns assertion review into Debug/NDEBUG boundary proof seeded by the duplicate-input inflation and compact-block assertion failures.
- **8. Locking, threading, and scheduler audit**: adds script-check lifetimes, callback teardown, DB cursor replacement, and restart publication schedules.
- **26. Bug fixed in one subsystem but present in another**: adds concrete recurring Core bug families such as ordered-key mutation, dirty-set loss, peer-state contamination, and cache provenance.
- **32. Whole-history incomplete-fix and migration mining**: requires advisory and knowledgebase archaeology with current ownership and negative-knowledge checks.
- **33. External vulnerability and advisory variant analysis**: converts official advisories into a modern root-cause matrix rather than literal CVE grepping.
- **38. Failure cleanup and crash-safety audit**: adds wallet, block/undo, index, prune-lock, snapshot, and retry transactionality.
- **49. Critical whole-history must-fix sweep**: uses a critical-first severity ladder and keeps already-owned resource variants from dominating history review.
- **61. Stateful contract-fuzzer expansion**: adds named Core fuzz targets, the knowledgebase restriction inventory, and guard-removal classification.
- **66. Cherry-pick, backport, and release-branch correctness audit**: adds semantic release-backport replay, covert/fix-the-fix dependencies, and disclosure handling.
- **72. Filesystem, power-loss, and crash-consistency injection**: adds the current Core durable-state failure families and exact restart convergence requirements.
- **85. Bitcoin consensus mutation-score and kill-test audit**: adds BIP50, duplicate-input inflation, Signet/cache provenance, and architecture/context mutations.
- **86. Bitcoin chainstate, reorg, prune, and index crash-symmetry audit**: adds a reference UTXO model, cache flags, undo, candidates, prune, and crash/restart symmetry.
- **87. Bitcoin mempool, package, and eviction-accounting audit**: adds orphan/censorship/request scheduling and exact-domain fee/cluster arithmetic.
- **88. Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit**: adds signing-intent authority, external signer substitution, MuSig/PSBT assertions, and database adoption ordering.
- **89. Bitcoin P2P transport, permission, and peer-accounting audit**: adds historical P2P liveness, peer-local ownership, headers/time, compact-block, and transaction-request seeds.
- **117. Security-agent calibration with historical bugs, mutants, and negative controls**: adds a held-out Core vulnerability and difficult-nonbug calibration battery.
- **128. Bitcoin full, compact, RPC, and disk block-ingress convergence**: expands block ingress into invalid-state containment, cache provenance, peer ownership, storage errors, and restart equivalence.

## New goals

- **142. Bitcoin Core advisory-root-cause variant matrix** (`bitcoin-core-advisory-variants`)
- **143. Bitcoin UTXO, coins cache, undo, and reorg conservation** (`bitcoin-utxo-undo-conservation`)
- **144. Bitcoin invalid-block containment and rejection taxonomy** (`bitcoin-invalid-block-containment`)
- **145. Bitcoin compact-block reconstruction ownership and one-shot lifecycle** (`bitcoin-compact-block-lifecycle`)
- **146. Bitcoin block-index candidate, unlinked, and comparator-key state machine** (`bitcoin-block-index-containers`)
- **147. Bitcoin durable batch retry and dirty-set transactionality** (`bitcoin-durable-batch-retry`)
- **148. Bitcoin AssumeUTXO trust, background validation, and cleanup convergence** (`bitcoin-assumeutxo-lifecycle`)
- **149. Bitcoin validation-cache provenance and mutable-object invalidation** (`bitcoin-validation-cache-provenance`)
- **150. Bitcoin asynchronous script-check and validation-work lifetime audit** (`bitcoin-scriptcheck-lifetimes`)
- **151. Bitcoin headers-sync, adjusted-time, and IBD slot liveness** (`bitcoin-headers-time-ibd-liveness`)
- **152. Bitcoin transaction-request, orphan, and censorship-resistance state machine** (`bitcoin-txrequest-orphan-censorship`)
- **153. Bitcoin external-signer and signing-intent authorization** (`bitcoin-external-signer-intent`)
- **154. Bitcoin wallet database transactionality and fault-injection matrix** (`bitcoin-wallet-db-transactionality`)
- **155. Bitcoin block and undo file cursor, seek, and format-width boundaries** (`bitcoin-block-undo-file-widths`)
- **156. Bitcoin release-branch security backport and disclosure parity** (`bitcoin-security-backport-parity`)
- **157. Bitcoin release verification, trusted-key quorum, and Git ancestry audit** (`bitcoin-release-verification-trust`)
- **158. Bitcoin critical RPC, REST, IPC, and C++ API boundary audit** (`bitcoin-critical-api-boundaries`)
- **159. Bitcoin valid-work adversarial block and miner-gated failure campaign** (`bitcoin-valid-work-adversarial-blocks`)
- **160. Bitcoin negative-control, supersession, and refutation replay** (`bitcoin-negative-control-replay`)

## Knowledgebase lessons encoded

- A finding status is part of the security evidence. The same patch or bug shape must not be repeatedly counted after it is owned, merged, superseded, or refuted.
- Fuzz exclusions and magic guards are suspicious, but removing one does not prove a production bug. The agent must distinguish production defects, harness defects, invalid oracles, and valid-domain coverage.
- State-machine failures in Core frequently look like races even when the root is ownership, partial rollback, one-shot reuse, callback replay, or comparator-key mutation.
- Saturating arithmetic is not a generic repair for fee and cluster code because exact-ratio, associativity, and reversibility invariants may matter.
- Local corruption and component API misuse can be real defects without being peer vulnerabilities. Severity must follow the demonstrated caller and trust boundary.
- Historical fixes are most useful as executable mutants and invariant seeds, not as keyword lists.
- The Core security profile preserves hard negative controls so later models spend less time reviving already disproved candidates.

## Files in the package

- `reusable-continuous-agent-goals-core-enriched.md`: complete 161-goal catalog.
- `bitcoin-core-security-profile.md`: advisory and knowledgebase-derived Core routing profile.
- `reusable-agent-goals-core-enriched.json`: canonical machine-readable catalog.
- `bitcoin-core-security-seeds.json`: machine-readable advisory/family/negative-control ledger.
- `core-enriched-catalog-integration-notes.md`: this document.
- `validate-core-enriched-goals.py`: byte-count, numbering, slug, and Markdown/JSON consistency validator.
