#include "common.h"
extern decltype(findPosFirstLast_c) findPosFirstLast_c_rvv;
namespace gather {
std::vector<std::string> entries(){return {"find"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"stride",32}};RNG r(seed);r.fill(in.add<int16_t>("coeff","i16",128,"gather"),128);auto s=in.add<uint16_t>("scan","u16",16,"index");for(int i=0;i<16;++i)s[i]=15-i;return in;}
void validate(Input &in){auto s=in.p("stride",4,32);require(s==4||s==8||s==16||s==32,"stride must be 4/8/16/32");in.a<int16_t>("coeff","i16",4*s);auto scan=in.a<uint16_t>("scan","u16",16);index_range(scan,16,16);require(std::set<uint16_t>(scan,scan+16).size()==16,"scan must be a permutation");roles(in,"coeff","gather");roles(in,"scan","index");}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto b=c->work.ptr<int16_t>("coeff");auto s=c->work.ptr<uint16_t>("scan");auto stride=in.params.at("stride");auto fn=SELECT(findPosFirstLast_c,findPosFirstLast_c_rvv);auto raw=c.get();c->call=[=]{raw->result=fn(b,stride,s);};return c;}
}
