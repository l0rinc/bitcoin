#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# Create ARCHIVE from REPO's HEAD, or verify an existing archive against the
# exact bytes Git would produce for that commit.
create_or_verify_git_archive() {
    local repo="$1"
    local prefix="$2"
    local archive="$3"
    local expected_hash actual_hash

    if [ -e "$archive" ]; then
        expected_hash="$(git -C "$repo" archive --format=tar.gz --prefix="$prefix" HEAD | sha256sum | cut -d' ' -f1)"
        actual_hash="$(sha256sum "$archive" | cut -d' ' -f1)"
        if [ "$expected_hash" != "$actual_hash" ]; then
            echo "ERR: Git archive '$archive' does not match HEAD in '$repo'." >&2
            return 1
        fi
        return 0
    fi

    mkdir -p "$(dirname "$archive")"
    git -C "$repo" archive --format=tar.gz --prefix="$prefix" --output="$archive" HEAD
}
