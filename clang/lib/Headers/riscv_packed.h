/*===---- riscv_packed.h - RISC-V P intrinsics -----------------------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __RISCV_PACKED_H
#define __RISCV_PACKED_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Packed SIMD Types */

typedef int8_t int8x4_t __attribute__((__vector_size__(4), __aligned__(4)));
typedef uint8_t uint8x4_t __attribute__((__vector_size__(4), __aligned__(4)));
typedef int16_t int16x2_t __attribute__((__vector_size__(4), __aligned__(4)));
typedef uint16_t uint16x2_t __attribute__((__vector_size__(4), __aligned__(4)));

typedef int8_t int8x8_t __attribute__((__vector_size__(8), __aligned__(8)));
typedef uint8_t uint8x8_t __attribute__((__vector_size__(8), __aligned__(8)));
typedef int16_t int16x4_t __attribute__((__vector_size__(8), __aligned__(8)));
typedef uint16_t uint16x4_t __attribute__((__vector_size__(8), __aligned__(8)));
typedef int32_t int32x2_t __attribute__((__vector_size__(8), __aligned__(8)));
typedef uint32_t uint32x2_t __attribute__((__vector_size__(8), __aligned__(8)));

#define __DEFAULT_FN_ATTRS __attribute__((__always_inline__, __nodebug__))

#define __packed_splat2(ty, x) ((ty){(x), (x)})
#define __packed_splat4(ty, x) ((ty){(x), (x), (x), (x)})
#define __packed_splat8(ty, x) ((ty){(x), (x), (x), (x), (x), (x), (x), (x)})

#define __packed_splat(name, ty, scalar_ty, splat)                             \
  static __inline__ ty __DEFAULT_FN_ATTRS __riscv_##name(scalar_ty __x) {      \
    return splat(ty, __x);                                                     \
  }

#define __packed_shift(name, ty, op, mask)                                     \
  static __inline__ ty __DEFAULT_FN_ATTRS                                      \
  __riscv_##name(ty __rs1, unsigned __rs2) {                                   \
    return __rs1 op (__rs2 & (mask));                                          \
  }
#define __packed_shift8(name, ty, op) __packed_shift(name, ty, op, 0x7)
#define __packed_shift16(name, ty, op) __packed_shift(name, ty, op, 0xf)
#define __packed_shift32(name, ty, op) __packed_shift(name, ty, op, 0x1f)

#define __packed_scalar_binary_op(name, ty, scalar_ty, op, splat)              \
  static __inline__ ty __DEFAULT_FN_ATTRS                                      \
  __riscv_##name(ty __rs1, scalar_ty __rs2) {                                  \
    return __rs1 op splat(ty, __rs2);                                          \
  }

#define __packed_binary_op(name, ty, op)                                       \
  static __inline__ ty __DEFAULT_FN_ATTRS                                      \
  __riscv_##name(ty __rs1, ty __rs2) {                                         \
    return __rs1 op __rs2;                                                     \
  }

#define __packed_unary_op(name, ty, op)                                        \
  static __inline__ ty __DEFAULT_FN_ATTRS __riscv_##name(ty __rs1) {           \
    return op __rs1;                                                           \
  }

#define __packed_minmax(name, ty, builtin)                                     \
  static __inline__ ty __DEFAULT_FN_ATTRS                                      \
  __riscv_##name(ty __rs1, ty __rs2) {                                         \
    return builtin(__rs1, __rs2);                                              \
  }

#define __packed_pm_horiz_binary(name, ret_ty, ty1, ty2)                       \
  static __inline__ ret_ty __DEFAULT_FN_ATTRS                                  \
  __riscv_##name(ty1 __rs1, ty2 __rs2) {                                       \
    return __builtin_riscv_##name(__rs1, __rs2);                               \
  }

#define __packed_pm_horiz_ternary(name, ret_ty, ty1, ty2)                      \
  static __inline__ ret_ty __DEFAULT_FN_ATTRS                                  \
  __riscv_##name(ret_ty __rd, ty1 __rs1, ty2 __rs2) {                          \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }

/* Packed Splat (32-bit) */
__packed_splat(pmv_s_u8x4, uint8x4_t, uint8_t, __packed_splat4)
__packed_splat(pmv_s_i8x4, int8x4_t, int8_t, __packed_splat4)
__packed_splat(pmv_s_u16x2, uint16x2_t, uint16_t, __packed_splat2)
__packed_splat(pmv_s_i16x2, int16x2_t, int16_t, __packed_splat2)

