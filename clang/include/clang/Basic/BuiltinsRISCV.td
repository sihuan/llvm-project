//==- BuiltinsRISCV.td - RISC-V Builtin function database ---*- tablegen -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the RISC-V-specific builtin function database.
//
//===----------------------------------------------------------------------===//

include "clang/Basic/BuiltinsBase.td"

class RISCVBuiltin<string prototype, string features = ""> : TargetBuiltin {
  let Spellings = ["__builtin_riscv_" # NAME];
  let Prototype = prototype;
  let Features = features;
}

let Attributes = [NoThrow, Const] in {
//===----------------------------------------------------------------------===//
// Zbb extension.
//===----------------------------------------------------------------------===//
def orc_b_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbb">;
def orc_b_64 : RISCVBuiltin<"uint64_t(uint64_t)", "zbb,64bit">;
def clz_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbb|xtheadbb">;
def clz_64 : RISCVBuiltin<"unsigned int(uint64_t)", "zbb|xtheadbb,64bit">;
def ctz_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbb">;
def ctz_64 : RISCVBuiltin<"unsigned int(uint64_t)", "zbb,64bit">;

//===----------------------------------------------------------------------===//
// Zbc or Zbkc extension.
//===----------------------------------------------------------------------===//
def clmul_32 : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)", "zbc|zbkc">;
def clmul_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)", "zbc|zbkc,64bit">;
def clmulh_32 : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)", "zbc|zbkc,32bit">;
def clmulh_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)", "zbc|zbkc,64bit">;
def clmulr_32 : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)", "zbc,32bit">;
def clmulr_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)", "zbc,64bit">;

//===----------------------------------------------------------------------===//
// Zbkx extension.
//===----------------------------------------------------------------------===//
let Features = "zbkx,32bit" in {
def xperm4_32 : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def xperm8_32 : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
} // Features = "zbkx,32bit"

let Features = "zbkx,64bit" in {
def xperm4_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def xperm8_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
} // Features = "zbkx,64bit"

//===----------------------------------------------------------------------===//
// Zbkb extension.
//===----------------------------------------------------------------------===//
def brev8_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbkb">;
def brev8_64 : RISCVBuiltin<"uint64_t(uint64_t)", "zbkb,64bit">;
def zip_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbkb,32bit">;
def unzip_32 : RISCVBuiltin<"unsigned int(unsigned int)", "zbkb,32bit">;

//===----------------------------------------------------------------------===//
// Zknd extension.
//===----------------------------------------------------------------------===//
let Features = "zknd,32bit" in {
def aes32dsi : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int)">;
def aes32dsmi : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int)">;
} // Features = "zknd,32bit"

let Features = "zknd,64bit" in {
def aes64ds : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def aes64dsm : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def aes64im : RISCVBuiltin<"uint64_t(uint64_t)">;
} // Features = "zknd,64bit"

//===----------------------------------------------------------------------===//
// Zknd & Zkne extension.
//===----------------------------------------------------------------------===//
let Features = "zknd|zkne,64bit" in {
def aes64ks1i : RISCVBuiltin<"uint64_t(uint64_t, _Constant unsigned int)">;
def aes64ks2 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
} // Features = "zknd|zkne,64bit"

//===----------------------------------------------------------------------===//
// Zkne extension.
//===----------------------------------------------------------------------===//
let Features = "zkne,32bit" in {
def aes32esi : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int)">;
def aes32esmi : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int)">;
} // Features = "zkne,32bit"

let Features = "zkne,64bit" in {
def aes64es : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def aes64esm : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
} // Features = "zkne,64bit"

//===----------------------------------------------------------------------===//
// Zknh extension.
//===----------------------------------------------------------------------===//
let Features = "zknh" in {
def sha256sig0 : RISCVBuiltin<"unsigned int(unsigned int)">;
def sha256sig1 : RISCVBuiltin<"unsigned int(unsigned int)">;
def sha256sum0 : RISCVBuiltin<"unsigned int(unsigned int)">;
def sha256sum1 : RISCVBuiltin<"unsigned int(unsigned int)">;
} // Features = "zknh"

let Features = "zknh,32bit" in {
def sha512sig0h : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def sha512sig0l : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def sha512sig1h : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def sha512sig1l : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def sha512sum0r : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
def sha512sum1r : RISCVBuiltin<"unsigned int(unsigned int, unsigned int)">;
} // Features = "zknh,32bit"

let Features = "zknh,64bit" in {
def sha512sig0 : RISCVBuiltin<"uint64_t(uint64_t)">;
def sha512sig1 : RISCVBuiltin<"uint64_t(uint64_t)">;
def sha512sum0 : RISCVBuiltin<"uint64_t(uint64_t)">;
def sha512sum1 : RISCVBuiltin<"uint64_t(uint64_t)">;
} // Features = "zknh,64bit"

//===----------------------------------------------------------------------===//
// Zksed extension.
//===----------------------------------------------------------------------===//
let Features = "zksed" in {
def sm4ed : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int )">;
def sm4ks : RISCVBuiltin<"unsigned int(unsigned int, unsigned int, _Constant unsigned int)">;
} // Features = "zksed"

//===----------------------------------------------------------------------===//
// Zksh extension.
//===----------------------------------------------------------------------===//
let Features = "zksh" in {
def sm3p0 : RISCVBuiltin<"unsigned int(unsigned int)">;
def sm3p1 : RISCVBuiltin<"unsigned int(unsigned int)">;
} // Features = "zksh"

