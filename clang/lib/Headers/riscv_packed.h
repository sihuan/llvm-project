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

#define __packed_exchange_add_sub(name, ty)                                    \
  static __inline__ ty __DEFAULT_FN_ATTRS                                      \
  __riscv_##name(ty __rs1, ty __rs2) {                                         \
    return __builtin_riscv_##name(__rs1, __rs2);                               \
  }

/* Packed multiply-high: take the high half of an element-wise widening
 * multiply. LLVM's DAG combiner recognizes the (sext * sext) >> N pattern and
 * lowers it to pmulh.{h,w} (and unsigned/mixed-sign variants). */
#define __packed_pmulh(name, r_ty, a_ty, b_ty, prod_ty, SHIFT)                 \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    prod_ty __p = __builtin_convertvector(__rs1, prod_ty) *                    \
                  __builtin_convertvector(__rs2, prod_ty);                     \
    return __builtin_convertvector(__p >> (SHIFT), r_ty);                      \
  }

/* Packed rounding multiply-high: add 1<<(SHIFT-1) before the shift.
 * The rounding constant uses 1LL to avoid UB when SHIFT == 32. */
#define __packed_pmulhr(name, r_ty, a_ty, b_ty, prod_ty, SHIFT)                \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    prod_ty __p = __builtin_convertvector(__rs1, prod_ty) *                    \
                  __builtin_convertvector(__rs2, prod_ty);                     \
    return __builtin_convertvector((__p + (1LL << ((SHIFT) - 1))) >> (SHIFT),  \
                                   r_ty);                                      \
  }

/* Packed multiply-high accumulate: rd + mulh(rs1, rs2). Pattern-matched in
 * RISCVInstrInfoP.td to PMHACC*_H/W when the mul-high is single-use. */
#define __packed_pmhacc(name, r_ty, a_ty, b_ty, prod_ty, SHIFT)                \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    prod_ty __p = __builtin_convertvector(__rs1, prod_ty) *                    \
                  __builtin_convertvector(__rs2, prod_ty);                     \
    return __rd + __builtin_convertvector(__p >> (SHIFT), r_ty);               \
  }

/* Cross-XLEN packed multiply-parts wrapper used on RV64 to expose the
 * "32-bit input" form of these intrinsics. The instruction processes packed
 * elements; we synthesize a wider operand vector with the unused upper half
 * left zero, then narrow the result back. */
#define __packed_pmul_parts_rv64_half(name, r_narrow_ty, r_wide_ty,            \
                                      a_narrow_ty, a_wide_ty,                  \
                                      b_narrow_ty, b_wide_ty, wide_name)       \
  static __inline__ r_narrow_ty __DEFAULT_FN_ATTRS                             \
  __riscv_##name(a_narrow_ty __rs1, b_narrow_ty __rs2) {                       \
    a_wide_ty __aw = {__rs1[0], __rs1[1], __rs1[2], __rs1[3], 0, 0, 0, 0};     \
    b_wide_ty __bw = {__rs2[0], __rs2[1], __rs2[2], __rs2[3], 0, 0, 0, 0};     \
    r_wide_ty __r = __builtin_riscv_##wide_name(__aw, __bw);                   \
    return (r_narrow_ty){__r[0], __r[1]};                                      \
  }

/* Cross-XLEN packed multiply-parts wrapper for the halfword scalar form
 * (mul.h00 etc.). On RV64 we use the pmul.w.h00 family and take lane 0. */
#define __packed_pmul_parts_rv64_scalar(name, r_ty, vec_r_ty,                  \
                                        a_ty, a_wide_ty,                       \
                                        b_ty, b_wide_ty, vec_name)             \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    a_wide_ty __aw = {__rs1[0], __rs1[1], 0, 0};                               \
    b_wide_ty __bw = {__rs2[0], __rs2[1], 0, 0};                               \
    vec_r_ty __r = __builtin_riscv_##vec_name(__aw, __bw);                     \
    return __r[0];                                                             \
  }

/* Cross-XLEN packed multiply-parts-accumulate wrapper for the halfword
 * scalar form (macc.h00 etc.). On RV64 use the pmacc.w.h00 family: place rd
 * in lane 0, widen the operands, and extract lane 0. */
#define __packed_pmul_parts_acc_rv64_scalar(name, r_ty, vec_r_ty,              \
                                            a_ty, a_wide_ty,                   \
                                            b_ty, b_wide_ty, vec_name)         \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    vec_r_ty __rdw = {__rd, 0};                                                \
    a_wide_ty __aw = {__rs1[0], __rs1[1], 0, 0};                               \
    b_wide_ty __bw = {__rs2[0], __rs2[1], 0, 0};                               \
    vec_r_ty __r = __builtin_riscv_##vec_name(__rdw, __aw, __bw);              \
    return __r[0];                                                             \
  }

/* Direct-builtin packed multiply-parts-accumulate wrapper (3 operands). */
#define __packed_pmul_parts_acc(name, r_ty, a_ty, b_ty)                        \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }

/* Cross-XLEN packed multiply-high-parts scalar wrapper for RV64. The scalar
 * form mulh.h0_i32 maps to pmulh.w.h0: place rs1 in lane 0 of a v2i32, widen
 * rs2 from i16x2 to i16x4, call vector intrinsic, extract lane 0. */
#define __packed_pmulh_parts_rv64_scalar(name, r_ty, vec_r_ty,                 \
                                         a_ty, vec_a_ty,                       \
                                         b_ty, b_wide_ty, vec_name)            \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    vec_a_ty __aw = {__rs1, 0};                                                \
    b_wide_ty __bw = {__rs2[0], __rs2[1], 0, 0};                               \
    vec_r_ty __r = __builtin_riscv_##vec_name(__aw, __bw);                     \
    return __r[0];                                                             \
  }

/* Cross-XLEN packed multiply-high-parts-accumulate scalar wrapper for RV64.
 * Maps mhacc.h0_i32 to pmhacc.w.h0: place rd in lane 0, place rs1 in lane 0,
 * widen rs2, call vector intrinsic, extract lane 0. */
#define __packed_pmulh_parts_acc_rv64_scalar(name, r_ty, vec_r_ty,             \
                                             a_ty, vec_a_ty,                   \
                                             b_ty, b_wide_ty, vec_name)        \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    vec_r_ty __rdw = {__rd, 0};                                                \
    vec_a_ty __aw = {__rs1, 0};                                                \
    b_wide_ty __bw = {__rs2[0], __rs2[1], 0, 0};                               \
    vec_r_ty __r = __builtin_riscv_##vec_name(__rdw, __aw, __bw);              \
    return __r[0];                                                             \
  }

/* Direct-builtin packed multiply-high-parts-accumulate wrapper (3 ops). */
#define __packed_pmulh_parts_acc(name, r_ty, a_ty, b_ty)                       \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }

/* Packed rounding multiply-high accumulate. */
#define __packed_pmhracc(name, r_ty, a_ty, b_ty, prod_ty, SHIFT)               \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    prod_ty __p = __builtin_convertvector(__rs1, prod_ty) *                    \
                  __builtin_convertvector(__rs2, prod_ty);                     \
    return __rd + __builtin_convertvector(                                     \
                      (__p + (1LL << ((SHIFT) - 1))) >> (SHIFT), r_ty);        \
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

/* Packed Exchanged Addition and Subtraction (halfword, 32-bit, RV32 and RV64) */
__packed_exchange_add_sub(pas_x_i16x2,  int16x2_t)
__packed_exchange_add_sub(psa_x_i16x2,  int16x2_t)
__packed_exchange_add_sub(psas_x_i16x2, int16x2_t)
__packed_exchange_add_sub(pssa_x_i16x2, int16x2_t)
__packed_exchange_add_sub(paas_x_i16x2, int16x2_t)
__packed_exchange_add_sub(pasa_x_i16x2, int16x2_t)

/* Packed SH1ADD / SSH1SADD (halfword, 32-bit, RV32 and RV64) */
__packed_exchange_add_sub(psh1add_i16x2,   int16x2_t)
__packed_exchange_add_sub(psh1add_u16x2,   uint16x2_t)
__packed_exchange_add_sub(pssh1sadd_i16x2, int16x2_t)

/* Packed "Q-format" Multiplication (halfword, 32-bit, RV32 and RV64) */
__packed_exchange_add_sub(pmulq_i16x2,  int16x2_t)
__packed_exchange_add_sub(pmulqr_i16x2, int16x2_t)

/* Packed Load and Store (element-aligned). */
#define __packed_pld(name, r_ty, p_ty)                                         \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(p_ty *__p) {                                                  \
    return __builtin_riscv_##name(__p);                                        \
  }
#define __packed_pst(name, v_ty, p_ty)                                         \
  static __inline__ void __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(p_ty *__p, v_ty __v) {                                        \
    __builtin_riscv_##name(__p, __v);                                          \
  }
