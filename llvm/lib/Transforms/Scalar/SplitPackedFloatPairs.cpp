//===- SplitPackedFloatPairs.cpp - recover FP lanes from ABI-packed i64 ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A small homogeneous floating-point aggregate, e.g. `struct { float m[4]; }`,
// is passed and returned in a pair of integer registers by several targets
// following their psABI (for RISC-V LP64D a 16-byte aggregate is passed in
// a0/a1).  Front-ends therefore coerce such aggregates to `[2 x i64]` in the
// IR, and after inlining and mem2reg the element-wise operations on the
// aggregate are represented as bit-packing on the i64 pair:
//
//   %acc  = phi i64 [ 0, %entry ], [ %pack, %loop ]
//   %lo   = bitcast (trunc i64 %acc) to float
//   %hi   = bitcast (trunc (lshr i64 %acc, 32)) to float
//   %nlo  = fadd float %lo, %x0
//   %nhi  = fadd float %hi, %x1
//   %pack = or disjoint (shl (zext (bitcast %nhi to i32) to i64), 32),
//                       (zext (bitcast %nlo to i32) to i64)
//
// The loop-carried value is then an integer `or` recurrence instead of a float
// reduction: the dataflow through the packed value is opaque to the optimizer
// (the loop vectorizer, in particular, no longer sees a reduction).  Scalar
// code is also full of pack/unpack around every float operation.
//
// This pass recognises this canonical "two floats packed into one i64" idiom on
// i64 PHIs and splits the PHI into two f32 PHIs, so the float dataflow becomes
// visible again.  It is deliberately conservative: it fires only when *every*
// incoming value is either a matching pack or a constant, and *every* use is a
// plain lane extract (trunc, or lshr-by-32 followed by trunc); anything else
// leaves the PHI untouched.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/SplitPackedFloatPairs.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "split-packed-float-pairs"

STATISTIC(NumSplit, "Number of packed float-pair PHIs split");

/// Match one lane of the canonical pack:
///
///   (zext (bitcast (F to i32) to i64))                  ; low lane
///   (shl (zext (bitcast (F to i32) to i64)), 32)        ; high lane
///
/// and return the f32 value \p F.
static bool matchLane(Value *V, bool High, Value *&F) {
  if (High && !match(V, m_Shl(m_Value(V), m_SpecificInt(32))))
    return false;

  if (!match(V, m_ZExt(m_Value(V))) || !V->getType()->isIntegerTy(32))
    return false;

  // The zext must widen a bitcast from f32, not an arbitrary i32.
  return match(V, m_BitCast(m_Value(F))) && F->getType()->isFloatTy();
}

/// Match `or (disjoint) <high lane>, <low lane>` in either operand order.
///
/// The `disjoint` flag is deliberately not required: the two zext'd i32 lanes
/// occupy disjoint bit ranges by construction, so a plain `or` has exactly the
/// same meaning.  This keeps the match working if the flag is dropped or
/// added later by another pass.
static bool matchPack(Value *V, Value *&Lo, Value *&Hi) {
  Value *Op0 = nullptr, *Op1 = nullptr;
  if (!match(V, m_Or(m_Value(Op0), m_Value(Op1))))
    return false;

  if (matchLane(Op0, /*High=*/true, Hi) &&
      matchLane(Op1, /*High=*/false, Lo))
    return true;

  if (matchLane(Op1, /*High=*/true, Hi) &&
      matchLane(Op0, /*High=*/false, Lo))
    return true;

  return false;
}

/// Decompose an incoming value into the (Lo, Hi) f32 pair, or return false.
/// \p IsPack is set when the value matched the canonical pack (rather than a
/// constant), so the caller can require at least one real pack.
static bool decompose(Value *V, Value *&Lo, Value *&Hi, bool &IsPack) {
  if (matchPack(V, Lo, Hi)) {
    IsPack = true;
    return true;
  }

  auto *C = dyn_cast<ConstantInt>(V);
  if (!C)
    return false;

  IsPack = false;

  LLVMContext &Ctx = V->getContext();
  Type *F32 = Type::getFloatTy(Ctx);
  uint64_t Bits = C->getZExtValue();
  Lo = ConstantFP::get(
      F32, APFloat(APFloat::IEEEsingle(),
                   APInt(32, static_cast<uint32_t>(Bits & 0xffffffffu))));
  Hi = ConstantFP::get(
      F32, APFloat(APFloat::IEEEsingle(),
                   APInt(32, static_cast<uint32_t>(Bits >> 32))));
  return true;
}

