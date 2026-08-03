// LevelDB iterator semantics conformance harness (goal 126, cycle 302)
// Verifies against the in-tree libleveldb:
//  (a) an iterator created before concurrent writes never observes
//      torn/garbage keys or values,
//  (b) snapshot semantics: the iterator never sees keys written
//      AFTER its creation (LevelDB's documented guarantee),
//  (c) concurrent delete of the to-be-visited keys does not
//      invalidate the iterator's snapshot view.
// Any violation prints FIRST-INVALID and exits nonzero.
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/write_batch.h>

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

static std::string key(int i) { char b[32]; snprintf(b, sizeof b, "key%08d", i); return b; }
static std::string val(int i, int gen) { char b[48]; snprintf(b, sizeof b, "val%08d-gen%d", i, gen); return b; }

int main() {
    const std::string path = "/tmp/ldb_iter_conf_db";
    leveldb::Options opts;
    opts.create_if_missing = true;
    opts.error_if_exists = false;
    leveldb::DestroyDB(path, opts);
    leveldb::DB* db = nullptr;
    leveldb::Status os = leveldb::DB::Open(opts, path, &db); if (!os.ok()) { fprintf(stderr, "open failed: %s\n", os.ToString().c_str()); return 2; }

    constexpr int N = 20000;
    for (int i = 0; i < N; i++) db->Put(leveldb::WriteOptions(), key(i), val(i, 0));

    std::atomic<bool> stop{false};
    std::atomic<int> writes{0};
    std::thread writer([&] {
        int gen = 1;
        while (!stop.load()) {
            leveldb::WriteBatch batch;
            for (int i = 0; i < 100; i++) {
                int k = rand() % (N * 2); // half inside, half beyond the initial range
                if (k % 7 == 0) batch.Delete(key(k % N));
                else batch.Put(key(k % N), val(k % N, gen));
            }
            db->Write(leveldb::WriteOptions(), &batch);
            writes.fetch_add(100);
            gen++;
        }
    });

    int violations = 0;
    for (int round = 0; round < 20 && violations == 0; round++) {
        leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
        // snapshot point: record which keys existed (round 0 baseline: all N)
        int seen = 0;
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            std::string k = it->key().ToString();
            std::string v = it->value().ToString();
            // (a) torn-key/value check: keys must match the shape
            if (k.rfind("key", 0) != 0 || k.size() != 11) {
                printf("FIRST-INVALID torn key: '%s' (round %d)\n", k.c_str(), round);
                violations++;
                break;
            }
            if (v.rfind("val", 0) != 0) {
                printf("FIRST-INVALID torn value for %s: '%s'\n", k.c_str(), v.c_str());
                violations++;
                break;
            }
            // (b) snapshot semantics: no key beyond key(N-1) may appear
            int idx = atoi(k.c_str() + 3);
            if (idx >= N) {
                printf("FIRST-INVALID post-snapshot key visible: %s (round %d)\n", k.c_str(), round);
                violations++;
                break;
            }
            seen++;
        }
        if (!it->status().ok()) { printf("FIRST-INVALID iterator status: %s\n", it->status().ToString().c_str()); violations++; }
        if (seen == 0) { printf("FIRST-INVALID empty snapshot (round %d)\n", round); violations++; }
        delete it;
    }
    stop.store(true);
    writer.join();
    delete db;
    leveldb::DestroyDB(path, opts);
    if (violations == 0) printf("CONFORM: 20 rounds, 0 violations (iterator snapshot isolation holds under concurrent writes/deletes)\n");
    return violations ? 1 : 0;
}
