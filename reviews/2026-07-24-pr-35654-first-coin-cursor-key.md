# Investigation: PR 35654 — txdb: reject undecodable first coin cursor key (2026-07-24)

Verdict: 🟢 APPROPRIATE as a correctness/hygiene fix (not a security fix). Merge.

## History
- **Issue #35172** (shuv-amp): `CCoinsViewDB::Cursor()` warmup ignored `GetKey()` failure. `CoinEntry` defaults its key marker to `DB_COIN`, so a malformed first key left `Valid()==true` and `GetKey()==true` with a phantom default outpoint (null hash, n=0xFFFFFFFF).
- **PR 35191, PR 35248**: two prior attempts, both closed unmerged — drive-by contributors, died on process/attention, not on wrong code.
- **PR 35654** (l0rinc): third attempt; reframed from "fix corruption handling" to "correctness improvement removing a weird harmless edge case" after sedited's pushback; sedited ACK'd 3837d9192a with that framing.
- **sedited's pushback**: (1) corrupt-chainstate-only, doesn't rescue a running node; (2) commit 822755a424 (2017, sipa) already handles "no first entry at all" (iterator invalid → keyTmp.first=0), so the remaining gap is only the narrower "iterator valid but key undecodable" case; (3) repeatedly AI-flagged busywork — but ACKs to end the re-flagging treadmill.

## Correctness verification
The diff mirrors `Next()` exactly (a3310b4d48's contract from #7890):
```cpp
i->keyTmp.first = 0;
if (i->pcursor->Valid()) {
    if (CoinEntry entry{&i->keyTmp.second}; i->pcursor->GetKey(entry)) {
        i->keyTmp.first = entry.key;
    }
}
```
Undecodable first key or missing first entry → cursor invalid; successful decode → cached key. Phantom-coin scenario eliminated. A failed GetKey may still have written keyTmp.second, but the invalid marker prevents it being handed out. Consumers (ComputeUTXOStats/gettxoutsetinfo, scantxoutset, dumptxoutset, snapshot rollback) iterate `while(Valid())` — no infinite loop; all only call Next() inside the loop.

## Appropriateness analysis
- Not consensus-relevant: ConnectBlock doesn't use this cursor. Assumeutxo snapshot validation uses ComputeUTXOStats(HASH_SERIALIZED) — corrupt DB → phantom coin or truncation, either way hash mismatch → snapshot rejected, same outcome.
- Both pre- and post-fix behaviors are "silent wrong" on corrupt DB (fabricated bogus coin vs silent truncation at position 0), but the fix (a) removes a genuine Cursor()/Next() inconsistency, (b) stops fabrication of a bogus UTXO into stats/snapshot outputs, (c) costs 5 lines + test, zero risk.
- sedited's "not worth it" has a point on impact, but cost-benefit trivially favors merging — and merging ends the repeated re-flagging (his own stated reason to ACK).

## Residual (follow-up, not blocking)
Mid-iteration corruption still silently truncates: `Next()` invalidation and end-of-DB are indistinguishable to consumers (`while(Valid())` ends both). A gettxoutsetinfo/dumptxoutset result can be silently partial with a self-consistent-but-wrong hash. The right long-term direction is the 34132 fatal-read model or an explicit "cursor died early" signal the RPCs can surface as an error.
