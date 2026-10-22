#include "harness.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace gather {
namespace fs=std::filesystem;
size_t type_size(const std::string &t) {
    if(t=="u8"||t=="i8") return 1;
    if(t=="u16"||t=="i16") return 2;
    if(t=="u32"||t=="i32"||t=="f32") return 4;
    if(t=="u64"||t=="i64"||t=="f64") return 8;
    throw std::runtime_error("unknown array type: "+t);
}
static void token(const std::string &s) {
    require(!s.empty() && s.size()<200 && s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos,"invalid identifier: "+s);
}
static void endian() { const uint16_t v=1; require(*reinterpret_cast<const uint8_t *>(&v)==1,"input format currently requires a little-endian host"); }
Input load(const std::string &dir) {
    endian(); Input in; std::ifstream f(fs::path(dir)/"manifest.txt"); require(bool(f),"cannot open manifest.txt");
    std::string magic; unsigned version=0; f>>magic>>version;
    require(magic=="gather-input" && version==1,"unsupported input format");
    std::string line; std::getline(f,line); require(line.empty(),"trailing header data");
    size_t total=0;
    while(std::getline(f,line)) {
        if(line.empty()||line[0]=='#') continue;
        std::istringstream s(line); std::string tag,name,extra; s>>tag>>name; token(name);
        if(tag=="kernel"||tag=="entry") { auto &v=tag=="kernel"?in.kernel:in.entry; require(v.empty(),"duplicate "+tag); v=name; }
        else if(tag=="param") { int64_t v; require(bool(s>>v),"invalid integer parameter"); require(in.params.emplace(name,v).second,"duplicate parameter"); }
        else if(tag=="array") {
            std::string type; uint64_t count; require(bool(s>>type>>count),"invalid array descriptor");
            size_t width=type_size(type);
            // Bound total file-backed allocation to 1 GiB, checked before multiplication.
            require(count<=((size_t(1)<<30)-total)/width,"input exceeds 1 GiB safety limit");
            size_t n=size_t(count)*width; total+=n;
            auto path=fs::path(dir)/(name+".bin");
            require(fs::file_size(path)==n,"array file length mismatch: "+name);
            Array a{type,"",false,true,std::vector<uint8_t>(n)};
            std::ifstream b(path,std::ios::binary); require(bool(b),"cannot open array: "+name);
            if(n) { b.read(reinterpret_cast<char *>(a.bytes.data()),n); require(bool(b),"short array read: "+name); }
            require(in.arrays.emplace(name,std::move(a)).second,"duplicate array");
        } else if(tag=="ref") {
            size_t index; std::string target; uint64_t offset;
            require(bool(s>>index>>target>>offset),"invalid reference"); token(target);
            auto &refs=in.refs[name]; require(index==refs.size(),"reference slots must be consecutive from zero");
            require(refs.size()<1000000,"too many references"); refs.push_back({target,offset});
        } else throw std::runtime_error("unknown manifest record: "+tag);
        require(!(s>>extra),"trailing manifest fields");
    }
    require(!in.kernel.empty()&&!in.entry.empty(),"missing kernel/entry");
    for(const auto &kv:in.refs) for(const auto &r:kv.second) {
        auto it=in.arrays.find(r.target); require(it!=in.arrays.end() && r.offset<=it->second.bytes.size(),"reference out of bounds");
    }
    return in;
}
void save(const Input &in,const std::string &dir) {
    endian(); fs::path path(dir);
    require(!fs::exists(path),"export directory already exists: "+dir);
    fs::create_directories(path);
    std::ofstream f(path/"manifest.txt"); require(bool(f),"cannot create manifest");
    f<<"gather-input 1\nkernel "<<in.kernel<<"\nentry "<<in.entry<<'\n';
    for(auto &kv:in.params) f<<"param "<<kv.first<<' '<<kv.second<<'\n';
    for(auto &kv:in.arrays) {
        const auto &a=kv.second; f<<"array "<<kv.first<<' '<<a.type<<' '<<a.bytes.size()/type_size(a.type)<<'\n';
        std::ofstream b(path/(kv.first+".bin"),std::ios::binary);
        if(!a.bytes.empty()) b.write(reinterpret_cast<const char *>(a.bytes.data()),a.bytes.size());
        b.close(); require(bool(b),"array export failed");
    }
    for(auto &kv:in.refs) for(size_t i=0;i<kv.second.size();++i)
        f<<"ref "<<kv.first<<' '<<i<<' '<<kv.second[i].target<<' '<<kv.second[i].offset<<'\n';
    f.close(); require(bool(f),"manifest export failed");
}
std::string identity(const Input &in) {
    uint64_t h=14695981039346656037ULL;
    auto bytes=[&](const void *p,size_t n) { auto b=static_cast<const uint8_t *>(p); while(n--) {h^=*b++;h*=1099511628211ULL;} };
    auto str=[&](const std::string &s) {bytes(s.data(),s.size()); uint8_t zero=0;bytes(&zero,1);};
    str(in.kernel);str(in.entry);
    for(auto &kv:in.params) {str(kv.first);str(std::to_string(kv.second));}
    for(auto &kv:in.arrays) {str(kv.first);str(kv.second.type);str(std::to_string(kv.second.bytes.size()));bytes(kv.second.bytes.data(),kv.second.bytes.size());}
    for(auto &kv:in.refs) {str(kv.first);for(auto &r:kv.second){str(r.target);str(std::to_string(r.offset));}}
    std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<h;return s.str();
}
}