__packed_pld(pld_i8x4,  int8x4_t,   int8_t)
__packed_pld(pld_u8x4,  uint8x4_t,  uint8_t)
__packed_pld(pld_i16x2, int16x2_t,  int16_t)
__packed_pld(pld_u16x2, uint16x2_t, uint16_t)
__packed_pst(pst_i8x4,  int8x4_t,   int8_t)
__packed_pst(pst_u8x4,  uint8x4_t,  uint8_t)
__packed_pst(pst_i16x2, int16x2_t,  int16_t)
__packed_pst(pst_u16x2, uint16x2_t, uint16_t)

__packed_pld(pld_i8x8,  int8x8_t,   int8_t)
__packed_pld(pld_u8x8,  uint8x8_t,  uint8_t)
__packed_pld(pld_i16x4, int16x4_t,  int16_t)
__packed_pld(pld_u16x4, uint16x4_t, uint16_t)
__packed_pld(pld_i32x2, int32x2_t,  int32_t)
__packed_pld(pld_u32x2, uint32x2_t, uint32_t)
__packed_pst(pst_i8x8,  int8x8_t,   int8_t)
__packed_pst(pst_u8x8,  uint8x8_t,  uint8_t)
__packed_pst(pst_i16x4, int16x4_t,  int16_t)
__packed_pst(pst_u16x4, uint16x4_t, uint16_t)
__packed_pst(pst_i32x2, int32x2_t,  int32_t)
__packed_pst(pst_u32x2, uint32x2_t, uint32_t)

/* Packed Element Insert and Extract. The idx argument must be a compile-time
 * constant, so the wrappers are macros to preserve that property at the
 * builtin call site. */
#define __riscv_pget_i8x4_i8(v, idx)   __builtin_riscv_pget_i8x4_i8((v), (idx))
#define __riscv_pget_u8x4_u8(v, idx)   __builtin_riscv_pget_u8x4_u8((v), (idx))
#define __riscv_pget_i16x2_i16(v, idx) __builtin_riscv_pget_i16x2_i16((v), (idx))
#define __riscv_pget_u16x2_u16(v, idx) __builtin_riscv_pget_u16x2_u16((v), (idx))
#define __riscv_pset_i8_i8x4(v, e, idx)   __builtin_riscv_pset_i8_i8x4((v), (e), (idx))
#define __riscv_pset_u8_u8x4(v, e, idx)   __builtin_riscv_pset_u8_u8x4((v), (e), (idx))
#define __riscv_pset_i16_i16x2(v, e, idx) __builtin_riscv_pset_i16_i16x2((v), (e), (idx))
#define __riscv_pset_u16_u16x2(v, e, idx) __builtin_riscv_pset_u16_u16x2((v), (e), (idx))

#define __riscv_pget_i8x8_i8(v, idx)   __builtin_riscv_pget_i8x8_i8((v), (idx))
#define __riscv_pget_u8x8_u8(v, idx)   __builtin_riscv_pget_u8x8_u8((v), (idx))
#define __riscv_pget_i16x4_i16(v, idx) __builtin_riscv_pget_i16x4_i16((v), (idx))
#define __riscv_pget_u16x4_u16(v, idx) __builtin_riscv_pget_u16x4_u16((v), (idx))
#define __riscv_pget_i32x2_i32(v, idx) __builtin_riscv_pget_i32x2_i32((v), (idx))
#define __riscv_pget_u32x2_u32(v, idx) __builtin_riscv_pget_u32x2_u32((v), (idx))
#define __riscv_pset_i8_i8x8(v, e, idx)   __builtin_riscv_pset_i8_i8x8((v), (e), (idx))
#define __riscv_pset_u8_u8x8(v, e, idx)   __builtin_riscv_pset_u8_u8x8((v), (e), (idx))
#define __riscv_pset_i16_i16x4(v, e, idx) __builtin_riscv_pset_i16_i16x4((v), (e), (idx))
#define __riscv_pset_u16_u16x4(v, e, idx) __builtin_riscv_pset_u16_u16x4((v), (e), (idx))
#define __riscv_pset_i32_i32x2(v, e, idx) __builtin_riscv_pset_i32_i32x2((v), (e), (idx))
#define __riscv_pset_u32_u32x2(v, e, idx) __builtin_riscv_pset_u32_u32x2((v), (e), (idx))

/* Packed Subvector Insert and Extract. idx is 0 (low) or 1 (high). */
#define __riscv_pget_i8x8_i8x4(v, idx)     __builtin_riscv_pget_i8x8_i8x4((v), (idx))
#define __riscv_pget_u8x8_u8x4(v, idx)     __builtin_riscv_pget_u8x8_u8x4((v), (idx))
#define __riscv_pget_i16x4_i16x2(v, idx)   __builtin_riscv_pget_i16x4_i16x2((v), (idx))
#define __riscv_pget_u16x4_u16x2(v, idx)   __builtin_riscv_pget_u16x4_u16x2((v), (idx))
#define __riscv_pset_i8x4_i8x8(v, s, idx)   __builtin_riscv_pset_i8x4_i8x8((v), (s), (idx))
#define __riscv_pset_u8x4_u8x8(v, s, idx)   __builtin_riscv_pset_u8x4_u8x8((v), (s), (idx))
#define __riscv_pset_i16x2_i16x4(v, s, idx) __builtin_riscv_pset_i16x2_i16x4((v), (s), (idx))
#define __riscv_pset_u16x2_u16x4(v, s, idx) __builtin_riscv_pset_u16x2_u16x4((v), (s), (idx))

/* Packed Subvector Join (two 32-bit subvectors -> 64-bit packed vector). */
#define __packed_psubv_join(name, r_ty, s_ty)                                  \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(s_ty __lo, s_ty __hi) {                                       \
    return __builtin_riscv_##name(__lo, __hi);                                 \
  }
__packed_psubv_join(pjoin2_i8x8,  int8x8_t,   int8x4_t)
__packed_psubv_join(pjoin2_u8x8,  uint8x8_t,  uint8x4_t)
__packed_psubv_join(pjoin2_i16x4, int16x4_t,  int16x2_t)
__packed_psubv_join(pjoin2_u16x4, uint16x4_t, uint16x2_t)
#undef __packed_psubv_join

/* Slide 1 up/down. */
#define __packed_pslide1(name, v_ty, e_ty)                                     \
  static __inline__ v_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(v_ty __rd, e_ty __rs1) {                                      \
    return __builtin_riscv_##name(__rd, __rs1);                                \
  }
__packed_pslide1(pslide1up_i8x4,    int8x4_t,   int8_t)
__packed_pslide1(pslide1up_u8x4,    uint8x4_t,  uint8_t)
__packed_pslide1(pslide1up_i16x2,   int16x2_t,  int16_t)
__packed_pslide1(pslide1up_u16x2,   uint16x2_t, uint16_t)
__packed_pslide1(pslide1down_i8x4,  int8x4_t,   int8_t)
__packed_pslide1(pslide1down_u8x4,  uint8x4_t,  uint8_t)
__packed_pslide1(pslide1down_i16x2, int16x2_t,  int16_t)
__packed_pslide1(pslide1down_u16x2, uint16x2_t, uint16_t)

__packed_pslide1(pslide1up_i8x8,    int8x8_t,   int8_t)
__packed_pslide1(pslide1up_u8x8,    uint8x8_t,  uint8_t)
__packed_pslide1(pslide1up_i16x4,   int16x4_t,  int16_t)
__packed_pslide1(pslide1up_u16x4,   uint16x4_t, uint16_t)
__packed_pslide1(pslide1up_i32x2,   int32x2_t,  int32_t)
__packed_pslide1(pslide1up_u32x2,   uint32x2_t, uint32_t)
__packed_pslide1(pslide1down_i8x8,  int8x8_t,   int8_t)
__packed_pslide1(pslide1down_u8x8,  uint8x8_t,  uint8_t)
__packed_pslide1(pslide1down_i16x4, int16x4_t,  int16_t)
__packed_pslide1(pslide1down_u16x4, uint16x4_t, uint16_t)
__packed_pslide1(pslide1down_i32x2, int32x2_t,  int32_t)
__packed_pslide1(pslide1down_u32x2, uint32x2_t, uint32_t)
#undef __packed_pslide1

/* Slide by variable element count. */
#define __packed_pslidex(name, v_ty)                                           \
  static __inline__ v_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(v_ty __rd, v_ty __rs1, unsigned __rs2) {                      \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }
__packed_pslidex(pslideupx_i8x4,     int8x4_t)
__packed_pslidex(pslideupx_u8x4,     uint8x4_t)
__packed_pslidex(pslideupx_i16x2,    int16x2_t)
__packed_pslidex(pslideupx_u16x2,    uint16x2_t)
__packed_pslidex(pslidedownx_i8x4,   int8x4_t)
__packed_pslidex(pslidedownx_u8x4,   uint8x4_t)
__packed_pslidex(pslidedownx_i16x2,  int16x2_t)
__packed_pslidex(pslidedownx_u16x2,  uint16x2_t)