//===----------------------------------------------------------------------===//
// Packed SIMD extension: Packed Multiplication with Horizontal Addition
//===----------------------------------------------------------------------===//
let Features = "experimental-p,32bit" in {
// pm4add family (4-way byte multiply-add, 32-bit input -> i32 result)
def pm4add_i8x4    : RISCVBuiltin<"int(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pm4adda_i8x4   : RISCVBuiltin<"int(int, _Vector<4, signed char>, _Vector<4, signed char>)">;
def pm4addu_u8x4   : RISCVBuiltin<"unsigned int(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pm4addau_u8x4  : RISCVBuiltin<"unsigned int(unsigned int, _Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pm4addsu_i8x4  : RISCVBuiltin<"int(_Vector<4, signed char>, _Vector<4, unsigned char>)">;
def pm4addasu_i8x4 : RISCVBuiltin<"int(int, _Vector<4, signed char>, _Vector<4, unsigned char>)">;

// pm2add family (2-way halfword multiply-add, 32-bit input -> i32 result)
def pm2add_i16x2     : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pm2adda_i16x2    : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def pm2add_x_i16x2   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pm2adda_x_i16x2  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def pm2addu_u16x2    : RISCVBuiltin<"unsigned int(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pm2addau_u16x2   : RISCVBuiltin<"unsigned int(unsigned int, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pm2addsu_i16x2   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, unsigned short>)">;
def pm2addasu_i16x2  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, unsigned short>)">;

// Q-format multiply-high add
def pmq2add_i16x2    : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pmqr2add_i16x2   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pmq2adda_i16x2   : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def pmqr2adda_i16x2  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;

// Saturating multiply-add (no accumulating form)
def pm2sadd_i16x2    : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pm2sadd_x_i16x2  : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;

// Multiply-subtract (subtract upper product from lower product)
def pm2sub_i16x2     : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pm2suba_i16x2    : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def pm2sub_x_i16x2   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def pm2suba_x_i16x2  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;

// Packed Multiply Parts: byte-pair multiply (RV32 form).
def pmul_b00_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmul_b01_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmul_b11_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmulu_b00_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmulu_b01_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmulu_b11_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmulsu_b00_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>, _Vector<4, unsigned char>)">;
def pmulsu_b11_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>, _Vector<4, unsigned char>)">;

// Packed Multiply Parts: halfword-pair multiply scalar form (RV32 only, mul.h*).
def mul_h00_i32   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def mul_h01_i32   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def mul_h11_i32   : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, short>)">;
def mulu_h00_u32  : RISCVBuiltin<"unsigned int(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def mulu_h01_u32  : RISCVBuiltin<"unsigned int(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def mulu_h11_u32  : RISCVBuiltin<"unsigned int(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def mulsu_h00_i32 : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, unsigned short>)">;
def mulsu_h11_i32 : RISCVBuiltin<"int(_Vector<2, short>, _Vector<2, unsigned short>)">;

// Packed Multiply Parts Accumulate: halfword-pair accumulate scalar form
// (RV32 only, macc.h*).
def macc_h00_i32   : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def macc_h01_i32   : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def macc_h11_i32   : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def maccu_h00_u32  : RISCVBuiltin<"unsigned int(unsigned int, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def maccu_h01_u32  : RISCVBuiltin<"unsigned int(unsigned int, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def maccu_h11_u32  : RISCVBuiltin<"unsigned int(unsigned int, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def maccsu_h00_i32 : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, unsigned short>)">;
def maccsu_h11_i32 : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, unsigned short>)">;

// Packed "Q-format" Multiply Parts Accumulate: halfword scalar form
// (RV32 only, mqacc.h* / mqracc.h*).
def mqacc_h00_i32  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def mqacc_h01_i32  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def mqacc_h11_i32  : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def mqracc_h00_i32 : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def mqracc_h01_i32 : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;
def mqracc_h11_i32 : RISCVBuiltin<"int(int, _Vector<2, short>, _Vector<2, short>)">;

// Packed Multiply High Parts: byte-lane subset (RV32 form).
def pmulh_b0_i16x2     : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<4, signed char>)">;
def pmulh_b1_i16x2     : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<4, signed char>)">;
def pmulhsu_b0_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<4, unsigned char>)">;
def pmulhsu_b1_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<4, unsigned char>)">;

// Packed Multiply High Parts: halfword-lane scalar form (RV32 only, mulh.h*).
def mulh_h0_i32     : RISCVBuiltin<"int(int, _Vector<2, short>)">;
def mulh_h1_i32     : RISCVBuiltin<"int(int, _Vector<2, short>)">;
def mulhsu_h0_i32   : RISCVBuiltin<"int(int, _Vector<2, unsigned short>)">;
def mulhsu_h1_i32   : RISCVBuiltin<"int(int, _Vector<2, unsigned short>)">;

// Packed Multiply High Parts Accumulate: byte-lane subset (RV32 form).
def pmhacc_b0_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, _Vector<4, signed char>)">;
def pmhacc_b1_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, _Vector<4, signed char>)">;
def pmhaccsu_b0_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, _Vector<4, unsigned char>)">;
def pmhaccsu_b1_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, _Vector<4, unsigned char>)">;

// Packed Multiply High Parts Accumulate: halfword-lane scalar form
// (RV32 only, mhacc.h* / mhaccsu.h*).
def mhacc_h0_i32     : RISCVBuiltin<"int(int, int, _Vector<2, short>)">;
def mhacc_h1_i32     : RISCVBuiltin<"int(int, int, _Vector<2, short>)">;
def mhaccsu_h0_i32   : RISCVBuiltin<"int(int, int, _Vector<2, unsigned short>)">;
def mhaccsu_h1_i32   : RISCVBuiltin<"int(int, int, _Vector<2, unsigned short>)">;

// Packed Widening Addition and Subtraction (RV32 only).
def pwadd_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pwadd_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>, _Vector<2, short>)">;
def pwaddu_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pwaddu_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pwsub_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pwsub_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>, _Vector<2, short>)">;
def pwsubu_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pwsubu_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Packed Widening Add/Sub Accumulate (RV32 only).
def pwadda_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, signed char>, _Vector<4, signed char>)">;
def pwadda_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, short>)">;
def pwaddau_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pwaddau_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pwsuba_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, signed char>, _Vector<4, signed char>)">;
def pwsuba_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, short>)">;
def pwsubau_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pwsubau_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Packed Widening Multiply (RV32 only).
def pwmul_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pwmul_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>, _Vector<2, short>)">;
def pwmulu_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pwmulu_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pwmulsu_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>, _Vector<4, unsigned char>)">;
def pwmulsu_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>, _Vector<2, unsigned short>)">;

