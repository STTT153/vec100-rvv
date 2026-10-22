#include "common.h"
extern decltype(pcre2_match_start_bits_scan) pcre2_match_start_bits_scan_rvv;
extern decltype(pcre2_dfa_match_start_bits_scan) pcre2_dfa_match_start_bits_scan_rvv;
namespace gather {
std::vector<std::string> entries(){return {"match","dfa"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"n",65536},{"mb",65536},{"options",0},{"present",1}};RNG r(seed);r.fill(in.add<uint8_t>("subject","u8",65536,"index"),65536);in.add<uint8_t>("bits","u8",32,"gather");in.add<int32_t>("nomatch","i32",1,"output",true);return in;}
void validate(Input &in){auto n=in.p("n",0,100000000);in.p("mb",0,n);in.p("options",0,in.entry=="match"?65535:UINT32_MAX);in.p("present",0,1);in.a<uint8_t>("subject","u8",n);in.a<uint8_t>("bits","u8",32);in.a<int32_t>("nomatch","i32",1);roles(in,"subject","index");roles(in,"bits","gather");roles(in,"nomatch","output",true);}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto s=c->work.ptr<uint8_t>("subject"),b=c->work.ptr<uint8_t>("bits");auto out=c->work.ptr<int>("nomatch");auto n=in.params.at("n"),mb=in.params.at("mb"),opt=in.params.at("options");if(!in.params.at("present"))b=nullptr;auto raw=c.get();if(in.entry=="match"){auto fn=SELECT(pcre2_match_start_bits_scan,pcre2_match_start_bits_scan_rvv);c->call=[=]{raw->result=fn(s,s+n,s+mb,b,opt,out)-s;};}else{auto fn=SELECT(pcre2_dfa_match_start_bits_scan,pcre2_dfa_match_start_bits_scan_rvv);c->call=[=]{raw->result=fn(s,s+n,s+mb,b,opt,out)-s;};}return c;}
}
