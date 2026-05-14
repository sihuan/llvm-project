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
} // Features = "experimental-p,32bit"

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
