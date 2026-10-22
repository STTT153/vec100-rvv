#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace gather {
inline void require(bool ok, const std::string &message) {
    if (!ok) throw std::runtime_error(message);
}
struct Array {
    std::string type, roles;
    bool writable = false, compare = true;
    std::vector<uint8_t> bytes;
};
struct Ref { std::string target; uint64_t offset; };
struct Input {
    std::string kernel, entry;
    std::map<std::string, int64_t> params;
    std::map<std::string, Array> arrays;
    std::map<std::string, std::vector<Ref>> refs;
    mutable std::set<std::string> used_params, used_arrays, used_refs;
    int64_t p(const std::string &name, int64_t lo, int64_t hi) const {
        auto it = params.find(name);
        require(it != params.end(), "missing parameter: " + name);
        require(it->second >= lo && it->second <= hi, "parameter out of range: " + name);
        used_params.insert(name); return it->second;
    }
    template<class T> const T *a(const std::string &name, const std::string &type,
                               size_t count) const {
        auto it = arrays.find(name);
        require(it != arrays.end(), "missing array: " + name);
        require(it->second.type == type && count <= SIZE_MAX / sizeof(T) &&
                it->second.bytes.size() == count * sizeof(T), "array type/size mismatch: " + name);
        used_arrays.insert(name);
        return reinterpret_cast<const T *>(it->second.bytes.data());
    }
    const std::vector<Ref> &r(const std::string &name, size_t count) const {
        auto it = refs.find(name);
        require(it != refs.end() && it->second.size() == count, "reference table mismatch: " + name);
        used_refs.insert(name); return it->second;
    }
    void checked() const {
        require(used_params.size()==params.size() && used_arrays.size()==arrays.size() &&
                used_refs.size()==refs.size(), "unknown parameters, arrays or reference tables");
    }
    template<class T> T *add(const std::string &name, const std::string &type, size_t count,
                            const std::string &roles, bool writable=false, bool compare=true) {
        require(count <= SIZE_MAX / sizeof(T), "array size overflow");
        Array b{type, roles, writable, compare, std::vector<uint8_t>(count*sizeof(T))};
        arrays[name] = std::move(b);
        return reinterpret_cast<T *>(arrays[name].bytes.data());
    }
    void role(const std::string &name, const std::string &roles, bool writable=false, bool compare=true) {
        auto &b=arrays.at(name); b.roles=roles; b.writable=writable; b.compare=compare;
    }
};
struct RNG {
    uint64_t state;
    explicit RNG(uint64_t seed):state(seed) {}
    uint64_t next() { state += 0x9e3779b97f4a7c15ULL; auto z=state;
        z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL; z=(z^(z>>27))*0x94d049bb133111ebULL; return z^(z>>31); }
    template<class T> void fill(T *p,size_t n) { for(size_t i=0;i<n;++i) p[i]=static_cast<T>(next()); }
};
struct Buffer {
    std::string name, roles;
    size_t size=0, alignment=64;
    bool writable=false, compare=false;
    void *address=nullptr;
};
struct Platform {
    virtual ~Platform()=default;
    virtual void *allocate(const Buffer &);
    virtual void release(const Buffer &) noexcept;
    virtual void prepare(const std::vector<Buffer> &);
    virtual void after_sample(const std::vector<Buffer> &) {}
    virtual uint64_t now();
    virtual const char *clock_name() const { return "CLOCK_MONOTONIC_RAW/ns"; }
    std::string policy="default";
    std::vector<std::string> warm;
};
// Replace platform.cpp or provide another implementation of this factory.
std::unique_ptr<Platform> make_platform(const std::string &,const std::vector<std::string> &);
struct Work {
    const Input &input;
    Platform &platform;
    std::vector<Buffer> buffers;
    Work(const Input &in,Platform &p):input(in),platform(p) {
        try { for(const auto &kv: in.arrays) {
            const auto &a=kv.second;
            add(kv.first,a.bytes.size(),a.roles,a.writable,a.compare);
            if(!a.bytes.empty()) std::memcpy(ptr<void>(kv.first),a.bytes.data(),a.bytes.size());
        }} catch(...) { clear(); throw; }
    }
    Work(const Work &)=delete;
    ~Work() { clear(); }
    void clear() noexcept { for(auto &b:buffers) platform.release(b); buffers.clear(); }
    void add(const std::string &name,size_t n,const std::string &roles,bool writable=false,bool compare=false) {
        for(auto &b:buffers) require(b.name!=name,"duplicate runtime buffer: "+name);
        Buffer b{name,roles,n,64,writable,compare,nullptr};
        b.address=platform.allocate(b);
        require(b.address!=nullptr,"allocation failed: "+name);
        if(reinterpret_cast<uintptr_t>(b.address)%b.alignment) { platform.release(b); throw std::runtime_error("unaligned allocation: "+name); }
        try { buffers.push_back(b); } catch(...) { platform.release(b); throw; }
        if(n) std::memset(b.address,0,n);
    }
    template<class T> T *ptr(const std::string &name) {
        for(auto &b:buffers) if(b.name==name) return static_cast<T *>(b.address);
        throw std::runtime_error("unknown work buffer: "+name);
    }
    void restore() {
        for(const auto &kv:input.arrays) if(kv.second.writable && !kv.second.bytes.empty())
            std::memcpy(ptr<void>(kv.first),kv.second.bytes.data(),kv.second.bytes.size());
    }
    void bind(const std::string &name, void **dest) {
        for(const auto &r:input.refs.at(name)) *dest++=ptr<uint8_t>(r.target)+r.offset;
    }
};
struct Case {
    Work work;
    uint64_t result=0;
    std::function<void()> rebind=[]{};
    std::function<void()> call;
    Case(const Input &in,Platform &p):work(in,p) {}
    void reset() { work.restore(); result=0; rebind(); }
    std::map<std::string,std::vector<uint8_t>> snapshot() const {
        std::map<std::string,std::vector<uint8_t>> out;
        for(auto &b:work.buffers) if(b.compare) {
            auto p=static_cast<const uint8_t *>(b.address); out[b.name]={p,p+b.size};
        }
        out["$result"]=std::vector<uint8_t>(sizeof(result));
        std::memcpy(out["$result"].data(),&result,sizeof(result)); return out;
    }
};
std::vector<std::string> entries();
Input generate(const std::string &,uint64_t);
void validate(Input &);
std::unique_ptr<Case> make_case(const Input &,Platform &,bool);
Input load(const std::string &);
void save(const Input &,const std::string &);
std::string identity(const Input &);
size_t type_size(const std::string &);
inline void barrier() { asm volatile("" ::: "memory"); }
inline void consume(const Case &c) { asm volatile("" :: "g"(&c.result) : "memory"); }
inline uint64_t sample_once(Case &c,Platform &p) {
    c.reset(); p.prepare(c.work.buffers);
    barrier(); auto start=p.now(); barrier();
    c.call();
    barrier(); auto stop=p.now(); barrier();
    require(stop>=start,"nonmonotonic timer");
    consume(c); p.after_sample(c.work.buffers);
    return stop-start;
}
inline void finite_values(const double *a,size_t n) {
    for(size_t i=0;i<n;++i) require(__builtin_isfinite(a[i]),"nonfinite double is outside this adapter's input contract");
}
}
#ifdef GATHER_HOST_ONLY
#define SELECT(original, rvv) original
#else
#define SELECT(original, rvv) (use_rvv ? rvv : original)
#endif
