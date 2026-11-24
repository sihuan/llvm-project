//===- SplitPackedFloatPairs.h - scalarize packed float pairs ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Some targets (e.g. RISC-V LP64D) pass/return a small homogeneous-float
// aggregate by coercing it to an integer pair (i64), because the psABI places
// it in the two integer argument registers.  After inlining, that leaves the
// float values represented as two packed i64 lanes: element-wise FP
// accumulations become `or(shl(zext(bitcast f)), zext(bitcast f))` / unpack
// sequences, and loop-carried accumulators are integer recurrences.  This
// hides FP reductions from the loop vectorizer and bloats scalar code.
//
// This pass splits such i64 "float pair" PHI recurrences back into two f32
// SSA values, recovering the float dataflow (analogous to what GCC's SRA
// does, but for the ABI-coerced representation).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_SCALAR_SPLITPACKEDFLOATPAIRS_H
#define LLVM_TRANSFORMS_SCALAR_SPLITPACKEDFLOATPAIRS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Function;

struct SplitPackedFloatPairsPass
    : public OptionalPassInfoMixin<SplitPackedFloatPairsPass> {
  LLVM_ABI PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_SPLITPACKEDFLOATPAIRS_H
