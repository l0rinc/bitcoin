/* #41 c3 tool: obfuscation-key archaeology on a scratch chainstate.
 * modes: dump | corruptkey | delkey | flipcoin */
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static const std::string OBF_KEY{"\x0e\000obfuscate_key", 15}; // compactsize(14) + name

int main(int argc, char** argv) {
    if (argc != 3) { fprintf(stderr, "usage: %s DBPATH {dump|corruptkey|delkey|flipcoin}\n", argv[0]); return 2; }
    leveldb::Options options;
    options.create_if_missing = false;
    leveldb::DB* raw{nullptr};
    const auto st{leveldb::DB::Open(options, argv[1], &raw)};
    if (!st.ok()) { fprintf(stderr, "open fail: %s\n", st.ToString().c_str()); return 1; }
    std::unique_ptr<leveldb::DB> db{raw};
    const std::string mode{argv[2]};

    if (mode == "dump") {
        std::string val;
        if (db->Get(leveldb::ReadOptions(), OBF_KEY, &val).ok()) {
            printf("obfuscation key record: %zu B hex=", val.size());
            for (unsigned char c : val) printf("%02x", c);
            printf("\n");
        } else {
            printf("NO obfuscation key record\n");
        }
        /* dump first 8 records' raw keys + value prefixes */
        auto it{std::unique_ptr<leveldb::Iterator>{db->NewIterator(leveldb::ReadOptions())}};
        int n = 0;
        for (it->SeekToFirst(); it->Valid() && n < 8; it->Next(), ++n) {
            const auto k{it->key()};
            printf("rec %d: keylen=%zu keyhex=", n, k.size());
            for (size_t i = 0; i < k.size() && i < 20; i++) printf("%02x", (unsigned char)k[i]);
            const auto v{it->value()};
            printf(" vlen=%zu vhex=", v.size());
            for (size_t i = 0; i < v.size() && i < 8; i++) printf("%02x", (unsigned char)v[i]);
            printf("\n");
        }
        return 0;
    }
    if (mode == "corruptkey") {
        std::string val;
        if (!db->Get(leveldb::ReadOptions(), OBF_KEY, &val).ok()) { printf("no key to corrupt\n"); return 1; }
        for (auto& c : val) c = char(c ^ 0x5a);
        leveldb::WriteOptions wo;
        wo.sync = true;
        if (!db->Put(wo, OBF_KEY, val).ok()) { fprintf(stderr, "put fail\n"); return 1; }
        printf("key record corrupted (%zu B)\n", val.size());
        return 0;
    }
    if (mode == "wrongkey") {
        std::string val;
        if (!db->Get(leveldb::ReadOptions(), OBF_KEY, &val).ok()) { printf("no key to corrupt\n"); return 1; }
        for (size_t i = 1; i < val.size(); i++) val[i] = char(val[i] ^ 0x5a); // keep the 08 length byte
        leveldb::WriteOptions wo;
        wo.sync = true;
        if (!db->Put(wo, OBF_KEY, val).ok()) { fprintf(stderr, "put fail\n"); return 1; }
        printf("key bytes corrupted, shape intact (%zu B)\n", val.size());
        return 0;
    }
    if (mode == "manglelen") {
        std::string val;
        if (!db->Get(leveldb::ReadOptions(), OBF_KEY, &val).ok()) { printf("no key to mangle\n"); return 1; }
        val[0] = char(val[0] ^ 0x7f); // break the serialized length byte
        leveldb::WriteOptions wo;
        wo.sync = true;
        if (!db->Put(wo, OBF_KEY, val).ok()) { fprintf(stderr, "put fail\n"); return 1; }
        printf("key value length byte mangled (%zu B)\n", val.size());
        return 0;
    }
    if (mode == "delkey") {
        leveldb::WriteOptions wo;
        wo.sync = true;
        if (!db->Delete(wo, OBF_KEY).ok()) { fprintf(stderr, "delete fail\n"); return 1; }
        printf("key record deleted\n");
        return 0;
    }
    if (mode == "flipcoin") {
        auto it{std::unique_ptr<leveldb::Iterator>{db->NewIterator(leveldb::ReadOptions())}};
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            const auto k{it->key()};
            if (k.size() == OBF_KEY.size() && memcmp(k.data(), OBF_KEY.data(), OBF_KEY.size()) == 0) continue;
            std::string v{it->value().ToString()};
            for (size_t i = 0; i < v.size() && i < 24; i++) v[i] ^= 0xff;
            leveldb::WriteOptions wo;
            wo.sync = true;
            if (!db->Put(wo, k, v).ok()) { fprintf(stderr, "put fail\n"); return 1; }
            printf("flipped first non-key record: keylen=%zu key0=%02x\n", k.size(), (unsigned char)k[0]);
            return 0;
        }
        printf("no data record found\n");
        return 1;
    }
    fprintf(stderr, "unknown mode\n");
    return 2;
}