// Packed Widening Multiply Accumulate (RV32 only).
def pwmacc_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, short>)">;
def pwmaccu_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pwmaccsu_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, unsigned short>)">;

// Packed Q-format Multiply with Widening Accumulate (RV32 only).
def pmqwacc_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, short>)">;
def pmqrwacc_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, short>, _Vector<2, short>)">;

// Packed Widening Shift Left (RV32 only).
def pwsll_s_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>, unsigned int)">;
def pwsll_s_u32x2 : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>, unsigned int)">;
def pwsla_s_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>, unsigned int)">;
def pwsla_s_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>, unsigned int)">;

// Packed Narrowing Shift Right (RV32 only).
def pnsrl_s_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned short>, unsigned int)">;
def pnsrl_s_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned int>, unsigned int)">;
def pnsra_s_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>, unsigned int)">;
def pnsra_s_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>, unsigned int)">;
def pnsrar_s_i8x4  : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>, unsigned int)">;
def pnsrar_s_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>, unsigned int)">;

// Packed Narrowing Clip (RV32 only).
def pnclipu_s_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned short>, unsigned int)">;
def pnclipu_s_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned int>, unsigned int)">;
def pnclipru_s_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned short>, unsigned int)">;
def pnclipru_s_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned int>, unsigned int)">;
def pnclip_s_i8x4     : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>, unsigned int)">;
def pnclip_s_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>, unsigned int)">;
def pnclipr_s_i8x4    : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>, unsigned int)">;
def pnclipr_s_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>, unsigned int)">;

// Packed Multiplication with Widening Horizontal Addition (RV32 only).
def pm2wadd_i64    : RISCVBuiltin<"int64_t(_Vector<2, short>, _Vector<2, short>)">;
def pm2wadda_i64   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, short>, _Vector<2, short>)">;
def pm2wadd_x_i64  : RISCVBuiltin<"int64_t(_Vector<2, short>, _Vector<2, short>)">;
def pm2wadda_x_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, short>, _Vector<2, short>)">;
def pm2waddu_u64   : RISCVBuiltin<"uint64_t(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pm2waddau_u64  : RISCVBuiltin<"uint64_t(uint64_t, _Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pm2wsub_i64    : RISCVBuiltin<"int64_t(_Vector<2, short>, _Vector<2, short>)">;
def pm2wsuba_i64   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, short>, _Vector<2, short>)">;
def pm2wsub_x_i64  : RISCVBuiltin<"int64_t(_Vector<2, short>, _Vector<2, short>)">;
def pm2wsuba_x_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, short>, _Vector<2, short>)">;
def pm2waddsu_u64  : RISCVBuiltin<"int64_t(_Vector<2, short>, _Vector<2, unsigned short>)">;
def pm2waddasu_u64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, short>, _Vector<2, unsigned short>)">;

// Packed Widening Convert (RV32 only).
def pwcvt_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>)">;
def pwcvt_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>)">;
def pwcvtu_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>)">;
def pwcvtu_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>)">;
def pwcvth_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, signed char>)">;
def pwcvth_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned char>)">;
def pwcvth_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, short>)">;
def pwcvth_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned short>)">;

// Packed Narrowing Convert (RV32 only). Implemented via pnsrli with the
// shift amount selecting the low (imm=0) or high (imm=element-width) half.
def pncvt_i8x4    : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>)">;
def pncvt_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned short>)">;
def pncvt_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>)">;
def pncvt_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned int>)">;
def pncvth_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, short>)">;
def pncvth_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned short>)">;
def pncvth_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, int>)">;
def pncvth_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned int>)">;

// Packed Zip (RV32 only).
def pzip_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pzip_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pzip_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<2, short>, _Vector<2, short>)">;
def pzip_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Packed Narrowing Zip (RV32 only).
def pnzip_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<2, short>, _Vector<2, short>)">;
def pnzip_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pnziph_i8x4  : RISCVBuiltin<"_Vector<4, signed char>(_Vector<2, short>, _Vector<2, short>)">;
def pnziph_u8x4  : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Packed Widening Unzip (RV32 only).
def pwunzipe_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>)">;
def pwunzipo_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>)">;
def pwunzipue_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>)">;
def pwunzipuo_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>)">;
def pwunziphe_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>)">;
def pwunzipho_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, signed char>)">;
def pwunziphe_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>)">;
def pwunzipho_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned char>)">;

// Packed Unzip (RV32 only). punzipe extracts the even-indexed elements
// (low half of each wide lane after bitcast); punzipo extracts the
// odd-indexed elements (high half). Implemented via pnsrli with the
// corresponding shift amount.
def punzipe_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<8, signed char>)">;
def punzipo_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<8, signed char>)">;
def punzipe_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<8, unsigned char>)">;
def punzipo_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<8, unsigned char>)">;
def punzipe_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, short>)">;
def punzipo_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, short>)">;
def punzipe_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned short>)">;
def punzipo_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned short>)">;

// Widening zip: 2 x i32 -> i64 (wzip8p / wzip16p). RV32 only; the result is
// held in a GPR pair. Reuses int_riscv_pzip via bitcasts.
def wzip8p_64  : RISCVBuiltin<"uint64_t(uint32_t, uint32_t)">;
def wzip16p_64 : RISCVBuiltin<"uint64_t(uint32_t, uint32_t)">;

// Narrowing clip / rounding shift: i64 input, i32 output (RV32 only --
// reuses the RV32-only pnclip* / pnsrar IR intrinsics).
def nclipu_u32  : RISCVBuiltin<"uint32_t(uint64_t, unsigned int)">;
def nclipru_u32 : RISCVBuiltin<"uint32_t(uint64_t, unsigned int)">;
def nclip_i32   : RISCVBuiltin<"int32_t(int64_t, unsigned int)">;
def nclipr_i32  : RISCVBuiltin<"int32_t(int64_t, unsigned int)">;
def nsrar_i32   : RISCVBuiltin<"int32_t(int64_t, unsigned int)">;
} // Features = "experimental-p,32bit"