__packed_pslidex(pslideupx_i8x8,     int8x8_t)
__packed_pslidex(pslideupx_u8x8,     uint8x8_t)
__packed_pslidex(pslideupx_i16x4,    int16x4_t)
__packed_pslidex(pslideupx_u16x4,    uint16x4_t)
__packed_pslidex(pslideupx_i32x2,    int32x2_t)
__packed_pslidex(pslideupx_u32x2,    uint32x2_t)
__packed_pslidex(pslidedownx_i8x8,   int8x8_t)
__packed_pslidex(pslidedownx_u8x8,   uint8x8_t)
__packed_pslidex(pslidedownx_i16x4,  int16x4_t)
__packed_pslidex(pslidedownx_u16x4,  uint16x4_t)
__packed_pslidex(pslidedownx_i32x2,  int32x2_t)
__packed_pslidex(pslidedownx_u32x2,  uint32x2_t)
#undef __packed_pslidex

/* Packed Element Join. */
#define __packed_pjoin4(name, r_ty, e_ty)                                      \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(e_ty __e0, e_ty __e1, e_ty __e2, e_ty __e3) {                 \
    return __builtin_riscv_##name(__e0, __e1, __e2, __e3);                     \
  }
#define __packed_pjoin2(name, r_ty, e_ty)                                      \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(e_ty __e0, e_ty __e1) {                                       \
    return __builtin_riscv_##name(__e0, __e1);                                 \
  }
__packed_pjoin4(pjoin4_i8x4,  int8x4_t,   int8_t)
__packed_pjoin4(pjoin4_u8x4,  uint8x4_t,  uint8_t)
__packed_pjoin2(pjoin2_i16x2, int16x2_t,  int16_t)
__packed_pjoin2(pjoin2_u16x2, uint16x2_t, uint16_t)

__packed_pjoin4(pjoin4_i16x4, int16x4_t,  int16_t)
__packed_pjoin4(pjoin4_u16x4, uint16x4_t, uint16_t)
__packed_pjoin2(pjoin2_i32x2, int32x2_t,  int32_t)
__packed_pjoin2(pjoin2_u32x2, uint32x2_t, uint32_t)
#undef __packed_pjoin4
#undef __packed_pjoin2

#if __riscv_xlen == 32
/* Packed Multiply Parts (RV32 form, direct builtin) */
__packed_pm_horiz_binary(pmul_b00_i16x2,    int16x2_t,  int8x4_t,  int8x4_t)
__packed_pm_horiz_binary(pmul_b01_i16x2,    int16x2_t,  int8x4_t,  int8x4_t)
__packed_pm_horiz_binary(pmul_b11_i16x2,    int16x2_t,  int8x4_t,  int8x4_t)
__packed_pm_horiz_binary(pmulu_b00_u16x2,   uint16x2_t, uint8x4_t, uint8x4_t)
__packed_pm_horiz_binary(pmulu_b01_u16x2,   uint16x2_t, uint8x4_t, uint8x4_t)
__packed_pm_horiz_binary(pmulu_b11_u16x2,   uint16x2_t, uint8x4_t, uint8x4_t)
__packed_pm_horiz_binary(pmulsu_b00_i16x2,  int16x2_t,  int8x4_t,  uint8x4_t)
__packed_pm_horiz_binary(pmulsu_b11_i16x2,  int16x2_t,  int8x4_t,  uint8x4_t)

__packed_pm_horiz_binary(mul_h00_i32,    int32_t,  int16x2_t,  int16x2_t)
__packed_pm_horiz_binary(mul_h01_i32,    int32_t,  int16x2_t,  int16x2_t)
__packed_pm_horiz_binary(mul_h11_i32,    int32_t,  int16x2_t,  int16x2_t)
__packed_pm_horiz_binary(mulu_h00_u32,   uint32_t, uint16x2_t, uint16x2_t)
__packed_pm_horiz_binary(mulu_h01_u32,   uint32_t, uint16x2_t, uint16x2_t)
__packed_pm_horiz_binary(mulu_h11_u32,   uint32_t, uint16x2_t, uint16x2_t)
__packed_pm_horiz_binary(mulsu_h00_i32,  int32_t,  int16x2_t,  uint16x2_t)
__packed_pm_horiz_binary(mulsu_h11_i32,  int32_t,  int16x2_t,  uint16x2_t)

/* Packed Multiply Parts Accumulate (RV32 scalar form, direct builtin) */
__packed_pmul_parts_acc(macc_h00_i32,   int32_t,  int16x2_t,  int16x2_t)
__packed_pmul_parts_acc(macc_h01_i32,   int32_t,  int16x2_t,  int16x2_t)
__packed_pmul_parts_acc(macc_h11_i32,   int32_t,  int16x2_t,  int16x2_t)
__packed_pmul_parts_acc(maccu_h00_u32,  uint32_t, uint16x2_t, uint16x2_t)
__packed_pmul_parts_acc(maccu_h01_u32,  uint32_t, uint16x2_t, uint16x2_t)
__packed_pmul_parts_acc(maccu_h11_u32,  uint32_t, uint16x2_t, uint16x2_t)
__packed_pmul_parts_acc(maccsu_h00_i32, int32_t,  int16x2_t,  uint16x2_t)
__packed_pmul_parts_acc(maccsu_h11_i32, int32_t,  int16x2_t,  uint16x2_t)

/* Packed "Q-format" Multiply Parts Accumulate (RV32 scalar form). */
__packed_pmul_parts_acc(mqacc_h00_i32,  int32_t, int16x2_t, int16x2_t)
__packed_pmul_parts_acc(mqacc_h01_i32,  int32_t, int16x2_t, int16x2_t)
__packed_pmul_parts_acc(mqacc_h11_i32,  int32_t, int16x2_t, int16x2_t)
__packed_pmul_parts_acc(mqracc_h00_i32, int32_t, int16x2_t, int16x2_t)
__packed_pmul_parts_acc(mqracc_h01_i32, int32_t, int16x2_t, int16x2_t)
__packed_pmul_parts_acc(mqracc_h11_i32, int32_t, int16x2_t, int16x2_t)

/* Packed Multiply High Parts: byte-lane subset (RV32 form, direct builtin) */
__packed_pm_horiz_binary(pmulh_b0_i16x2,    int16x2_t, int16x2_t, int8x4_t)
__packed_pm_horiz_binary(pmulh_b1_i16x2,    int16x2_t, int16x2_t, int8x4_t)
__packed_pm_horiz_binary(pmulhsu_b0_i16x2,  int16x2_t, int16x2_t, uint8x4_t)
__packed_pm_horiz_binary(pmulhsu_b1_i16x2,  int16x2_t, int16x2_t, uint8x4_t)

/* Packed Multiply High Parts: halfword-lane scalar form (RV32 only). */
__packed_pm_horiz_binary(mulh_h0_i32,    int32_t, int32_t,  int16x2_t)
__packed_pm_horiz_binary(mulh_h1_i32,    int32_t, int32_t,  int16x2_t)
__packed_pm_horiz_binary(mulhsu_h0_i32,  int32_t, int32_t,  uint16x2_t)
__packed_pm_horiz_binary(mulhsu_h1_i32,  int32_t, int32_t,  uint16x2_t)

/* Packed Multiply High Parts Accumulate: byte-lane subset (RV32 form). */
__packed_pmulh_parts_acc(pmhacc_b0_i16x2,    int16x2_t, int16x2_t, int8x4_t)
__packed_pmulh_parts_acc(pmhacc_b1_i16x2,    int16x2_t, int16x2_t, int8x4_t)
__packed_pmulh_parts_acc(pmhaccsu_b0_i16x2,  int16x2_t, int16x2_t, uint8x4_t)
__packed_pmulh_parts_acc(pmhaccsu_b1_i16x2,  int16x2_t, int16x2_t, uint8x4_t)

/* Packed Multiply High Parts Accumulate: halfword-lane scalar form
 * (RV32 only). */
__packed_pmulh_parts_acc(mhacc_h0_i32,    int32_t, int32_t, int16x2_t)
__packed_pmulh_parts_acc(mhacc_h1_i32,    int32_t, int32_t, int16x2_t)
__packed_pmulh_parts_acc(mhaccsu_h0_i32,  int32_t, int32_t, uint16x2_t)
__packed_pmulh_parts_acc(mhaccsu_h1_i32,  int32_t, int32_t, uint16x2_t)

/* Packed Widening Addition and Subtraction (RV32 only). The builtin returns
 * the natural vector type; the underlying IR intrinsic uses i64 which clang
 * CodeGen bitcasts to the vector. */
#define __packed_pwadd_sub(name, r_ty, a_ty, b_ty)                             \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    return __builtin_riscv_##name(__rs1, __rs2);                               \
  }
