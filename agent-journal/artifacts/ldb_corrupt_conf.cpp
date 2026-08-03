// LevelDB checksum/corruption conformance (goal 127, cycle 304)
// corrupt one byte in the table file -> reads must surface
// Corruption status (never silent bad data) with verify_checksums.
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <cstdio>
#include <string>
#include <filesystem>
static std::string key(int i){char b[32];snprintf(b,sizeof b,"key%08d",i);return b;}
static std::string val(int i){char b[96];snprintf(b,sizeof b,"val%08d-payload-payload-payload",i);return b;}
int main(){
    const std::string path="/tmp/ldb_corrupt_conf_db";
    leveldb::Options opts; opts.create_if_missing=true;
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    if(!leveldb::DB::Open(opts,path,&db).ok()){fprintf(stderr,"open fail\n");return 2;}
    constexpr int N=5000;
    for(int i=0;i<N;i++) db->Put(leveldb::WriteOptions(),key(i),val(i));
    db->CompactRange(nullptr,nullptr); // force everything into a table file
    delete db;
    // corrupt one byte deep into the .ldb table file
    std::string table;
    for (auto& e : std::filesystem::directory_iterator(path))
        if (e.path().extension()==".ldb") table=e.path().string();
    if (table.empty()){fprintf(stderr,"no table file\n");return 2;}
    FILE* f=fopen(table.c_str(),"r+b");
    fseek(f,0,SEEK_END); long sz=ftell(f);
    fseek(f,sz/2,SEEK_SET);
    int orig=fgetc(f); fseek(f,sz/2,SEEK_SET); fputc(orig^0xFF,f); fclose(f);
    printf("corrupted byte at %ld of %s (%d -> %d)\n",sz/2,table.c_str(),orig,orig^0xFF);
    // reopen and read with verify_checksums=true (dbwrapper's setting)
    leveldb::Options opts2;
    leveldb::Status rs=leveldb::DB::Open(opts2,path,&db);
    printf("reopen status: %s\n", rs.ToString().c_str());
    leveldb::ReadOptions ro; ro.verify_checksums=true;
    int silent=0, corrupt=0, ok=0;
    for(int i=0;i<N;i++){
        std::string out;
        leveldb::Status s=db->Get(ro,key(i),&out);
        if(s.ok()){ if(out!=val(i)){printf("FIRST-INVALID silent wrong data %s -> %s\n",key(i).c_str(),out.c_str());silent++;} else ok++; }
        else if(s.IsCorruption()) corrupt++;
        else if(s.IsNotFound()){} // key may sit in uncorrupted region
    }
    printf("reads: ok=%d corruption-surfaced=%d silent-wrong=%d\n",ok,corrupt,silent);
    delete db; leveldb::DestroyDB(path,opts);
    return silent?1:0;
}
