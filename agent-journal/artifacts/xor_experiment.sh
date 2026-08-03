#!/bin/bash
# xor-key short-write fault-injection experiment (goal93-xor-key / d7d3559a30)
# Usage: xor_experiment.sh <phase>   where phase = prefix | postfix
# Requires: build-after/bin/bitcoind + bitcoin-cli built from the tree under test.
set -u
PHASE="${1:?phase: prefix|postfix}"
BIN=/mnt/my_storage/bitcoin/build-after/bin
DD=/tmp/xor-test-$PHASE
SO=/tmp/xor_interpose.so
rm -rf "$DD"; mkdir -p "$DD"

echo "== phase=$PHASE step1: first boot with short-write injection =="
LD_PRELOAD=$SO "$BIN/bitcoind" -regtest -datadir="$DD" -daemon=0 -listen=0 -dnsseed=0 -fixedseeds=1 >"$DD/boot1.log" 2>&1
rc1=$?
echo "boot1 exit=$rc1"
grep -E 'Error|error|failure' "$DD/boot1.log" | head -3
XOR="$DD/regtest/blocks/xor.dat"
if [ -f "$XOR" ]; then echo "xor.dat LEFT BEHIND: $(stat -c %s "$XOR") bytes"; else echo "xor.dat ABSENT (removed on failure)"; fi

echo "== step2: restart WITHOUT injection =="
"$BIN/bitcoind" -regtest -datadir="$DD" -daemon -listen=0 -dnsseed=0 -fixedseeds=1 >"$DD/boot2.log" 2>&1
rc2=$?
sleep 2
if [ "$rc2" = 0 ] && "$BIN/bitcoin-cli" -regtest -datadir="$DD" getblockcount 2>/dev/null; then
    echo "RESTART BOOTED, getblockcount OK; xor.dat=$(stat -c %s "$XOR") bytes"
    "$BIN/bitcoin-cli" -regtest -datadir="$DD" stop; sleep 2
else
    echo "RESTART FAILED rc2=$rc2"
    grep -E 'Error|error|failure|end of file' "$DD/boot2.log" "$DD/regtest/debug.log" 2>/dev/null | head -3
fi
echo "== phase=$PHASE done =="