__packed_pwadd_sub(pwadd_i16x4,   int16x4_t,  int8x4_t,  int8x4_t)
__packed_pwadd_sub(pwadd_i32x2,   int32x2_t,  int16x2_t, int16x2_t)
__packed_pwadd_sub(pwaddu_u16x4,  uint16x4_t, uint8x4_t, uint8x4_t)
__packed_pwadd_sub(pwaddu_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)
__packed_pwadd_sub(pwsub_i16x4,   int16x4_t,  int8x4_t,  int8x4_t)
__packed_pwadd_sub(pwsub_i32x2,   int32x2_t,  int16x2_t, int16x2_t)
__packed_pwadd_sub(pwsubu_u16x4,  uint16x4_t, uint8x4_t, uint8x4_t)
__packed_pwadd_sub(pwsubu_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)

/* Packed Widening Add/Sub Accumulate (RV32 only). */
#define __packed_pwadd_sub_acc(name, r_ty, a_ty, b_ty)                         \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }
__packed_pwadd_sub_acc(pwadda_i16x4,   int16x4_t,  int8x4_t,  int8x4_t)
__packed_pwadd_sub_acc(pwadda_i32x2,   int32x2_t,  int16x2_t, int16x2_t)
__packed_pwadd_sub_acc(pwaddau_u16x4,  uint16x4_t, uint8x4_t, uint8x4_t)
__packed_pwadd_sub_acc(pwaddau_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)
__packed_pwadd_sub_acc(pwsuba_i16x4,   int16x4_t,  int8x4_t,  int8x4_t)
__packed_pwadd_sub_acc(pwsuba_i32x2,   int32x2_t,  int16x2_t, int16x2_t)
__packed_pwadd_sub_acc(pwsubau_u16x4,  uint16x4_t, uint8x4_t, uint8x4_t)
__packed_pwadd_sub_acc(pwsubau_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)

/* Packed Widening Multiply (RV32 only). */
__packed_pwadd_sub(pwmul_i16x4,   int16x4_t,  int8x4_t,  int8x4_t)
__packed_pwadd_sub(pwmul_i32x2,   int32x2_t,  int16x2_t, int16x2_t)
__packed_pwadd_sub(pwmulu_u16x4,  uint16x4_t, uint8x4_t, uint8x4_t)
__packed_pwadd_sub(pwmulu_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)
__packed_pwadd_sub(pwmulsu_i16x4, int16x4_t,  int8x4_t,  uint8x4_t)
__packed_pwadd_sub(pwmulsu_i32x2, int32x2_t,  int16x2_t, uint16x2_t)

/* Packed Widening Multiply Accumulate (RV32 only). */
__packed_pwadd_sub_acc(pwmacc_i32x2,   int32x2_t,  int16x2_t,  int16x2_t)
__packed_pwadd_sub_acc(pwmaccu_u32x2,  uint32x2_t, uint16x2_t, uint16x2_t)
__packed_pwadd_sub_acc(pwmaccsu_i32x2, int32x2_t,  int16x2_t,  uint16x2_t)

/* Packed Q-format Multiply with Widening Accumulate (RV32 only). */
__packed_pwadd_sub_acc(pmqwacc_i32x2,  int32x2_t, int16x2_t, int16x2_t)
__packed_pwadd_sub_acc(pmqrwacc_i32x2, int32x2_t, int16x2_t, int16x2_t)

/* Packed Widening / Narrowing Shift (RV32 only). */
#define __packed_pwshift(name, r_ty, a_ty)                                     \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, unsigned __shamt) {                               \
    return __builtin_riscv_##name(__rs1, __shamt);                             \
  }
__packed_pwshift(pwsll_s_u16x4, uint16x4_t, uint8x4_t)
__packed_pwshift(pwsll_s_u32x2, uint32x2_t, uint16x2_t)
__packed_pwshift(pwsla_s_i16x4, int16x4_t,  int8x4_t)
__packed_pwshift(pwsla_s_i32x2, int32x2_t,  int16x2_t)

__packed_pwshift(pnsrl_s_u8x4,   uint8x4_t,  uint16x4_t)
__packed_pwshift(pnsrl_s_u16x2,  uint16x2_t, uint32x2_t)
__packed_pwshift(pnsra_s_i8x4,   int8x4_t,   int16x4_t)
__packed_pwshift(pnsra_s_i16x2,  int16x2_t,  int32x2_t)
__packed_pwshift(pnsrar_s_i8x4,  int8x4_t,   int16x4_t)
__packed_pwshift(pnsrar_s_i16x2, int16x2_t,  int32x2_t)

__packed_pwshift(pnclipu_s_u8x4,    uint8x4_t,  uint16x4_t)
__packed_pwshift(pnclipu_s_u16x2,   uint16x2_t, uint32x2_t)
__packed_pwshift(pnclipru_s_u8x4,   uint8x4_t,  uint16x4_t)
__packed_pwshift(pnclipru_s_u16x2,  uint16x2_t, uint32x2_t)
__packed_pwshift(pnclip_s_i8x4,     int8x4_t,   int16x4_t)
__packed_pwshift(pnclip_s_i16x2,    int16x2_t,  int32x2_t)
__packed_pwshift(pnclipr_s_i8x4,    int8x4_t,   int16x4_t)
__packed_pwshift(pnclipr_s_i16x2,   int16x2_t,  int32x2_t)

/* Packed Multiplication with Widening Horizontal Addition (RV32 only). */
#define __packed_pm2w(name, r_ty, a_ty, b_ty)                                  \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1, b_ty __rs2) {                                     \
    return __builtin_riscv_##name(__rs1, __rs2);                               \
  }
#define __packed_pm2wa(name, r_ty, a_ty, b_ty)                                 \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(r_ty __rd, a_ty __rs1, b_ty __rs2) {                          \
    return __builtin_riscv_##name(__rd, __rs1, __rs2);                         \
  }
__packed_pm2w (pm2wadd_i64,     int64_t,  int16x2_t,  int16x2_t)
__packed_pm2wa(pm2wadda_i64,    int64_t,  int16x2_t,  int16x2_t)
__packed_pm2w (pm2wadd_x_i64,   int64_t,  int16x2_t,  int16x2_t)
__packed_pm2wa(pm2wadda_x_i64,  int64_t,  int16x2_t,  int16x2_t)
__packed_pm2w (pm2waddu_u64,    uint64_t, uint16x2_t, uint16x2_t)
__packed_pm2wa(pm2waddau_u64,   uint64_t, uint16x2_t, uint16x2_t)
__packed_pm2w (pm2wsub_i64,     int64_t,  int16x2_t,  int16x2_t)
__packed_pm2wa(pm2wsuba_i64,    int64_t,  int16x2_t,  int16x2_t)
__packed_pm2w (pm2wsub_x_i64,   int64_t,  int16x2_t,  int16x2_t)
__packed_pm2wa(pm2wsuba_x_i64,  int64_t,  int16x2_t,  int16x2_t)
__packed_pm2w (pm2waddsu_u64,   int64_t,  int16x2_t,  uint16x2_t)
__packed_pm2wa(pm2waddasu_u64,  int64_t,  int16x2_t,  uint16x2_t)

/* Packed Widening Convert (RV32 only). */
#define __packed_pwcvt(name, r_ty, a_ty)                                       \
  static __inline__ r_ty __DEFAULT_FN_ATTRS                                    \
  __riscv_##name(a_ty __rs1) {                                                 \
    return __builtin_riscv_##name(__rs1);                                      \
  }
__packed_pwcvt(pwcvt_i16x4,  int16x4_t,  int8x4_t)
__packed_pwcvt(pwcvt_i32x2,  int32x2_t,  int16x2_t)
__packed_pwcvt(pwcvtu_u16x4, uint16x4_t, uint8x4_t)
__packed_pwcvt(pwcvtu_u32x2, uint32x2_t, uint16x2_t)
__packed_pwcvt(pwcvth_i16x4, int16x4_t,  int8x4_t)
__packed_pwcvt(pwcvth_u16x4, uint16x4_t, uint8x4_t)
__packed_pwcvt(pwcvth_i32x2, int32x2_t,  int16x2_t)
__packed_pwcvt(pwcvth_u32x2, uint32x2_t, uint16x2_t)

/* Packed Narrowing Convert (RV32 only). */
__packed_pwcvt(pncvt_i8x4,   int8x4_t,   int16x4_t)
__packed_pwcvt(pncvt_u8x4,   uint8x4_t,  uint16x4_t)
__packed_pwcvt(pncvt_i16x2,  int16x2_t,  int32x2_t)
__packed_pwcvt(pncvt_u16x2,  uint16x2_t, uint32x2_t)
__packed_pwcvt(pncvth_i8x4,  int8x4_t,   int16x4_t)
__packed_pwcvt(pncvth_u8x4,  uint8x4_t,  uint16x4_t)
__packed_pwcvt(pncvth_i16x2, int16x2_t,  int32x2_t)
__packed_pwcvt(pncvth_u16x2, uint16x2_t, uint32x2_t)