/* Packed Splat (64-bit) */
__packed_splat(pmv_s_u8x8, uint8x8_t, uint8_t, __packed_splat8)
__packed_splat(pmv_s_i8x8, int8x8_t, int8_t, __packed_splat8)
__packed_splat(pmv_s_u16x4, uint16x4_t, uint16_t, __packed_splat4)
__packed_splat(pmv_s_i16x4, int16x4_t, int16_t, __packed_splat4)
__packed_splat(pmv_s_u32x2, uint32x2_t, uint32_t, __packed_splat2)
__packed_splat(pmv_s_i32x2, int32x2_t, int32_t, __packed_splat2)

/* Packed Shifts (32-bit) */
__packed_shift8(psll_s_u8x4, uint8x4_t, <<)
__packed_shift8(psll_s_i8x4, int8x4_t, <<)
__packed_shift16(psll_s_u16x2, uint16x2_t, <<)
__packed_shift16(psll_s_i16x2, int16x2_t, <<)
__packed_shift8(psrl_s_u8x4, uint8x4_t, >>)
__packed_shift16(psrl_s_u16x2, uint16x2_t, >>)
__packed_shift8(psra_s_i8x4, int8x4_t, >>)
__packed_shift16(psra_s_i16x2, int16x2_t, >>)

/* Packed Shifts (64-bit) */
__packed_shift8(psll_s_u8x8, uint8x8_t, <<)
__packed_shift8(psll_s_i8x8, int8x8_t, <<)
__packed_shift16(psll_s_u16x4, uint16x4_t, <<)
__packed_shift16(psll_s_i16x4, int16x4_t, <<)
__packed_shift32(psll_s_u32x2, uint32x2_t, <<)
__packed_shift32(psll_s_i32x2, int32x2_t, <<)
__packed_shift8(psrl_s_u8x8, uint8x8_t, >>)
__packed_shift16(psrl_s_u16x4, uint16x4_t, >>)
__packed_shift32(psrl_s_u32x2, uint32x2_t, >>)
__packed_shift8(psra_s_i8x8, int8x8_t, >>)
__packed_shift16(psra_s_i16x4, int16x4_t, >>)
__packed_shift32(psra_s_i32x2, int32x2_t, >>)

/* Packed Addition with Scalar (32-bit) */
__packed_scalar_binary_op(padd_s_u8x4, uint8x4_t, uint8_t, +, __packed_splat4)
__packed_scalar_binary_op(padd_s_i8x4, int8x4_t, int8_t, +, __packed_splat4)
__packed_scalar_binary_op(padd_s_u16x2, uint16x2_t, uint16_t, +,
                          __packed_splat2)
__packed_scalar_binary_op(padd_s_i16x2, int16x2_t, int16_t, +,
                          __packed_splat2)

/* Packed Addition with Scalar (64-bit) */
__packed_scalar_binary_op(padd_s_u8x8, uint8x8_t, uint8_t, +, __packed_splat8)
__packed_scalar_binary_op(padd_s_i8x8, int8x8_t, int8_t, +, __packed_splat8)
__packed_scalar_binary_op(padd_s_u16x4, uint16x4_t, uint16_t, +,
                          __packed_splat4)
__packed_scalar_binary_op(padd_s_i16x4, int16x4_t, int16_t, +,
                          __packed_splat4)
__packed_scalar_binary_op(padd_s_u32x2, uint32x2_t, uint32_t, +,
                          __packed_splat2)
__packed_scalar_binary_op(padd_s_i32x2, int32x2_t, int32_t, +,
                          __packed_splat2)

/* Packed Addition and Subtraction (32-bit) */
__packed_binary_op(padd_i8x4, int8x4_t, +)
__packed_binary_op(padd_u8x4, uint8x4_t, +)
__packed_binary_op(padd_i16x2, int16x2_t, +)
__packed_binary_op(padd_u16x2, uint16x2_t, +)
__packed_binary_op(psub_i8x4, int8x4_t, -)
__packed_binary_op(psub_u8x4, uint8x4_t, -)
__packed_binary_op(psub_i16x2, int16x2_t, -)
__packed_binary_op(psub_u16x2, uint16x2_t, -)
__packed_unary_op(pneg_i8x4, int8x4_t, -)
__packed_unary_op(pneg_i16x2, int16x2_t, -)

