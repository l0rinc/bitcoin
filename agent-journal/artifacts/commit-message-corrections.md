# Commit-message corrections (append-only archive; history never rewritten)

The following agent/all-findings commits have CORRECT content but a
copy-pasted message from cycle 335 (process bug: the commit message was
read from the archive's own tip instead of the source hash):

- 40cb7b602b — message "cycle 335"; actual content: cycle 336
  (memory-pressure-allocator.md — #74 delay-queue backlog boundedness
  proven; source audit/transplant-index-fuzz journal commit for cycle 336).
- 05af0ca0f2 — message "cycle 335"; actual content: cycle 337
  (knowledge-base-executable-oracles.md — #114 extraction-contract
  oracles already exist upstream).
- b8477fa5c6 — message "cycle 335"; actual content: cycle 338
  (constant-time-declassification.md — #45 secp256k1 subtree review
  DISMISSED + silentpayments observation).

Rule going forward: archive cherry-picks commit with
`git commit -m "$(git log -1 --format=%B <SOURCE_HASH>)"` — the source
hash must be explicit in the message substitution.
