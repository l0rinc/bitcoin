#!/usr/bin/env python3
import argparse
import datetime as dt
import json
from pathlib import Path

GIB = 1024 ** 3
SECTOR_BYTES = 512


def parse_ts(value):
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def load_proc_io(path):
    rows = []
    with path.open() as f:
        next(f, None)
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if not parts or not parts[0]:
                continue
            row = {"ts": parse_ts(parts[0])}
            for part in parts[1:]:
                if "=" in part:
                    key, value = part.split("=", 1)
                    try:
                        row[key] = int(value)
                    except ValueError:
                        row[key] = value
            rows.append(row)
    return rows


def load_diskstats(path):
    rows = []
    with path.open() as f:
        next(f, None)
        for line in f:
            parts = line.rstrip("\n").split()
            if len(parts) < 12:
                continue
            # The monitor writes: ts major minor name read_ios ...
            row = {
                "ts": parse_ts(parts[0]),
                "device": parts[3],
                "read_sectors": int(parts[6]),
                "write_sectors": int(parts[10]),
            }
            rows.append(row)
    return rows


def load_chaininfo(path):
    rows = []
    with path.open() as f:
        for line in f:
            if not line.strip():
                continue
            try:
                item = json.loads(line)
            except json.JSONDecodeError:
                continue
            result = item.get("result") or {}
            rows.append({
                "ts": parse_ts(item["ts"]),
                "ok": item.get("ok", False),
                "blocks": result.get("blocks"),
                "headers": result.get("headers"),
                "initialblockdownload": result.get("initialblockdownload"),
                "verificationprogress": result.get("verificationprogress"),
            })
    return rows


def delta_summary(rows, fields):
    if len(rows) < 2:
        return {}
    first, last = rows[0], rows[-1]
    hours = (last["ts"] - first["ts"]).total_seconds() / 3600
    out = {
        "start": first["ts"].isoformat().replace("+00:00", "Z"),
        "end": last["ts"].isoformat().replace("+00:00", "Z"),
        "hours": hours,
    }
    for field in fields:
        if field not in first or field not in last:
            continue
        delta = last[field] - first[field]
        if field.endswith("_sectors"):
            delta *= SECTOR_BYTES
            name = field.removesuffix("_sectors") + "_bytes"
        else:
            name = field
        out[name] = delta
        out[name + "_gib"] = delta / GIB
        if hours > 0:
            out[name + "_gib_per_hour"] = delta / GIB / hours
            out[name + "_gib_per_day"] = delta / GIB / hours * 24
    return out


def filter_rows(rows, since=None, until=None):
    return [
        row for row in rows
        if (since is None or row["ts"] >= since) and (until is None or row["ts"] < until)
    ]


def first_steady_state_ts(chain_rows, require_headers_synced=False):
    for row in chain_rows:
        if not (row.get("ok") and row.get("initialblockdownload") is False):
            continue
        if require_headers_synced and row.get("blocks") != row.get("headers"):
            continue
        return row["ts"]
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--since", help="Only include samples at or after this UTC ISO timestamp")
    parser.add_argument("--until", help="Only include samples before this UTC ISO timestamp")
    parser.add_argument("--steady-state", action="store_true", help="Start at first RPC sample with initialblockdownload=false")
    parser.add_argument("--fully-synced", action="store_true", help="With --steady-state, require blocks==headers as well as initialblockdownload=false")
    args = parser.parse_args()

    proc_rows = load_proc_io(args.run_dir / "proc_io.tsv")
    disk_rows = load_diskstats(args.run_dir / "diskstats.tsv")
    chain_rows = load_chaininfo(args.run_dir / "blockchaininfo.jsonl")
    since = parse_ts(args.since) if args.since else None
    until = parse_ts(args.until) if args.until else None
    steady_since = None
    if args.steady_state:
        steady_since = first_steady_state_ts(chain_rows, args.fully_synced)
        if steady_since and (since is None or steady_since > since):
            since = steady_since
    proc_rows = filter_rows(proc_rows, since, until)
    disk_rows = filter_rows(disk_rows, since, until)
    chain_rows = filter_rows(chain_rows, since, until)

    summary = {
        "run_dir": str(args.run_dir),
        "since": since.isoformat().replace("+00:00", "Z") if since else None,
        "until": until.isoformat().replace("+00:00", "Z") if until else None,
        "steady_state_requested": args.steady_state,
        "steady_state_found": steady_since is not None if args.steady_state else None,
        "proc_io": delta_summary(proc_rows, ["read_bytes", "write_bytes", "cancelled_write_bytes", "rchar", "wchar"]),
        "diskstats": delta_summary(disk_rows, ["read_sectors", "write_sectors"]),
    }
    if chain_rows:
        summary["chain_first"] = chain_rows[0]
        summary["chain_last"] = chain_rows[-1]
    print(json.dumps(summary, indent=2, default=str))


if __name__ == "__main__":
    main()
