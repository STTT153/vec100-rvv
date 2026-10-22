#include "common.h"
extern decltype(copy_and_encode_isolated) copy_and_encode_rvv;
extern decltype(encode_in_place_isolated) encode_in_place_rvv;
namespace gather {
std::vector<std::string> entries(){return {"copy","in_place"};}
Input generate(const std::string &entry,uint64_t seed){Input in;in.entry=entry;in.params={{"n",65536},{"distance",127},{"pos",0}};RNG r(seed);r.fill(in.add<uint8_t>("input","u8",65536,"input",entry=="in_place"),65536);r.fill(in.add<uint8_t>("history","u8",256,"initial_state"),256);in.add<uint8_t>("output","u8",65536,"output",true);return in;}
void validate(Input &in){auto n=in.p("n",0,100000000);in.p("distance",1,256);in.p("pos",0,255);in.a<uint8_t>("input","u8",n);in.a<uint8_t>("history","u8",256);in.a<uint8_t>("output","u8",n);roles(in,"input",in.entry=="in_place"?"input,output":"input",in.entry=="in_place");roles(in,"history","initial_state");roles(in,"output","output",true);}
std::unique_ptr<Case> make_case(const Input &in,Platform &p,bool use_rvv){auto c=std::make_unique<Case>(in,p);c->work.add("coder",sizeof(lzma_delta_coder),"state,gather",true,true);auto state=c->work.ptr<lzma_delta_coder>("coder");new(state) lzma_delta_coder{};auto a=c->work.ptr<uint8_t>("input"),b=c->work.ptr<uint8_t>("output"),h=c->work.ptr<uint8_t>("history");auto n=in.params.at("n"),d=in.params.at("distance"),pos=in.params.at("pos");c->rebind=[=]{state->distance=d;state->pos=pos;std::memcpy(state->history,h,256);};if(in.entry=="copy"){auto fn=SELECT(copy_and_encode_isolated,copy_and_encode_rvv);c->call=[=]{fn(state,a,b,n);};}else{auto fn=SELECT(encode_in_place_isolated,encode_in_place_rvv);c->call=[=]{fn(state,a,n);};}return c;}
}