/* Packed Addition and Subtraction (64-bit) */
__packed_binary_op(padd_i8x8, int8x8_t, +)
__packed_binary_op(padd_u8x8, uint8x8_t, +)
__packed_binary_op(padd_i16x4, int16x4_t, +)
__packed_binary_op(padd_u16x4, uint16x4_t, +)
__packed_binary_op(padd_i32x2, int32x2_t, +)
__packed_binary_op(padd_u32x2, uint32x2_t, +)
__packed_binary_op(psub_i8x8, int8x8_t, -)
__packed_binary_op(psub_u8x8, uint8x8_t, -)
__packed_binary_op(psub_i16x4, int16x4_t, -)
__packed_binary_op(psub_u16x4, uint16x4_t, -)
__packed_binary_op(psub_i32x2, int32x2_t, -)
__packed_binary_op(psub_u32x2, uint32x2_t, -)
__packed_unary_op(pneg_i8x8, int8x8_t, -)
__packed_unary_op(pneg_i16x4, int16x4_t, -)
__packed_unary_op(pneg_i32x2, int32x2_t, -)

/* Packed Minimum and Maximum (32-bit) */
__packed_minmax(pmin_i8x4, int8x4_t, __builtin_elementwise_min)
__packed_minmax(pmin_i16x2, int16x2_t, __builtin_elementwise_min)
__packed_minmax(pminu_u8x4, uint8x4_t, __builtin_elementwise_min)
__packed_minmax(pminu_u16x2, uint16x2_t, __builtin_elementwise_min)
__packed_minmax(pmax_i8x4, int8x4_t, __builtin_elementwise_max)
__packed_minmax(pmax_i16x2, int16x2_t, __builtin_elementwise_max)
__packed_minmax(pmaxu_u8x4, uint8x4_t, __builtin_elementwise_max)
__packed_minmax(pmaxu_u16x2, uint16x2_t, __builtin_elementwise_max)

/* Packed Minimum and Maximum (64-bit) */
__packed_minmax(pmin_i8x8, int8x8_t, __builtin_elementwise_min)
__packed_minmax(pmin_i16x4, int16x4_t, __builtin_elementwise_min)
__packed_minmax(pmin_i32x2, int32x2_t, __builtin_elementwise_min)
__packed_minmax(pminu_u8x8, uint8x8_t, __builtin_elementwise_min)
__packed_minmax(pminu_u16x4, uint16x4_t, __builtin_elementwise_min)
__packed_minmax(pminu_u32x2, uint32x2_t, __builtin_elementwise_min)
__packed_minmax(pmax_i8x8, int8x8_t, __builtin_elementwise_max)
__packed_minmax(pmax_i16x4, int16x4_t, __builtin_elementwise_max)
__packed_minmax(pmax_i32x2, int32x2_t, __builtin_elementwise_max)
__packed_minmax(pmaxu_u8x8, uint8x8_t, __builtin_elementwise_max)
__packed_minmax(pmaxu_u16x4, uint16x4_t, __builtin_elementwise_max)
__packed_minmax(pmaxu_u32x2, uint32x2_t, __builtin_elementwise_max)

/* Packed Logical Operations (32-bit) */
__packed_binary_op(pand_i8x4, int8x4_t, &)
__packed_binary_op(pand_u8x4, uint8x4_t, &)
__packed_binary_op(pand_i16x2, int16x2_t, &)
__packed_binary_op(pand_u16x2, uint16x2_t, &)
__packed_binary_op(por_i8x4, int8x4_t, |)
__packed_binary_op(por_u8x4, uint8x4_t, |)
__packed_binary_op(por_i16x2, int16x2_t, |)
__packed_binary_op(por_u16x2, uint16x2_t, |)
__packed_binary_op(pxor_i8x4, int8x4_t, ^)
__packed_binary_op(pxor_u8x4, uint8x4_t, ^)
__packed_binary_op(pxor_i16x2, int16x2_t, ^)
__packed_binary_op(pxor_u16x2, uint16x2_t, ^)
__packed_unary_op(pnot_i8x4, int8x4_t, ~)
__packed_unary_op(pnot_u8x4, uint8x4_t, ~)
__packed_unary_op(pnot_i16x2, int16x2_t, ~)
__packed_unary_op(pnot_u16x2, uint16x2_t, ~)