/* Packed Zip (RV32 only). */
__packed_pwadd_sub(pzip_i8x8,  int8x8_t,   int8x4_t,   int8x4_t)
__packed_pwadd_sub(pzip_u8x8,  uint8x8_t,  uint8x4_t,  uint8x4_t)
__packed_pwadd_sub(pzip_i16x4, int16x4_t,  int16x2_t,  int16x2_t)
__packed_pwadd_sub(pzip_u16x4, uint16x4_t, uint16x2_t, uint16x2_t)

/* Packed Narrowing Zip (RV32 only). */
__packed_pwadd_sub(pnzip_i8x4,  int8x4_t,  int16x2_t,  int16x2_t)
__packed_pwadd_sub(pnzip_u8x4,  uint8x4_t, uint16x2_t, uint16x2_t)
__packed_pwadd_sub(pnziph_i8x4, int8x4_t,  int16x2_t,  int16x2_t)
__packed_pwadd_sub(pnziph_u8x4, uint8x4_t, uint16x2_t, uint16x2_t)

/* Packed Widening Unzip (RV32 only). */
__packed_pwcvt(pwunzipe_i16x2,  int16x2_t,  int8x4_t)
__packed_pwcvt(pwunzipo_i16x2,  int16x2_t,  int8x4_t)
__packed_pwcvt(pwunzipue_u16x2, uint16x2_t, uint8x4_t)
__packed_pwcvt(pwunzipuo_u16x2, uint16x2_t, uint8x4_t)
__packed_pwcvt(pwunziphe_i16x2, int16x2_t,  int8x4_t)
__packed_pwcvt(pwunzipho_i16x2, int16x2_t,  int8x4_t)
__packed_pwcvt(pwunziphe_u16x2, uint16x2_t, uint8x4_t)
__packed_pwcvt(pwunzipho_u16x2, uint16x2_t, uint8x4_t)

/* Packed Unzip (RV32 only). */
__packed_pwcvt(punzipe_i8x4,  int8x4_t,   int8x8_t)
__packed_pwcvt(punzipo_i8x4,  int8x4_t,   int8x8_t)
__packed_pwcvt(punzipe_u8x4,  uint8x4_t,  uint8x8_t)
__packed_pwcvt(punzipo_u8x4,  uint8x4_t,  uint8x8_t)
__packed_pwcvt(punzipe_i16x2, int16x2_t,  int16x4_t)
__packed_pwcvt(punzipo_i16x2, int16x2_t,  int16x4_t)
__packed_pwcvt(punzipe_u16x2, uint16x2_t, uint16x4_t)
__packed_pwcvt(punzipo_u16x2, uint16x2_t, uint16x4_t)
#endif

/* Packed Multiply High (halfword, 32-bit, RV32 and RV64).
 * Intermediate product is a 32-bit-element-wide vector with the same lane
 * count as the input. */
__packed_pmulh (pmulh_i16x2,    int16x2_t,  int16x2_t,  int16x2_t,  int32x2_t,  16)
__packed_pmulhr(pmulhr_i16x2,   int16x2_t,  int16x2_t,  int16x2_t,  int32x2_t,  16)
__packed_pmulh (pmulhu_u16x2,   uint16x2_t, uint16x2_t, uint16x2_t, uint32x2_t, 16)
__packed_pmulhr(pmulhru_u16x2,  uint16x2_t, uint16x2_t, uint16x2_t, uint32x2_t, 16)
__packed_pmulh (pmulhsu_i16x2,  int16x2_t,  int16x2_t,  uint16x2_t, int32x2_t,  16)
__packed_pmulhr(pmulhrsu_i16x2, int16x2_t,  int16x2_t,  uint16x2_t, int32x2_t,  16)

/* Packed Multiply High Accumulate (halfword, 32-bit, RV32 and RV64) */
__packed_pmhacc (pmhacc_i16x2,    int16x2_t,  int16x2_t,  int16x2_t,  int32x2_t,  16)
__packed_pmhracc(pmhracc_i16x2,   int16x2_t,  int16x2_t,  int16x2_t,  int32x2_t,  16)
__packed_pmhacc (pmhaccu_u16x2,   uint16x2_t, uint16x2_t, uint16x2_t, uint32x2_t, 16)
__packed_pmhracc(pmhraccu_u16x2,  uint16x2_t, uint16x2_t, uint16x2_t, uint32x2_t, 16)
__packed_pmhacc (pmhaccsu_i16x2,  int16x2_t,  int16x2_t,  uint16x2_t, int32x2_t,  16)
__packed_pmhracc(pmhraccsu_i16x2, int16x2_t,  int16x2_t,  uint16x2_t, int32x2_t,  16)

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

/* Packed Exchanged Addition and Subtraction (64-bit, RV64 only) */
__packed_exchange_add_sub(pas_x_i16x4,  int16x4_t)
__packed_exchange_add_sub(psa_x_i16x4,  int16x4_t)
__packed_exchange_add_sub(psas_x_i16x4, int16x4_t)
__packed_exchange_add_sub(pssa_x_i16x4, int16x4_t)
__packed_exchange_add_sub(paas_x_i16x4, int16x4_t)
__packed_exchange_add_sub(pasa_x_i16x4, int16x4_t)

__packed_exchange_add_sub(pas_x_i32x2,  int32x2_t)
__packed_exchange_add_sub(psa_x_i32x2,  int32x2_t)
__packed_exchange_add_sub(psas_x_i32x2, int32x2_t)
__packed_exchange_add_sub(pssa_x_i32x2, int32x2_t)
__packed_exchange_add_sub(paas_x_i32x2, int32x2_t)
__packed_exchange_add_sub(pasa_x_i32x2, int32x2_t)

/* Packed SH1ADD / SSH1SADD (64-bit, RV64 only) */
__packed_exchange_add_sub(psh1add_i16x4,   int16x4_t)
__packed_exchange_add_sub(psh1add_u16x4,   uint16x4_t)
__packed_exchange_add_sub(psh1add_i32x2,   int32x2_t)
__packed_exchange_add_sub(psh1add_u32x2,   uint32x2_t)
__packed_exchange_add_sub(pssh1sadd_i16x4, int16x4_t)
__packed_exchange_add_sub(pssh1sadd_i32x2, int32x2_t)

/* Packed "Q-format" Multiplication (64-bit, RV64 only) */
__packed_exchange_add_sub(pmulq_i16x4,  int16x4_t)
__packed_exchange_add_sub(pmulq_i32x2,  int32x2_t)
__packed_exchange_add_sub(pmulqr_i16x4, int16x4_t)
__packed_exchange_add_sub(pmulqr_i32x2, int32x2_t)

/* Wider intermediate vector types used by Packed Multiply High wrappers
 * below. Prefixed with __riscv_ to avoid leaking into the user namespace. */
typedef int32_t  __riscv_int32x4_t  __attribute__((__vector_size__(16), __aligned__(16)));
typedef uint32_t __riscv_uint32x4_t __attribute__((__vector_size__(16), __aligned__(16)));
typedef int64_t  __riscv_int64x2_t  __attribute__((__vector_size__(16), __aligned__(16)));
typedef uint64_t __riscv_uint64x2_t __attribute__((__vector_size__(16), __aligned__(16)));

/* Packed Multiply High (halfword, 64-bit, RV64 only) */
__packed_pmulh (pmulh_i16x4,    int16x4_t,  int16x4_t,  int16x4_t,  __riscv_int32x4_t,  16)
__packed_pmulhr(pmulhr_i16x4,   int16x4_t,  int16x4_t,  int16x4_t,  __riscv_int32x4_t,  16)
__packed_pmulh (pmulhu_u16x4,   uint16x4_t, uint16x4_t, uint16x4_t, __riscv_uint32x4_t, 16)
__packed_pmulhr(pmulhru_u16x4,  uint16x4_t, uint16x4_t, uint16x4_t, __riscv_uint32x4_t, 16)
__packed_pmulh (pmulhsu_i16x4,  int16x4_t,  int16x4_t,  uint16x4_t, __riscv_int32x4_t,  16)
__packed_pmulhr(pmulhrsu_i16x4, int16x4_t,  int16x4_t,  uint16x4_t, __riscv_int32x4_t,  16)

/* Packed Multiply High (word, 64-bit, RV64 only) */
__packed_pmulh (pmulh_i32x2,    int32x2_t,  int32x2_t,  int32x2_t,  __riscv_int64x2_t,  32)
__packed_pmulhr(pmulhr_i32x2,   int32x2_t,  int32x2_t,  int32x2_t,  __riscv_int64x2_t,  32)
__packed_pmulh (pmulhu_u32x2,   uint32x2_t, uint32x2_t, uint32x2_t, __riscv_uint64x2_t, 32)
__packed_pmulhr(pmulhru_u32x2,  uint32x2_t, uint32x2_t, uint32x2_t, __riscv_uint64x2_t, 32)
__packed_pmulh (pmulhsu_i32x2,  int32x2_t,  int32x2_t,  uint32x2_t, __riscv_int64x2_t,  32)
__packed_pmulhr(pmulhrsu_i32x2, int32x2_t,  int32x2_t,  uint32x2_t, __riscv_int64x2_t,  32)

