#include "harness.h"
#include <cassert>
#include <iostream>
using namespace gather;
struct Recording:Platform {
    std::vector<std::string> events;
    std::map<void *,void *> allocations;
    unsigned ticks=0;
    void *allocate(const Buffer &b) override {
        // Deliberately return an interior aligned address in a larger allocation.
        Buffer larger=b;larger.size=b.size+128;
        void *base=Platform::allocate(larger);
        auto p=static_cast<uint8_t *>(base)+64;allocations[p]=base;return p;
    }
    void release(const Buffer &b) noexcept override {
        Buffer base=b;base.address=allocations[b.address];Platform::release(base);allocations.erase(b.address);
    }
    void prepare(const std::vector<Buffer>&b) override {
        events.push_back("prepare");
        assert(*static_cast<int32_t *>(b[0].address)==7);
    }
    uint64_t now() override {events.push_back("clock");return ++ticks*10;}
    void after_sample(const std::vector<Buffer>&) override {events.push_back("after");}
};
int main(){
    Input in;*in.add<int32_t>("value","i32",1,"output",true)=7;
    in.refs["alias"]={{"value",0},{"value",0}};
    Recording p;
    {
        Case c(in,p);c.work.add("pointers",2*sizeof(void *),"pointer");
        auto x=c.work.ptr<int32_t>("value");auto refs=c.work.ptr<void *>("pointers");
        c.rebind=[&]{p.events.push_back("rebind");c.work.bind("alias",refs);assert(refs[0]==x&&refs[1]==x);};
        c.call=[&]{p.events.push_back("call");assert(*x==7);*x+=5;c.result=12;};
        for(int i=0;i<3;++i){p.events.clear();assert(sample_once(c,p)==10);assert(*x==12);assert((p.events==std::vector<std::string>{"rebind","prepare","clock","call","clock","after"}));}
    }
    assert(p.allocations.empty());
    bool rejected=false;try{auto p=make_platform("tcm",{});}catch(const std::exception&){rejected=true;}assert(rejected);
    std::cout<<"harness ordering, restoration, alias rebinding and relocation passed\n";
}
