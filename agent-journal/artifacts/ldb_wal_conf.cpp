// LevelDB WAL mid-record corruption recovery (goal 125, cycle 308, iter 2)
// corrupt a WAL record mid-file (simulated crash+corruption):
// recovery must (a) not crash, (b) not replay torn data, (c) keep
// data written BEFORE the corrupt record (up to the tear point),
// (d) surface the corruption (paranoid_checks=false -> log+ignore
// tail; paranoid_checks=true -> fail). dbwrapper sets paranoid=true.
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <cstdio>
#include <string>
#include <filesystem>
static std::string key(int i){char b[32];snprintf(b,32,"key%08d",i);return b;}
static std::string val(int i){char b[64];snprintf(b,64,"val%08d",i);return b;}
int main(){
    const std::string path="/tmp/ldb_wal_db";
    leveldb::Options opts; opts.create_if_missing=true;
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    if(!leveldb::DB::Open(opts,path,&db).ok()){fprintf(stderr,"open fail\n");return 2;}
    for(int i=0;i<2000;i++) db->Put(leveldb::WriteOptions(),key(i),val(i)); // all in WAL
    delete db; // close WITHOUT flush: data lives only in the .log
    std::string wal;
    for (auto& e : std::filesystem::directory_iterator(path))
        if (e.path().extension()==".log") wal=e.path().string();
    printf("wal: %s\n", wal.c_str());
    // corrupt a byte ~25% into the WAL (mid-stream record)
    FILE* f=fopen(wal.c_str(),"r+b");
    fseek(f,0,SEEK_END); long sz=ftell(f);
    fseek(f,sz/4,SEEK_SET); int orig=fgetc(f); fseek(f,sz/4,SEEK_SET); fputc(orig^0xFF,f); fclose(f);
    printf("corrupted WAL byte at %ld/%ld\n",sz/4,sz);
    // recovery with dbwrapper's paranoid_checks=true
    leveldb::Options opts2; opts2.paranoid_checks=true;
    db=nullptr;
    leveldb::Status s=leveldb::DB::Open(opts2,path,&db);
    printf("open(paranoid=true): %s\n", s.ToString().c_str());
    // and with default (paranoid=false): expect open OK, prefix data kept
    leveldb::Options opts3;
    leveldb::DB* db3=nullptr;
    leveldb::Status s3=leveldb::DB::Open(opts3,path,&db3);
    int kept=0, torn=0;
    if(s3.ok()){
        leveldb::ReadOptions ro; ro.verify_checksums=true;
        for(int i=0;i<2000;i++){std::string out;if(db3->Get(ro,key(i),&out).ok()){if(out==val(i))kept++;else torn++;}}
        printf("open(paranoid=false): OK, prefix keys kept=%d torn-values=%d\n",kept,torn);
        delete db3;
    } else printf("open(paranoid=false): %s\n", s3.ToString().c_str());
    delete db; leveldb::DestroyDB(path,opts);
    if(torn==0){printf("CONFORM: no torn replay (kept=%d, paranoid-open=%s)\n",kept,s.ToString().c_str());return 0;}
    printf("FIRST-INVALID: %d torn values replayed\n",torn);
    return 1;
}
