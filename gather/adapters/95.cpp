#include "common.h"
namespace gather {
std::vector<std::string> entries(){return {"recount"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"count",1024},{"last",1024}};RNG r(seed);auto a=in.add<int16_t>("coeff","i16",1024,"gather,output",true);auto b=in.add<int16_t>("residual","i16",1024,"gather");auto s=in.add<uint16_t>("scan","u16",1024,"index");for(int i=0;i<1024;++i){a[i]=r.next()%100;b[i]=int(r.next()%201)-100;s[i]=1023-i;}return in;}
void validate(Input &in){auto n=in.p("count",1,1024),last=in.p("last",0,n);in.a<int16_t>("coeff","i16",n);in.a<int16_t>("residual","i16",n);index_range(in.a<uint16_t>("scan","u16",last),last,n);roles(in,"coeff","gather,output",true);roles(in,"residual","gather");roles(in,"scan","index");}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto a=c->work.ptr<int16_t>("coeff"),b=c->work.ptr<int16_t>("residual");auto s=c->work.ptr<uint16_t>("scan");int n=in.params.at("count"),last=in.params.at("last");auto fn=SELECT(rdo_recount_sign,rdo_recount_sign_rvv);auto raw=c.get();c->call=[=]{raw->result=fn(s,a,b,last,n);};return c;}
}