// Packed Exchanged Addition and Subtraction (halfword form, RV32 and RV64).
// Uses the same pas.hx instruction on both XLEN; on RV64 it operates on the
// lower 32 bits of the GPR.
let Features = "experimental-p" in {
def pas_x_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def psa_x_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def psas_x_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def pssa_x_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def paas_x_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def pasa_x_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;

// Packed SH1ADD / SSH1SADD (halfword form, RV32 and RV64).
def psh1add_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def psh1add_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pssh1sadd_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;

// Packed "Q-format" Multiplication (halfword form, RV32 and RV64).
def pmulq_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def pmulqr_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;

// Packed Load and Store (element-aligned). The vector types are 32-bit on the
// "32-bit" set and 64-bit on the "64-bit" set; both sets are supported on
// both XLENs (on RV32 a 64-bit load/store is materialized as two lw/sw via
// the standard i64 legalization).
def pld_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(signed char *)">;
def pld_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(unsigned char *)">;
def pld_i16x2  : RISCVBuiltin<"_Vector<2, short>(short *)">;
def pld_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(unsigned short *)">;
def pst_i8x4   : RISCVBuiltin<"void(signed char *, _Vector<4, signed char>)">;
def pst_u8x4   : RISCVBuiltin<"void(unsigned char *, _Vector<4, unsigned char>)">;
def pst_i16x2  : RISCVBuiltin<"void(short *, _Vector<2, short>)">;
def pst_u16x2  : RISCVBuiltin<"void(unsigned short *, _Vector<2, unsigned short>)">;

def pld_i8x8   : RISCVBuiltin<"_Vector<8, signed char>(signed char *)">;
def pld_u8x8   : RISCVBuiltin<"_Vector<8, unsigned char>(unsigned char *)">;
def pld_i16x4  : RISCVBuiltin<"_Vector<4, short>(short *)">;
def pld_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(unsigned short *)">;
def pld_i32x2  : RISCVBuiltin<"_Vector<2, int>(int *)">;
def pld_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(unsigned int *)">;
def pst_i8x8   : RISCVBuiltin<"void(signed char *, _Vector<8, signed char>)">;
def pst_u8x8   : RISCVBuiltin<"void(unsigned char *, _Vector<8, unsigned char>)">;
def pst_i16x4  : RISCVBuiltin<"void(short *, _Vector<4, short>)">;
def pst_u16x4  : RISCVBuiltin<"void(unsigned short *, _Vector<4, unsigned short>)">;
def pst_i32x2  : RISCVBuiltin<"void(int *, _Vector<2, int>)">;
def pst_u32x2  : RISCVBuiltin<"void(unsigned int *, _Vector<2, unsigned int>)">;

// Packed Element Insert and Extract (32-bit and 64-bit packed types, both
// XLENs). Index must be a constant.
def pget_i8x4_i8     : RISCVBuiltin<"signed char(_Vector<4, signed char>, _Constant unsigned int)">;
def pget_u8x4_u8     : RISCVBuiltin<"unsigned char(_Vector<4, unsigned char>, _Constant unsigned int)">;
def pget_i16x2_i16   : RISCVBuiltin<"short(_Vector<2, short>, _Constant unsigned int)">;
def pget_u16x2_u16   : RISCVBuiltin<"unsigned short(_Vector<2, unsigned short>, _Constant unsigned int)">;
def pset_i8_i8x4     : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, signed char, _Constant unsigned int)">;
def pset_u8_u8x4     : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, unsigned char, _Constant unsigned int)">;
def pset_i16_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, short, _Constant unsigned int)">;
def pset_u16_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, unsigned short, _Constant unsigned int)">;

def pget_i8x8_i8     : RISCVBuiltin<"signed char(_Vector<8, signed char>, _Constant unsigned int)">;
def pget_u8x8_u8     : RISCVBuiltin<"unsigned char(_Vector<8, unsigned char>, _Constant unsigned int)">;
def pget_i16x4_i16   : RISCVBuiltin<"short(_Vector<4, short>, _Constant unsigned int)">;
def pget_u16x4_u16   : RISCVBuiltin<"unsigned short(_Vector<4, unsigned short>, _Constant unsigned int)">;
def pget_i32x2_i32   : RISCVBuiltin<"int(_Vector<2, int>, _Constant unsigned int)">;
def pget_u32x2_u32   : RISCVBuiltin<"unsigned int(_Vector<2, unsigned int>, _Constant unsigned int)">;
def pset_i8_i8x8     : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, signed char, _Constant unsigned int)">;
def pset_u8_u8x8     : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, unsigned char, _Constant unsigned int)">;
def pset_i16_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, short, _Constant unsigned int)">;
def pset_u16_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, unsigned short, _Constant unsigned int)">;
def pset_i32_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, int, _Constant unsigned int)">;
def pset_u32_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, unsigned int, _Constant unsigned int)">;

// Packed Element Join (32-bit and 64-bit packed types, both XLENs).
def pjoin4_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(signed char, signed char, signed char, signed char)">;
def pjoin4_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(unsigned char, unsigned char, unsigned char, unsigned char)">;
def pjoin2_i16x2  : RISCVBuiltin<"_Vector<2, short>(short, short)">;
def pjoin2_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(unsigned short, unsigned short)">;

def pjoin4_i16x4  : RISCVBuiltin<"_Vector<4, short>(short, short, short, short)">;
def pjoin4_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(unsigned short, unsigned short, unsigned short, unsigned short)">;
def pjoin2_i32x2  : RISCVBuiltin<"_Vector<2, int>(int, int)">;
def pjoin2_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(unsigned int, unsigned int)">;

// Packed Subvector Insert and Extract (32-bit subvector <-> 64-bit packed).
def pget_i8x8_i8x4    : RISCVBuiltin<"_Vector<4, signed char>(_Vector<8, signed char>, _Constant unsigned int)">;
def pget_u8x8_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<8, unsigned char>, _Constant unsigned int)">;
def pget_i16x4_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<4, short>, _Constant unsigned int)">;
def pget_u16x4_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<4, unsigned short>, _Constant unsigned int)">;
def pset_i8x4_i8x8    : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<4, signed char>, _Constant unsigned int)">;
def pset_u8x4_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<4, unsigned char>, _Constant unsigned int)">;
def pset_i16x2_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<2, short>, _Constant unsigned int)">;
def pset_u16x2_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<2, unsigned short>, _Constant unsigned int)">;

