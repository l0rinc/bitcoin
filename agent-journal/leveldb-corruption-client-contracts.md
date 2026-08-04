
## Cycle 335 (2026-08-04) — REOPENED by F33's client-assumption class; class now CLOSED

Draw r153 (raw=5637745048392333951 -> #127). No upstream LevelDB/
dbwrapper changes in either delta, BUT F33 (unlocked UTXO-scan
cursor vs assumeutxo ResizeCache) proved a client-side violation
class inside this goal's scope. Iteration: is the class exhausted
beyond F33's fix, on the new base (17c5e33e9c)?

Evidence walk (current tree):
- txdb.h:41-66: m_db_mutex documented as preventing CompactFull and
  cursors from racing ResizeCache's m_db replacement; annotations
  EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_db_mutex) on Cursor(),
  ResizeCache(), CompactFullAsync() — thread-safety analysis enforces.
- txdb.cpp:252: Cursor() moves a UniqueLock(m_db_mutex) INTO the
  cursor object (lifetime-bound) — F33's fix intact post-rebase.
- txdb.cpp: ResizeCache takes LOCK(m_db_mutex) before m_db.reset() —
  the reset side is serialized against live cursors. Contract closed
  on BOTH sides.
- All other LevelDB iterator users are function-local with no
  concurrent reset path: rpc/blockchain.cpp:3333 temp_db (snapshot
  write, function-local DB), node/blockstorage.cpp:135 (blocktree,
  no runtime reset exists), index DBs coinstatsindex:219,
  blockfilterindex:288/325, txospenderindex:166 (created at init,
  stable for process lifetime; no public reset/resize mid-run).
- Wallet sqlite cursors: different engine, out of scope.

VERDICT: F33 class DISMISSED as a further-defect source — the only
runtime-resettable LevelDB client serializes reset against cursor
lifetimes, and every other iterator is scope-local. The fix survived
both rebases. No new defect; no new cell.
