# Assessment: Buzz (block/buzz) mirror of bitcoin/bitcoin (2026-07-24)

## What Buzz provides
Self-hosted Nostr-relay workspace with git hosting backend (smart HTTP: /git/{owner}/{repo}/info/refs, git-upload-pack, git-receive-pack), CAS repo storage, pack cache, pre-receive hook → localhost policy engine (buzz-protect rules on NIP-34 kind:30617; channel role = repo role). NIP-34 git events work today.

## Hard constraint
transport.rs:8 — "Auth: NIP-98 on all routes (clone + push). No public repos for v1." Any authenticated key can clone, but NO anonymous read. A truly public mirror like mirror.b10c.me needs a ~20-line patch to GitAuth (allow unauthenticated GET info/refs + POST upload-pack) — or accept key-gated access.

## Complexity by goal
- **Git mirror + sync cron (LOW, ~1 day)**: docker compose up (Postgres 17, Redis 7, MinIO) → buzz-admin generate-key + git-credential-nostr (git ≥2.46) → `git clone --mirror` + `git push --mirror` (initial import on relay host — policy callback is localhost-only) → external cron `git fetch && git push --mirror`. Buzz YAML workflows CANNOT run git (no shell action) — sync lives outside.
- **b10c-style PR/issue listing (HIGH, ~1 week)**: ~35.7k PRs + ~3-4k issues + ~300-600k comments + ~5-10k users ≈ 0.5-0.7M signed events (~1-2GB Postgres). Mapping: PR/issue → kind:45001 forum post; comments → kind:45003 threaded; users → kind:0 profiles; reactions → kind:7; state changes → parameterized-replaceable. Importer (~500-800 lines, GitHub GraphQL) + syncer cron. Attribution is textual (mirror-bot signs), not cryptographic — standard mirror trade.
- **Sweet spot (LOW-MEDIUM, works today)**: git mirror + channels per work area + NIP-34 patch events + buzz-cli/ACP agents doing review workflows (our audit/PR-review tooling ports directly).

## Recommendation
Start with git mirror + sync cron (1 day), verify a full --mirror push survives at bitcoin/bitcoin scale (100k+ commits, ~700 tags — young implementation, smoke-test first), then decide on the custom GitHub-metadata importer. Two-way write-back bridge (Buzz comments → GitHub API) is a later, separate decision.