// Packed Subvector Join (two 32-bit subvectors -> 64-bit packed vector).
def pjoin2_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pjoin2_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pjoin2_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<2, short>, _Vector<2, short>)">;
def pjoin2_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Slide 1 up/down: rd is the packed vector, rs1 is a scalar new element.
//   pslide1up:   result = [rs1, rd[0], rd[1], ..., rd[N-2]]
//   pslide1down: result = [rd[1], rd[2], ..., rd[N-1], rs1]
def pslide1up_i8x4    : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, signed char)">;
def pslide1up_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, unsigned char)">;
def pslide1up_i16x2   : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, short)">;
def pslide1up_u16x2   : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, unsigned short)">;
def pslide1down_i8x4  : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, signed char)">;
def pslide1down_u8x4  : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, unsigned char)">;
def pslide1down_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, short)">;
def pslide1down_u16x2 : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, unsigned short)">;

def pslide1up_i8x8    : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, signed char)">;
def pslide1up_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, unsigned char)">;
def pslide1up_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, short)">;
def pslide1up_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, unsigned short)">;
def pslide1up_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, int)">;
def pslide1up_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, unsigned int)">;
def pslide1down_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, signed char)">;
def pslide1down_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, unsigned char)">;
def pslide1down_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, short)">;
def pslide1down_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, unsigned short)">;
def pslide1down_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, int)">;
def pslide1down_u32x2 : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, unsigned int)">;

// Slide by variable element count. rs2 is masked to log2(N) bits, then the
// concatenation (rd : rs1) is funnel-shifted.
//   pslideupx:   result = fshl(rd, rs1, (rs2 & (N-1)) * elt_bits)
//   pslidedownx: result = fshr(rs1, rd, (rs2 & (N-1)) * elt_bits)
def pslideupx_i8x4     : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>, unsigned int)">;
def pslideupx_u8x4     : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>, unsigned int)">;
def pslideupx_i16x2    : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, unsigned int)">;
def pslideupx_u16x2    : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>, unsigned int)">;
def pslidedownx_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>, unsigned int)">;
def pslidedownx_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>, unsigned int)">;
def pslidedownx_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>, unsigned int)">;
def pslidedownx_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>, unsigned int)">;

def pslideupx_i8x8     : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>, unsigned int)">;
def pslideupx_u8x8     : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>, unsigned int)">;
def pslideupx_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, unsigned int)">;
def pslideupx_u16x4    : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>, unsigned int)">;
def pslideupx_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, unsigned int)">;
def pslideupx_u32x2    : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>, unsigned int)">;
def pslidedownx_i8x8   : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>, unsigned int)">;
def pslidedownx_u8x8   : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>, unsigned int)">;
def pslidedownx_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, unsigned int)">;
def pslidedownx_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>, unsigned int)">;
def pslidedownx_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, unsigned int)">;
def pslidedownx_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>, unsigned int)">;

// Scalar Intrinsics (C class): mapped through new IR intrinsics with custom
// CodeGen. Available on both XLENs (the IR intrinsic is overloaded and Clang
// extends/truncates around it on RV64).

// Averaging subtract.
def asub_i32  : RISCVBuiltin<"int32_t(int32_t, int32_t)">;
def asubu_u32 : RISCVBuiltin<"uint32_t(uint32_t, uint32_t)">;

// Saturate to N bits (constant shift amount).
def sati_i32  : RISCVBuiltin<"int32_t(int32_t, _Constant unsigned int)">;
def usati_u32 : RISCVBuiltin<"uint32_t(int32_t, _Constant unsigned int)">;

// Saturating sh1add.
def ssh1sadd_i32 : RISCVBuiltin<"int32_t(int32_t, int32_t)">;

// Rounding multiply-high.
def mulhr_i32   : RISCVBuiltin<"int32_t(int32_t, int32_t)">;
def mulhru_u32  : RISCVBuiltin<"uint32_t(uint32_t, uint32_t)">;
def mulhrsu_i32 : RISCVBuiltin<"int32_t(int32_t, uint32_t)">;

// Multiply-high accumulate.
def mhacc_i32    : RISCVBuiltin<"int32_t(int32_t, int32_t, int32_t)">;
def mhracc_i32   : RISCVBuiltin<"int32_t(int32_t, int32_t, int32_t)">;
def mhaccu_u32   : RISCVBuiltin<"uint32_t(uint32_t, uint32_t, uint32_t)">;
def mhraccu_u32  : RISCVBuiltin<"uint32_t(uint32_t, uint32_t, uint32_t)">;
def mhaccsu_i32  : RISCVBuiltin<"int32_t(int32_t, int32_t, uint32_t)">;
def mhraccsu_i32 : RISCVBuiltin<"int32_t(int32_t, int32_t, uint32_t)">;

// Q-format multiply.
def mulq_i32  : RISCVBuiltin<"int32_t(int32_t, int32_t)">;
def mulqr_i32 : RISCVBuiltin<"int32_t(int32_t, int32_t)">;

// Q-format multiply with widening accumulate (i64 accumulator + 2 x i32).
def mqwacc_i64  : RISCVBuiltin<"int64_t(int64_t, int32_t, int32_t)">;
def mqrwacc_i64 : RISCVBuiltin<"int64_t(int64_t, int32_t, int32_t)">;

// Compare: produce all-ones (-1) or zero. Naming mirrors riscv_p_asm.h.
def mseq_i32  : RISCVBuiltin<"int32_t(int32_t, int32_t)">;
def mseq_u32  : RISCVBuiltin<"uint32_t(uint32_t, uint32_t)">;
def mslt_u32  : RISCVBuiltin<"uint32_t(int32_t, int32_t)">;
def msgt_u32  : RISCVBuiltin<"uint32_t(int32_t, int32_t)">;
def msltu_u32 : RISCVBuiltin<"uint32_t(uint32_t, uint32_t)">;
def msgtu_u32 : RISCVBuiltin<"uint32_t(uint32_t, uint32_t)">;

