# Core-Enriched Reusable Agent Goals

This package extends the supplied 142-goal catalog with Bitcoin Core historical-vulnerability knowledge and the attached audit knowledgebase.

## Files

- `reusable-continuous-agent-goals-core-enriched.md` - complete 161-goal catalog.
- `bitcoin-core-security-profile.md` - mandatory Core threat, history, ownership, negative-control, and experiment profile.
- `reusable-agent-goals-core-enriched.json` - canonical machine-readable catalog.
- `bitcoin-core-security-seeds.json` - advisory and knowledgebase seed ledger.
- `core-enriched-catalog-integration-notes.md` - what changed and why.
- `validate-core-enriched-goals.py` - validates numbering, metadata, byte limits, Markdown/JSON equality, and canonical hash.

## Suggested installation

```bash
mkdir -p agent-journal
cp reusable-continuous-agent-goals-core-enriched.md agent-journal/campaign-goals.md
cp bitcoin-core-security-profile.md agent-journal/bitcoin-core-security-profile.md
```

Keep the full supplied Core/secp knowledgebase in a private local location and make it available to the agent. Do not publish undisclosed findings or private branch details merely because they appear in the local knowledgebase.

## Codex Security use

For a Core scan, provide both the focused campaign prompt and the knowledge sources. For example:

```bash
npx @openai/codex-security scan . \
  --mode deep \
  --model gpt-daybreak-blue-latest \
  --effort xhigh \
  --knowledge-base agent-journal/bitcoin-core-security-profile.md \
  --knowledge-base /path/to/PR_KNOWLEDGEBASE.txt \
  --scan-prompt-file /path/to/selected-goal.txt
```

Use `--workers`, `--subagents`, `--stop-after-no-new`, and `--max-discovery-runs` according to the host and cost budget. The catalog's Core gate requires current ownership and upstream verification before any fix is created.

## Validation

```bash
./validate-core-enriched-goals.py
```

Expected canonical goal-catalog SHA-256:

```text
d00cc061c7dbc9911898a148386820ea40f2caa3550b247a5130297db71b986a
```