/* Packed Multiply High Accumulate (halfword, 64-bit, RV64 only) */
__packed_pmhacc (pmhacc_i16x4,    int16x4_t,  int16x4_t,  int16x4_t,  __riscv_int32x4_t,  16)
__packed_pmhracc(pmhracc_i16x4,   int16x4_t,  int16x4_t,  int16x4_t,  __riscv_int32x4_t,  16)
__packed_pmhacc (pmhaccu_u16x4,   uint16x4_t, uint16x4_t, uint16x4_t, __riscv_uint32x4_t, 16)
__packed_pmhracc(pmhraccu_u16x4,  uint16x4_t, uint16x4_t, uint16x4_t, __riscv_uint32x4_t, 16)
__packed_pmhacc (pmhaccsu_i16x4,  int16x4_t,  int16x4_t,  uint16x4_t, __riscv_int32x4_t,  16)
__packed_pmhracc(pmhraccsu_i16x4, int16x4_t,  int16x4_t,  uint16x4_t, __riscv_int32x4_t,  16)

/* Packed Multiply High Accumulate (word, 64-bit, RV64 only) */
__packed_pmhacc (pmhacc_i32x2,    int32x2_t,  int32x2_t,  int32x2_t,  __riscv_int64x2_t,  32)
__packed_pmhracc(pmhracc_i32x2,   int32x2_t,  int32x2_t,  int32x2_t,  __riscv_int64x2_t,  32)
__packed_pmhacc (pmhaccu_u32x2,   uint32x2_t, uint32x2_t, uint32x2_t, __riscv_uint64x2_t, 32)
__packed_pmhracc(pmhraccu_u32x2,  uint32x2_t, uint32x2_t, uint32x2_t, __riscv_uint64x2_t, 32)
__packed_pmhacc (pmhaccsu_i32x2,  int32x2_t,  int32x2_t,  uint32x2_t, __riscv_int64x2_t,  32)
__packed_pmhracc(pmhraccsu_i32x2, int32x2_t,  int32x2_t,  uint32x2_t, __riscv_int64x2_t,  32)

/* Packed Multiply Parts: byte-pair (RV64 64-bit form, direct builtin) */
__packed_pm_horiz_binary(pmul_b00_i16x4,    int16x4_t,  int8x8_t,  int8x8_t)
__packed_pm_horiz_binary(pmul_b01_i16x4,    int16x4_t,  int8x8_t,  int8x8_t)
__packed_pm_horiz_binary(pmul_b11_i16x4,    int16x4_t,  int8x8_t,  int8x8_t)
__packed_pm_horiz_binary(pmulu_b00_u16x4,   uint16x4_t, uint8x8_t, uint8x8_t)
__packed_pm_horiz_binary(pmulu_b01_u16x4,   uint16x4_t, uint8x8_t, uint8x8_t)
__packed_pm_horiz_binary(pmulu_b11_u16x4,   uint16x4_t, uint8x8_t, uint8x8_t)
__packed_pm_horiz_binary(pmulsu_b00_i16x4,  int16x4_t,  int8x8_t,  uint8x8_t)
__packed_pm_horiz_binary(pmulsu_b11_i16x4,  int16x4_t,  int8x8_t,  uint8x8_t)