// Saturating compound shifts with signed shift amount (negative = right
// shift). RV32 emits ssha / sshar / sshl / sshlr; RV64 emits the
// packed-word forms pssha.ws / psshar.ws / psshl.ws / psshlr.ws.
def ssha_i32  : RISCVBuiltin<"int32_t(int32_t, int)">;
def sshar_i32 : RISCVBuiltin<"int32_t(int32_t, int)">;
def sshl_u32  : RISCVBuiltin<"uint32_t(int32_t, int)">;
def sshlr_u32 : RISCVBuiltin<"uint32_t(int32_t, int)">;

// Packed Pair (32-bit table, both XLENs). Four operations (ppaire = even/even,
// ppaireo = even/odd, ppairoe = odd/even, ppairo = odd/odd) over
// {signed,unsigned} x {8x4, 16x2} elements.
def ppaire_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def ppaire_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def ppaireo_i8x4  : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def ppaireo_u8x4  : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def ppairoe_i8x4  : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def ppairoe_u8x4  : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def ppairo_i8x4   : RISCVBuiltin<"_Vector<4, signed char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def ppairo_u8x4   : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def ppaire_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def ppaire_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def ppaireo_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def ppaireo_u16x2 : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def ppairoe_i16x2 : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def ppairoe_u16x2 : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def ppairo_i16x2  : RISCVBuiltin<"_Vector<2, short>(_Vector<2, short>, _Vector<2, short>)">;
def ppairo_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

// Packed Comparisons (32-bit table, both XLENs). Result is an unsigned packed
// vector of the same lane shape. Three native operations (pmseq / pmslt /
// pmsltu) and nine derived ops (pmsne / pmsgt / pmsgtu / pmsge / pmsle /
// pmsleu / pmsgeu) realized via operand-swap + XOR in clang CodeGen.
def pmseq_i8x4_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmseq_u8x4_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmseq_i16x2_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmseq_u16x2_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pmslt_u8x4         : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmslt_u16x2        : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmsgt_u8x4         : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmsgt_u16x2        : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmsltu_u8x4        : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmsltu_u16x2       : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pmsgtu_u8x4        : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmsgtu_u16x2       : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pmsne_i8x4_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmsne_u8x4_u8x4    : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmsne_i16x2_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmsne_u16x2_u16x2  : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pmsge_u8x4         : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmsge_u16x2        : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmsle_u8x4         : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, signed char>, _Vector<4, signed char>)">;
def pmsle_u16x2        : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, short>, _Vector<2, short>)">;
def pmsleu_u8x4        : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmsleu_u16x2       : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;
def pmsgeu_u8x4        : RISCVBuiltin<"_Vector<4, unsigned char>(_Vector<4, unsigned char>, _Vector<4, unsigned char>)">;
def pmsgeu_u16x2       : RISCVBuiltin<"_Vector<2, unsigned short>(_Vector<2, unsigned short>, _Vector<2, unsigned short>)">;

} // Features = "experimental-p"

let Features = "experimental-p,64bit" in {
// pm4add 8-byte (RV64 only, 64-bit input -> 2 lanes of i32)
def pm4add_i8x8    : RISCVBuiltin<"_Vector<2, int>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pm4adda_i8x8   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<8, signed char>, _Vector<8, signed char>)">;
def pm4addu_u8x8   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pm4addau_u8x8  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pm4addsu_i8x8  : RISCVBuiltin<"_Vector<2, int>(_Vector<8, signed char>, _Vector<8, unsigned char>)">;
def pm4addasu_i8x8 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<8, signed char>, _Vector<8, unsigned char>)">;

// pm4add 4-halfword (RV64 only, 64-bit input -> i64 scalar)
def pm4add_i16x4    : RISCVBuiltin<"int64_t(_Vector<4, short>, _Vector<4, short>)">;
def pm4adda_i16x4   : RISCVBuiltin<"int64_t(int64_t, _Vector<4, short>, _Vector<4, short>)">;
def pm4addu_u16x4   : RISCVBuiltin<"uint64_t(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pm4addau_u16x4  : RISCVBuiltin<"uint64_t(uint64_t, _Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pm4addsu_i16x4  : RISCVBuiltin<"int64_t(_Vector<4, short>, _Vector<4, unsigned short>)">;
def pm4addasu_i16x4 : RISCVBuiltin<"int64_t(int64_t, _Vector<4, short>, _Vector<4, unsigned short>)">;

// pm2add halfword 4-element (RV64 only, 64-bit input -> 2 lanes of i32)
def pm2add_i16x4     : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pm2adda_i16x4    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pm2add_x_i16x4   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pm2adda_x_i16x4  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pm2addu_u16x4    : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pm2addau_u16x4   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pm2addsu_i16x4   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, unsigned short>)">;
def pm2addasu_i16x4  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, unsigned short>)">;

// pm2add word 2-element (RV64 only, 64-bit input -> i64 scalar)
def pm2add_i32x2     : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pm2adda_i32x2    : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def pm2add_x_i32x2   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pm2adda_x_i32x2  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def pm2addu_u32x2    : RISCVBuiltin<"uint64_t(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pm2addau_u32x2   : RISCVBuiltin<"uint64_t(uint64_t, _Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pm2addsu_i32x2   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, unsigned int>)">;
def pm2addasu_i32x2  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, unsigned int>)">;

// Q-format halfword 4-element (RV64 only, 64-bit input -> 2 lanes of i32)
def pmq2add_i16x4    : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pmqr2add_i16x4   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pmq2adda_i16x4   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqr2adda_i16x4  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;

// Q-format word 2-element (RV64 only, 64-bit input -> i64 scalar)
def pmq2add_i32x2    : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pmqr2add_i32x2   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pmq2adda_i32x2   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def pmqr2adda_i32x2  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;

// Saturating halfword 4-element (RV64 only, no accumulator)
def pm2sadd_i16x4    : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pm2sadd_x_i16x4  : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;

