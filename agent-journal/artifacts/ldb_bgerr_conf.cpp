// LevelDB background-error propagation (goal 127, cycle 304, iter 2)
// After a background compaction fails (table dir made unwritable),
// subsequent writes MUST surface the recorded bg error (not swallow).
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
static std::string key(long i){char b[32];snprintf(b,sizeof b,"key%012ld",i);return b;}
int main(){
    const std::string path="/tmp/ldb_bgerr_conf_db";
    leveldb::Options opts; opts.create_if_missing=true;
    opts.write_buffer_size=64*1024; // small memtable -> fast L0 files
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    if(!leveldb::DB::Open(opts,path,&db).ok()){fprintf(stderr,"open fail\n");return 2;}
    std::string big(8192,'x');
    long seq=0;
    // produce several L0 files so a background compaction becomes due
    for(int r=0;r<8;r++){ for(int i=0;i<12;i++) db->Put(leveldb::WriteOptions(),key(seq++),big); }
    rename(path.c_str(), (path+".away").c_str()); // dir gone: new table files fail
    leveldb::Status last;
    long writes_after=0;
    bool surfaced=false;
    for(int r=0;r<200 && !surfaced;r++){
        for(int i=0;i<12;i++){
            last=db->Put(leveldb::WriteOptions(),key(seq++),big);
            writes_after++;
            if(!last.ok()){surfaced=true;break;}
        }
        usleep(50*1000); // let the background compaction attempt run
    }
    printf("write-after-fault: %s after %ld writes\n", last.ToString().c_str(), writes_after);
    rename((path+".away").c_str(), path.c_str());
    delete db; leveldb::DestroyDB(path,opts);
    if(surfaced){printf("CONFORM: background error surfaced on subsequent write (%s)\n", last.ToString().c_str()); return 0;}
    printf("FIRST-INVALID: writes kept succeeding against failed background compaction\n");
    return 1;
}
