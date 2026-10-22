#include "common.h"
namespace gather {
std::vector<std::string> entries(){return {"dot"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"n",8192}};RNG r(seed);auto x=in.add<uint16_t>("x","u16",8192,"index,input"),y=in.add<uint16_t>("y","u16",8192,"index,input");for(int i=0;i<8192;++i){x[i]=r.next()%0x7c00;y[i]=r.next()%0x7c00;}auto table=in.add<float>("table","f32",65536,"gather");float dummy;ggml_fp16_t zero=0;ggml_vec_dot_f16_isolated(0,&dummy,0,&zero,0,&zero,0,1);std::memcpy(table,ggml_table_f32_f16,65536*sizeof(float));in.add<float>("sum","f32",1,"output",true);return in;}
void validate(Input &in){auto n=in.p("n",0,10000000);in.a<uint16_t>("x","u16",n);in.a<uint16_t>("y","u16",n);in.a<float>("table","f32",65536);in.a<float>("sum","f32",1);roles(in,"x","index,input");roles(in,"y","index,input");roles(in,"table","gather");roles(in,"sum","output",true);}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);auto x=c->work.ptr<ggml_fp16_t>("x"),y=c->work.ptr<ggml_fp16_t>("y");auto table=c->work.ptr<float>("table"),sum=c->work.ptr<float>("sum");int n=in.params.at("n");auto fn=SELECT(ggml_vec_dot_f16_with_table,ggml_vec_dot_f16_with_table_rvv);c->call=[=]{fn(n,sum,x,y,table);};return c;}
}