// Multiply-subtract halfword 4-element (RV64 only)
def pm2sub_i16x4     : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pm2suba_i16x4    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pm2sub_x_i16x4   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pm2suba_x_i16x4  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;

// Multiply-subtract word 2-element (RV64 only)
def pm2sub_i32x2     : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pm2suba_i32x2    : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def pm2sub_x_i32x2   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def pm2suba_x_i32x2  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;

// Packed Exchanged Addition and Subtraction (halfword, 64-bit input, RV64 only)
def pas_x_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def psa_x_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def psas_x_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def pssa_x_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def paas_x_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def pasa_x_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;

// Packed Exchanged Addition and Subtraction (word, RV64 only)
def pas_x_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def psa_x_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def psas_x_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def pssa_x_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def paas_x_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def pasa_x_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;

// Packed SH1ADD / SSH1SADD (RV64 only, 64-bit input).
def psh1add_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def psh1add_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def psh1add_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def psh1add_u32x2   : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pssh1sadd_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def pssh1sadd_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;

// Packed "Q-format" Multiplication (RV64 only, 64-bit input).
def pmulq_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def pmulq_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def pmulqr_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def pmulqr_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;

// Packed Multiply Parts: byte-pair multiply (RV64 64-bit form).
def pmul_b00_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmul_b01_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmul_b11_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmulu_b00_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmulu_b01_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmulu_b11_u16x4   : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmulsu_b00_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>, _Vector<8, unsigned char>)">;
def pmulsu_b11_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>, _Vector<8, unsigned char>)">;

// Packed Multiply Parts: halfword-pair multiply vector form (RV64 only).
def pmul_h00_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pmul_h01_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pmul_h11_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, short>)">;
def pmulu_h00_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmulu_h01_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmulu_h11_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmulsu_h00_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, unsigned short>)">;
def pmulsu_h11_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>, _Vector<4, unsigned short>)">;

// Packed Multiply Parts: word-pair multiply scalar form (RV64 only, mul.w*).
def mul_w00_i64   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def mul_w01_i64   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def mul_w11_i64   : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, int>)">;
def mulu_w00_u64  : RISCVBuiltin<"uint64_t(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def mulu_w01_u64  : RISCVBuiltin<"uint64_t(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def mulu_w11_u64  : RISCVBuiltin<"uint64_t(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def mulsu_w00_i64 : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, unsigned int>)">;
def mulsu_w11_i64 : RISCVBuiltin<"int64_t(_Vector<2, int>, _Vector<2, unsigned int>)">;

// Packed Multiply Parts Accumulate: halfword-pair accumulate vector form
// (RV64 only, pmacc.w.h*).
def pmacc_h00_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmacc_h01_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmacc_h11_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmaccu_h00_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmaccu_h01_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmaccu_h11_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmaccsu_h00_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, unsigned short>)">;
def pmaccsu_h11_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, unsigned short>)">;

// Packed Multiply Parts Accumulate: word-pair accumulate scalar form
// (RV64 only, macc.w*).
def macc_w00_i64   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def macc_w01_i64   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def macc_w11_i64   : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def maccu_w00_u64  : RISCVBuiltin<"uint64_t(uint64_t, _Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def maccu_w01_u64  : RISCVBuiltin<"uint64_t(uint64_t, _Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def maccu_w11_u64  : RISCVBuiltin<"uint64_t(uint64_t, _Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def maccsu_w00_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, unsigned int>)">;
def maccsu_w11_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, unsigned int>)">;

// Packed "Q-format" Multiply Parts Accumulate: halfword vector form
// (RV64 only, pmqacc.w.h* / pmqracc.w.h*).
def pmqacc_h00_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqacc_h01_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqacc_h11_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqracc_h00_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqracc_h01_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;
def pmqracc_h11_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>, _Vector<4, short>)">;

// Packed "Q-format" Multiply Parts Accumulate: word scalar form
// (RV64 only, mqacc.w* / mqracc.w*).
def mqacc_w00_i64  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def mqacc_w01_i64  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def mqacc_w11_i64  : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def mqracc_w00_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def mqracc_w01_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;
def mqracc_w11_i64 : RISCVBuiltin<"int64_t(int64_t, _Vector<2, int>, _Vector<2, int>)">;

// Packed Multiply High Parts: byte-lane subset (RV64 64-bit form).
def pmulh_b0_i16x4     : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<8, signed char>)">;
def pmulh_b1_i16x4     : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<8, signed char>)">;
def pmulhsu_b0_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<8, unsigned char>)">;
def pmulhsu_b1_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<8, unsigned char>)">;

// Packed Multiply High Parts: halfword-lane vector form (RV64 only, pmulh.w.h*).
def pmulh_h0_i32x2     : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>)">;
def pmulh_h1_i32x2     : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, short>)">;
def pmulhsu_h0_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, unsigned short>)">;
def pmulhsu_h1_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<4, unsigned short>)">;

// Packed Multiply High Parts Accumulate: byte-lane subset (RV64 form).
def pmhacc_b0_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, _Vector<8, signed char>)">;
def pmhacc_b1_i16x4    : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, _Vector<8, signed char>)">;
def pmhaccsu_b0_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, _Vector<8, unsigned char>)">;
def pmhaccsu_b1_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>, _Vector<8, unsigned char>)">;

// Packed Multiply High Parts Accumulate: halfword-lane vector form
// (RV64 only, pmhacc.w.h* / pmhaccsu.w.h*).
def pmhacc_h0_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, _Vector<4, short>)">;
def pmhacc_h1_i32x2    : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, _Vector<4, short>)">;
def pmhaccsu_h0_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, _Vector<4, unsigned short>)">;
def pmhaccsu_h1_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>, _Vector<4, unsigned short>)">;

// RV64-only scalar intrinsics (C class).
def rev16_64    : RISCVBuiltin<"uint64_t(uint64_t)">;
def sati_i64    : RISCVBuiltin<"int64_t(int64_t, _Constant unsigned int)">;
def usati_u64   : RISCVBuiltin<"uint64_t(int64_t, _Constant unsigned int)">;
def zip8p_64    : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def zip16p_64   : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def zip8hp_64   : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def zip16hp_64  : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def unzip8p_64  : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def unzip16p_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def unzip8hp_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;
def unzip16hp_64 : RISCVBuiltin<"uint64_t(uint64_t, uint64_t)">;

