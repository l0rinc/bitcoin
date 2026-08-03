// LevelDB compaction-vs-iterator conformance (goal 126, cycle 302, iter 2)
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <cstdio>
#include <string>
static std::string key(int i){char b[32];snprintf(b,sizeof b,"key%08d",i);return b;}
static std::string val(int i,int g){char b[48];snprintf(b,sizeof b,"val%08d-gen%d",i,g);return b;}
int main(){
    const std::string path="/tmp/ldb_compact_conf_db";
    leveldb::Options opts; opts.create_if_missing=true;
    leveldb::DestroyDB(path,opts);
    leveldb::DB* db=nullptr;
    leveldb::Status s=leveldb::DB::Open(opts,path,&db);
    if(!s.ok()){fprintf(stderr,"open: %s\n",s.ToString().c_str());return 2;}
    constexpr int N=20000;
    for(int i=0;i<N;i++) db->Put(leveldb::WriteOptions(),key(i),val(i,0));
    // live iterator pinned BEFORE mutations+compaction
    leveldb::Iterator* it=db->NewIterator(leveldb::ReadOptions());
    // mutate: overwrite half, delete a quarter, then force full compaction
    leveldb::WriteBatch b;
    for(int i=0;i<N/2;i++) b.Put(key(i),val(i,1));
    for(int i=N/2;i<3*N/4;i++) b.Delete(key(i));
    db->Write(leveldb::WriteOptions(),&b);
    db->CompactRange(nullptr,nullptr);
    // the pinned iterator must still see the ORIGINAL snapshot (gen 0, all N keys)
    int seen=0, bad=0;
    for(it->SeekToFirst(); it->Valid(); it->Next()){
        std::string k=it->key().ToString(), v=it->value().ToString();
        int idx=atoi(k.c_str()+3);
        if(idx<0||idx>=N||v!=val(idx,0)){printf("FIRST-INVALID %s -> %s (expected %s)\n",k.c_str(),v.c_str(),val(idx,0).c_str());bad++;break;}
        seen++;
    }
    if(!it->status().ok()){printf("FIRST-INVALID status: %s\n",it->status().ToString().c_str());bad++;}
    if(seen!=N){printf("FIRST-INVALID snapshot key count %d != %d\n",seen,N);bad++;}
    delete it; delete db; leveldb::DestroyDB(path,opts);
    if(!bad) printf("CONFORM: pinned iterator survives overwrite+delete+CompactRange with full snapshot view (%d keys)\n",seen);
    return bad?1:0;
}