/* Packed Logical Operations (64-bit) */
__packed_binary_op(pand_i8x8, int8x8_t, &)
__packed_binary_op(pand_u8x8, uint8x8_t, &)
__packed_binary_op(pand_i16x4, int16x4_t, &)
__packed_binary_op(pand_u16x4, uint16x4_t, &)
__packed_binary_op(pand_i32x2, int32x2_t, &)
__packed_binary_op(pand_u32x2, uint32x2_t, &)
__packed_binary_op(por_i8x8, int8x8_t, |)
__packed_binary_op(por_u8x8, uint8x8_t, |)
__packed_binary_op(por_i16x4, int16x4_t, |)
__packed_binary_op(por_u16x4, uint16x4_t, |)
__packed_binary_op(por_i32x2, int32x2_t, |)
__packed_binary_op(por_u32x2, uint32x2_t, |)
__packed_binary_op(pxor_i8x8, int8x8_t, ^)
__packed_binary_op(pxor_u8x8, uint8x8_t, ^)
__packed_binary_op(pxor_i16x4, int16x4_t, ^)
__packed_binary_op(pxor_u16x4, uint16x4_t, ^)
__packed_binary_op(pxor_i32x2, int32x2_t, ^)
__packed_binary_op(pxor_u32x2, uint32x2_t, ^)
__packed_unary_op(pnot_i8x8, int8x8_t, ~)
__packed_unary_op(pnot_u8x8, uint8x8_t, ~)
__packed_unary_op(pnot_i16x4, int16x4_t, ~)
__packed_unary_op(pnot_u16x4, uint16x4_t, ~)
__packed_unary_op(pnot_i32x2, int32x2_t, ~)
__packed_unary_op(pnot_u32x2, uint32x2_t, ~)

#if __riscv_xlen == 32
/* Packed Multiplication with Horizontal Addition (32-bit) */
__packed_pm_horiz_binary(pm4add_i8x4,    int32_t,  int8x4_t,  int8x4_t)
__packed_pm_horiz_ternary(pm4adda_i8x4,  int32_t,  int8x4_t,  int8x4_t)
__packed_pm_horiz_binary(pm4addu_u8x4,   uint32_t, uint8x4_t, uint8x4_t)
__packed_pm_horiz_ternary(pm4addau_u8x4, uint32_t, uint8x4_t, uint8x4_t)
__packed_pm_horiz_binary(pm4addsu_i8x4,   int32_t, int8x4_t,  uint8x4_t)
__packed_pm_horiz_ternary(pm4addasu_i8x4, int32_t, int8x4_t,  uint8x4_t)

