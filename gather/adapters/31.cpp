#include "common.h"
namespace gather {
std::vector<std::string> entries(){return {"subref"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"n",8192},{"source_count",65536},{"element_bytes",8}};RNG r(seed);r.fill(in.add<uint8_t>("source","u8",65536*8,"gather"),65536*8);in.add<uint8_t>("output","u8",8192*8,"output",true);auto i=in.add<uint32_t>("indices","u32",8192,"index");for(int k=0;k<8192;++k)i[k]=r.next()%65536;return in;}
void validate(Input &in){auto n=in.p("n",0,10000000),src=in.p("source_count",1,10000000),sz=in.p("element_bytes",1,256);in.a<uint8_t>("source","u8",src*sz);in.a<uint8_t>("output","u8",n*sz);index_range(in.a<uint32_t>("indices","u32",n),n,src);roles(in,"source","gather");roles(in,"output","output",true);roles(in,"indices","index");}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto a=c->work.ptr<GB_void>("source"),b=c->work.ptr<GB_void>("output");auto i=c->work.ptr<uint32_t>("indices");auto n=in.params.at("n"),sz=in.params.at("element_bytes");auto fn=SELECT(GB_bitmap_subref,GB_bitmap_subref_rvv);c->call=[=]{fn(b,a,sz,i,nullptr,GB_LIST,nullptr,0,n,0,0);};return c;}
}
