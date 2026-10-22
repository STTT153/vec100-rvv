# 包含 vluxei（gather）的 kernel

扫描当前 `kernels/` 源码，共找到 **15 个 kernel**，包含 **31 处 `vluxei` intrinsic 调用**，均位于 `variants/rvv.cpp`。

统计口径：包含 `__riscv_vluxei*` 调用的 kernel，包含 masked 形式；数量为源码中的静态调用点数，不是运行时执行次数。此清单基于源码检索，不包含仅由编译器自动生成的 gather 指令。

| Kernel | vluxei intrinsic | 调用点数 | 源码位置（行号） |
|---|---|---:|---|
| `06_copy_and_encode` | `__riscv_vluxei8_v_u8m1` | 1 | [kernels/06_copy_and_encode/variants/rvv.cpp](kernels/06_copy_and_encode/variants/rvv.cpp)：40 |
| `16_encode_mcu_ac_first_prepare` | `__riscv_vluxei32_v_i16m1` | 1 | [kernels/16_encode_mcu_ac_first_prepare/variants/rvv.cpp](kernels/16_encode_mcu_ac_first_prepare/variants/rvv.cpp)：31 |
| `25_findposfirstlast_c` | `__riscv_vluxei32_v_i16m2` | 1 | [kernels/25_findposfirstlast_c/variants/rvv.cpp](kernels/25_findposfirstlast_c/variants/rvv.cpp)：20 |
| `30_gb_add_full_32` | `__riscv_vluxei64_v_f64m1` | 1 | [kernels/30_gb_add_full_32/variants/rvv.cpp](kernels/30_gb_add_full_32/variants/rvv.cpp)：55 |
| `31_gb_bitmap_subref` | `__riscv_vluxei64_v_u8mf8` | 1 | [kernels/31_gb_bitmap_subref/variants/rvv.cpp](kernels/31_gb_bitmap_subref/variants/rvv.cpp)：55 |
| `32_gb_bld_template` | `__riscv_vluxei64_v_f64m1` | 1 | [kernels/32_gb_bld_template/variants/rvv.cpp](kernels/32_gb_bld_template/variants/rvv.cpp)：47 |
| `33_gb_emult_02a` | `__riscv_vluxei64_v_f64m1_m`, `__riscv_vluxei64_v_u8mf8` | 2 | [kernels/33_gb_emult_02a/variants/rvv.cpp](kernels/33_gb_emult_02a/variants/rvv.cpp)：35, 51 |
| `41_ggml_vec_dot_f16` | `__riscv_vluxei32_v_f32m2` | 2 | [kernels/41_ggml_vec_dot_f16/variants/rvv.cpp](kernels/41_ggml_vec_dot_f16/variants/rvv.cpp)：28, 30 |
| `44_gmp_primesieve` | `__riscv_vluxei64_v_u64m1` | 1 | [kernels/44_gmp_primesieve/variants/rvv.cpp](kernels/44_gmp_primesieve/variants/rvv.cpp)：24 |
| `47_hb_linear_gradient_region` | `__riscv_vluxei32_v_u32m1` | 1 | [kernels/47_hb_linear_gradient_region/variants/rvv.cpp](kernels/47_hb_linear_gradient_region/variants/rvv.cpp)：60 |
| `55_jpeg_ac_refine` | `__riscv_vluxei32_v_i16m1` | 1 | [kernels/55_jpeg_ac_refine/variants/rvv.cpp](kernels/55_jpeg_ac_refine/variants/rvv.cpp)：41 |
| `61_mpn_fft_mul_2exp_modf` | `__riscv_vluxei64_v_u64m1` | 6 | [kernels/61_mpn_fft_mul_2exp_modf/variants/rvv.cpp](kernels/61_mpn_fft_mul_2exp_modf/variants/rvv.cpp)：79, 80, 104, 105, 125, 126 |
| `69_pcre2_match` | `__riscv_vluxei8_v_u8m1` | 1 | [kernels/69_pcre2_match/variants/rvv.cpp](kernels/69_pcre2_match/variants/rvv.cpp)：12 |
| `95_x265_rdo_recount_sign` | `__riscv_vluxei16_v_i16m1` | 2 | [kernels/95_x265_rdo_recount_sign/variants/rvv.cpp](kernels/95_x265_rdo_recount_sign/variants/rvv.cpp)：44, 45 |
| `98_yuv2rgb_c_24_rgb` | `__riscv_vluxei64_v_i32m4`, `__riscv_vluxei64_v_u64m8`, `__riscv_vluxei64_v_u8m1` | 9 | [kernels/98_yuv2rgb_c_24_rgb/variants/rvv.cpp](kernels/98_yuv2rgb_c_24_rgb/variants/rvv.cpp)：22, 23, 30, 32, 34, 37, 43, 45, 47 |

复查命令：

```sh
rg -n 'vluxei' kernels
```
