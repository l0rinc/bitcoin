# Persisted GCS filter payload validation and recovery audit

## Seed journal

This goal was added after Cycle 308's serialization sweep found a deliberately
unchecked path in `BlockFilterIndex::ReadFilterFromDisk()`. The path verifies
the encoded filter's hash against the LevelDB row and then constructs the
`BlockFilter` with `skip_decode_check=true`, while network/RPC deserialization
performs full Golomb-Rice decoding. The historical rationale says the hash is a
cheaper corruption check, so this is a follow-up investigation rather than a
finding: determine whether coupled local corruption, stale database metadata,
partial writes, or an in-process writer bug can produce a malformed but
hash-consistent payload and what query/restart contract should follow.

Prior evidence:

- `src/index/blockfilterindex.cpp:161-189` reads a block hash and encoded
  filter, checks the expected block hash and `Hash(encoded_filter)`, then uses
  the unchecked constructor.
- `src/blockfilter.cpp:51-75` checks the CompactSize `N` and computes `m_F`,
  but skips all bitstream validation when requested.
- `src/blockfilter.cpp:111-141` later trusts `m_N` in `MatchInternal()` and
  decodes that many Golomb-Rice values.
- Commit `b0a53d50d9` explicitly introduced the skip for the index because a
  hash check was intended to be cheaper than decoding. Preserve that rationale
  and test its actual threat model before proposing a fix.
- A tiny unchecked payload declaring `N=UINT32_MAX` is a useful bounded-work
  probe, but it is not by itself a defect: an ordinary one-sided flat-file bit
  flip fails the stored hash check, and changing both the file and DB hash is a
  local corruption model that must be justified.

## Required cycle protocol

Create a dedicated branch and record base/HEAD, dirty state, exact fixtures,
commands, raw output, and all corruption schedules here. Search prior journals,
history, tests, and review discussion before selecting a case. Keep scratch
datadirs, filter files, and databases isolated; never touch a default datadir.

For every candidate, distinguish ordinary flat-file corruption, DB-only
corruption, coupled file/DB corruption, crash-window state, and malicious
in-process modification. Verify the expected contract from callers and docs:
return a clean lookup failure, reject index startup, rebuild, or preserve a
bounded error state. Exercise truncated varints, huge `N`, unary runs that
exhaust the payload, excess trailing bits/bytes, wrong block hash, wrong filter
hash, wrong position, and stale height/hash rows.

Require an independent oracle for every conclusion: a valid filter generated
from a block, a reference GCS decoder, a full checked constructor, a restart
and re-query comparison, or a mutation that proves the test detects skipped
validation. Measure work and memory with low limits rather than uncontrolled
large inputs. If no local defect is proved, retain the smallest fixture and
add the next distinct persistence/recovery hypothesis rather than weakening
the intentional performance shortcut.
