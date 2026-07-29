# Defensive review shard 07b

- Assigned manifest slice: lines 5139–5344 inclusive
- Assigned paths: 206
- Reviewed paths: 206
- Unique coverage paths: 206
- Raw candidates: 4
- Unreadable or blocked paths: 0

## Scope and accounting

The slice contains 100 production C/C++ files, 16 Qt test files, 59 embedded
or source GUI images, and 31 secp256k1 documentation, metadata, build, and
nested CI files. Every file was read. All 39 PNGs decoded successfully with
small fixed dimensions; SVG sources contained no scripts, entities, event
handlers, or external resource loads. The secp256k1 nested workflow files are
not active Bitcoin root workflows, while the bundled library and build inputs
were still reviewed for relevant product and supply-chain reachability.

Production review covered Qt command/history handling, BIP21 and legacy wallet
metadata display, PSBT import/export, wallet create/open/restore/migration
activities, transaction rendering/export, randomness and entropy collection,
REST request handlers, RPC dispatch and privileged methods, scheduler
lifecycle, consensus script interpretation, signing, descriptor/miniscript
parsing, and lax DER parsing. Generated assets and tests were distinguished
from shipping execution paths, but were not omitted from coverage.

## Raw candidates

1. `src/qt/transactiondesc.cpp` parses a legacy stored BIP70 merchant common
   name without validating the type/length bytes or the claimed length against
   the remaining buffer before calling `QString::fromUtf8`.
2. `src/qt/walletframe.cpp` enforces the PSBT limit using one file stream, then
   reopens the path and reads to EOF without a bound, allowing replacement or
   concurrent growth to bypass the intended local-file resource limit.
3. `src/qt/rpcconsole.cpp` omits `importdescriptors` and
   `descriptorprocesspsbt` from its explicit private-command redaction list,
   leaving private descriptor material visible and retained in console history.
4. The BIP21-label-to-transaction-history flow exports formula-leading labels
   to CSV with quote escaping only, leaving spreadsheet formula interpretation
   possible after the user exports and opens the file.

These are discovery candidates for the canonical validation and reachability
phases, not final findings. All other reviewed behaviors were bounded,
privileged/operator-intentional, test/build-only, generated/declarative, or
lacked a supported lower-trust path under the supplied threat model.
