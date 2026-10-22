#include "harness.h"
#include <time.h>
namespace gather {
void *Platform::allocate(const Buffer &b) {
    void *p=nullptr;
    if(posix_memalign(&p,b.alignment,std::max<size_t>(b.size,1))) return nullptr;
    return p;
}
void Platform::release(const Buffer &b) noexcept { std::free(b.address); }
void Platform::prepare(const std::vector<Buffer> &buffers) {
    if(policy!="warm") return;
    // Read every byte: independent of the board's cache-line size. Outside timing.
    // This is a pre-touch, not a cache-residency guarantee.
    uint8_t sum=0;
    for(const auto &b:buffers) if(std::find(warm.begin(),warm.end(),b.name)!=warm.end()) {
        const volatile uint8_t *p=static_cast<const volatile uint8_t *>(b.address);
        for(size_t i=0;i<b.size;++i) sum ^= p[i];
    }
    asm volatile("" :: "r"(sum) : "memory");
}
uint64_t Platform::now() {
    timespec t{};
    require(clock_gettime(CLOCK_MONOTONIC_RAW,&t)==0,"clock_gettime failed");
    return uint64_t(t.tv_sec)*1000000000ULL+uint64_t(t.tv_nsec);
}
std::unique_ptr<Platform> make_platform(const std::string &policy,const std::vector<std::string> &warm) {
    require(policy=="default" || policy=="warm", "unsupported memory policy: "+policy+" (TCM requires a board-specific allocator)");
    require(policy=="warm" || warm.empty(),"--warm-buffer requires --memory-policy warm");
    require(policy!="warm" || !warm.empty(),"warm policy requires at least one --warm-buffer");
    auto p=std::make_unique<Platform>(); p->policy=policy; p->warm=warm; return p;
}
}
