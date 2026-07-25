# Follow-up: PR 35744 new push — shared_mutex redesign and its review thread (2026-07-25)

Context: andrewtoth proposed shared_mutex over the atomic cursor counter (d3484251b3). The new 3-commit series (791ab17e8c, f9b96fe758, 2f125947d4 — reviewed separately, verdict CORRECT) adopts it. l0rinc raised two concerns in the thread; both resolve in favor of the new design.

## Concern 1: "cursors must be destroyed on the creating thread"

C++ requires `unlock_shared()` from the thread holding the shared lock (thread.sharedmutex.requirements#12). With the lock stored in the cursor, the dtor must run on the creating thread.

- Verified satisfied: every cursor user runs scan + destruction on one thread (b-http worker for the RPC scans; validation/RPC thread for the snapshot paths).
- Not a new constraint: LevelDB iterators already require single-thread access (leveldb doc/index.md#concurrency, cited by andrewtoth), so cursors were never legitimately cross-thread anyway. The shared_lock merely makes the pre-existing requirement explicit.

## Concern 2: "std::shared_mutex loses Core's lock checking"

Core's `Mutex` (AnnotatedMixin) feeds the DEBUG_LOCKORDER runtime deadlock detector; plain std::shared_mutex does not, so m_db_mutex drops out of the runtime lock-order graph.

- Precedent is real: `SignatureCache::cs_sigcache` is std::shared_mutex (src/script/sigcache.h:49) with shared_lock/unique_lock and no AnnotatedMixin — andrewtoth's point stands.
- Residual risk is small: exclusive acquisitions happen only under cs_main (annotation-documented), and cursor (shared-lock) scopes never take another lock. The main thing DEBUG_LOCKORDER would have caught is a *future* change introducing a lock inside cursor lifetime — mitigate with the debug assertion suggested in the main review (`AssertLockNotHeld(cs_main)` in ~CCoinsViewDBCursor).
- The alternative that keeps runtime checking is the old atomic-counter push — correct but hand-rolled; the shared_mutex version is strictly cleaner and the project has already accepted this trade elsewhere.

Verdict on the push: the shared_mutex redesign is the right call; both thread concerns are addressed by caller verification + precedent, not by reverting to the counter.
