#!/usr/bin/env python3
"""Generate the readable goal catalog from goals.tsv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "goals.tsv"
CATALOG = ROOT / "REUSABLE_AGENT_GOALS.md"

RUN_PROTOCOL = """/goal
Create or check out a dedicated branch before changing code. Treat this as a continuing, evidence-first investigation: after every cycle update `agent-journal/{slug}.md`, re-rank unchecked surfaces from accumulated evidence, choose the next distinct hypothesis, and continue. Never claim the repository is exhausted or follow a stale queue blindly. Stop only at a real session/tool limit or external blocker and leave an exact handoff.

Journal the base and HEAD, dirty state, scope ledger, hypotheses, exact commands and key output, confirmed/dismissed/inconclusive candidates, unrelated leads, source links and versions, review precedent, limitations, and next queue. Search the journal, issues, pull requests, and history before reporting to avoid repeats. For every online pull request, record stated priorities, accepted and rejected approaches, whether preferences are general or contextual, and likely review objections.

Prefer few definitive findings. Use one independent, self-sufficient commit per finding, authored as `Lőrinc <pap.lorinc@gmail.com>`, including its journal update. Every commit must build and test alone and be correct without later commits. Keep the smallest correct diff: no whitespace churn, broad refactors, speculative cleanup, or needless helpers. Use scratch state, fixed seeds and temporary directories; never use default datadirs, wallets, keys, or production databases. Do not hide failures with timeouts, narrower inputs, catches, assumptions, or broad suppressions.

For each candidate, state the hypothesis and trust boundary; trace callers, history, tests, docs, and invariants; reproduce on clean HEAD; classify local code, test, documentation, tool, dependency, or other-project behavior; and lock a verdict of confirmed, dismissed, or inconclusive before drafting a fix. Keep discovery and verification independent when practical. External reports and implementations are seeds, not oracles; document remote-only bugs with a report-ready reproducer.

Require hard proof: a failing-before/passing-after test, minimized fuzz seed or fixture, first-invalid-operation sanitizer/static trace, mutation or coverage delta, benchmark/profile table, build-matrix log, or rigorous proof when execution is impossible. For consensus, wallet/key, crypto, persistence, or remotely reachable findings, use two independent verifier forms when practical. Check patches apply; run narrow then broad validation and a per-commit stack loop. Commit messages must cover mechanism, reachability, impact, seed/source, exact commands and key output, correctness, limitations, and handoff. If the session ends without a fix, commit at most one clearly labeled journal-only handoff snapshot.

Campaign execution rules: inventory the relevant surface, state the expected contract or invariant before testing, and choose the smallest deterministic experiment that can falsify it. Preserve minimized inputs, raw traces, profiles, coverage, and rejected hypotheses. Re-evaluate priorities after every cycle and immediately continue with a distinct high-value hypothesis. Do not manufacture commits to show activity.

Campaign focus:
{focus}
"""


def load_rows():
    rows = []
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        number, slug, title, focus = line.split("\t", 3)
        rows.append((int(number), slug, title, focus))
    return rows


def main():
    rows = load_rows()
    assert rows
    assert [row[0] for row in rows] == list(range(len(rows)))
    assert len({row[1] for row in rows}) == len(rows)

    sections = [
        "# Reusable Continuous Agent Goals for Bitcoin Core and libsecp256k1",
        "",
        f"This local catalog contains {len(rows)} standalone `/goal` prompts. Each fenced block is self-contained and can be selected independently. The catalog is generated from `goals.tsv`; keep the manifest and this file together when moving it.",
        "",
        "## Goals",
    ]
    for number, slug, title, focus in rows:
        prompt = RUN_PROTOCOL.format(slug=slug, focus=focus)
        assert prompt.startswith("/goal\n")
        assert len(prompt.encode("utf-8")) < 4000, (number, len(prompt.encode("utf-8")))
        sections.extend(
            [
                "",
                f'<a id="goal-{number}"></a>',
                "",
                f"### {number}. {title}",
                "",
                f"<!-- slug: {slug}; prompt-bytes: {len(prompt.encode('utf-8'))} -->",
                "",
                "```text",
                prompt.rstrip(),
                "```",
            ]
        )
    CATALOG.write_text("\n".join(sections) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