/* Packed Multiply Parts: halfword-pair vector form (RV64 only, pmul.w.h*) */
__packed_pm_horiz_binary(pmul_h00_i32x2,    int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pmul_h01_i32x2,    int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pmul_h11_i32x2,    int32x2_t,  int16x4_t,  int16x4_t)
__packed_pm_horiz_binary(pmulu_h00_u32x2,   uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_binary(pmulu_h01_u32x2,   uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_binary(pmulu_h11_u32x2,   uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pm_horiz_binary(pmulsu_h00_i32x2,  int32x2_t,  int16x4_t,  uint16x4_t)
__packed_pm_horiz_binary(pmulsu_h11_i32x2,  int32x2_t,  int16x4_t,  uint16x4_t)

/* Packed Multiply Parts: word-pair scalar form (RV64 only, mul.w*) */
__packed_pm_horiz_binary(mul_w00_i64,    int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_binary(mul_w01_i64,    int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_binary(mul_w11_i64,    int64_t,  int32x2_t,  int32x2_t)
__packed_pm_horiz_binary(mulu_w00_u64,   uint64_t, uint32x2_t, uint32x2_t)
__packed_pm_horiz_binary(mulu_w01_u64,   uint64_t, uint32x2_t, uint32x2_t)
__packed_pm_horiz_binary(mulu_w11_u64,   uint64_t, uint32x2_t, uint32x2_t)
__packed_pm_horiz_binary(mulsu_w00_i64,  int64_t,  int32x2_t,  uint32x2_t)
__packed_pm_horiz_binary(mulsu_w11_i64,  int64_t,  int32x2_t,  uint32x2_t)

/* Cross-XLEN forms exposed on RV64 via widening of the i16x4 / i32x2 form. */
__packed_pmul_parts_rv64_half(pmul_b00_i16x2,    int16x2_t, int16x4_t,
                              int8x4_t, int8x8_t, int8x4_t, int8x8_t,
                              pmul_b00_i16x4)
__packed_pmul_parts_rv64_half(pmul_b01_i16x2,    int16x2_t, int16x4_t,
                              int8x4_t, int8x8_t, int8x4_t, int8x8_t,
                              pmul_b01_i16x4)
__packed_pmul_parts_rv64_half(pmul_b11_i16x2,    int16x2_t, int16x4_t,
                              int8x4_t, int8x8_t, int8x4_t, int8x8_t,
                              pmul_b11_i16x4)
__packed_pmul_parts_rv64_half(pmulu_b00_u16x2,   uint16x2_t, uint16x4_t,
                              uint8x4_t, uint8x8_t, uint8x4_t, uint8x8_t,
                              pmulu_b00_u16x4)
__packed_pmul_parts_rv64_half(pmulu_b01_u16x2,   uint16x2_t, uint16x4_t,
                              uint8x4_t, uint8x8_t, uint8x4_t, uint8x8_t,
                              pmulu_b01_u16x4)
__packed_pmul_parts_rv64_half(pmulu_b11_u16x2,   uint16x2_t, uint16x4_t,
                              uint8x4_t, uint8x8_t, uint8x4_t, uint8x8_t,
                              pmulu_b11_u16x4)
__packed_pmul_parts_rv64_half(pmulsu_b00_i16x2,  int16x2_t, int16x4_t,
                              int8x4_t, int8x8_t, uint8x4_t, uint8x8_t,
                              pmulsu_b00_i16x4)
__packed_pmul_parts_rv64_half(pmulsu_b11_i16x2,  int16x2_t, int16x4_t,
                              int8x4_t, int8x8_t, uint8x4_t, uint8x8_t,
                              pmulsu_b11_i16x4)

__packed_pmul_parts_rv64_scalar(mul_h00_i32,   int32_t,  int32x2_t,
                                int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                pmul_h00_i32x2)
__packed_pmul_parts_rv64_scalar(mul_h01_i32,   int32_t,  int32x2_t,
                                int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                pmul_h01_i32x2)
__packed_pmul_parts_rv64_scalar(mul_h11_i32,   int32_t,  int32x2_t,
                                int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                pmul_h11_i32x2)
__packed_pmul_parts_rv64_scalar(mulu_h00_u32,  uint32_t, uint32x2_t,
                                uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                pmulu_h00_u32x2)
__packed_pmul_parts_rv64_scalar(mulu_h01_u32,  uint32_t, uint32x2_t,
                                uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                pmulu_h01_u32x2)
__packed_pmul_parts_rv64_scalar(mulu_h11_u32,  uint32_t, uint32x2_t,
                                uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                pmulu_h11_u32x2)
__packed_pmul_parts_rv64_scalar(mulsu_h00_i32, int32_t,  int32x2_t,
                                int16x2_t, int16x4_t, uint16x2_t, uint16x4_t,
                                pmulsu_h00_i32x2)
__packed_pmul_parts_rv64_scalar(mulsu_h11_i32, int32_t,  int32x2_t,
                                int16x2_t, int16x4_t, uint16x2_t, uint16x4_t,
                                pmulsu_h11_i32x2)

/* Packed Multiply Parts Accumulate: halfword-pair vector form (RV64 only). */
__packed_pmul_parts_acc(pmacc_h00_i32x2,   int32x2_t,  int16x4_t,  int16x4_t)
__packed_pmul_parts_acc(pmacc_h01_i32x2,   int32x2_t,  int16x4_t,  int16x4_t)
__packed_pmul_parts_acc(pmacc_h11_i32x2,   int32x2_t,  int16x4_t,  int16x4_t)
__packed_pmul_parts_acc(pmaccu_h00_u32x2,  uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pmul_parts_acc(pmaccu_h01_u32x2,  uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pmul_parts_acc(pmaccu_h11_u32x2,  uint32x2_t, uint16x4_t, uint16x4_t)
__packed_pmul_parts_acc(pmaccsu_h00_i32x2, int32x2_t,  int16x4_t,  uint16x4_t)
__packed_pmul_parts_acc(pmaccsu_h11_i32x2, int32x2_t,  int16x4_t,  uint16x4_t)

/* Packed Multiply Parts Accumulate: word-pair scalar form (RV64 only). */
__packed_pmul_parts_acc(macc_w00_i64,   int64_t,  int32x2_t,  int32x2_t)
__packed_pmul_parts_acc(macc_w01_i64,   int64_t,  int32x2_t,  int32x2_t)
__packed_pmul_parts_acc(macc_w11_i64,   int64_t,  int32x2_t,  int32x2_t)
__packed_pmul_parts_acc(maccu_w00_u64,  uint64_t, uint32x2_t, uint32x2_t)
__packed_pmul_parts_acc(maccu_w01_u64,  uint64_t, uint32x2_t, uint32x2_t)
__packed_pmul_parts_acc(maccu_w11_u64,  uint64_t, uint32x2_t, uint32x2_t)
__packed_pmul_parts_acc(maccsu_w00_i64, int64_t,  int32x2_t,  uint32x2_t)
__packed_pmul_parts_acc(maccsu_w11_i64, int64_t,  int32x2_t,  uint32x2_t)

/* Cross-XLEN macc.h_i32 forms on RV64: wrap pmacc.w.h vector form. */
__packed_pmul_parts_acc_rv64_scalar(macc_h00_i32,   int32_t,  int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmacc_h00_i32x2)
__packed_pmul_parts_acc_rv64_scalar(macc_h01_i32,   int32_t,  int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmacc_h01_i32x2)
__packed_pmul_parts_acc_rv64_scalar(macc_h11_i32,   int32_t,  int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmacc_h11_i32x2)
__packed_pmul_parts_acc_rv64_scalar(maccu_h00_u32,  uint32_t, uint32x2_t,
                                    uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                    pmaccu_h00_u32x2)
__packed_pmul_parts_acc_rv64_scalar(maccu_h01_u32,  uint32_t, uint32x2_t,
                                    uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                    pmaccu_h01_u32x2)
__packed_pmul_parts_acc_rv64_scalar(maccu_h11_u32,  uint32_t, uint32x2_t,
                                    uint16x2_t, uint16x4_t, uint16x2_t, uint16x4_t,
                                    pmaccu_h11_u32x2)
__packed_pmul_parts_acc_rv64_scalar(maccsu_h00_i32, int32_t,  int32x2_t,
                                    int16x2_t, int16x4_t, uint16x2_t, uint16x4_t,
                                    pmaccsu_h00_i32x2)
__packed_pmul_parts_acc_rv64_scalar(maccsu_h11_i32, int32_t,  int32x2_t,
                                    int16x2_t, int16x4_t, uint16x2_t, uint16x4_t,
                                    pmaccsu_h11_i32x2)

/* Packed "Q-format" Multiply Parts Accumulate: halfword vector form
 * (RV64 only, pmqacc.w.h* / pmqracc.w.h*). */
__packed_pmul_parts_acc(pmqacc_h00_i32x2,  int32x2_t, int16x4_t, int16x4_t)
__packed_pmul_parts_acc(pmqacc_h01_i32x2,  int32x2_t, int16x4_t, int16x4_t)
__packed_pmul_parts_acc(pmqacc_h11_i32x2,  int32x2_t, int16x4_t, int16x4_t)
__packed_pmul_parts_acc(pmqracc_h00_i32x2, int32x2_t, int16x4_t, int16x4_t)
__packed_pmul_parts_acc(pmqracc_h01_i32x2, int32x2_t, int16x4_t, int16x4_t)
__packed_pmul_parts_acc(pmqracc_h11_i32x2, int32x2_t, int16x4_t, int16x4_t)

/* Packed "Q-format" Multiply Parts Accumulate: word scalar form
 * (RV64 only, mqacc.w* / mqracc.w*). */
__packed_pmul_parts_acc(mqacc_w00_i64,  int64_t, int32x2_t, int32x2_t)
__packed_pmul_parts_acc(mqacc_w01_i64,  int64_t, int32x2_t, int32x2_t)
__packed_pmul_parts_acc(mqacc_w11_i64,  int64_t, int32x2_t, int32x2_t)
__packed_pmul_parts_acc(mqracc_w00_i64, int64_t, int32x2_t, int32x2_t)
__packed_pmul_parts_acc(mqracc_w01_i64, int64_t, int32x2_t, int32x2_t)
__packed_pmul_parts_acc(mqracc_w11_i64, int64_t, int32x2_t, int32x2_t)

/* Cross-XLEN mqacc.h_i32 / mqracc.h_i32 forms on RV64 (wrap pmqacc.w.h). */
__packed_pmul_parts_acc_rv64_scalar(mqacc_h00_i32,  int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqacc_h00_i32x2)
__packed_pmul_parts_acc_rv64_scalar(mqacc_h01_i32,  int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqacc_h01_i32x2)
__packed_pmul_parts_acc_rv64_scalar(mqacc_h11_i32,  int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqacc_h11_i32x2)
__packed_pmul_parts_acc_rv64_scalar(mqracc_h00_i32, int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqracc_h00_i32x2)
__packed_pmul_parts_acc_rv64_scalar(mqracc_h01_i32, int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqracc_h01_i32x2)
__packed_pmul_parts_acc_rv64_scalar(mqracc_h11_i32, int32_t, int32x2_t,
                                    int16x2_t, int16x4_t, int16x2_t, int16x4_t,
                                    pmqracc_h11_i32x2)

/* Packed Multiply High Parts: byte-lane subset (RV64 64-bit form). */
__packed_pm_horiz_binary(pmulh_b0_i16x4,    int16x4_t, int16x4_t, int8x8_t)
__packed_pm_horiz_binary(pmulh_b1_i16x4,    int16x4_t, int16x4_t, int8x8_t)
__packed_pm_horiz_binary(pmulhsu_b0_i16x4,  int16x4_t, int16x4_t, uint8x8_t)
__packed_pm_horiz_binary(pmulhsu_b1_i16x4,  int16x4_t, int16x4_t, uint8x8_t)

/* Packed Multiply High Parts: halfword-lane vector form (RV64 only). */
__packed_pm_horiz_binary(pmulh_h0_i32x2,    int32x2_t, int32x2_t, int16x4_t)
__packed_pm_horiz_binary(pmulh_h1_i32x2,    int32x2_t, int32x2_t, int16x4_t)
__packed_pm_horiz_binary(pmulhsu_h0_i32x2,  int32x2_t, int32x2_t, uint16x4_t)
__packed_pm_horiz_binary(pmulhsu_h1_i32x2,  int32x2_t, int32x2_t, uint16x4_t)

/* Cross-XLEN byte-lane forms on RV64: widen the int16x2_t rs1 to int16x4_t,
 * widen the int8x4_t rs2 to int8x8_t, call the i16x4 builtin, and narrow. */
#define __packed_pmulh_b_rv64_half(name, narrow_v_ty, wide_v_ty,               \
                                   b_narrow_ty, b_wide_ty, wide_name)          \
  static __inline__ narrow_v_ty __DEFAULT_FN_ATTRS                             \
  __riscv_##name(narrow_v_ty __rs1, b_narrow_ty __rs2) {                       \
    wide_v_ty __aw = {__rs1[0], __rs1[1], 0, 0};                               \
    b_wide_ty __bw = {__rs2[0], __rs2[1], __rs2[2], __rs2[3], 0, 0, 0, 0};     \
    wide_v_ty __r = __builtin_riscv_##wide_name(__aw, __bw);                   \
    return (narrow_v_ty){__r[0], __r[1]};                                      \
  }

__packed_pmulh_b_rv64_half(pmulh_b0_i16x2,    int16x2_t, int16x4_t,
                           int8x4_t,  int8x8_t,  pmulh_b0_i16x4)
__packed_pmulh_b_rv64_half(pmulh_b1_i16x2,    int16x2_t, int16x4_t,
                           int8x4_t,  int8x8_t,  pmulh_b1_i16x4)
__packed_pmulh_b_rv64_half(pmulhsu_b0_i16x2,  int16x2_t, int16x4_t,
                           uint8x4_t, uint8x8_t, pmulhsu_b0_i16x4)
__packed_pmulh_b_rv64_half(pmulhsu_b1_i16x2,  int16x2_t, int16x4_t,
                           uint8x4_t, uint8x8_t, pmulhsu_b1_i16x4)

/* Cross-XLEN mulh.h_i32 forms on RV64 (wrap pmulh.w.h vector form). */
__packed_pmulh_parts_rv64_scalar(mulh_h0_i32,    int32_t, int32x2_t,
                                 int32_t, int32x2_t,
                                 int16x2_t, int16x4_t, pmulh_h0_i32x2)
__packed_pmulh_parts_rv64_scalar(mulh_h1_i32,    int32_t, int32x2_t,
                                 int32_t, int32x2_t,
                                 int16x2_t, int16x4_t, pmulh_h1_i32x2)
__packed_pmulh_parts_rv64_scalar(mulhsu_h0_i32,  int32_t, int32x2_t,
                                 int32_t, int32x2_t,
                                 uint16x2_t, uint16x4_t, pmulhsu_h0_i32x2)
__packed_pmulh_parts_rv64_scalar(mulhsu_h1_i32,  int32_t, int32x2_t,
                                 int32_t, int32x2_t,
                                 uint16x2_t, uint16x4_t, pmulhsu_h1_i32x2)

/* Packed Multiply High Parts Accumulate: byte-lane subset (RV64 form). */
__packed_pmulh_parts_acc(pmhacc_b0_i16x4,    int16x4_t, int16x4_t, int8x8_t)
__packed_pmulh_parts_acc(pmhacc_b1_i16x4,    int16x4_t, int16x4_t, int8x8_t)
__packed_pmulh_parts_acc(pmhaccsu_b0_i16x4,  int16x4_t, int16x4_t, uint8x8_t)
__packed_pmulh_parts_acc(pmhaccsu_b1_i16x4,  int16x4_t, int16x4_t, uint8x8_t)

/* Packed Multiply High Parts Accumulate: halfword-lane vector form
 * (RV64 only, pmhacc.w.h* / pmhaccsu.w.h*). */
__packed_pmulh_parts_acc(pmhacc_h0_i32x2,    int32x2_t, int32x2_t, int16x4_t)
__packed_pmulh_parts_acc(pmhacc_h1_i32x2,    int32x2_t, int32x2_t, int16x4_t)
__packed_pmulh_parts_acc(pmhaccsu_h0_i32x2,  int32x2_t, int32x2_t, uint16x4_t)
__packed_pmulh_parts_acc(pmhaccsu_h1_i32x2,  int32x2_t, int32x2_t, uint16x4_t)

/* Cross-XLEN byte-lane forms on RV64: widen rs1 from i16x2 to i16x4 and rs2
 * from i8x4 to i8x8, call the i16x4 builtin, narrow result. */
#define __packed_pmhacc_b_rv64_half(name, narrow_v_ty, wide_v_ty,              \
                                    b_narrow_ty, b_wide_ty, wide_name)         \
  static __inline__ narrow_v_ty __DEFAULT_FN_ATTRS                             \
  __riscv_##name(narrow_v_ty __rd, narrow_v_ty __rs1, b_narrow_ty __rs2) {     \
    wide_v_ty __rdw = {__rd[0], __rd[1], 0, 0};                                \
    wide_v_ty __aw = {__rs1[0], __rs1[1], 0, 0};                               \
    b_wide_ty __bw = {__rs2[0], __rs2[1], __rs2[2], __rs2[3], 0, 0, 0, 0};     \
    wide_v_ty __r = __builtin_riscv_##wide_name(__rdw, __aw, __bw);            \
    return (narrow_v_ty){__r[0], __r[1]};                                      \
  }

__packed_pmhacc_b_rv64_half(pmhacc_b0_i16x2,   int16x2_t, int16x4_t,
                            int8x4_t,  int8x8_t,  pmhacc_b0_i16x4)
__packed_pmhacc_b_rv64_half(pmhacc_b1_i16x2,   int16x2_t, int16x4_t,
                            int8x4_t,  int8x8_t,  pmhacc_b1_i16x4)
__packed_pmhacc_b_rv64_half(pmhaccsu_b0_i16x2, int16x2_t, int16x4_t,
                            uint8x4_t, uint8x8_t, pmhaccsu_b0_i16x4)
__packed_pmhacc_b_rv64_half(pmhaccsu_b1_i16x2, int16x2_t, int16x4_t,
                            uint8x4_t, uint8x8_t, pmhaccsu_b1_i16x4)

/* Cross-XLEN mhacc.h_i32 / mhaccsu.h_i32 forms on RV64. */
__packed_pmulh_parts_acc_rv64_scalar(mhacc_h0_i32,    int32_t, int32x2_t,
                                     int32_t, int32x2_t,
                                     int16x2_t, int16x4_t, pmhacc_h0_i32x2)
__packed_pmulh_parts_acc_rv64_scalar(mhacc_h1_i32,    int32_t, int32x2_t,
                                     int32_t, int32x2_t,
                                     int16x2_t, int16x4_t, pmhacc_h1_i32x2)
__packed_pmulh_parts_acc_rv64_scalar(mhaccsu_h0_i32,  int32_t, int32x2_t,
                                     int32_t, int32x2_t,
                                     uint16x2_t, uint16x4_t, pmhaccsu_h0_i32x2)
__packed_pmulh_parts_acc_rv64_scalar(mhaccsu_h1_i32,  int32_t, int32x2_t,
                                     int32_t, int32x2_t,
                                     uint16x2_t, uint16x4_t, pmhaccsu_h1_i32x2)
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
#undef __packed_exchange_add_sub
#undef __packed_pmulh
#undef __packed_pmulhr
#undef __packed_pmhacc
#undef __packed_pmhracc
#undef __packed_pmul_parts_rv64_half
#undef __packed_pmul_parts_rv64_scalar
#undef __packed_pmul_parts_acc
#undef __packed_pmul_parts_acc_rv64_scalar
#undef __packed_pmulh_parts_rv64_scalar
#undef __packed_pmulh_b_rv64_half
#undef __packed_pmulh_parts_acc
#undef __packed_pmulh_parts_acc_rv64_scalar
#undef __packed_pmhacc_b_rv64_half
#undef __packed_pwadd_sub
#undef __packed_pwadd_sub_acc
#undef __packed_pwshift
#undef __packed_pm2w
#undef __packed_pm2wa
#undef __packed_pwcvt
#undef __packed_pld
#undef __packed_pst

/*===---------------------------------------------------------------------===
 * Scalar Intrinsics Common to RV32 and RV64
 *===---------------------------------------------------------------------===*/

static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_abs_u32(int32_t __rs1) {
  return (uint32_t)__builtin_abs(__rs1);
}

static __inline__ unsigned __DEFAULT_FN_ATTRS
__riscv_cls_32(int32_t __rs1) {
  return (unsigned)__builtin_clrsb(__rs1);
}

static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_rev_32(uint32_t __rs1) {
  return __builtin_bitreverse32(__rs1);
}

/* Saturating add/sub. */
static __inline__ int32_t __DEFAULT_FN_ATTRS
__riscv_sadd_i32(int32_t __rs1, int32_t __rs2) {
  return __builtin_elementwise_add_sat(__rs1, __rs2);
}

static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_saddu_u32(uint32_t __rs1, uint32_t __rs2) {
  return __builtin_elementwise_add_sat(__rs1, __rs2);
}

static __inline__ int32_t __DEFAULT_FN_ATTRS
__riscv_ssub_i32(int32_t __rs1, int32_t __rs2) {
  return __builtin_elementwise_sub_sat(__rs1, __rs2);
}

static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_ssubu_u32(uint32_t __rs1, uint32_t __rs2) {
  return __builtin_elementwise_sub_sat(__rs1, __rs2);
}

/* Funnel shift. */
static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_slx_32(uint32_t __rd, uint32_t __rs1, unsigned __shamt) {
  return __builtin_elementwise_fshl(__rd, __rs1, __shamt);
}

static __inline__ uint32_t __DEFAULT_FN_ATTRS
__riscv_srx_32(uint32_t __rd, uint32_t __rs1, unsigned __shamt) {
  return __builtin_elementwise_fshr(__rs1, __rd, __shamt);
}

/*===---------------------------------------------------------------------===
 * RV64 Only Scalar Intrinsics
 *===---------------------------------------------------------------------===*/
#if defined(__riscv_xlen) && (__riscv_xlen == 64)

static __inline__ uint64_t __DEFAULT_FN_ATTRS
__riscv_abs_u64(int64_t __rs1) {
  return (uint64_t)__builtin_llabs(__rs1);
}

static __inline__ unsigned __DEFAULT_FN_ATTRS
__riscv_cls_64(int64_t __rs1) {
  return (unsigned)__builtin_clrsbll(__rs1);
}

static __inline__ uint64_t __DEFAULT_FN_ATTRS
__riscv_rev_64(uint64_t __rs1) {
  return __builtin_bitreverse64(__rs1);
}

static __inline__ uint64_t __DEFAULT_FN_ATTRS
__riscv_slx_64(uint64_t __rd, uint64_t __rs1, unsigned __shamt) {
  return __builtin_elementwise_fshl(__rd, __rs1, (uint64_t)__shamt);
}

static __inline__ uint64_t __DEFAULT_FN_ATTRS
__riscv_srx_64(uint64_t __rd, uint64_t __rs1, unsigned __shamt) {
  return __builtin_elementwise_fshr(__rs1, __rd, (uint64_t)__shamt);
}

#endif /* __riscv_xlen == 64 */

#undef __DEFAULT_FN_ATTRS

#if defined(__cplusplus)
}
#endif

#endif /* __RISCV_PACKED_H */
