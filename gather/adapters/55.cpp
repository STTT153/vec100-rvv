#include "common.h"
extern decltype(encode_mcu_AC_refine_prepare) encode_mcu_AC_refine_prepare_rvv;
namespace gather {
std::vector<std::string> entries(){return {"refine"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"Sl",63},{"Al",1}};RNG r(seed);r.fill(in.add<int16_t>("block","i16",64,"gather"),64);auto o=in.add<int32_t>("order","i32",63,"index");for(int i=0;i<63;++i)o[i]=r.next()%64;in.add<uint16_t>("values","u16",64,"output",true);in.add<uint64_t>("bits","u64",2,"output",true);return in;}
void validate(Input &in){auto n=in.p("Sl",0,63);in.p("Al",0,13);in.a<int16_t>("block","i16",64);index_range(in.a<int32_t>("order","i32",n),n,64);in.a<uint16_t>("values","u16",64);in.a<uint64_t>("bits","u64",2);roles(in,"block","gather");roles(in,"order","index");roles(in,"values","output",true);roles(in,"bits","output",true);}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto b=c->work.ptr<JCOEF>("block");auto o=c->work.ptr<int>("order");auto v=c->work.ptr<UJCOEF>("values");auto bits=c->work.ptr<size_t>("bits");int n=in.params.at("Sl"),al=in.params.at("Al");auto fn=SELECT(encode_mcu_AC_refine_prepare,encode_mcu_AC_refine_prepare_rvv);auto raw=c.get();c->call=[=]{raw->result=fn(b,o,n,al,v,bits);};return c;}
}