// RV64-only non-saturating compound shifts with signed shift amount
// (negative = right shift). Emits sha / shar / shl / shlr.
def sha_i64  : RISCVBuiltin<"int64_t(int64_t, int)">;
def shar_i64 : RISCVBuiltin<"int64_t(int64_t, int)">;
def shl_u64  : RISCVBuiltin<"uint64_t(uint64_t, int)">;
def shlr_u64 : RISCVBuiltin<"uint64_t(uint64_t, int)">;

// Packed Narrowing Zip (RV64 only).
def pnzip_i8x8   : RISCVBuiltin<"_Vector<8, signed char>(_Vector<4, short>, _Vector<4, short>)">;
def pnzip_u8x8   : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pnzip_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<2, int>, _Vector<2, int>)">;
def pnzip_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pnziph_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<4, short>, _Vector<4, short>)">;
def pnziph_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pnziph_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<2, int>, _Vector<2, int>)">;
def pnziph_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;

// Packed Widening Unzip (RV64 only).
def pwunzipe_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>)">;
def pwunzipo_i16x4   : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>)">;
def pwunzipue_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>)">;
def pwunzipuo_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>)">;
def pwunziphe_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>)">;
def pwunzipho_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<8, signed char>)">;
def pwunziphe_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>)">;
def pwunzipho_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<8, unsigned char>)">;

def pwunzipe_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>)">;
def pwunzipo_i32x2   : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>)">;
def pwunzipue_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>)">;
def pwunzipuo_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>)">;
def pwunziphe_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>)">;
def pwunzipho_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<4, short>)">;
def pwunziphe_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>)">;
def pwunzipho_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<4, unsigned short>)">;

// Packed Pair (64-bit table, RV64 only). RV32 64-bit-table forms requiring
// p*.d* register-pair instructions are deferred.
def ppaire_i8x8   : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def ppaire_u8x8   : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def ppaireo_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def ppaireo_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def ppairoe_i8x8  : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def ppairoe_u8x8  : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def ppairo_i8x8   : RISCVBuiltin<"_Vector<8, signed char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def ppairo_u8x8   : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def ppaire_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def ppaire_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def ppaireo_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def ppaireo_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def ppairoe_i16x4 : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def ppairoe_u16x4 : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def ppairo_i16x4  : RISCVBuiltin<"_Vector<4, short>(_Vector<4, short>, _Vector<4, short>)">;
def ppairo_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def ppaire_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def ppaire_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def ppaireo_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def ppaireo_u32x2 : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def ppairoe_i32x2 : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def ppairoe_u32x2 : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def ppairo_i32x2  : RISCVBuiltin<"_Vector<2, int>(_Vector<2, int>, _Vector<2, int>)">;
def ppairo_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;

// Packed Comparisons (64-bit table, RV64 only). RV32 64-bit-table forms
// requiring p*.d* register-pair instructions are deferred. Same 12
// operations as the 32-bit table; result lane type is unsigned.
def pmseq_i8x8_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmseq_u8x8_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmseq_i16x4_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmseq_u16x4_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmseq_i32x2_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmseq_u32x2_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pmslt_u8x8         : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmslt_u16x4        : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmslt_u32x2        : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmsgt_u8x8         : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmsgt_u16x4        : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmsgt_u32x2        : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmsltu_u8x8        : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmsltu_u16x4       : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmsltu_u32x2       : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pmsgtu_u8x8        : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmsgtu_u16x4       : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmsgtu_u32x2       : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pmsne_i8x8_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmsne_u8x8_u8x8    : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmsne_i16x4_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmsne_u16x4_u16x4  : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmsne_i32x2_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmsne_u32x2_u32x2  : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pmsge_u8x8         : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmsge_u16x4        : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmsge_u32x2        : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmsle_u8x8         : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, signed char>, _Vector<8, signed char>)">;
def pmsle_u16x4        : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, short>, _Vector<4, short>)">;
def pmsle_u32x2        : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, int>, _Vector<2, int>)">;
def pmsleu_u8x8        : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmsleu_u16x4       : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmsleu_u32x2       : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
def pmsgeu_u8x8        : RISCVBuiltin<"_Vector<8, unsigned char>(_Vector<8, unsigned char>, _Vector<8, unsigned char>)">;
def pmsgeu_u16x4       : RISCVBuiltin<"_Vector<4, unsigned short>(_Vector<4, unsigned short>, _Vector<4, unsigned short>)">;
def pmsgeu_u32x2       : RISCVBuiltin<"_Vector<2, unsigned int>(_Vector<2, unsigned int>, _Vector<2, unsigned int>)">;
} // Features = "experimental-p,64bit"

} // Attributes = [Const, NoThrow]

//===----------------------------------------------------------------------===//
// Zihintntl extension.
//===----------------------------------------------------------------------===//
let Features = "zihintntl", Attributes = [CustomTypeChecking] in {
def ntl_load : RISCVBuiltin<"void(...)">;
def ntl_store : RISCVBuiltin<"void(...)">;
} // Features = "zihintntl", Attributes = [CustomTypeChecking]

//===----------------------------------------------------------------------===//
// Zihintpause extension.
//===----------------------------------------------------------------------===//
let Features = "zihintpause", Attributes = [NoThrow] in
def pause : RISCVBuiltin<"void()">;

//===----------------------------------------------------------------------===//
// XCV extensions.
//===----------------------------------------------------------------------===//
include "clang/Basic/BuiltinsRISCVXCV.td"

//===----------------------------------------------------------------------===//
// XAndes extensions.
//===----------------------------------------------------------------------===//
include "clang/Basic/BuiltinsRISCVXAndes.td"

//===----------------------------------------------------------------------===//
// MIPS extensions.
//===----------------------------------------------------------------------===//
include "clang/Basic/BuiltinsRISCVXMIPS.td"
