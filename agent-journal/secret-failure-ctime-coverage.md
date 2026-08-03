# Secret-input failure-path ctime coverage audit

## Seed from Cycle 320

This campaign was added after Goal 14 (`secret-control-flow`) found a focused
coverage gap in current libsecp256k1. The valid MuSig nonce paths were covered
by `src/secp256k1/src/ctime_tests.c`, but no case marked an invalid MuSig
secret key undefined and exercised the documented failure path.

The added case is in commit `83014b6d02`, with detailed source review and
raw-result notes in `agent-journal/secret-control-flow.md`. It uses a valid
public key/cache, a zero invalid secret key, nonzero session randomness, and
defines only the documented public output/status after the call. Clang 19
MSan with `SECP256K1_ASM=OFF`, Debug/O1 rebuilt and passed `ctime_tests` with
status 0 and an empty log. Replacing the constant-time invalidation call with
an ordinary branch on the secret-derived status caused exit 86; the first
diagnostic was at `src/secp256k1/src/modules/musig/session_impl.h:422:9`.

## Campaign scope

Audit secret-bearing failure paths across ECDSA, ECDH, Schnorr, MuSig,
ElligatorSwift, Silent Payments, wallet key handling, and maintained
bindings. Inventory zero and overflow scalars, invalid session randomness,
malformed secret-state objects, failed callbacks, output-on-failure cleanup,
secret lifetime transitions, and declassification of status or public output.

For every candidate, establish whether the branch depends on secret bytes,
public metadata, or an explicitly documented invalid-input result. Mark only
secret bytes undefined in ctime harnesses; define public outputs and return
statuses at their documented boundaries. Use minimized MSan/Valgrind ctime
cases and a temporary secret-branch mutation to prove that a new oracle is
sensitive. A passing ctime run is regression evidence, not timing proof.

## Prior evidence to avoid repeating

- ECDH invalid-scalar ctime coverage and invalid public-key propagation are
  already covered in `constant-time-boundary.md` and `secp-nonce-session.md`.
- MuSig public-key, public-nonce, session-status, and EllSwift auxiliary-input
  declassification boundaries have independent ctime mutation evidence in
  `statistical-timing.md` and `constant-time-boundary.md`.
- Valid secret paths and the Cycle 320 invalid MuSig secret-key case already
  run in the current ctime build. Reopen them only for a distinct caller,
  compiler, backend, or failure contract.

## First queue

1. Inventory all `ctime_tests.c` secret calls against public headers and
   current failure tests; record missing invalid-input cells without changing
   production code.
2. Add one deterministic case for a distinct secret-input failure path, if
   its output/status contract and constant-time expectation are explicit.
3. Use a temporary mutation at the exact secret-to-status or cleanup boundary,
   restore it, and retain the first-invalid trace and restored run.
4. Compare ctime coverage with sanitizer, test, and binding error paths; do
   not broaden suppressions or treat variable-time public APIs as secret bugs.

## Constraints

Preserve the existing user and agent artifacts, use scratch data only, and
avoid large builds while storage is critically constrained. Record exact build
flags, seeds, raw logs, source commit, and limitations in every cycle.
