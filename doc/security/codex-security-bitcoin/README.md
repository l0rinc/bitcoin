# Codex Security investigation artifacts

This directory contains the locally reviewed results from the Codex Security
scan of the Bitcoin Core checkout on 2026-07-29.

The scan was interrupted after the model response was blocked by the security
policy. It therefore produced no final scan-manifest.json, findings.json,
coverage.json, or report.md; Codex did not confirm any finding. The
investigation-report.md file is the independent source review performed
afterwards. It records prerequisites, reproducible local tests, expected
behavior, false-positive conditions, and minimal fixes for every discovery
candidate. A candidate is called source-established only where the code path
itself is sufficient proof; otherwise it remains a reproduction-pending
hypothesis.

## Included evidence

* evidence/raw_candidates_*.jsonl — the raw candidate records emitted by the
  scan, including empty shards so the shard set is complete.
* evidence/normalized_candidates_08_check.jsonl — the normalization check for
  the last candidate shard.
* evidence/context/ — the scan threat model, guidance, snapshot digest, and
  source inventory.
* evidence/coverage/ — all discovery coverage summaries and receipt JSONL
  files.

The original full partial scan remains at
/data/my_storage/codex-security-results/bitcoin-full. Its .work/ build
products, temporary probe binaries, generated compiler output, and large
*_sensitive_* intermediate files are intentionally not copied into the Git
history: they are not findings, are not needed to reproduce the review, and
would add generated binaries and source-sensitive material to the repository.
The original absolute paths and the missing final artifacts are recorded in
the report.

No Bitcoin Core source or test files were changed by this investigation commit.
