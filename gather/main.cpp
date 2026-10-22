#include "harness.h"
#include "build_info.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace gather;
static std::string csv(const std::string &s) {std::string r="\"";for(char c:s){r+=c;if(c=='"')r+='"';}return r+'"';}
static uint64_t number(const std::string &s) {
    require(!s.empty()&&s.find_first_not_of("0123456789")==std::string::npos,"expected unsigned decimal integer");
    size_t used; auto n=std::stoull(s,&used);require(used==s.size(),"invalid integer");return n;
}
int main(int argc,char **argv) {
 try {
    std::string entry,input,export_dir,output,policy="default";
#ifdef GATHER_HOST_ONLY
    std::string impl="original";
#else
    std::string impl="rvv";
#endif
    uint64_t seed=1,samples=100; bool gen=false,verify=false,entry_set=false,seed_set=false;
    std::vector<std::string> warm;
    for(int i=1;i<argc;++i) {
        std::string opt=argv[i];
        if(opt=="--help") {std::cout<<"--list-entries | [--entry NAME] [--input DIR | --generate [--seed N]]\n"
            "[--export-input NEW_DIR] [--samples N] [--impl rvv|original] [--verify]\n"
            "[--memory-policy default|warm] [--warm-buffer NAME ...] [--output FILE]\n"
            "Export only: --generate --export-input DIR --samples 0\n";return 0;}
        if(opt=="--list-entries") {for(auto &e:entries())std::cout<<e<<'\n';return 0;}
        if(opt=="--generate") {gen=true;continue;}
        if(opt=="--verify") {verify=true;continue;}
        require(i+1<argc,"missing value for "+opt);std::string v=argv[++i];
        if(opt=="--entry") {entry=v;entry_set=true;}
        else if(opt=="--input") input=v;
        else if(opt=="--export-input") export_dir=v;
        else if(opt=="--output") output=v;
        else if(opt=="--impl") impl=v;
        else if(opt=="--memory-policy") policy=v;
        else if(opt=="--warm-buffer") warm.push_back(v);
        else if(opt=="--seed") {seed=number(v);seed_set=true;}
        else if(opt=="--samples") samples=number(v);
        else throw std::runtime_error("unknown option: "+opt);
    }
    require(!(gen&&!input.empty()) && !(seed_set&&!input.empty()),"generation/seed and input loading are mutually exclusive");
    require(samples<=10000000,"samples exceeds safety limit");
    require(impl=="rvv"||impl=="original","invalid implementation");
#ifdef GATHER_HOST_ONLY
    require(impl=="original","host-only build does not contain RVV; use only for functional checks");
#endif
    Input in;
    if(input.empty()) {if(entry.empty())entry=entries().front();in=generate(entry,seed);in.kernel=GATHER_KERNEL;}
    else {in=load(input);require(!entry_set||entry==in.entry,"--entry conflicts with manifest");entry=in.entry;}
    require(in.kernel==GATHER_KERNEL,"input kernel does not match executable");
    auto names=entries();require(std::find(names.begin(),names.end(),entry)!=names.end(),"unknown entry: "+entry);
    validate(in);in.checked();
    if(!export_dir.empty()) save(in,export_dir);
    require(samples||!export_dir.empty(),"zero samples requires --export-input");
    if(!samples) {std::cerr<<"exported "<<identity(in)<<'\n';return 0;}
    auto platform=make_platform(policy,warm);
    auto test=make_case(in,*platform,impl=="rvv");
    for(auto &name:warm) {
        bool found=false;for(auto &b:test->work.buffers) found|=name==b.name;
        require(found,"unknown warm buffer: "+name);
    }
    // All allocation and CSV metadata construction precede the first sample.
    std::vector<uint64_t> times(samples);
    std::ostringstream desc;
    for(auto &b:test->work.buffers) desc<<b.name<<':'<<b.size<<':'<<b.roles<<":align="<<b.alignment<<":mutable="<<b.writable<<';';
    barrier();uint64_t t0=platform->now();barrier();uint64_t t1=platform->now();barrier();
    require(t1>=t0,"nonmonotonic timer");uint64_t overhead=t1-t0;
    for(size_t i=0;i<samples;++i) {
        times[i]=sample_once(*test,*platform);
    }
    // Reference setup and every byte comparison happen only after all samples.
    std::string verified="not_requested";
    if(verify) {
        auto actual=test->snapshot();
        Platform reference_platform;
        auto reference=make_case(in,reference_platform,false);
        reference->reset();reference->call();
        auto expected=reference->snapshot();
        require(actual==expected,"verification failed (final measured sample differs from original)");
        test->reset();test->call();
        require(test->snapshot()==expected,"verification failed after reset");
        verified="passed";
    }
    const auto input_id=identity(in);
    std::string warm_names;for(auto &n:warm)warm_names+=n+";";
    std::ofstream file;std::ostream *out=&std::cout;
    if(!output.empty()) {file.open(output);require(bool(file),"cannot open output CSV");out=&file;}
    *out<<"kernel,entry,input_fnv1a64,impl,sample,elapsed_ns,empty_timer_ns,clock,memory_policy,warm_buffers,buffers,build,verification\n";
    for(size_t i=0;i<times.size();++i)
        *out<<csv(in.kernel)<<','<<csv(entry)<<','<<input_id<<','<<impl<<','<<i<<','<<times[i]<<','<<overhead<<','
            <<csv(platform->clock_name())<<','<<csv(policy)<<','<<csv(warm_names)<<','<<csv(desc.str())<<','<<csv(GATHER_BUILD_INFO)<<','<<verified<<'\n';
    out->flush();require(bool(*out),"CSV write failed");
    auto sorted=times;std::sort(sorted.begin(),sorted.end());
    double median=sorted.size()%2 ? double(sorted[sorted.size()/2]) : double(sorted[sorted.size()/2-1])/2+double(sorted[sorted.size()/2])/2;
    std::cerr<<"kernel="<<in.kernel<<" entry="<<entry<<" samples="<<samples<<" min_ns="<<sorted.front()<<" median_ns="<<median
             <<" p95_ns="<<sorted[size_t(std::ceil(sorted.size()*.95))-1]<<" empty_timer_ns="<<overhead<<" verification="<<verified<<'\n';
    return 0;
 } catch(const std::exception &e) {std::cerr<<"gather: "<<e.what()<<'\n';return 1;}
}
