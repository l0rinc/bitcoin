// LevelDB MANIFEST corruption (goal 125, cycle 308, iter 1)
// corrupt MANIFEST bytes -> DB::Open must fail loudly (Corruption),
// never silently create a fresh empty DB over live tables.
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <cstdio>
#include <string>
#include <filesystem>
int main(){
    const std::string path="/tmp/ldb_manifest_db";
    leveldb::Options opts; opts.create_if_missing=true;
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    if(!leveldb::DB::Open(opts,path,&db).ok()){fprintf(stderr,"open fail\n");return 2;}
    for(int i=0;i<1000;i++){char k[32],v[64];snprintf(k,32,"key%08d",i);snprintf(v,64,"val%08d",i);db->Put(leveldb::WriteOptions(),k,v);}
    db->CompactRange(nullptr,nullptr);
    delete db;
    std::string manifest;
    for (auto& e : std::filesystem::directory_iterator(path))
        if (e.path().filename().string().rfind("MANIFEST",0)==0) manifest=e.path().string();
    printf("manifest: %s\n", manifest.c_str());
    FILE* f=fopen(manifest.c_str(),"r+b");
    fseek(f,0,SEEK_END); long sz=ftell(f);
    fseek(f,sz/2,SEEK_SET); int orig=fgetc(f); fseek(f,sz/2,SEEK_SET); fputc(orig^0xFF,f); fclose(f);
    printf("corrupted byte at %ld/%ld (%d->%d)\n",sz/2,sz,orig,orig^0xFF);
    // attempt open WITH create_if_missing=true (dbwrapper's setting)
    leveldb::Options opts2; opts2.create_if_missing=true;
    db=nullptr;
    leveldb::Status s=leveldb::DB::Open(opts2,path,&db);
    printf("open-after-corruption: %s\n", s.ToString().c_str());
    int silent_empty=0;
    if(s.ok()){
        // did it silently recreate an empty DB over live tables?
        leveldb::Iterator* it=db->NewIterator(leveldb::ReadOptions());
        it->SeekToFirst();
        if(!it->Valid()){printf("FIRST-INVALID: silently recreated EMPTY db over live tables!\n");silent_empty++;}
        delete it; delete db;
    }
    leveldb::DestroyDB(path,opts);
    if(!s.ok() && !silent_empty){printf("CONFORM: corrupt MANIFEST fails loud (%s)\n", s.ToString().c_str());return 0;}
    return silent_empty?1:2;
}
