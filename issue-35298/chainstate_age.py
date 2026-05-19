#!/usr/bin/env python3
import argparse
import time
from pathlib import Path

GIB = 1024 ** 3
BUCKETS = [
    ("<=15m", 15 * 60),
    ("<=30m", 30 * 60),
    ("<=1h", 60 * 60),
    ("<=2h", 2 * 60 * 60),
    ("<=6h", 6 * 60 * 60),
    ("<=24h", 24 * 60 * 60),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("chainstate", type=Path)
    args = parser.parse_args()

    now = time.time()
    rows = []
    for path in args.chainstate.iterdir():
        if path.suffix not in {".ldb", ".sst"} or not path.is_file():
            continue
        stat = path.stat()
        rows.append((now - stat.st_mtime, stat.st_size))

    total_bytes = sum(size for _, size in rows)
    print("bucket\tfiles\tbytes\tgib\tpct_bytes")
    print(f"all\t{len(rows)}\t{total_bytes}\t{total_bytes / GIB:.3f}\t100.00")
    for label, limit in BUCKETS:
        bucket = [(age, size) for age, size in rows if age <= limit]
        bucket_bytes = sum(size for _, size in bucket)
        pct = bucket_bytes / total_bytes * 100 if total_bytes else 0
        print(f"{label}\t{len(bucket)}\t{bucket_bytes}\t{bucket_bytes / GIB:.3f}\t{pct:.2f}")
    old = [(age, size) for age, size in rows if age > 2 * 60 * 60]
    old_bytes = sum(size for _, size in old)
    pct = old_bytes / total_bytes * 100 if total_bytes else 0
    print(f">2h\t{len(old)}\t{old_bytes}\t{old_bytes / GIB:.3f}\t{pct:.2f}")


if __name__ == "__main__":
    main()
