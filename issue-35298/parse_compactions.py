#!/usr/bin/env python3
import argparse
import datetime as dt
import json
import re
from collections import defaultdict
from pathlib import Path

TS_RE = re.compile(r"^(?P<ts>\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})Z ")
L0_RE = re.compile(r"\[leveldb\] Level-0 table #(?P<table>\d+): started")
COMPACT_RE = re.compile(r"\[leveldb\] Compacting (?P<a>\d+)@(?P<level>\d+) \+ (?P<b>\d+)@(?P<next>\d+) files")
COMPACTED_BYTES_RE = re.compile(r"\[leveldb\] Compacted (?P<a>\d+)@(?P<level>\d+) \+ (?P<b>\d+)@(?P<next>\d+) files => (?P<bytes>\d+) bytes")
GENERATED_RE = re.compile(r"\[leveldb\] Generated table #(?P<table>\d+)@(?P<level>\d+): (?P<keys>\d+) keys, (?P<bytes>\d+) bytes")
DELETE_RE = re.compile(r"\[leveldb\] Delete type=2 #(?P<table>\d+)")
TO_RE = re.compile(r"\[leveldb\] compacted to: (?P<summary>files\[.*\])")
FLUSH_RE = re.compile(r"Writing chainstate to disk: (?P<detail>.*)")
BENCH_RE = re.compile(r"\[bench\] (?P<detail>.*)")
TIP_RE = re.compile(r"UpdateTip: .* height=(?P<height>\d+) .* progress=(?P<progress>[0-9.]+) cache=(?P<cache>[0-9.]+)MiB\((?P<txo>\d+)txo\)")


def parse_ts(line):
    match = TS_RE.match(line)
    if not match:
        return None
    return dt.datetime.fromisoformat(match.group("ts")).replace(tzinfo=dt.timezone.utc)


def parse_cli_ts(value):
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def event_kind(line):
    for kind, regex in (
        ("l0", L0_RE),
        ("compact", COMPACT_RE),
        ("compacted_bytes", COMPACTED_BYTES_RE),
        ("generated", GENERATED_RE),
        ("delete", DELETE_RE),
        ("compacted_to", TO_RE),
        ("flush", FLUSH_RE),
        ("bench", BENCH_RE),
        ("tip", TIP_RE),
    ):
        match = regex.search(line)
        if match:
            return kind, match.groupdict()
    return None, None


def build_waves(core_events, context_events, gap, context_margin):
    waves = []
    current = None
    for event in core_events:
        if current is None or event["ts"] - current["end"] > gap:
            current = {"start": event["ts"], "end": event["ts"], "events": []}
            waves.append(current)
        current["end"] = event["ts"]
        current["events"].append(event)
    for wave in waves:
        context_start = wave["start"] - context_margin
        context_end = wave["end"] + context_margin
        wave["context"] = [
            event for event in context_events
            if context_start <= event["ts"] <= context_end
        ]
    return waves


def format_gib_map(values):
    return ",".join(
        f"{level}:{values[level] / (1024 ** 3):.3f}"
        for level in sorted(values, key=int)
        if values[level]
    )


def format_count_map(values):
    return ",".join(
        f"{level}:{values[level]}"
        for level in sorted(values, key=int)
        if values[level]
    )


