#pragma once
#include "harness.h"
#include "kernel.h"
using namespace gather;
inline void index_range(const uint32_t *v,size_t n,size_t limit) {for(size_t i=0;i<n;++i)require(v[i]<limit,"index out of range");}
inline void index_range(const uint16_t *v,size_t n,size_t limit) {for(size_t i=0;i<n;++i)require(v[i]<limit,"index out of range");}
inline void index_range(const int32_t *v,size_t n,size_t limit) {for(size_t i=0;i<n;++i)require(v[i]>=0&&size_t(v[i])<limit,"index out of range");}
inline void roles(Input &in,const std::string &name,const std::string &role,bool mut=false,bool compare=true){in.role(name,role,mut,compare);}
