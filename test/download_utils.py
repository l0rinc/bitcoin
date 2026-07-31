#!/usr/bin/env python3
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import hashlib
import sys
import time
import urllib.request

# Update the URL and hash together when the qa-assets test vectors change.
SCRIPT_ASSETS_URL = "https://raw.githubusercontent.com/bitcoin-core/qa-assets/b33d85102d169b54d966ea315ad81a636680aefa/unit_test_data/script_assets_test.json"
SCRIPT_ASSETS_SHA256 = "cd789a58ec45916e1721cdd14e82ca4c93100959f1cef4e229b22e3bf539f095"


def download_from_url(url, archive):
    print(f"Fetching: {url}")
    last_print_time = time.time()

    def progress_hook(progress_bytes, total_size):
        nonlocal last_print_time
        now = time.time()
        percent = min(100, (progress_bytes * 100) / total_size)
        bar_length = 40
        filled_length = int(bar_length * percent / 100)
        bar = '#' * filled_length + '-' * (bar_length - filled_length)
        if now - last_print_time >= 1 or percent >= 100:
            print(f'\rDownloading: [{bar}] {percent:.1f}%', flush=True, end="")
            last_print_time = now

    with urllib.request.urlopen(url) as response:
        if response.status != 200:
            raise RuntimeError(f"HTTP request failed with status code: {response.status}")

        sock_info = response.fp.raw._sock.getpeername()
        print(f"Connected to {sock_info[0]}")

        total_size = int(response.getheader("Content-Length"))
        progress_bytes = 0

        with open(archive, 'wb') as file:
            while True:
                chunk = response.read(8192)
                if not chunk:
                    break
                file.write(chunk)
                progress_bytes += len(chunk)
                progress_hook(progress_bytes, total_size)

        if progress_bytes < total_size:
            raise RuntimeError(f"Download incomplete: expected {total_size} bytes, got {progress_bytes} bytes")

    print('\n', flush=True, end="") # Flush to avoid error output on the same line.


def download_script_assets(script_assets_dir):
    script_assets_dir.mkdir(parents=True, exist_ok=True)
    script_assets = script_assets_dir / "script_assets_test.json"

    def download_and_verify():
        download_from_url(SCRIPT_ASSETS_URL, script_assets)
        hasher = hashlib.sha256()
        with open(script_assets, "rb") as file:
            for chunk in iter(lambda: file.read(1024 * 1024), b""):
                hasher.update(chunk)
        digest = hasher.hexdigest()
        if digest != SCRIPT_ASSETS_SHA256:
            raise RuntimeError(f"Checksum {digest} did not match expected {SCRIPT_ASSETS_SHA256}")

    try:
        download_and_verify()
    except Exception as e:
        print(f"\nDownload failed: {e}", file=sys.stderr)
        print("Retrying download after failure ...", file=sys.stderr)
        time.sleep(12)
        try:
            download_and_verify()
        except Exception as e2:
            sys.exit(e2)