def summarize_wave(idx, wave, prev_end):
    generated_bytes = 0
    generated_tables = 0
    compacted_output_bytes = 0
    generated_by_level = defaultdict(int)
    generated_by_output_level = defaultdict(int)
    compacted_output_by_level = defaultdict(int)
    compacted_output_by_output_level = defaultdict(int)
    compactions_by_level = defaultdict(int)
    deletes = 0
    l0_starts = 0
    compactions = 0
    levels = set()
    last_summary = ""
    flushes = 0
    flush_details = []
    benches = 0
    tip_heights = []
    cache_mib = []
    for event in wave["events"]:
        kind = event["kind"]
        data = event["data"]
        if kind == "generated":
            generated_tables += 1
            bytes_written = int(data["bytes"])
            generated_bytes += bytes_written
            generated_by_level[data["level"]] += bytes_written
            generated_by_output_level[str(int(data["level"]) + 1)] += bytes_written
        elif kind == "delete":
            deletes += 1
        elif kind == "l0":
            l0_starts += 1
        elif kind == "compact":
            compactions += 1
            compactions_by_level[data["level"]] += 1
            levels.add(f'{data["level"]}->{data["next"]}')
        elif kind == "compacted_bytes":
            bytes_written = int(data["bytes"])
            compacted_output_bytes += bytes_written
            compacted_output_by_level[data["level"]] += bytes_written
            compacted_output_by_output_level[str(int(data["level"]) + 1)] += bytes_written
            levels.add(f'{data["level"]}->{data["next"]}')
        elif kind == "compacted_to":
            last_summary = data["summary"]
    for event in wave.get("context", []):
        kind = event["kind"]
        data = event["data"]
        if kind == "flush":
            flushes += 1
            flush_details.append(data["detail"])
        elif kind == "bench":
            benches += 1
        elif kind == "tip":
            tip_heights.append(int(data["height"]))
            cache_mib.append(float(data["cache"]))
    duration = (wave["end"] - wave["start"]).total_seconds()
    interval = "" if prev_end is None else (wave["start"] - prev_end).total_seconds()
    return {
        "wave": idx,
        "start": wave["start"].isoformat().replace("+00:00", "Z"),
        "end": wave["end"].isoformat().replace("+00:00", "Z"),
        "duration_s": duration,
        "interval_since_prev_s": interval,
        "l0_starts": l0_starts,
        "compactions": compactions,
        "generated_tables": generated_tables,
        "generated_bytes": generated_bytes,
        "generated_gib": generated_bytes / (1024 ** 3),
        "generated_gib_by_level": format_gib_map(generated_by_level),
        "generated_gib_by_output_level": format_gib_map(generated_by_output_level),
        "compacted_output_bytes": compacted_output_bytes,
        "compacted_output_gib": compacted_output_bytes / (1024 ** 3),
        "compacted_output_gib_by_level": format_gib_map(compacted_output_by_level),
        "compacted_output_gib_by_output_level": format_gib_map(compacted_output_by_output_level),
        "compactions_by_level": format_count_map(compactions_by_level),
        "deletes": deletes,
        "levels": ",".join(sorted(levels)),
        "flush_lines": flushes,
        "bench_lines": benches,
        "first_tip_height": min(tip_heights) if tip_heights else "",
        "last_tip_height": max(tip_heights) if tip_heights else "",
        "min_cache_mib": min(cache_mib) if cache_mib else "",
        "max_cache_mib": max(cache_mib) if cache_mib else "",
        "last_level_summary": last_summary,
        "flush_details": ";".join(flush_details),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("debug_log", type=Path)
    parser.add_argument("--wave-gap-min", type=float, default=5)
    parser.add_argument("--context-margin-s", type=float, default=60)
    parser.add_argument("--since", help="Only include events at or after this UTC ISO timestamp")
    parser.add_argument("--until", help="Only include events before this UTC ISO timestamp")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    since = parse_cli_ts(args.since) if args.since else None
    until = parse_cli_ts(args.until) if args.until else None

    events = []
    with args.debug_log.open("r", errors="replace") as f:
        for line in f:
            ts = parse_ts(line)
            if ts is None:
                continue
            if since and ts < since:
                continue
            if until and ts >= until:
                continue
            kind, data = event_kind(line)
            if kind:
                events.append({"ts": ts, "kind": kind, "data": data, "line": line.rstrip("\n")})

    core_events = [
        e for e in events
        if e["kind"] in {"l0", "compact", "compacted_bytes", "generated", "delete", "compacted_to"}
    ]
    context_events = [
        e for e in events
        if e["kind"] in {"flush", "bench", "tip"}
    ]
    waves = build_waves(
        core_events,
        context_events,
        dt.timedelta(minutes=args.wave_gap_min),
        dt.timedelta(seconds=args.context_margin_s),
    )
    summaries = []
    prev_end = None
    for idx, wave in enumerate(waves, 1):
        summary = summarize_wave(idx, wave, prev_end)
        if summary["l0_starts"] or summary["compactions"] or summary["generated_tables"] or summary["flush_lines"]:
            summaries.append(summary)
            prev_end = wave["end"]

    if args.json:
        print(json.dumps(summaries, indent=2))
        return

    fields = [
        "wave", "start", "end", "duration_s", "interval_since_prev_s",
        "l0_starts", "compactions", "generated_tables", "generated_gib",
        "generated_gib_by_level", "generated_gib_by_output_level",
        "compacted_output_gib", "compacted_output_gib_by_level", "compacted_output_gib_by_output_level",
        "compactions_by_level", "deletes", "levels", "flush_lines", "bench_lines",
        "first_tip_height", "last_tip_height", "min_cache_mib", "max_cache_mib",
        "last_level_summary",
        "flush_details",
    ]
    print("\t".join(fields))
    for summary in summaries:
        print("\t".join(str(summary[field]) for field in fields))


if __name__ == "__main__":
    main()
