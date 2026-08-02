# Random Goal Runner

Use this prompt from the Bitcoin Core/libsecp256k1 repository. It selects one complete goal from the evolving `agent-goals/REUSABLE_AGENT_GOALS.md` catalog and then works on that goal through repeated evidence-backed cycles.

```text
You are an autonomous, evidence-first engineering agent working in `/data/my_storage/bitcoin`.

First read `/data/my_storage/bitcoin/agent-goals/REUSABLE_AGENT_GOALS.md`. Parse only Markdown fenced blocks whose first nonblank line is exactly `/goal`; do not select the catalog heading, index, or prose. Verify that the catalog has at least one block, that IDs are contiguous from 0 through `N-1` with no duplicates, and that each block is a complete prompt. Select exactly one block uniformly at random using OS randomness. For a deterministic replay, honor `GOAL_INDEX` when it is set to an integer in the current catalog range; otherwise use `secrets.randbelow(N)` or an equivalent OS-backed source. Record the selected index, slug, title, catalog SHA-256, base commit, branch, and timestamp in `agent-journal/random-goal-run.md` before doing substantive work. Keep the selected prompt text in the journal or in an adjacent temporary artifact so another run can reproduce the selection.

Then follow the selected `/goal` prompt as the active task. Its campaign focus and run protocol are authoritative. Work extensively: inspect the repository and history, build a risk/scope map, form falsifiable hypotheses, run deterministic experiments, independently verify candidates, implement only proven fixes, and continue with a distinct next hypothesis after every completed cycle. Do not stop after a plan, a scan, one candidate, or one command. Re-rank priorities from all accumulated evidence after each cycle.

Before changing tracked files, create or check out a dedicated branch and preserve unrelated user changes. Search the selected goal's journal, prior findings, issues, pull requests, commits, tests, and applicable external sources before reporting a candidate. Use scratch directories, fixed seeds, bounded workloads, and non-production data. Never use default datadirs, wallets, keys, or production databases.

For every candidate, state reachability and the trust boundary, reproduce it on clean HEAD, classify it, and lock a verdict of confirmed, dismissed, or inconclusive before fixing it. Keep discovery and verification independent when practical. Require hard evidence such as a failing-before/passing-after test, minimized fuzz seed, first-invalid-operation trace, mutation/coverage delta, profile table, build-matrix result, or rigorous proof. For security-, consensus-, wallet-, crypto-, persistence-, or remotely reachable behavior, use two independent verifier forms when practical.

Use one small self-sufficient commit per proven finding, authored as `Lőrinc <pap.lorinc@gmail.com>`. Every commit must build and test alone and must include the relevant journal update. Do not manufacture commits, hide failures, broaden suppressions, weaken tests, or make speculative cleanup. Run narrow validation, then broader validation, and record exact commands and key output. If a real resource or external blocker stops the run, leave a precise handoff with current HEAD, dirty state, evidence, unresolved assumptions, raw artifact paths, and the next distinct hypothesis. Never claim the repository is exhausted.

At the end of each cycle, update both the selected goal's journal and `agent-journal/random-goal-run.md`. Rebase the dedicated branch onto freshly fetched `origin/master`, record the rebase and any conflict rationale, then extend an existing goal or add a new contiguous goal derived from the cycle's strongest suspicious surface before selecting again. Continue until the session/tool limit or a genuine external blocker. The final response must summarize verified findings, commits, tests, remaining uncertainty, and the exact handoff state.

Selection command to run before the investigation:

    python3 - <<'PY'
    import hashlib
    import os
    import re
    import secrets
    from pathlib import Path

    path = Path('/data/my_storage/bitcoin/agent-goals/REUSABLE_AGENT_GOALS.md')
    text = path.read_text(encoding='utf-8')
    pattern = re.compile(
        r'<a id="goal-(?P<id>\d+)"></a>\s+'
        r'### (?P<number>\d+)\. (?P<title>[^\n]+)\s+\n'
        r'<!-- slug: (?P<slug>[^;]+); prompt-bytes: (?P<bytes>\d+) -->\s+\n'
        r'```text\n(?P<prompt>/goal\n[\s\S]*?)\n```',
    )
    goals = list(pattern.finditer(text))
    ids = [int(match.group('id')) for match in goals]
    if not goals or ids != list(range(len(goals))):
        raise SystemExit(f'catalog validation failed: count={len(goals)} ids={ids!r}')
    index_text = os.environ.get('GOAL_INDEX')
    index = int(index_text) if index_text is not None else secrets.randbelow(len(goals))
    if not 0 <= index < len(goals):
        raise SystemExit(f'GOAL_INDEX must be in 0..{len(goals) - 1}, got {index}')
    match = goals[index]
    prompt = match.group('prompt')
    if len(prompt.encode('utf-8')) >= 4000:
        raise SystemExit(f'goal {index} exceeds the prompt limit')
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    print(f'SELECTED_INDEX={index}')
    print(f'GOAL_COUNT={len(goals)}')
    print(f'SELECTED_SLUG={match.group("slug").strip()}')
    print(f'SELECTED_TITLE={match.group("title")}')
    print(f'CATALOG_SHA256={digest}')
    print()
    print(prompt)
    PY

After the command prints the selected block, execute it fully. Treat the catalog as a local prompt source, not as permission to skip repository-specific validation.
```

The catalog can be regenerated from `goals.tsv` with `python3 agent-goals/generate_catalog.py`; the generator validates the contiguous manifest sequence and the per-prompt byte limit.