/// True if the only uses of \p P are plain lane extracts: `trunc`, or
/// `lshr P, 32` whose only users are `trunc`.
static bool usesAreLaneExtracts(PHINode *P) {
  if (P->use_empty())
    return false;

  for (User *U : P->users()) {
    if (auto *T = dyn_cast<TruncInst>(U)) {
      if (!T->getType()->isIntegerTy(32))
        return false;
      continue;
    }

    auto *Sh = dyn_cast<BinaryOperator>(U);
    if (!Sh || Sh->getOpcode() != Instruction::LShr ||
        Sh->getOperand(0) != P)
      return false;

    auto *C = dyn_cast<ConstantInt>(Sh->getOperand(1));
    if (!C || C->getZExtValue() != 32)
      return false;

    for (User *U2 : Sh->users()) {
      auto *T2 = dyn_cast<TruncInst>(U2);
      if (!T2 || !T2->getType()->isIntegerTy(32))
        return false;
    }
  }
  return true;
}

/// Try to split a single i64 PHI that carries two packed floats into two f32
/// PHIs.  Returns true if the IR was changed.
static bool splitI64Phi(PHINode *P) {
  SmallVector<std::pair<Value *, Value *>, 8> Incoming;
  SmallVector<Value *, 8> OldIncoming;
  bool SawPack = false;

  for (unsigned K = 0, E = P->getNumIncomingValues(); K != E; ++K) {
    Value *V = P->getIncomingValue(K);
    Value *Lo = nullptr, *Hi = nullptr;
    bool IsPack = false;
    if (!decompose(V, Lo, Hi, IsPack))
      return false;
    SawPack |= IsPack;
    Incoming.push_back({Lo, Hi});
    OldIncoming.push_back(V);
  }

  // Constants alone are not interesting: require at least one real pack.  All
  // uses must be lane extracts, otherwise the packed value is used as an
  // integer and splitting it would not remove the pack/unpack traffic.
  if (!SawPack || !usesAreLaneExtracts(P))
    return false;

  LLVM_DEBUG(dbgs() << "Splitting packed float pair: " << *P << "\n");

  LLVMContext &Ctx = P->getContext();
  Type *F32 = Type::getFloatTy(Ctx);
  IRBuilder<> B(P);
  PHINode *LoPhi = B.CreatePHI(F32, P->getNumIncomingValues(),
                               P->getName() + ".lo");
  PHINode *HiPhi = B.CreatePHI(F32, P->getNumIncomingValues(),
                               P->getName() + ".hi");
  for (unsigned K = 0, E = P->getNumIncomingValues(); K != E; ++K) {
    LoPhi->addIncoming(Incoming[K].first, P->getIncomingBlock(K));
    HiPhi->addIncoming(Incoming[K].second, P->getIncomingBlock(K));
  }

  SmallVector<Instruction *, 8> Dead;
  for (User *U : make_early_inc_range(P->users())) {
    if (auto *T = dyn_cast<TruncInst>(U)) {
      IRBuilder<> UB(T);
      T->replaceAllUsesWith(UB.CreateBitCast(LoPhi, T->getType()));
      Dead.push_back(T);
      continue;
    }

    auto *Sh = cast<BinaryOperator>(U); // lshr, validated by usesAreLaneExtracts
    for (User *U2 : make_early_inc_range(Sh->users())) {
      auto *T2 = cast<TruncInst>(U2);
      IRBuilder<> UB(T2);
      T2->replaceAllUsesWith(UB.CreateBitCast(HiPhi, T2->getType()));
      Dead.push_back(T2);
    }
    Dead.push_back(Sh);
  }

  for (Instruction *D : Dead)
    if (D->use_empty())
      D->eraseFromParent();

  if (P->use_empty())
    P->eraseFromParent();

  // The old pack chain (`or`/`shl`/`zext`/`bitcast`) is dead unless something
  // else still uses it.  Delete it (and whatever it makes dead) now instead of
  // re-scanning the whole function.
  for (Value *V : OldIncoming)
    RecursivelyDeleteTriviallyDeadInstructions(V);

  ++NumSplit;
  return true;
}

PreservedAnalyses SplitPackedFloatPairsPass::run(Function &F,
                                                 FunctionAnalysisManager &) {
  bool Changed = false;
  bool LocalChanged = true;
  while (LocalChanged) {
    LocalChanged = false;
    // Snapshot the candidate PHIs first: splitI64Phi() erases IR, which would
    // invalidate a walk over the instructions being modified.  A split can
    // expose another packed PHI through the bitcasts that replace the lane
    // extracts, so iterate to a fixed point.
    SmallVector<PHINode *, 16> Phis;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        auto *P = dyn_cast<PHINode>(&I);
        if (P && P->getType()->isIntegerTy(64))
          Phis.push_back(P);
      }
    for (PHINode *P : Phis)
      if (splitI64Phi(P)) {
        LocalChanged = true;
        Changed = true;
      }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
