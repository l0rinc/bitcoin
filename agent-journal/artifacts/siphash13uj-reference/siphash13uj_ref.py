#!/usr/bin/env python3
"""Independent SipHash-1-3-UJ reference, written from the documented
construction in src/crypto/siphash.h (NOT from the C++ code):
- init: standard SipHash constants XOR key
- normal block d: v3^=d; SipRound; v0^=d
- jumbo (d0..d3 LE64): v3^=d0,v0^=d1,v1^=d2,v2^=d3; SipRound; v0^=d0,v1^=d1,v2^=d2,v3^=d3
- finalize: v2 ^= 0x6465646461706e75 ("unpadded" LE64); 3 SipRounds; v0^v1^v2^v3
Then cross-check all pinned vectors in src/test/data/siphash.json.
"""
import json

M = (1 << 64) - 1

def rotl(x, b):
    return ((x << b) | (x >> (64 - b))) & M

def sipround(v):
    v0, v1, v2, v3 = v
    v0 = (v0 + v1) & M; v1 = rotl(v1, 13); v1 ^= v0
    v0 = rotl(v0, 32)
    v2 = (v2 + v3) & M; v3 = rotl(v3, 16); v3 ^= v2
    v0 = (v0 + v3) & M; v3 = rotl(v3, 21); v3 ^= v0
    v2 = (v2 + v1) & M; v1 = rotl(v1, 17); v1 ^= v2
    v2 = rotl(v2, 32)
    return [v0, v1, v2, v3]

def siphash13uj(k0, k1, blocks):
    v = [0x736f6d6570736575 ^ k0, 0x646f72616e646f6d ^ k1,
         0x6c7967656e657261 ^ k0, 0x7465646279746573 ^ k1]
    for blk in blocks:
        if len(blk) == 8:
            (d,) = [int.from_bytes(blk, 'little')]
            v[3] ^= d
            v = sipround(v)
            v[0] ^= d
        else:
            assert len(blk) == 32
            d = [int.from_bytes(blk[i*8:(i+1)*8], 'little') for i in range(4)]
            v[3] ^= d[0]; v[0] ^= d[1]; v[1] ^= d[2]; v[2] ^= d[3]
            v = sipround(v)
            v[0] ^= d[0]; v[1] ^= d[1]; v[2] ^= d[2]; v[3] ^= d[3]
    v[2] ^= 0x6465646461706e75
    for _ in range(3):
        v = sipround(v)
    return v[0] ^ v[1] ^ v[2] ^ v[3]

def siphash24(k0, k1, data):
    # canonical SipHash-2-4 with padding, for anchoring the JSON's 2-4 values
    v = [0x736f6d6570736575 ^ k0, 0x646f72616e646f6d ^ k1,
         0x6c7967656e657261 ^ k0, 0x7465646279746573 ^ k1]
    n = len(data)
    for off in range(0, n - n % 8, 8):
        d = int.from_bytes(data[off:off+8], 'little')
        v[3] ^= d
        v = sipround(v); v = sipround(v)
        v[0] ^= d
    last = data[n - n % 8:] + bytes([n & 0xff])
    last = last.ljust(8, b'\0')
    # Bitcoin's JSON uses raw-byte inputs; the standard pads with length in the
    # HIGH byte of the final block: b = (len & 0xff) << 56 | remaining bytes
    rem = data[n - n % 8:]
    b = ((n & 0xff) << 56) | int.from_bytes(rem, 'little')
    v[3] ^= b
    v = sipround(v); v = sipround(v)
    v[0] ^= b
    v[2] ^= 0xff
    for _ in range(4):
        v = sipround(v)
    return v[0] ^ v[1] ^ v[2] ^ v[3]

tests = json.load(open('src/test/data/siphash.json'))
n13_checked = n24_checked = mismatches = 0
for i, t in enumerate(tests):
    k0 = int(t['key'][0], 16)
    k1 = int(t['key'][1], 16)
    blocks = [bytes.fromhex(x) for x in t['input']]
    exp = t['expected']
    if 'siphash24' in exp:
        # JSON inputs are block-structured (8/32 bytes); for 2-4 the C++ side
        # hashes each block's bytes sequentially (CalculateSipHash24 writes
        # block bytes via CSipHasher::Write span), so concatenate.
        got = siphash24(k0, k1, b''.join(blocks))
        want = int(exp['siphash24'], 16)
        n24_checked += 1
        if got != want:
            mismatches += 1
            print(f'MISMATCH 2-4 vector {i}: got {got:016x} want {want:016x}')
    if 'siphash13uj' in exp:
        got = siphash13uj(k0, k1, blocks)
        want = int(exp['siphash13uj'], 16)
        n13_checked += 1
        if got != want:
            mismatches += 1
            print(f'MISMATCH 13UJ vector {i}: got {got:016x} want {want:016x}')
print(f'checked 13uj={n13_checked} 24={n24_checked} mismatches={mismatches}')

# Structural invariant from the doc: jumbo(d0,0,0,0) == normal(d0)
import random
random.seed(42)
for _ in range(1000):
    k0, k1 = random.getrandbits(64), random.getrandbits(64)
    d0 = random.getrandbits(64)
    a = siphash13uj(k0, k1, [d0.to_bytes(8, 'little')])
    b = siphash13uj(k0, k1, [d0.to_bytes(8, 'little') + bytes(24)])
    assert a == b, 'jumbo-zero-extension invariant broken'
print('jumbo-zero-extension invariant: OK over 1000 random cases')

# Empty-input check: finalize(initial state) directly
print(f'empty-input (k=0): {siphash13uj(0, 0, []):016x}')
