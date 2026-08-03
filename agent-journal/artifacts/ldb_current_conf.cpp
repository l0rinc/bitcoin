// LevelDB CURRENT-file loss (goal 125, cycle 308, iter 3)
// delete CURRENT -> open with create_if_missing=true (dbwrapper's
// setting): does LevelDB silently orphan live tables (fresh empty
// DB), fail loud, or recover?
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <cstdio>
#include <string>
#include <filesystem>
int main(){
    const std::string path="/tmp/ldb_current_db";
    leveldb::Options opts; opts.create_if_missing=true;
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    if(!leveldb::DB::Open(opts,path,&db).ok()){fprintf(stderr,"open fail\n");return 2;}
    for(int i=0;i<1000;i++){char k[32],v[64];snprintf(k,32,"key%08d",i);snprintf(v,64,"val%08d",i);db->Put(leveldb::WriteOptions(),k,v);}
    db->CompactRange(nullptr,nullptr);
    delete db;
    std::filesystem::remove(path+"/CURRENT");
    printf("CURRENT deleted\n");
    leveldb::Options opts2; opts2.create_if_missing=true;
    db=nullptr;
    leveldb::Status s=leveldb::DB::Open(opts2,path,&db);
    printf("open-no-CURRENT(create_if_missing): %s\n", s.ToString().c_str());
    if(s.ok()){
        leveldb::Iterator* it=db->NewIterator(leveldb::ReadOptions());
        it->SeekToFirst();
        printf("visible keys after reopen: %s\n", it->Valid()?"NONEMPTY (recovered)":"EMPTY (tables orphaned!)");
        delete it; delete db;
    }
    // list files to see whether old tables got garbage-collected
    for (auto& e : std::filesystem::directory_iterator(path))
        printf("  leftover: %s\n", e.path().filename().string().c_str());
    leveldb::DestroyDB(path,opts);
    return 0;
}