__packed_pm_horiz_binary(pm2add_i16x2,    int32_t, int16x2_t,  int16x2_t)
__packed_pm_horiz_ternary(pm2adda_i16x2,  int32_t, int16x2_t,  int16x2_t)
__packed_pm_horiz_binary(pm2add_x_i16x2,    int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_ternary(pm2adda_x_i16x2,  int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_binary(pm2addu_u16x2,   uint32_t, uint16x2_t, uint16x2_t)
__packed_pm_horiz_ternary(pm2addau_u16x2, uint32_t, uint16x2_t, uint16x2_t)
__packed_pm_horiz_binary(pm2addsu_i16x2,    int32_t, int16x2_t, uint16x2_t)
__packed_pm_horiz_ternary(pm2addasu_i16x2,  int32_t, int16x2_t, uint16x2_t)

__packed_pm_horiz_binary(pmq2add_i16x2,    int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_binary(pmqr2add_i16x2,   int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_ternary(pmq2adda_i16x2,  int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_ternary(pmqr2adda_i16x2, int32_t, int16x2_t, int16x2_t)

__packed_pm_horiz_binary(pm2sadd_i16x2,    int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_binary(pm2sadd_x_i16x2,  int32_t, int16x2_t, int16x2_t)

__packed_pm_horiz_binary(pm2sub_i16x2,     int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_ternary(pm2suba_i16x2,   int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_binary(pm2sub_x_i16x2,   int32_t, int16x2_t, int16x2_t)
__packed_pm_horiz_ternary(pm2suba_x_i16x2, int32_t, int16x2_t, int16x2_t)
#endif

#if __riscv_xlen == 64
/* Packed Multiplication with Horizontal Addition (64-bit, RV64 only) */
__packed_pm_horiz_binary(pm4add_i8x8,     int32x2_t,  int8x8_t,  int8x8_t)
__packed_pm_horiz_ternary(pm4adda_i8x8,   int32x2_t,  int8x8_t,  int8x8_t)
__packed_pm_horiz_binary(pm4addu_u8x8,    uint32x2_t, uint8x8_t, uint8x8_t)
__packed_pm_horiz_ternary(pm4addau_u8x8,  uint32x2_t, uint8x8_t, uint8x8_t)
__packed_pm_horiz_binary(pm4addsu_i8x8,    int32x2_t, int8x8_t,  uint8x8_t)
__packed_pm_horiz_ternary(pm4addasu_i8x8,  int32x2_t, int8x8_t,  uint8x8_t)

__packed_pm_horiz_binary(pm4add_i16x4,    int64_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_ternary(pm4adda_i16x4,  int64_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pm4addu_u16x4,   uint64_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_ternary(pm4addau_u16x4, uint64_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_binary(pm4addsu_i16x4,   int64_t, int16x4_t,  uint16x4_t)
__packed_pm_horiz_ternary(pm4addasu_i16x4, int64_t, int16x4_t,  uint16x4_t)

__packed_pm_horiz_binary(pm2add_i16x4,     int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_ternary(pm2adda_i16x4,   int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pm2add_x_i16x4,   int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_ternary(pm2adda_x_i16x4, int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pm2addu_u16x4,    uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_ternary(pm2addau_u16x4,  uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_binary(pm2addsu_i16x4,    int32x2_t, int16x4_t,  uint16x4_t)
__packed_pm_horiz_ternary(pm2addasu_i16x4,  int32x2_t, int16x4_t,  uint16x4_t)

__packed_pm_horiz_binary(pm2add_i32x2,     int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_ternary(pm2adda_i32x2,   int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_binary(pm2add_x_i32x2,   int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_ternary(pm2adda_x_i32x2, int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_binary(pm2addu_u32x2,    uint64_t, uint32x2_t, uint32x2_t)
__packed_pm_horiz_ternary(pm2addau_u32x2,  uint64_t, uint32x2_t, uint32x2_t)
__packed_pm_horiz_binary(pm2addsu_i32x2,    int64_t, int32x2_t,  uint32x2_t)
__packed_pm_horiz_ternary(pm2addasu_i32x2,  int64_t, int32x2_t,  uint32x2_t)

__packed_pm_horiz_binary(pmq2add_i16x4,    int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_binary(pmqr2add_i16x4,   int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_ternary(pmq2adda_i16x4,  int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_ternary(pmqr2adda_i16x4, int32x2_t, int16x4_t, int16x4_t)

__packed_pm_horiz_binary(pmq2add_i32x2,    int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_binary(pmqr2add_i32x2,   int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_ternary(pmq2adda_i32x2,  int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_ternary(pmqr2adda_i32x2, int64_t, int32x2_t, int32x2_t)

__packed_pm_horiz_binary(pm2sadd_i16x4,    int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_binary(pm2sadd_x_i16x4,  int32x2_t, int16x4_t, int16x4_t)

__packed_pm_horiz_binary(pm2sub_i16x4,     int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_ternary(pm2suba_i16x4,   int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_binary(pm2sub_x_i16x4,   int32x2_t, int16x4_t, int16x4_t)
__packed_pm_horiz_ternary(pm2suba_x_i16x4, int32x2_t, int16x4_t, int16x4_t)

__packed_pm_horiz_binary(pm2sub_i32x2,     int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_ternary(pm2suba_i32x2,   int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_binary(pm2sub_x_i32x2,   int64_t, int32x2_t, int32x2_t)
__packed_pm_horiz_ternary(pm2suba_x_i32x2, int64_t, int32x2_t, int32x2_t)
#endif

#undef __packed_splat2
#undef __packed_splat4
#undef __packed_splat8
#undef __packed_splat
#undef __packed_shift
#undef __packed_shift8
#undef __packed_shift16
#undef __packed_shift32
#undef __packed_scalar_binary_op
#undef __packed_binary_op
#undef __packed_unary_op
#undef __packed_minmax
#undef __packed_pm_horiz_binary
#undef __packed_pm_horiz_ternary
#undef __DEFAULT_FN_ATTRS

#if defined(__cplusplus)
}
#endif

#endif /* __RISCV_PACKED_H */
