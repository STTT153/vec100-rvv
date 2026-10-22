#include "common.h"
extern decltype(gmp_primesieve) gmp_primesieve_rvv;
namespace gather {
std::vector<std::string> entries(){return {"sieve"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"n",1000000}};size_t count=(((1000000-5)|1)/3)/64+1;RNG r(seed);r.fill(in.add<uint64_t>("bitmap","u64",count,"gather,output",true),count);return in;}
void validate(Input &in){auto n=in.p("n",5,1000000000);size_t count=(((n-5)|1)/3)/64+1;in.a<uint64_t>("bitmap","u64",count);roles(in,"bitmap","gather,output",true);}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto a=c->work.ptr<mp_limb_t>("bitmap");auto n=in.params.at("n");auto fn=SELECT(gmp_primesieve,gmp_primesieve_rvv);auto raw=c.get();c->call=[=]{raw->result=fn(a,n);};return c;}
}
