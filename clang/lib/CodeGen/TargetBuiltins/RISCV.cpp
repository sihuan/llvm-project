//===-------- RISCV.cpp - Emit LLVM Code for builtins ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit Builtin calls as LLVM code.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/IR/IntrinsicsRISCV.h"
#include "llvm/TargetParser/RISCVISAInfo.h"
#include "llvm/TargetParser/RISCVTargetParser.h"

using namespace clang;
using namespace CodeGen;
using namespace llvm;

// The 0th bit simulates the `vta` of RVV
// The 1st bit simulates the `vma` of RVV
static constexpr unsigned RVV_VTA = 0x1;
static constexpr unsigned RVV_VMA = 0x2;

// RISC-V Vector builtin helper functions are marked NOINLINE to prevent
// excessive inlining in CodeGenFunction::EmitRISCVBuiltinExpr's large switch
// statement, which would significantly increase compilation time.
static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVVLEFFBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                    ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                    Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    // Move mask to right before vl.
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    IntrinsicTypes = {ResultType, Ops[4]->getType(), Ops[2]->getType()};
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
    IntrinsicTypes = {ResultType, Ops[3]->getType(), Ops[1]->getType()};
  }
  Value *NewVL = Ops[2];
  Ops.erase(Ops.begin() + 2);
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  llvm::Value *LoadValue = Builder.CreateCall(F, Ops, "");
  llvm::Value *V = Builder.CreateExtractValue(LoadValue, {0});
  // Store new_vl.
  clang::CharUnits Align;
  if (IsMasked)
    Align = CGM.getNaturalPointeeTypeAlignment(
        E->getArg(E->getNumArgs() - 2)->getType());
  else
    Align = CGM.getNaturalPointeeTypeAlignment(E->getArg(1)->getType());
  llvm::Value *Val = Builder.CreateExtractValue(LoadValue, {1});
  Builder.CreateStore(Val, Address(NewVL, Val->getType(), Align));
  return V;
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVVSSEBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                   ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                   Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                   int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    // Builtin: (mask, ptr, stride, value, vl). Intrinsic: (value, ptr, stride,
    // mask, vl)
    std::swap(Ops[0], Ops[3]);
  } else {
    // Builtin: (ptr, stride, value, vl). Intrinsic: (value, ptr, stride, vl)
    std::rotate(Ops.begin(), Ops.begin() + 2, Ops.begin() + 3);
  }
  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[4]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[3]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVIndexedStoreBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  if (IsMasked) {
    // Builtin: (mask, ptr, index, value, vl).
    // Intrinsic: (value, ptr, index, mask, vl)
    std::swap(Ops[0], Ops[3]);
  } else {
    // Builtin: (ptr, index, value, vl).
    // Intrinsic: (value, ptr, index, vl)
    std::rotate(Ops.begin(), Ops.begin() + 2, Ops.begin() + 3);
  }
  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType(),
                      Ops[4]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVPseudoUnaryBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                          ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                          Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                          int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  }
  auto ElemTy = cast<llvm::VectorType>(ResultType)->getElementType();
  Ops.insert(Ops.begin() + 2, llvm::Constant::getNullValue(ElemTy));
  if (IsMasked) {
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    // maskedoff, op1, op2, mask, vl, policy
    IntrinsicTypes = {ResultType, ElemTy, Ops[4]->getType()};
  } else {
    // passthru, op1, op2, vl
    IntrinsicTypes = {ResultType, ElemTy, Ops[3]->getType()};
  }
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVPseudoVNotBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                         ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                         Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                         int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  }
  auto ElemTy = cast<llvm::VectorType>(ResultType)->getElementType();
  Ops.insert(Ops.begin() + 2, llvm::Constant::getAllOnesValue(ElemTy));
  if (IsMasked) {
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    // maskedoff, op1, po2, mask, vl, policy
    IntrinsicTypes = {ResultType, ElemTy, Ops[4]->getType()};
  } else {
    // passthru, op1, op2, vl
    IntrinsicTypes = {ResultType, ElemTy, Ops[3]->getType()};
  }
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVPseudoMaskBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                         ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                         Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                         int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  // op1, vl
  IntrinsicTypes = {ResultType, Ops[1]->getType()};
  Ops.insert(Ops.begin() + 1, Ops[0]);
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVPseudoVFUnaryBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
    Ops.insert(Ops.begin() + 2, Ops[1]);
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    // maskedoff, op1, op2, mask, vl
    IntrinsicTypes = {ResultType, Ops[2]->getType(), Ops.back()->getType()};
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
    // op1, po2, vl
    IntrinsicTypes = {ResultType, Ops[1]->getType(), Ops[2]->getType()};
    Ops.insert(Ops.begin() + 2, Ops[1]);
  }
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVPseudoVWCVTBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                          ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                          Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                          int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  if (IsMasked) {
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  }
  auto ElemTy = cast<llvm::VectorType>(Ops[1]->getType())->getElementType();
  Ops.insert(Ops.begin() + 2, llvm::Constant::getNullValue(ElemTy));
  if (IsMasked) {
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    // maskedoff, op1, op2, mask, vl, policy
    IntrinsicTypes = {ResultType, Ops[1]->getType(), ElemTy, Ops[4]->getType()};
  } else {
    // passtru, op1, op2, vl
    IntrinsicTypes = {ResultType, Ops[1]->getType(), ElemTy, Ops[3]->getType()};
  }
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVPseudoVNCVTBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                          ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                          Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                          int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  if (IsMasked) {
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
    if ((PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA))
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  } else {
    if (PolicyAttrs & RVV_VTA)
      Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  }
  Ops.insert(Ops.begin() + 2,
             llvm::Constant::getNullValue(Ops.back()->getType()));
  if (IsMasked) {
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
    // maskedoff, op1, xlen, mask, vl
    IntrinsicTypes = {ResultType, Ops[1]->getType(), Ops[4]->getType(),
                      Ops[4]->getType()};
  } else {
    // passthru, op1, xlen, vl
    IntrinsicTypes = {ResultType, Ops[1]->getType(), Ops[3]->getType(),
                      Ops[3]->getType()};
  }
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVVlenbBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                    ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                    Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  LLVMContext &Context = CGM.getLLVMContext();
  llvm::MDBuilder MDHelper(Context);
  llvm::Metadata *OpsMD[] = {llvm::MDString::get(Context, "vlenb")};
  llvm::MDNode *RegName = llvm::MDNode::get(Context, OpsMD);
  llvm::Value *Metadata = llvm::MetadataAsValue::get(Context, RegName);
  llvm::Function *F =
      CGM.getIntrinsic(llvm::Intrinsic::read_register, {CGF->SizeTy});
  return Builder.CreateCall(F, Metadata);
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVVsetvliBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                      ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                      Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                      int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::Function *F = CGM.getIntrinsic(ID, {ResultType});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVVSEMaskBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                      ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                      Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                      int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  if (IsMasked) {
    // Builtin: (mask, ptr, value, vl).
    // Intrinsic: (value, ptr, mask, vl)
    std::swap(Ops[0], Ops[2]);
  } else {
    // Builtin: (ptr, value, vl).
    // Intrinsic: (value, ptr, vl)
    std::swap(Ops[0], Ops[1]);
  }
  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[3]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVUnitStridedSegLoadTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  bool NoPassthru =
      (IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) |
      (!IsMasked && (PolicyAttrs & RVV_VTA));
  unsigned Offset = IsMasked ? NoPassthru ? 1 : 2 : NoPassthru ? 0 : 1;
  if (IsMasked)
    IntrinsicTypes = {ResultType, Ops[Offset]->getType(), Ops[0]->getType(),
                      Ops.back()->getType()};
  else
    IntrinsicTypes = {ResultType, Ops[Offset]->getType(),
                      Ops.back()->getType()};
  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
  if (NoPassthru)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  llvm::Value *LoadValue = Builder.CreateCall(F, Ops, "");
  if (ReturnValue.isNull())
    return LoadValue;
  return Builder.CreateStore(LoadValue, ReturnValue.getValue());
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVUnitStridedSegStoreTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  // Masked
  // Builtin: (mask, ptr, v_tuple, vl)
  // Intrinsic: (tuple, ptr, mask, vl, SegInstSEW)
  // Unmasked
  // Builtin: (ptr, v_tuple, vl)
  // Intrinsic: (tuple, ptr, vl, SegInstSEW)
  if (IsMasked)
    std::swap(Ops[0], Ops[2]);
  else
    std::swap(Ops[0], Ops[1]);
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));
  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVUnitStridedSegLoadFFTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  bool NoPassthru =
      (IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) |
      (!IsMasked && (PolicyAttrs & RVV_VTA));
  unsigned Offset = IsMasked ? NoPassthru ? 1 : 2 : NoPassthru ? 0 : 1;
  if (IsMasked)
    IntrinsicTypes = {ResultType, Ops.back()->getType(), Ops[Offset]->getType(),
                      Ops[0]->getType()};
  else
    IntrinsicTypes = {ResultType, Ops.back()->getType(),
                      Ops[Offset]->getType()};
  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
  if (NoPassthru)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));
  Value *NewVL = Ops[2];
  Ops.erase(Ops.begin() + 2);
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  llvm::Value *LoadValue = Builder.CreateCall(F, Ops, "");
  // Get alignment from the new vl operand
  clang::CharUnits Align =
      CGM.getNaturalPointeeTypeAlignment(E->getArg(Offset + 1)->getType());
  llvm::Value *ReturnTuple = Builder.CreateExtractValue(LoadValue, 0);
  // Store new_vl
  llvm::Value *V = Builder.CreateExtractValue(LoadValue, 1);
  Builder.CreateStore(V, Address(NewVL, V->getType(), Align));
  if (ReturnValue.isNull())
    return ReturnTuple;
  return Builder.CreateStore(ReturnTuple, ReturnValue.getValue());
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVStridedSegLoadTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  bool NoPassthru =
      (IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) |
      (!IsMasked && (PolicyAttrs & RVV_VTA));
  unsigned Offset = IsMasked ? NoPassthru ? 1 : 2 : NoPassthru ? 0 : 1;
  if (IsMasked)
    IntrinsicTypes = {ResultType, Ops[Offset]->getType(), Ops.back()->getType(),
                      Ops[0]->getType()};
  else
    IntrinsicTypes = {ResultType, Ops[Offset]->getType(),
                      Ops.back()->getType()};
  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
  if (NoPassthru)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));
  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  llvm::Value *LoadValue = Builder.CreateCall(F, Ops, "");
  if (ReturnValue.isNull())
    return LoadValue;
  return Builder.CreateStore(LoadValue, ReturnValue.getValue());
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVStridedSegStoreTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 4> IntrinsicTypes;
  // Masked
  // Builtin: (mask, ptr, stride, v_tuple, vl)
  // Intrinsic: (tuple, ptr, stride, mask, vl, SegInstSEW)
  // Unmasked
  // Builtin: (ptr, stride, v_tuple, vl)
  // Intrinsic: (tuple, ptr, stride, vl, SegInstSEW)
  if (IsMasked)
    std::swap(Ops[0], Ops[3]);
  else
    std::rotate(Ops.begin(), Ops.begin() + 2, Ops.begin() + 3);
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));
  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[4]->getType(),
                      Ops[3]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[3]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVAveragingBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                        ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                        Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                        int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, op1, round_mode, vl)
  // Masked:   (passthru, vector_in, vector_in/scalar_in, mask, vxrm, vl,
  // policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F = CGM.getIntrinsic(
      ID, {ResultType, Ops[2]->getType(), Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVNarrowingClipBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, op1, round_mode, vl)
  // Masked:   (passthru, vector_in, vector_in/scalar_in, mask, vxrm, vl,
  // policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F =
      CGM.getIntrinsic(ID, {ResultType, Ops[1]->getType(), Ops[2]->getType(),
                            Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVFloatingPointBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, op1, round_mode, vl)
  // Masked:   (passthru, vector_in, vector_in/scalar_in, mask, frm, vl, policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));
  bool HasRoundModeOp =
      IsMasked ? (HasMaskedOff ? Ops.size() == 6 : Ops.size() == 5)
               : (HasMaskedOff ? Ops.size() == 5 : Ops.size() == 4);

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F = CGM.getIntrinsic(
      ID, {ResultType, Ops[2]->getType(), Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVWideningFloatingPointBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, op1, round_mode, vl)
  // Masked:   (passthru, vector_in, vector_in/scalar_in, mask, frm, vl, policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));
  bool HasRoundModeOp =
      IsMasked ? (HasMaskedOff ? Ops.size() == 6 : Ops.size() == 5)
               : (HasMaskedOff ? Ops.size() == 5 : Ops.size() == 4);

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F =
      CGM.getIntrinsic(ID, {ResultType, Ops[1]->getType(), Ops[2]->getType(),
                            Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVIndexedSegLoadTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 5> IntrinsicTypes;

  bool NoPassthru =
      (IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) |
      (!IsMasked && (PolicyAttrs & RVV_VTA));

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 1);
  if (NoPassthru)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));
  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));

  if (IsMasked)
    IntrinsicTypes = {ResultType, Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType(), Ops[4]->getType()};
  else
    IntrinsicTypes = {ResultType, Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  llvm::Value *LoadValue = Builder.CreateCall(F, Ops, "");

  if (ReturnValue.isNull())
    return LoadValue;
  return Builder.CreateStore(LoadValue, ReturnValue.getValue());
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVIndexedSegStoreTupleBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 5> IntrinsicTypes;
  // Masked
  // Builtin: (mask, ptr, index, v_tuple, vl)
  // Intrinsic: (tuple, ptr, index, mask, vl, SegInstSEW)
  // Unmasked
  // Builtin: (ptr, index, v_tuple, vl)
  // Intrinsic: (tuple, ptr, index, vl, SegInstSEW)

  if (IsMasked)
    std::swap(Ops[0], Ops[3]);
  else
    std::rotate(Ops.begin(), Ops.begin() + 2, Ops.begin() + 3);

  Ops.push_back(ConstantInt::get(Ops.back()->getType(), SegInstSEW));

  if (IsMasked)
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType(), Ops[4]->getType()};
  else
    IntrinsicTypes = {Ops[0]->getType(), Ops[1]->getType(), Ops[2]->getType(),
                      Ops[3]->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVFMABuiltin(CodeGenFunction *CGF, const CallExpr *E,
                  ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                  Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                  int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (vector_in, vector_in/scalar_in, vector_in, round_mode,
  //            vl, policy)
  // Masked:   (vector_in, vector_in/scalar_in, vector_in, mask, frm,
  //            vl, policy)

  bool HasRoundModeOp = IsMasked ? Ops.size() == 6 : Ops.size() == 5;

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F = CGM.getIntrinsic(
      ID, {ResultType, Ops[1]->getType(), Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVWideningFMABuiltin(CodeGenFunction *CGF, const CallExpr *E,
                          ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                          Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                          int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (vector_in, vector_in/scalar_in, vector_in, round_mode, vl,
  // policy) Masked:   (vector_in, vector_in/scalar_in, vector_in, mask, frm,
  // vl, policy)

  bool HasRoundModeOp = IsMasked ? Ops.size() == 6 : Ops.size() == 5;

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.begin() + 4);

  Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F =
      CGM.getIntrinsic(ID, {ResultType, Ops[1]->getType(), Ops[2]->getType(),
                            Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVFloatingUnaryBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  llvm::SmallVector<llvm::Type *, 3> IntrinsicTypes;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, round_mode, vl)
  // Masked:   (passthru, op0, mask, frm, vl, policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));
  bool HasRoundModeOp =
      IsMasked ? (HasMaskedOff ? Ops.size() == 5 : Ops.size() == 4)
               : (HasMaskedOff ? Ops.size() == 4 : Ops.size() == 3);

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  IntrinsicTypes = {ResultType, Ops.back()->getType()};
  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVFloatingConvBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, frm, vl)
  // Masked:   (passthru, op0, mask, frm, vl, policy)
  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));
  bool HasRoundModeOp =
      IsMasked ? (HasMaskedOff ? Ops.size() == 5 : Ops.size() == 4)
               : (HasMaskedOff ? Ops.size() == 4 : Ops.size() == 3);

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  if (IsMasked)
    Ops.push_back(ConstantInt::get(Ops.back()->getType(), PolicyAttrs));

  llvm::Function *F = CGM.getIntrinsic(
      ID, {ResultType, Ops[1]->getType(), Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *emitRVVFloatingReductionBuiltin(
    CodeGenFunction *CGF, const CallExpr *E, ReturnValueSlot ReturnValue,
    llvm::Type *ResultType, Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
    int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;
  // LLVM intrinsic
  // Unmasked: (passthru, op0, op1, round_mode, vl)
  // Masked:   (passthru, vector_in, vector_in/scalar_in, mask, frm, vl, policy)

  bool HasMaskedOff =
      !((IsMasked && (PolicyAttrs & RVV_VTA) && (PolicyAttrs & RVV_VMA)) ||
        (!IsMasked && PolicyAttrs & RVV_VTA));
  bool HasRoundModeOp =
      IsMasked ? (HasMaskedOff ? Ops.size() == 6 : Ops.size() == 5)
               : (HasMaskedOff ? Ops.size() == 5 : Ops.size() == 4);

  if (!HasRoundModeOp)
    Ops.insert(Ops.end() - 1,
               ConstantInt::get(Ops.back()->getType(), 7)); // frm

  if (IsMasked)
    std::rotate(Ops.begin(), Ops.begin() + 1, Ops.end() - 2);

  if (!HasMaskedOff)
    Ops.insert(Ops.begin(), llvm::PoisonValue::get(ResultType));

  llvm::Function *F = CGM.getIntrinsic(
      ID, {ResultType, Ops[1]->getType(), Ops.back()->getType()});
  return Builder.CreateCall(F, Ops, "");
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVReinterpretBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                          ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                          Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                          int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto &CGM = CGF->CGM;

  if (ResultType->isIntOrIntVectorTy(1) ||
      Ops[0]->getType()->isIntOrIntVectorTy(1)) {
    assert(isa<ScalableVectorType>(ResultType) &&
           isa<ScalableVectorType>(Ops[0]->getType()));

    LLVMContext &Context = CGM.getLLVMContext();
    ScalableVectorType *Boolean64Ty =
        ScalableVectorType::get(llvm::Type::getInt1Ty(Context), 64);

    if (ResultType->isIntOrIntVectorTy(1)) {
      // Casting from m1 vector integer -> vector boolean
      // Ex: <vscale x 8 x i8>
      //     --(bitcast)--------> <vscale x 64 x i1>
      //     --(vector_extract)-> <vscale x  8 x i1>
      llvm::Value *BitCast = Builder.CreateBitCast(Ops[0], Boolean64Ty);
      return Builder.CreateExtractVector(ResultType, BitCast,
                                         ConstantInt::get(CGF->Int64Ty, 0));
    } else {
      // Casting from vector boolean -> m1 vector integer
      // Ex: <vscale x  1 x i1>
      //       --(vector_insert)-> <vscale x 64 x i1>
      //       --(bitcast)-------> <vscale x  8 x i8>
      llvm::Value *Boolean64Val = Builder.CreateInsertVector(
          Boolean64Ty, llvm::PoisonValue::get(Boolean64Ty), Ops[0],
          ConstantInt::get(CGF->Int64Ty, 0));
      return Builder.CreateBitCast(Boolean64Val, ResultType);
    }
  }
  return Builder.CreateBitCast(Ops[0], ResultType);
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVGetBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                  ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                  Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                  int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  auto *VecTy = cast<ScalableVectorType>(ResultType);
  if (auto *OpVecTy = dyn_cast<ScalableVectorType>(Ops[0]->getType())) {
    unsigned MaxIndex =
        OpVecTy->getMinNumElements() / VecTy->getMinNumElements();
    assert(isPowerOf2_32(MaxIndex));
    // Mask to only valid indices.
    Ops[1] = Builder.CreateZExt(Ops[1], Builder.getInt64Ty());
    Ops[1] = Builder.CreateAnd(Ops[1], MaxIndex - 1);
    Ops[1] =
        Builder.CreateMul(Ops[1], ConstantInt::get(Ops[1]->getType(),
                                                   VecTy->getMinNumElements()));
    return Builder.CreateExtractVector(ResultType, Ops[0], Ops[1]);
  }

  return Builder.CreateIntrinsic(
      Intrinsic::riscv_tuple_extract, {ResultType, Ops[0]->getType()},
      {Ops[0], Builder.CreateTrunc(Ops[1], Builder.getInt32Ty())});
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVSetBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                  ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                  Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                  int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  if (auto *ResVecTy = dyn_cast<ScalableVectorType>(ResultType)) {
    auto *VecTy = cast<ScalableVectorType>(Ops[2]->getType());
    unsigned MaxIndex =
        ResVecTy->getMinNumElements() / VecTy->getMinNumElements();
    assert(isPowerOf2_32(MaxIndex));
    // Mask to only valid indices.
    Ops[1] = Builder.CreateZExt(Ops[1], Builder.getInt64Ty());
    Ops[1] = Builder.CreateAnd(Ops[1], MaxIndex - 1);
    Ops[1] =
        Builder.CreateMul(Ops[1], ConstantInt::get(Ops[1]->getType(),
                                                   VecTy->getMinNumElements()));
    return Builder.CreateInsertVector(ResultType, Ops[0], Ops[2], Ops[1]);
  }

  return Builder.CreateIntrinsic(
      Intrinsic::riscv_tuple_insert, {ResultType, Ops[2]->getType()},
      {Ops[0], Ops[2], Builder.CreateTrunc(Ops[1], Builder.getInt32Ty())});
}

static LLVM_ATTRIBUTE_NOINLINE Value *
emitRVVCreateBuiltin(CodeGenFunction *CGF, const CallExpr *E,
                     ReturnValueSlot ReturnValue, llvm::Type *ResultType,
                     Intrinsic::ID ID, SmallVectorImpl<Value *> &Ops,
                     int PolicyAttrs, bool IsMasked, unsigned SegInstSEW) {
  auto &Builder = CGF->Builder;
  llvm::Value *ReturnVector = llvm::PoisonValue::get(ResultType);
  auto *VecTy = cast<ScalableVectorType>(Ops[0]->getType());
  for (unsigned I = 0, N = Ops.size(); I < N; ++I) {
    if (isa<ScalableVectorType>(ResultType)) {
      llvm::Value *Idx = ConstantInt::get(Builder.getInt64Ty(),
                                          VecTy->getMinNumElements() * I);
      ReturnVector =
          Builder.CreateInsertVector(ResultType, ReturnVector, Ops[I], Idx);
    } else {
      llvm::Value *Idx = ConstantInt::get(Builder.getInt32Ty(), I);
      ReturnVector = Builder.CreateIntrinsic(Intrinsic::riscv_tuple_insert,
                                             {ResultType, Ops[I]->getType()},
                                             {ReturnVector, Ops[I], Idx});
    }
  }
  return ReturnVector;
}

Value *CodeGenFunction::EmitRISCVCpuInit() {
  llvm::FunctionType *FTy = llvm::FunctionType::get(VoidTy, {VoidPtrTy}, false);
  llvm::FunctionCallee Func =
      CGM.CreateRuntimeFunction(FTy, "__init_riscv_feature_bits");
  auto *CalleeGV = cast<llvm::GlobalValue>(Func.getCallee());
  CalleeGV->setDSOLocal(true);
  CalleeGV->setDLLStorageClass(llvm::GlobalValue::DefaultStorageClass);
  return Builder.CreateCall(Func, {llvm::ConstantPointerNull::get(VoidPtrTy)});
}

Value *CodeGenFunction::EmitRISCVCpuSupports(const CallExpr *E) {

  const Expr *FeatureExpr = E->getArg(0)->IgnoreParenCasts();
  StringRef FeatureStr = cast<StringLiteral>(FeatureExpr)->getString();
  if (!getContext().getTargetInfo().validateCpuSupports(FeatureStr))
    return Builder.getFalse();

  return EmitRISCVCpuSupports(ArrayRef<StringRef>(FeatureStr));
}

static Value *loadRISCVFeatureBits(unsigned Index, CGBuilderTy &Builder,
                                   CodeGenModule &CGM) {
  llvm::Type *Int32Ty = Builder.getInt32Ty();
  llvm::Type *Int64Ty = Builder.getInt64Ty();
  llvm::ArrayType *ArrayOfInt64Ty =
      llvm::ArrayType::get(Int64Ty, llvm::RISCVISAInfo::FeatureBitSize);
  llvm::Type *StructTy = llvm::StructType::get(Int32Ty, ArrayOfInt64Ty);
  llvm::Constant *RISCVFeaturesBits =
      CGM.CreateRuntimeVariable(StructTy, "__riscv_feature_bits");
  cast<llvm::GlobalValue>(RISCVFeaturesBits)->setDSOLocal(true);
  Value *IndexVal = llvm::ConstantInt::get(Int32Ty, Index);
  llvm::Value *GEPIndices[] = {Builder.getInt32(0), Builder.getInt32(1),
                               IndexVal};
  Value *Ptr =
      Builder.CreateInBoundsGEP(StructTy, RISCVFeaturesBits, GEPIndices);
  Value *FeaturesBit =
      Builder.CreateAlignedLoad(Int64Ty, Ptr, CharUnits::fromQuantity(8));
  return FeaturesBit;
}

Value *CodeGenFunction::EmitRISCVCpuSupports(ArrayRef<StringRef> FeaturesStrs) {
  const unsigned RISCVFeatureLength = llvm::RISCVISAInfo::FeatureBitSize;
  uint64_t RequireBitMasks[RISCVFeatureLength] = {0};

  for (auto Feat : FeaturesStrs) {
    auto [GroupID, BitPos] = RISCVISAInfo::getRISCVFeaturesBitsInfo(Feat);

    // If there isn't BitPos for this feature, skip this version.
    // It also report the warning to user during compilation.
    if (BitPos == -1)
      return Builder.getFalse();

    RequireBitMasks[GroupID] |= (1ULL << BitPos);
  }

  Value *Result = nullptr;
  for (unsigned Idx = 0; Idx < RISCVFeatureLength; Idx++) {
    if (RequireBitMasks[Idx] == 0)
      continue;

    Value *Mask = Builder.getInt64(RequireBitMasks[Idx]);
    Value *Bitset =
        Builder.CreateAnd(loadRISCVFeatureBits(Idx, Builder, CGM), Mask);
    Value *CmpV = Builder.CreateICmpEQ(Bitset, Mask);
    Result = (!Result) ? CmpV : Builder.CreateAnd(Result, CmpV);
  }

  assert(Result && "Should have value here.");

  return Result;
}

Value *CodeGenFunction::EmitRISCVCpuIs(const CallExpr *E) {
  const Expr *CPUExpr = E->getArg(0)->IgnoreParenCasts();
  StringRef CPUStr = cast<clang::StringLiteral>(CPUExpr)->getString();
  return EmitRISCVCpuIs(CPUStr);
}

Value *CodeGenFunction::EmitRISCVCpuIs(StringRef CPUStr) {
  llvm::Type *Int32Ty = Builder.getInt32Ty();
  llvm::Type *Int64Ty = Builder.getInt64Ty();
  llvm::StructType *StructTy = llvm::StructType::get(Int32Ty, Int64Ty, Int64Ty);
  llvm::Constant *RISCVCPUModel =
      CGM.CreateRuntimeVariable(StructTy, "__riscv_cpu_model");
  cast<llvm::GlobalValue>(RISCVCPUModel)->setDSOLocal(true);

  auto loadRISCVCPUID = [&](unsigned Index) {
    Value *Ptr = Builder.CreateStructGEP(StructTy, RISCVCPUModel, Index);
    Value *CPUID = Builder.CreateAlignedLoad(StructTy->getTypeAtIndex(Index),
                                             Ptr, llvm::MaybeAlign());
    return CPUID;
  };

  const llvm::RISCV::CPUModel Model = llvm::RISCV::getCPUModel(CPUStr);

  // Compare mvendorid.
  Value *VendorID = loadRISCVCPUID(0);
  Value *Result =
      Builder.CreateICmpEQ(VendorID, Builder.getInt32(Model.MVendorID));

  // Compare marchid.
  Value *ArchID = loadRISCVCPUID(1);
  Result = Builder.CreateAnd(
      Result, Builder.CreateICmpEQ(ArchID, Builder.getInt64(Model.MArchID)));

  // Compare mimpid.
  Value *ImpID = loadRISCVCPUID(2);
  Result = Builder.CreateAnd(
      Result, Builder.CreateICmpEQ(ImpID, Builder.getInt64(Model.MImpID)));

  return Result;
}

Value *CodeGenFunction::EmitRISCVBuiltinExpr(unsigned BuiltinID,
                                             const CallExpr *E,
                                             ReturnValueSlot ReturnValue) {

  if (BuiltinID == Builtin::BI__builtin_cpu_supports)
    return EmitRISCVCpuSupports(E);
  if (BuiltinID == Builtin::BI__builtin_cpu_init)
    return EmitRISCVCpuInit();
  if (BuiltinID == Builtin::BI__builtin_cpu_is)
    return EmitRISCVCpuIs(E);

  SmallVector<Value *, 4> Ops;
  llvm::Type *ResultType = ConvertType(E->getType());

  // Find out if any arguments are required to be integer constant expressions.
  unsigned ICEArguments = 0;
  ASTContext::GetBuiltinTypeError Error;
  getContext().GetBuiltinType(BuiltinID, Error, &ICEArguments);
  if (Error == ASTContext::GE_Missing_type) {
    // Vector intrinsics don't have a type string.
    assert(BuiltinID >= clang::RISCV::FirstRVVBuiltin &&
           BuiltinID <= clang::RISCV::LastRVVBuiltin);
    ICEArguments = 0;
    if (BuiltinID == RISCVVector::BI__builtin_rvv_vget_v ||
        BuiltinID == RISCVVector::BI__builtin_rvv_vset_v)
      ICEArguments = 1 << 1;
  } else {
    assert(Error == ASTContext::GE_None && "Unexpected error");
  }

  if (BuiltinID == RISCV::BI__builtin_riscv_ntl_load)
    ICEArguments |= (1 << 1);
  if (BuiltinID == RISCV::BI__builtin_riscv_ntl_store)
    ICEArguments |= (1 << 2);

  for (unsigned i = 0, e = E->getNumArgs(); i != e; i++) {
    // Handle aggregate argument, namely RVV tuple types in segment load/store
    if (hasAggregateEvaluationKind(E->getArg(i)->getType())) {
      LValue L = EmitAggExprToLValue(E->getArg(i));
      llvm::Value *AggValue = Builder.CreateLoad(L.getAddress());
      Ops.push_back(AggValue);
      continue;
    }
    Ops.push_back(EmitScalarOrConstFoldImmArg(ICEArguments, i, E));
  }

  Intrinsic::ID ID = Intrinsic::not_intrinsic;
  int PolicyAttrs = 0;
  bool IsMasked = false;
  // This is used by segment load/store to determine it's llvm type.
  unsigned SegInstSEW = 8;
  // This is used by XSfmm.
  unsigned TWiden = 0;

  // Required for overloaded intrinsics.
  llvm::SmallVector<llvm::Type *, 2> IntrinsicTypes;
  switch (BuiltinID) {
  default: llvm_unreachable("unexpected builtin ID");
  case RISCV::BI__builtin_riscv_orc_b_32:
  case RISCV::BI__builtin_riscv_orc_b_64:
  case RISCV::BI__builtin_riscv_clmul_32:
  case RISCV::BI__builtin_riscv_clmul_64:
  case RISCV::BI__builtin_riscv_clmulh_32:
  case RISCV::BI__builtin_riscv_clmulh_64:
  case RISCV::BI__builtin_riscv_clmulr_32:
  case RISCV::BI__builtin_riscv_clmulr_64:
  case RISCV::BI__builtin_riscv_xperm4_32:
  case RISCV::BI__builtin_riscv_xperm4_64:
  case RISCV::BI__builtin_riscv_xperm8_32:
  case RISCV::BI__builtin_riscv_xperm8_64:
  case RISCV::BI__builtin_riscv_brev8_32:
  case RISCV::BI__builtin_riscv_brev8_64:
  case RISCV::BI__builtin_riscv_zip_32:
  case RISCV::BI__builtin_riscv_unzip_32: {
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin ID");
    // Zbb
    case RISCV::BI__builtin_riscv_orc_b_32:
    case RISCV::BI__builtin_riscv_orc_b_64:
      ID = Intrinsic::riscv_orc_b;
      break;

    // Zbc
    case RISCV::BI__builtin_riscv_clmul_32:
    case RISCV::BI__builtin_riscv_clmul_64:
      ID = Intrinsic::clmul;
      break;
    case RISCV::BI__builtin_riscv_clmulh_32:
    case RISCV::BI__builtin_riscv_clmulh_64:
      ID = Intrinsic::riscv_clmulh;
      break;
    case RISCV::BI__builtin_riscv_clmulr_32:
    case RISCV::BI__builtin_riscv_clmulr_64:
      ID = Intrinsic::riscv_clmulr;
      break;

    // Zbkx
    case RISCV::BI__builtin_riscv_xperm8_32:
    case RISCV::BI__builtin_riscv_xperm8_64:
      ID = Intrinsic::riscv_xperm8;
      break;
    case RISCV::BI__builtin_riscv_xperm4_32:
    case RISCV::BI__builtin_riscv_xperm4_64:
      ID = Intrinsic::riscv_xperm4;
      break;

    // Zbkb
    case RISCV::BI__builtin_riscv_brev8_32:
    case RISCV::BI__builtin_riscv_brev8_64:
      ID = Intrinsic::riscv_brev8;
      break;
    case RISCV::BI__builtin_riscv_zip_32:
      ID = Intrinsic::riscv_zip;
      break;
    case RISCV::BI__builtin_riscv_unzip_32:
      ID = Intrinsic::riscv_unzip;
      break;
    }

    IntrinsicTypes = {ResultType};
    break;
  }

  // Zk builtins

  // Zknh
  case RISCV::BI__builtin_riscv_sha256sig0:
    ID = Intrinsic::riscv_sha256sig0;
    break;
  case RISCV::BI__builtin_riscv_sha256sig1:
    ID = Intrinsic::riscv_sha256sig1;
    break;
  case RISCV::BI__builtin_riscv_sha256sum0:
    ID = Intrinsic::riscv_sha256sum0;
    break;
  case RISCV::BI__builtin_riscv_sha256sum1:
    ID = Intrinsic::riscv_sha256sum1;
    break;

  // Zksed
  case RISCV::BI__builtin_riscv_sm4ks:
    ID = Intrinsic::riscv_sm4ks;
    break;
  case RISCV::BI__builtin_riscv_sm4ed:
    ID = Intrinsic::riscv_sm4ed;
    break;

  // Zksh
  case RISCV::BI__builtin_riscv_sm3p0:
    ID = Intrinsic::riscv_sm3p0;
    break;
  case RISCV::BI__builtin_riscv_sm3p1:
    ID = Intrinsic::riscv_sm3p1;
    break;

  // Packed multiply-horizontal-add: 2-operand non-accumulating forms.
  // Intrinsic types are {ResultType, InputVectorType}.
#define RVP_MHA_BINARY_CASES(BUILTIN, INTRINSIC)                               \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType, Ops[0]->getType()};                          \
    break

  RVP_MHA_BINARY_CASES(pm4add_i8x4, riscv_pm4add);
  RVP_MHA_BINARY_CASES(pm4add_i8x8, riscv_pm4add);
  RVP_MHA_BINARY_CASES(pm4add_i16x4, riscv_pm4add);
  RVP_MHA_BINARY_CASES(pm4addu_u8x4, riscv_pm4addu);
  RVP_MHA_BINARY_CASES(pm4addu_u8x8, riscv_pm4addu);
  RVP_MHA_BINARY_CASES(pm4addu_u16x4, riscv_pm4addu);
  RVP_MHA_BINARY_CASES(pm4addsu_i8x4, riscv_pm4addsu);
  RVP_MHA_BINARY_CASES(pm4addsu_i8x8, riscv_pm4addsu);
  RVP_MHA_BINARY_CASES(pm4addsu_i16x4, riscv_pm4addsu);

  RVP_MHA_BINARY_CASES(pm2add_i16x2, riscv_pm2add);
  RVP_MHA_BINARY_CASES(pm2add_i16x4, riscv_pm2add);
  RVP_MHA_BINARY_CASES(pm2add_i32x2, riscv_pm2add);
  RVP_MHA_BINARY_CASES(pm2add_x_i16x2, riscv_pm2add_x);
  RVP_MHA_BINARY_CASES(pm2add_x_i16x4, riscv_pm2add_x);
  RVP_MHA_BINARY_CASES(pm2add_x_i32x2, riscv_pm2add_x);
  RVP_MHA_BINARY_CASES(pm2addu_u16x2, riscv_pm2addu);
  RVP_MHA_BINARY_CASES(pm2addu_u16x4, riscv_pm2addu);
  RVP_MHA_BINARY_CASES(pm2addu_u32x2, riscv_pm2addu);
  RVP_MHA_BINARY_CASES(pm2addsu_i16x2, riscv_pm2addsu);
  RVP_MHA_BINARY_CASES(pm2addsu_i16x4, riscv_pm2addsu);
  RVP_MHA_BINARY_CASES(pm2addsu_i32x2, riscv_pm2addsu);

  RVP_MHA_BINARY_CASES(pmq2add_i16x2, riscv_pmq2add);
  RVP_MHA_BINARY_CASES(pmq2add_i16x4, riscv_pmq2add);
  RVP_MHA_BINARY_CASES(pmq2add_i32x2, riscv_pmq2add);
  RVP_MHA_BINARY_CASES(pmqr2add_i16x2, riscv_pmqr2add);
  RVP_MHA_BINARY_CASES(pmqr2add_i16x4, riscv_pmqr2add);
  RVP_MHA_BINARY_CASES(pmqr2add_i32x2, riscv_pmqr2add);

  RVP_MHA_BINARY_CASES(pm2sadd_i16x2, riscv_pm2sadd);
  RVP_MHA_BINARY_CASES(pm2sadd_i16x4, riscv_pm2sadd);
  RVP_MHA_BINARY_CASES(pm2sadd_x_i16x2, riscv_pm2sadd_x);
  RVP_MHA_BINARY_CASES(pm2sadd_x_i16x4, riscv_pm2sadd_x);

  RVP_MHA_BINARY_CASES(pm2sub_i16x2, riscv_pm2sub);
  RVP_MHA_BINARY_CASES(pm2sub_i16x4, riscv_pm2sub);
  RVP_MHA_BINARY_CASES(pm2sub_i32x2, riscv_pm2sub);
  RVP_MHA_BINARY_CASES(pm2sub_x_i16x2, riscv_pm2sub_x);
  RVP_MHA_BINARY_CASES(pm2sub_x_i16x4, riscv_pm2sub_x);
  RVP_MHA_BINARY_CASES(pm2sub_x_i32x2, riscv_pm2sub_x);
#undef RVP_MHA_BINARY_CASES

  // Packed multiply-horizontal-add: 3-operand accumulating forms (rd, rs1, rs2).
  // Intrinsic types are {ResultType, InputVectorType (Ops[1])}.
#define RVP_MHA_TERNARY_CASES(BUILTIN, INTRINSIC)                              \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType, Ops[1]->getType()};                          \
    break

  RVP_MHA_TERNARY_CASES(pm4adda_i8x4, riscv_pm4adda);
  RVP_MHA_TERNARY_CASES(pm4adda_i8x8, riscv_pm4adda);
  RVP_MHA_TERNARY_CASES(pm4adda_i16x4, riscv_pm4adda);
  RVP_MHA_TERNARY_CASES(pm4addau_u8x4, riscv_pm4addau);
  RVP_MHA_TERNARY_CASES(pm4addau_u8x8, riscv_pm4addau);
  RVP_MHA_TERNARY_CASES(pm4addau_u16x4, riscv_pm4addau);
  RVP_MHA_TERNARY_CASES(pm4addasu_i8x4, riscv_pm4addasu);
  RVP_MHA_TERNARY_CASES(pm4addasu_i8x8, riscv_pm4addasu);
  RVP_MHA_TERNARY_CASES(pm4addasu_i16x4, riscv_pm4addasu);

  RVP_MHA_TERNARY_CASES(pm2adda_i16x2, riscv_pm2adda);
  RVP_MHA_TERNARY_CASES(pm2adda_i16x4, riscv_pm2adda);
  RVP_MHA_TERNARY_CASES(pm2adda_i32x2, riscv_pm2adda);
  RVP_MHA_TERNARY_CASES(pm2adda_x_i16x2, riscv_pm2adda_x);
  RVP_MHA_TERNARY_CASES(pm2adda_x_i16x4, riscv_pm2adda_x);
  RVP_MHA_TERNARY_CASES(pm2adda_x_i32x2, riscv_pm2adda_x);
  RVP_MHA_TERNARY_CASES(pm2addau_u16x2, riscv_pm2addau);
  RVP_MHA_TERNARY_CASES(pm2addau_u16x4, riscv_pm2addau);
  RVP_MHA_TERNARY_CASES(pm2addau_u32x2, riscv_pm2addau);
  RVP_MHA_TERNARY_CASES(pm2addasu_i16x2, riscv_pm2addasu);
  RVP_MHA_TERNARY_CASES(pm2addasu_i16x4, riscv_pm2addasu);
  RVP_MHA_TERNARY_CASES(pm2addasu_i32x2, riscv_pm2addasu);

  RVP_MHA_TERNARY_CASES(pmq2adda_i16x2, riscv_pmq2adda);
  RVP_MHA_TERNARY_CASES(pmq2adda_i16x4, riscv_pmq2adda);
  RVP_MHA_TERNARY_CASES(pmq2adda_i32x2, riscv_pmq2adda);
  RVP_MHA_TERNARY_CASES(pmqr2adda_i16x2, riscv_pmqr2adda);
  RVP_MHA_TERNARY_CASES(pmqr2adda_i16x4, riscv_pmqr2adda);
  RVP_MHA_TERNARY_CASES(pmqr2adda_i32x2, riscv_pmqr2adda);

  RVP_MHA_TERNARY_CASES(pm2suba_i16x2, riscv_pm2suba);
  RVP_MHA_TERNARY_CASES(pm2suba_i16x4, riscv_pm2suba);
  RVP_MHA_TERNARY_CASES(pm2suba_i32x2, riscv_pm2suba);
  RVP_MHA_TERNARY_CASES(pm2suba_x_i16x2, riscv_pm2suba_x);
  RVP_MHA_TERNARY_CASES(pm2suba_x_i16x4, riscv_pm2suba_x);
  RVP_MHA_TERNARY_CASES(pm2suba_x_i32x2, riscv_pm2suba_x);
#undef RVP_MHA_TERNARY_CASES

  // Packed Exchanged Addition and Subtraction: vector-in / vector-out, same type.
  // Intrinsic type list is just {ResultType}.
#define RVP_EAS_CASES(BUILTIN, INTRINSIC)                                      \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType};                                             \
    break

  RVP_EAS_CASES(pas_x_i16x2, riscv_pas_x);
  RVP_EAS_CASES(pas_x_i16x4, riscv_pas_x);
  RVP_EAS_CASES(pas_x_i32x2, riscv_pas_x);
  RVP_EAS_CASES(psa_x_i16x2, riscv_psa_x);
  RVP_EAS_CASES(psa_x_i16x4, riscv_psa_x);
  RVP_EAS_CASES(psa_x_i32x2, riscv_psa_x);
  RVP_EAS_CASES(psas_x_i16x2, riscv_psas_x);
  RVP_EAS_CASES(psas_x_i16x4, riscv_psas_x);
  RVP_EAS_CASES(psas_x_i32x2, riscv_psas_x);
  RVP_EAS_CASES(pssa_x_i16x2, riscv_pssa_x);
  RVP_EAS_CASES(pssa_x_i16x4, riscv_pssa_x);
  RVP_EAS_CASES(pssa_x_i32x2, riscv_pssa_x);
  RVP_EAS_CASES(paas_x_i16x2, riscv_paas_x);
  RVP_EAS_CASES(paas_x_i16x4, riscv_paas_x);
  RVP_EAS_CASES(paas_x_i32x2, riscv_paas_x);
  RVP_EAS_CASES(pasa_x_i16x2, riscv_pasa_x);
  RVP_EAS_CASES(pasa_x_i16x4, riscv_pasa_x);
  RVP_EAS_CASES(pasa_x_i32x2, riscv_pasa_x);

  RVP_EAS_CASES(psh1add_i16x2, riscv_psh1add);
  RVP_EAS_CASES(psh1add_u16x2, riscv_psh1add);
  RVP_EAS_CASES(psh1add_i16x4, riscv_psh1add);
  RVP_EAS_CASES(psh1add_u16x4, riscv_psh1add);
  RVP_EAS_CASES(psh1add_i32x2, riscv_psh1add);
  RVP_EAS_CASES(psh1add_u32x2, riscv_psh1add);
  RVP_EAS_CASES(pssh1sadd_i16x2, riscv_pssh1sadd);
  RVP_EAS_CASES(pssh1sadd_i16x4, riscv_pssh1sadd);
  RVP_EAS_CASES(pssh1sadd_i32x2, riscv_pssh1sadd);

  RVP_EAS_CASES(pmulq_i16x2,  riscv_pmulq);
  RVP_EAS_CASES(pmulq_i16x4,  riscv_pmulq);
  RVP_EAS_CASES(pmulq_i32x2,  riscv_pmulq);
  RVP_EAS_CASES(pmulqr_i16x2, riscv_pmulqr);
  RVP_EAS_CASES(pmulqr_i16x4, riscv_pmulqr);
  RVP_EAS_CASES(pmulqr_i32x2, riscv_pmulqr);

  // Packed Multiply Parts: result and input vector types are independently
  // overloaded, so the intrinsic-types list has two entries.
#define RVP_MULP_CASES(BUILTIN, INTRINSIC)                                     \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType, Ops[0]->getType()};                          \
    break

  // Packed Multiply Parts: byte-pair (RV32 and RV64).
  RVP_MULP_CASES(pmul_b00_i16x2,    riscv_pmul_b00);
  RVP_MULP_CASES(pmul_b00_i16x4,    riscv_pmul_b00);
  RVP_MULP_CASES(pmul_b01_i16x2,    riscv_pmul_b01);
  RVP_MULP_CASES(pmul_b01_i16x4,    riscv_pmul_b01);
  RVP_MULP_CASES(pmul_b11_i16x2,    riscv_pmul_b11);
  RVP_MULP_CASES(pmul_b11_i16x4,    riscv_pmul_b11);
  RVP_MULP_CASES(pmulu_b00_u16x2,   riscv_pmulu_b00);
  RVP_MULP_CASES(pmulu_b00_u16x4,   riscv_pmulu_b00);
  RVP_MULP_CASES(pmulu_b01_u16x2,   riscv_pmulu_b01);
  RVP_MULP_CASES(pmulu_b01_u16x4,   riscv_pmulu_b01);
  RVP_MULP_CASES(pmulu_b11_u16x2,   riscv_pmulu_b11);
  RVP_MULP_CASES(pmulu_b11_u16x4,   riscv_pmulu_b11);
  RVP_MULP_CASES(pmulsu_b00_i16x2,  riscv_pmulsu_b00);
  RVP_MULP_CASES(pmulsu_b00_i16x4,  riscv_pmulsu_b00);
  RVP_MULP_CASES(pmulsu_b11_i16x2,  riscv_pmulsu_b11);
  RVP_MULP_CASES(pmulsu_b11_i16x4,  riscv_pmulsu_b11);

  // Packed Multiply Parts: halfword-pair scalar (RV32) and vector (RV64).
  RVP_MULP_CASES(mul_h00_i32,    riscv_pmul_h00);
  RVP_MULP_CASES(mul_h01_i32,    riscv_pmul_h01);
  RVP_MULP_CASES(mul_h11_i32,    riscv_pmul_h11);
  RVP_MULP_CASES(mulu_h00_u32,   riscv_pmulu_h00);
  RVP_MULP_CASES(mulu_h01_u32,   riscv_pmulu_h01);
  RVP_MULP_CASES(mulu_h11_u32,   riscv_pmulu_h11);
  RVP_MULP_CASES(mulsu_h00_i32,  riscv_pmulsu_h00);
  RVP_MULP_CASES(mulsu_h11_i32,  riscv_pmulsu_h11);
  RVP_MULP_CASES(pmul_h00_i32x2,   riscv_pmul_h00);
  RVP_MULP_CASES(pmul_h01_i32x2,   riscv_pmul_h01);
  RVP_MULP_CASES(pmul_h11_i32x2,   riscv_pmul_h11);
  RVP_MULP_CASES(pmulu_h00_u32x2,  riscv_pmulu_h00);
  RVP_MULP_CASES(pmulu_h01_u32x2,  riscv_pmulu_h01);
  RVP_MULP_CASES(pmulu_h11_u32x2,  riscv_pmulu_h11);
  RVP_MULP_CASES(pmulsu_h00_i32x2, riscv_pmulsu_h00);
  RVP_MULP_CASES(pmulsu_h11_i32x2, riscv_pmulsu_h11);

  // Packed Multiply Parts: word-pair scalar (RV64 only, mul.w*).
  RVP_MULP_CASES(mul_w00_i64,   riscv_pmul_w00);
  RVP_MULP_CASES(mul_w01_i64,   riscv_pmul_w01);
  RVP_MULP_CASES(mul_w11_i64,   riscv_pmul_w11);
  RVP_MULP_CASES(mulu_w00_u64,  riscv_pmulu_w00);
  RVP_MULP_CASES(mulu_w01_u64,  riscv_pmulu_w01);
  RVP_MULP_CASES(mulu_w11_u64,  riscv_pmulu_w11);
  RVP_MULP_CASES(mulsu_w00_i64, riscv_pmulsu_w00);
  RVP_MULP_CASES(mulsu_w11_i64, riscv_pmulsu_w11);

  // Packed Multiply Parts Accumulate: 3-operand (rd, rs1, rs2). Intrinsic
  // types are {ResultType, InputVectorType (Ops[1])}.
#define RVP_MULPA_CASES(BUILTIN, INTRINSIC)                                    \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType, Ops[1]->getType()};                          \
    break

  RVP_MULPA_CASES(macc_h00_i32,    riscv_pmacc_h00);
  RVP_MULPA_CASES(macc_h01_i32,    riscv_pmacc_h01);
  RVP_MULPA_CASES(macc_h11_i32,    riscv_pmacc_h11);
  RVP_MULPA_CASES(maccu_h00_u32,   riscv_pmaccu_h00);
  RVP_MULPA_CASES(maccu_h01_u32,   riscv_pmaccu_h01);
  RVP_MULPA_CASES(maccu_h11_u32,   riscv_pmaccu_h11);
  RVP_MULPA_CASES(maccsu_h00_i32,  riscv_pmaccsu_h00);
  RVP_MULPA_CASES(maccsu_h11_i32,  riscv_pmaccsu_h11);

  RVP_MULPA_CASES(pmacc_h00_i32x2,   riscv_pmacc_h00);
  RVP_MULPA_CASES(pmacc_h01_i32x2,   riscv_pmacc_h01);
  RVP_MULPA_CASES(pmacc_h11_i32x2,   riscv_pmacc_h11);
  RVP_MULPA_CASES(pmaccu_h00_u32x2,  riscv_pmaccu_h00);
  RVP_MULPA_CASES(pmaccu_h01_u32x2,  riscv_pmaccu_h01);
  RVP_MULPA_CASES(pmaccu_h11_u32x2,  riscv_pmaccu_h11);
  RVP_MULPA_CASES(pmaccsu_h00_i32x2, riscv_pmaccsu_h00);
  RVP_MULPA_CASES(pmaccsu_h11_i32x2, riscv_pmaccsu_h11);

  RVP_MULPA_CASES(macc_w00_i64,   riscv_pmacc_w00);
  RVP_MULPA_CASES(macc_w01_i64,   riscv_pmacc_w01);
  RVP_MULPA_CASES(macc_w11_i64,   riscv_pmacc_w11);
  RVP_MULPA_CASES(maccu_w00_u64,  riscv_pmaccu_w00);
  RVP_MULPA_CASES(maccu_w01_u64,  riscv_pmaccu_w01);
  RVP_MULPA_CASES(maccu_w11_u64,  riscv_pmaccu_w11);
  RVP_MULPA_CASES(maccsu_w00_i64, riscv_pmaccsu_w00);
  RVP_MULPA_CASES(maccsu_w11_i64, riscv_pmaccsu_w11);

  // Packed "Q-format" Multiply Parts Accumulate.
  RVP_MULPA_CASES(mqacc_h00_i32,    riscv_pmqacc_h00);
  RVP_MULPA_CASES(mqacc_h01_i32,    riscv_pmqacc_h01);
  RVP_MULPA_CASES(mqacc_h11_i32,    riscv_pmqacc_h11);
  RVP_MULPA_CASES(mqracc_h00_i32,   riscv_pmqracc_h00);
  RVP_MULPA_CASES(mqracc_h01_i32,   riscv_pmqracc_h01);
  RVP_MULPA_CASES(mqracc_h11_i32,   riscv_pmqracc_h11);

  RVP_MULPA_CASES(pmqacc_h00_i32x2,  riscv_pmqacc_h00);
  RVP_MULPA_CASES(pmqacc_h01_i32x2,  riscv_pmqacc_h01);
  RVP_MULPA_CASES(pmqacc_h11_i32x2,  riscv_pmqacc_h11);
  RVP_MULPA_CASES(pmqracc_h00_i32x2, riscv_pmqracc_h00);
  RVP_MULPA_CASES(pmqracc_h01_i32x2, riscv_pmqracc_h01);
  RVP_MULPA_CASES(pmqracc_h11_i32x2, riscv_pmqracc_h11);

  RVP_MULPA_CASES(mqacc_w00_i64,    riscv_pmqacc_w00);
  RVP_MULPA_CASES(mqacc_w01_i64,    riscv_pmqacc_w01);
  RVP_MULPA_CASES(mqacc_w11_i64,    riscv_pmqacc_w11);
  RVP_MULPA_CASES(mqracc_w00_i64,   riscv_pmqracc_w00);
  RVP_MULPA_CASES(mqracc_w01_i64,   riscv_pmqracc_w01);
  RVP_MULPA_CASES(mqracc_w11_i64,   riscv_pmqracc_w11);

  // Packed Multiply High Parts: 2 operands (rs1, rs2), rs1 matches result.
  // Intrinsic types are {ResultType, RS2VectorType (Ops[1])}.
  RVP_MULPA_CASES(pmulh_b0_i16x2,    riscv_pmulh_b0);
  RVP_MULPA_CASES(pmulh_b1_i16x2,    riscv_pmulh_b1);
  RVP_MULPA_CASES(pmulhsu_b0_i16x2,  riscv_pmulhsu_b0);
  RVP_MULPA_CASES(pmulhsu_b1_i16x2,  riscv_pmulhsu_b1);
  RVP_MULPA_CASES(pmulh_b0_i16x4,    riscv_pmulh_b0);
  RVP_MULPA_CASES(pmulh_b1_i16x4,    riscv_pmulh_b1);
  RVP_MULPA_CASES(pmulhsu_b0_i16x4,  riscv_pmulhsu_b0);
  RVP_MULPA_CASES(pmulhsu_b1_i16x4,  riscv_pmulhsu_b1);

  RVP_MULPA_CASES(mulh_h0_i32,       riscv_pmulh_h0);
  RVP_MULPA_CASES(mulh_h1_i32,       riscv_pmulh_h1);
  RVP_MULPA_CASES(mulhsu_h0_i32,     riscv_pmulhsu_h0);
  RVP_MULPA_CASES(mulhsu_h1_i32,     riscv_pmulhsu_h1);

  RVP_MULPA_CASES(pmulh_h0_i32x2,    riscv_pmulh_h0);
  RVP_MULPA_CASES(pmulh_h1_i32x2,    riscv_pmulh_h1);
  RVP_MULPA_CASES(pmulhsu_h0_i32x2,  riscv_pmulhsu_h0);
  RVP_MULPA_CASES(pmulhsu_h1_i32x2,  riscv_pmulhsu_h1);

  // Packed Multiply High Parts Accumulate: 3 operands (rd, rs1, rs2) where
  // result, rd, rs1 all share a type and rs2 is the overloaded narrow vector.
  // Intrinsic types are {ResultType, Ops[2]->getType()}.
#define RVP_MULHPA_CASES(BUILTIN, INTRINSIC)                                   \
  case RISCV::BI__builtin_riscv_##BUILTIN:                                     \
    ID = Intrinsic::INTRINSIC;                                                 \
    IntrinsicTypes = {ResultType, Ops[2]->getType()};                          \
    break

  RVP_MULHPA_CASES(pmhacc_b0_i16x2,    riscv_pmhacc_b0);
  RVP_MULHPA_CASES(pmhacc_b1_i16x2,    riscv_pmhacc_b1);
  RVP_MULHPA_CASES(pmhaccsu_b0_i16x2,  riscv_pmhaccsu_b0);
  RVP_MULHPA_CASES(pmhaccsu_b1_i16x2,  riscv_pmhaccsu_b1);
  RVP_MULHPA_CASES(pmhacc_b0_i16x4,    riscv_pmhacc_b0);
  RVP_MULHPA_CASES(pmhacc_b1_i16x4,    riscv_pmhacc_b1);
  RVP_MULHPA_CASES(pmhaccsu_b0_i16x4,  riscv_pmhaccsu_b0);
  RVP_MULHPA_CASES(pmhaccsu_b1_i16x4,  riscv_pmhaccsu_b1);

  RVP_MULHPA_CASES(mhacc_h0_i32,       riscv_pmhacc_h0);
  RVP_MULHPA_CASES(mhacc_h1_i32,       riscv_pmhacc_h1);
  RVP_MULHPA_CASES(mhaccsu_h0_i32,     riscv_pmhaccsu_h0);
  RVP_MULHPA_CASES(mhaccsu_h1_i32,     riscv_pmhaccsu_h1);

  RVP_MULHPA_CASES(pmhacc_h0_i32x2,    riscv_pmhacc_h0);
  RVP_MULHPA_CASES(pmhacc_h1_i32x2,    riscv_pmhacc_h1);
  RVP_MULHPA_CASES(pmhaccsu_h0_i32x2,  riscv_pmhaccsu_h0);
  RVP_MULHPA_CASES(pmhaccsu_h1_i32x2,  riscv_pmhaccsu_h1);
#undef RVP_EAS_CASES
#undef RVP_MULP_CASES
#undef RVP_MULPA_CASES
#undef RVP_MULHPA_CASES

  // Packed Widening Addition and Subtraction (RV32 only). The IR intrinsic
  // returns i64 (matching the GPR pair ABI on RV32); the builtin's declared
  // result is a 64-bit vector. Bitcast the call result to the vector type.
  case RISCV::BI__builtin_riscv_pwadd_i16x4:
  case RISCV::BI__builtin_riscv_pwadd_i32x2:
  case RISCV::BI__builtin_riscv_pwaddu_u16x4:
  case RISCV::BI__builtin_riscv_pwaddu_u32x2:
  case RISCV::BI__builtin_riscv_pwsub_i16x4:
  case RISCV::BI__builtin_riscv_pwsub_i32x2:
  case RISCV::BI__builtin_riscv_pwsubu_u16x4:
  case RISCV::BI__builtin_riscv_pwsubu_u32x2:
  case RISCV::BI__builtin_riscv_pwmul_i16x4:
  case RISCV::BI__builtin_riscv_pwmul_i32x2:
  case RISCV::BI__builtin_riscv_pwmulu_u16x4:
  case RISCV::BI__builtin_riscv_pwmulu_u32x2:
  case RISCV::BI__builtin_riscv_pwmulsu_i16x4:
  case RISCV::BI__builtin_riscv_pwmulsu_i32x2:
  case RISCV::BI__builtin_riscv_pzip_i8x8:
  case RISCV::BI__builtin_riscv_pzip_u8x8:
  case RISCV::BI__builtin_riscv_pzip_i16x4:
  case RISCV::BI__builtin_riscv_pzip_u16x4: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwadd_i16x4:
    case RISCV::BI__builtin_riscv_pwadd_i32x2:
      IntID = Intrinsic::riscv_pwadd;
      break;
    case RISCV::BI__builtin_riscv_pwaddu_u16x4:
    case RISCV::BI__builtin_riscv_pwaddu_u32x2:
      IntID = Intrinsic::riscv_pwaddu;
      break;
    case RISCV::BI__builtin_riscv_pwsub_i16x4:
    case RISCV::BI__builtin_riscv_pwsub_i32x2:
      IntID = Intrinsic::riscv_pwsub;
      break;
    case RISCV::BI__builtin_riscv_pwsubu_u16x4:
    case RISCV::BI__builtin_riscv_pwsubu_u32x2:
      IntID = Intrinsic::riscv_pwsubu;
      break;
    case RISCV::BI__builtin_riscv_pwmul_i16x4:
    case RISCV::BI__builtin_riscv_pwmul_i32x2:
      IntID = Intrinsic::riscv_pwmul;
      break;
    case RISCV::BI__builtin_riscv_pwmulu_u16x4:
    case RISCV::BI__builtin_riscv_pwmulu_u32x2:
      IntID = Intrinsic::riscv_pwmulu;
      break;
    case RISCV::BI__builtin_riscv_pwmulsu_i16x4:
    case RISCV::BI__builtin_riscv_pwmulsu_i32x2:
      IntID = Intrinsic::riscv_pwmulsu;
      break;
    case RISCV::BI__builtin_riscv_pzip_i8x8:
    case RISCV::BI__builtin_riscv_pzip_u8x8:
    case RISCV::BI__builtin_riscv_pzip_i16x4:
    case RISCV::BI__builtin_riscv_pzip_u16x4:
      IntID = Intrinsic::riscv_pzip;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[0]->getType());
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Widening Add/Sub Accumulate (RV32 only). The IR intrinsic takes
  // (i64 rd, vec rs1, vec rs2) and returns i64; the builtin signature uses
  // 64-bit vector types for rd and the result, so bitcast on both ends.
  case RISCV::BI__builtin_riscv_pwadda_i16x4:
  case RISCV::BI__builtin_riscv_pwadda_i32x2:
  case RISCV::BI__builtin_riscv_pwaddau_u16x4:
  case RISCV::BI__builtin_riscv_pwaddau_u32x2:
  case RISCV::BI__builtin_riscv_pwsuba_i16x4:
  case RISCV::BI__builtin_riscv_pwsuba_i32x2:
  case RISCV::BI__builtin_riscv_pwsubau_u16x4:
  case RISCV::BI__builtin_riscv_pwsubau_u32x2:
  case RISCV::BI__builtin_riscv_pwmacc_i32x2:
  case RISCV::BI__builtin_riscv_pwmaccu_u32x2:
  case RISCV::BI__builtin_riscv_pwmaccsu_i32x2:
  case RISCV::BI__builtin_riscv_pmqwacc_i32x2:
  case RISCV::BI__builtin_riscv_pmqrwacc_i32x2: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwadda_i16x4:
    case RISCV::BI__builtin_riscv_pwadda_i32x2:
      IntID = Intrinsic::riscv_pwadda;
      break;
    case RISCV::BI__builtin_riscv_pwaddau_u16x4:
    case RISCV::BI__builtin_riscv_pwaddau_u32x2:
      IntID = Intrinsic::riscv_pwaddau;
      break;
    case RISCV::BI__builtin_riscv_pwsuba_i16x4:
    case RISCV::BI__builtin_riscv_pwsuba_i32x2:
      IntID = Intrinsic::riscv_pwsuba;
      break;
    case RISCV::BI__builtin_riscv_pwsubau_u16x4:
    case RISCV::BI__builtin_riscv_pwsubau_u32x2:
      IntID = Intrinsic::riscv_pwsubau;
      break;
    case RISCV::BI__builtin_riscv_pwmacc_i32x2:
      IntID = Intrinsic::riscv_pwmacc;
      break;
    case RISCV::BI__builtin_riscv_pwmaccu_u32x2:
      IntID = Intrinsic::riscv_pwmaccu;
      break;
    case RISCV::BI__builtin_riscv_pwmaccsu_i32x2:
      IntID = Intrinsic::riscv_pwmaccsu;
      break;
    case RISCV::BI__builtin_riscv_pmqwacc_i32x2:
      IntID = Intrinsic::riscv_pmqwacc;
      break;
    case RISCV::BI__builtin_riscv_pmqrwacc_i32x2:
      IntID = Intrinsic::riscv_pmqrwacc;
      break;
    }
    // Bitcast rd from the 64-bit vector type to i64.
    Ops[0] = Builder.CreateBitCast(Ops[0], Int64Ty);
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[1]->getType());
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Multiplication with Widening Horizontal Addition (RV32 only).
  // Result and rd (for accumulating variants) are int64_t scalars, so no
  // bitcast is needed at the call boundary.
  case RISCV::BI__builtin_riscv_pm2wadd_i64:
  case RISCV::BI__builtin_riscv_pm2wadd_x_i64:
  case RISCV::BI__builtin_riscv_pm2waddu_u64:
  case RISCV::BI__builtin_riscv_pm2wsub_i64:
  case RISCV::BI__builtin_riscv_pm2wsub_x_i64:
  case RISCV::BI__builtin_riscv_pm2waddsu_u64: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pm2wadd_i64:
      IntID = Intrinsic::riscv_pm2wadd;
      break;
    case RISCV::BI__builtin_riscv_pm2wadd_x_i64:
      IntID = Intrinsic::riscv_pm2wadd_x;
      break;
    case RISCV::BI__builtin_riscv_pm2waddu_u64:
      IntID = Intrinsic::riscv_pm2waddu;
      break;
    case RISCV::BI__builtin_riscv_pm2wsub_i64:
      IntID = Intrinsic::riscv_pm2wsub;
      break;
    case RISCV::BI__builtin_riscv_pm2wsub_x_i64:
      IntID = Intrinsic::riscv_pm2wsub_x;
      break;
    case RISCV::BI__builtin_riscv_pm2waddsu_u64:
      IntID = Intrinsic::riscv_pm2waddsu;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[0]->getType());
    return Builder.CreateCall(F, Ops);
  }

  // Packed Widening Unzip. Each variant maps to a small mix of existing
  // operations / intrinsics:
  //   pwunzipe_i : psext.{h.b,w.h}
  //   pwunzipo_i : ashr wide vector by half-element-width (matches PSRAI.{H,W})
  //   pwunzipue_u: pnzip(input, 0)
  //   pwunzipuo_u: pnziph(input, 0)
  //   pwunziphe  : shl wide vector by half-element-width (matches PSLLI.{H,W})
  //   pwunzipho  : pnziph(0, input)
  //
  // The narrow input vector and wide output vector are determined by the
  // builtin name. The pnzip/pnziph IR intrinsics are polymorphic on the
  // (result, input) type pair; for the RV32 x2 family the IR call uses
  // (narrow, narrow) byte-level shape (matching the historical RV32 tablegen
  // patterns), while the RV64 families use (narrow, wide).
  case RISCV::BI__builtin_riscv_pwunzipe_i16x2:
  case RISCV::BI__builtin_riscv_pwunzipo_i16x2:
  case RISCV::BI__builtin_riscv_pwunzipue_u16x2:
  case RISCV::BI__builtin_riscv_pwunzipuo_u16x2:
  case RISCV::BI__builtin_riscv_pwunziphe_i16x2:
  case RISCV::BI__builtin_riscv_pwunziphe_u16x2:
  case RISCV::BI__builtin_riscv_pwunzipho_i16x2:
  case RISCV::BI__builtin_riscv_pwunzipho_u16x2:
  case RISCV::BI__builtin_riscv_pwunzipe_i16x4:
  case RISCV::BI__builtin_riscv_pwunzipo_i16x4:
  case RISCV::BI__builtin_riscv_pwunzipue_u16x4:
  case RISCV::BI__builtin_riscv_pwunzipuo_u16x4:
  case RISCV::BI__builtin_riscv_pwunziphe_i16x4:
  case RISCV::BI__builtin_riscv_pwunziphe_u16x4:
  case RISCV::BI__builtin_riscv_pwunzipho_i16x4:
  case RISCV::BI__builtin_riscv_pwunzipho_u16x4:
  case RISCV::BI__builtin_riscv_pwunzipe_i32x2:
  case RISCV::BI__builtin_riscv_pwunzipo_i32x2:
  case RISCV::BI__builtin_riscv_pwunzipue_u32x2:
  case RISCV::BI__builtin_riscv_pwunzipuo_u32x2:
  case RISCV::BI__builtin_riscv_pwunziphe_i32x2:
  case RISCV::BI__builtin_riscv_pwunziphe_u32x2:
  case RISCV::BI__builtin_riscv_pwunzipho_i32x2:
  case RISCV::BI__builtin_riscv_pwunzipho_u32x2: {
    // NarrowVecTy: the IR vector of the input C type (e.g. v4i8 / v8i8 / v4i16).
    // WideVecTy:   the IR vector of the result C type (e.g. v2i16 / v4i16 / v2i32).
    // PnzipInputTy: the IR vector used as the pnzip/pnziph input operand.
    //               RV32 keeps v4i8 (byte-level), RV64 uses WideVecTy.
    llvm::Type *NarrowVecTy, *WideVecTy, *PnzipInputTy;
    auto *V4I8Ty  = llvm::FixedVectorType::get(Int8Ty,  4);
    auto *V8I8Ty  = llvm::FixedVectorType::get(Int8Ty,  8);
    auto *V2I16Ty = llvm::FixedVectorType::get(Int16Ty, 2);
    auto *V4I16Ty = llvm::FixedVectorType::get(Int16Ty, 4);
    auto *V2I32Ty = llvm::FixedVectorType::get(Int32Ty, 2);
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwunzipe_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipo_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipue_u16x2:
    case RISCV::BI__builtin_riscv_pwunzipuo_u16x2:
    case RISCV::BI__builtin_riscv_pwunziphe_i16x2:
    case RISCV::BI__builtin_riscv_pwunziphe_u16x2:
    case RISCV::BI__builtin_riscv_pwunzipho_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipho_u16x2:
      NarrowVecTy = V4I8Ty; WideVecTy = V2I16Ty; PnzipInputTy = V4I8Ty;
      break;
    case RISCV::BI__builtin_riscv_pwunzipe_i16x4:
    case RISCV::BI__builtin_riscv_pwunzipo_i16x4:
    case RISCV::BI__builtin_riscv_pwunzipue_u16x4:
    case RISCV::BI__builtin_riscv_pwunzipuo_u16x4:
    case RISCV::BI__builtin_riscv_pwunziphe_i16x4:
    case RISCV::BI__builtin_riscv_pwunziphe_u16x4:
    case RISCV::BI__builtin_riscv_pwunzipho_i16x4:
    case RISCV::BI__builtin_riscv_pwunzipho_u16x4:
      NarrowVecTy = V8I8Ty; WideVecTy = V4I16Ty; PnzipInputTy = V4I16Ty;
      break;
    case RISCV::BI__builtin_riscv_pwunzipe_i32x2:
    case RISCV::BI__builtin_riscv_pwunzipo_i32x2:
    case RISCV::BI__builtin_riscv_pwunzipue_u32x2:
    case RISCV::BI__builtin_riscv_pwunzipuo_u32x2:
    case RISCV::BI__builtin_riscv_pwunziphe_i32x2:
    case RISCV::BI__builtin_riscv_pwunziphe_u32x2:
    case RISCV::BI__builtin_riscv_pwunzipho_i32x2:
    case RISCV::BI__builtin_riscv_pwunzipho_u32x2:
      NarrowVecTy = V4I16Ty; WideVecTy = V2I32Ty; PnzipInputTy = V2I32Ty;
      break;
    }
    llvm::Value *Input = Builder.CreateBitCast(Ops[0], NarrowVecTy);
    llvm::Value *Result;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwunzipe_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipe_i16x4: {
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_psext_h_b,
                                           {WideVecTy, NarrowVecTy});
      Result = Builder.CreateCall(F, {Input});
      break;
    }
    case RISCV::BI__builtin_riscv_pwunzipe_i32x2: {
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_psext_w_h);
      Result = Builder.CreateCall(F, {Input});
      break;
    }
    case RISCV::BI__builtin_riscv_pwunzipo_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipo_i16x4:
    case RISCV::BI__builtin_riscv_pwunzipo_i32x2: {
      llvm::Value *V = Builder.CreateBitCast(Input, WideVecTy);
      auto *WideElt = cast<llvm::IntegerType>(V->getType()->getScalarType());
      unsigned ShiftBits = WideElt->getBitWidth() / 2;
      llvm::Value *Shamt = ConstantVector::getSplat(
          cast<llvm::FixedVectorType>(WideVecTy)->getElementCount(),
          llvm::ConstantInt::get(WideElt, ShiftBits));
      Result = Builder.CreateAShr(V, Shamt);
      break;
    }
    case RISCV::BI__builtin_riscv_pwunziphe_i16x2:
    case RISCV::BI__builtin_riscv_pwunziphe_u16x2:
    case RISCV::BI__builtin_riscv_pwunziphe_i16x4:
    case RISCV::BI__builtin_riscv_pwunziphe_u16x4:
    case RISCV::BI__builtin_riscv_pwunziphe_i32x2:
    case RISCV::BI__builtin_riscv_pwunziphe_u32x2: {
      llvm::Value *V = Builder.CreateBitCast(Input, WideVecTy);
      auto *WideElt = cast<llvm::IntegerType>(V->getType()->getScalarType());
      unsigned ShiftBits = WideElt->getBitWidth() / 2;
      llvm::Value *Shamt = ConstantVector::getSplat(
          cast<llvm::FixedVectorType>(WideVecTy)->getElementCount(),
          llvm::ConstantInt::get(WideElt, ShiftBits));
      Result = Builder.CreateShl(V, Shamt);
      break;
    }
    case RISCV::BI__builtin_riscv_pwunzipue_u16x2:
    case RISCV::BI__builtin_riscv_pwunzipue_u16x4:
    case RISCV::BI__builtin_riscv_pwunzipue_u32x2: {
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_pnzip,
                                           {NarrowVecTy, PnzipInputTy});
      llvm::Value *Zero = llvm::Constant::getNullValue(PnzipInputTy);
      llvm::Value *PnzipIn = Builder.CreateBitCast(Input, PnzipInputTy);
      Result = Builder.CreateCall(F, {PnzipIn, Zero});
      break;
    }
    case RISCV::BI__builtin_riscv_pwunzipuo_u16x2:
    case RISCV::BI__builtin_riscv_pwunzipuo_u16x4:
    case RISCV::BI__builtin_riscv_pwunzipuo_u32x2: {
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_pnziph,
                                           {NarrowVecTy, PnzipInputTy});
      llvm::Value *Zero = llvm::Constant::getNullValue(PnzipInputTy);
      llvm::Value *PnzipIn = Builder.CreateBitCast(Input, PnzipInputTy);
      Result = Builder.CreateCall(F, {PnzipIn, Zero});
      break;
    }
    case RISCV::BI__builtin_riscv_pwunzipho_i16x2:
    case RISCV::BI__builtin_riscv_pwunzipho_u16x2:
    case RISCV::BI__builtin_riscv_pwunzipho_i16x4:
    case RISCV::BI__builtin_riscv_pwunzipho_u16x4:
    case RISCV::BI__builtin_riscv_pwunzipho_i32x2:
    case RISCV::BI__builtin_riscv_pwunzipho_u32x2: {
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_pnziph,
                                           {NarrowVecTy, PnzipInputTy});
      llvm::Value *Zero = llvm::Constant::getNullValue(PnzipInputTy);
      llvm::Value *PnzipIn = Builder.CreateBitCast(Input, PnzipInputTy);
      Result = Builder.CreateCall(F, {Zero, PnzipIn});
      break;
    }
    }
    return Builder.CreateBitCast(Result, ResultType);
  }

  // Packed Narrowing Zip. The underlying ppaire.{b,h} / ppairo.{b,h}
  // instructions operate at a fixed element granularity per family. The IR
  // intrinsic is polymorphic on result/input vector types. The RV32 case
  // keeps its historical "byte-level interpretation" shape with both inputs
  // bitcast to v4i8; the new RV64 cases use the spec-typed wider inputs.
  case RISCV::BI__builtin_riscv_pnzip_i8x4:
  case RISCV::BI__builtin_riscv_pnzip_u8x4:
  case RISCV::BI__builtin_riscv_pnziph_i8x4:
  case RISCV::BI__builtin_riscv_pnziph_u8x4:
  case RISCV::BI__builtin_riscv_pnzip_i8x8:
  case RISCV::BI__builtin_riscv_pnzip_u8x8:
  case RISCV::BI__builtin_riscv_pnziph_i8x8:
  case RISCV::BI__builtin_riscv_pnziph_u8x8:
  case RISCV::BI__builtin_riscv_pnzip_i16x4:
  case RISCV::BI__builtin_riscv_pnzip_u16x4:
  case RISCV::BI__builtin_riscv_pnziph_i16x4:
  case RISCV::BI__builtin_riscv_pnziph_u16x4: {
    unsigned IntID;
    // (Narrow,Wide) IR vector types for the (result, input) of the
    // polymorphic intrinsic call. NarrowVecTy is the result; WideVecTy is
    // the per-input type.
    llvm::Type *NarrowVecTy, *WideVecTy;
    auto *V4I8Ty  = llvm::FixedVectorType::get(Int8Ty, 4);
    auto *V8I8Ty  = llvm::FixedVectorType::get(Int8Ty, 8);
    auto *V4I16Ty = llvm::FixedVectorType::get(Int16Ty, 4);
    auto *V2I32Ty = llvm::FixedVectorType::get(Int32Ty, 2);
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pnzip_i8x4:
    case RISCV::BI__builtin_riscv_pnzip_u8x4:
      IntID = Intrinsic::riscv_pnzip;
      NarrowVecTy = V4I8Ty; WideVecTy = V4I8Ty;
      break;
    case RISCV::BI__builtin_riscv_pnziph_i8x4:
    case RISCV::BI__builtin_riscv_pnziph_u8x4:
      IntID = Intrinsic::riscv_pnziph;
      NarrowVecTy = V4I8Ty; WideVecTy = V4I8Ty;
      break;
    case RISCV::BI__builtin_riscv_pnzip_i8x8:
    case RISCV::BI__builtin_riscv_pnzip_u8x8:
      IntID = Intrinsic::riscv_pnzip;
      NarrowVecTy = V8I8Ty; WideVecTy = V4I16Ty;
      break;
    case RISCV::BI__builtin_riscv_pnziph_i8x8:
    case RISCV::BI__builtin_riscv_pnziph_u8x8:
      IntID = Intrinsic::riscv_pnziph;
      NarrowVecTy = V8I8Ty; WideVecTy = V4I16Ty;
      break;
    case RISCV::BI__builtin_riscv_pnzip_i16x4:
    case RISCV::BI__builtin_riscv_pnzip_u16x4:
      IntID = Intrinsic::riscv_pnzip;
      NarrowVecTy = V4I16Ty; WideVecTy = V2I32Ty;
      break;
    case RISCV::BI__builtin_riscv_pnziph_i16x4:
    case RISCV::BI__builtin_riscv_pnziph_u16x4:
      IntID = Intrinsic::riscv_pnziph;
      NarrowVecTy = V4I16Ty; WideVecTy = V2I32Ty;
      break;
    }
    Ops[0] = Builder.CreateBitCast(Ops[0], WideVecTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], WideVecTy);
    llvm::Function *F = CGM.getIntrinsic(IntID, {NarrowVecTy, WideVecTy});
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Pair. Four operations (ppaire/eo/oe/o) over the 32-bit table
  // (v4i8 / v2i16, both XLENs) and the RV64-only 64-bit table
  // (v8i8 / v4i16 / v2i32). The IR intrinsic is polymorphic on the vector
  // type with both inputs / result of the same type; clang bitcasts the
  // scalar argument coercions to the vector type and bitcasts the call
  // result back to the scalar return type.
  case RISCV::BI__builtin_riscv_ppaire_i8x4:
  case RISCV::BI__builtin_riscv_ppaire_u8x4:
  case RISCV::BI__builtin_riscv_ppaireo_i8x4:
  case RISCV::BI__builtin_riscv_ppaireo_u8x4:
  case RISCV::BI__builtin_riscv_ppairoe_i8x4:
  case RISCV::BI__builtin_riscv_ppairoe_u8x4:
  case RISCV::BI__builtin_riscv_ppairo_i8x4:
  case RISCV::BI__builtin_riscv_ppairo_u8x4:
  case RISCV::BI__builtin_riscv_ppaire_i16x2:
  case RISCV::BI__builtin_riscv_ppaire_u16x2:
  case RISCV::BI__builtin_riscv_ppaireo_i16x2:
  case RISCV::BI__builtin_riscv_ppaireo_u16x2:
  case RISCV::BI__builtin_riscv_ppairoe_i16x2:
  case RISCV::BI__builtin_riscv_ppairoe_u16x2:
  case RISCV::BI__builtin_riscv_ppairo_i16x2:
  case RISCV::BI__builtin_riscv_ppairo_u16x2:
  case RISCV::BI__builtin_riscv_ppaire_i8x8:
  case RISCV::BI__builtin_riscv_ppaire_u8x8:
  case RISCV::BI__builtin_riscv_ppaireo_i8x8:
  case RISCV::BI__builtin_riscv_ppaireo_u8x8:
  case RISCV::BI__builtin_riscv_ppairoe_i8x8:
  case RISCV::BI__builtin_riscv_ppairoe_u8x8:
  case RISCV::BI__builtin_riscv_ppairo_i8x8:
  case RISCV::BI__builtin_riscv_ppairo_u8x8:
  case RISCV::BI__builtin_riscv_ppaire_i16x4:
  case RISCV::BI__builtin_riscv_ppaire_u16x4:
  case RISCV::BI__builtin_riscv_ppaireo_i16x4:
  case RISCV::BI__builtin_riscv_ppaireo_u16x4:
  case RISCV::BI__builtin_riscv_ppairoe_i16x4:
  case RISCV::BI__builtin_riscv_ppairoe_u16x4:
  case RISCV::BI__builtin_riscv_ppairo_i16x4:
  case RISCV::BI__builtin_riscv_ppairo_u16x4:
  case RISCV::BI__builtin_riscv_ppaire_i32x2:
  case RISCV::BI__builtin_riscv_ppaire_u32x2:
  case RISCV::BI__builtin_riscv_ppaireo_i32x2:
  case RISCV::BI__builtin_riscv_ppaireo_u32x2:
  case RISCV::BI__builtin_riscv_ppairoe_i32x2:
  case RISCV::BI__builtin_riscv_ppairoe_u32x2:
  case RISCV::BI__builtin_riscv_ppairo_i32x2:
  case RISCV::BI__builtin_riscv_ppairo_u32x2: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_ppaire_i8x4:
    case RISCV::BI__builtin_riscv_ppaire_u8x4:
    case RISCV::BI__builtin_riscv_ppaire_i16x2:
    case RISCV::BI__builtin_riscv_ppaire_u16x2:
    case RISCV::BI__builtin_riscv_ppaire_i8x8:
    case RISCV::BI__builtin_riscv_ppaire_u8x8:
    case RISCV::BI__builtin_riscv_ppaire_i16x4:
    case RISCV::BI__builtin_riscv_ppaire_u16x4:
    case RISCV::BI__builtin_riscv_ppaire_i32x2:
    case RISCV::BI__builtin_riscv_ppaire_u32x2:
      IntID = Intrinsic::riscv_ppaire;
      break;
    case RISCV::BI__builtin_riscv_ppaireo_i8x4:
    case RISCV::BI__builtin_riscv_ppaireo_u8x4:
    case RISCV::BI__builtin_riscv_ppaireo_i16x2:
    case RISCV::BI__builtin_riscv_ppaireo_u16x2:
    case RISCV::BI__builtin_riscv_ppaireo_i8x8:
    case RISCV::BI__builtin_riscv_ppaireo_u8x8:
    case RISCV::BI__builtin_riscv_ppaireo_i16x4:
    case RISCV::BI__builtin_riscv_ppaireo_u16x4:
    case RISCV::BI__builtin_riscv_ppaireo_i32x2:
    case RISCV::BI__builtin_riscv_ppaireo_u32x2:
      IntID = Intrinsic::riscv_ppaireo;
      break;
    case RISCV::BI__builtin_riscv_ppairoe_i8x4:
    case RISCV::BI__builtin_riscv_ppairoe_u8x4:
    case RISCV::BI__builtin_riscv_ppairoe_i16x2:
    case RISCV::BI__builtin_riscv_ppairoe_u16x2:
    case RISCV::BI__builtin_riscv_ppairoe_i8x8:
    case RISCV::BI__builtin_riscv_ppairoe_u8x8:
    case RISCV::BI__builtin_riscv_ppairoe_i16x4:
    case RISCV::BI__builtin_riscv_ppairoe_u16x4:
    case RISCV::BI__builtin_riscv_ppairoe_i32x2:
    case RISCV::BI__builtin_riscv_ppairoe_u32x2:
      IntID = Intrinsic::riscv_ppairoe;
      break;
    case RISCV::BI__builtin_riscv_ppairo_i8x4:
    case RISCV::BI__builtin_riscv_ppairo_u8x4:
    case RISCV::BI__builtin_riscv_ppairo_i16x2:
    case RISCV::BI__builtin_riscv_ppairo_u16x2:
    case RISCV::BI__builtin_riscv_ppairo_i8x8:
    case RISCV::BI__builtin_riscv_ppairo_u8x8:
    case RISCV::BI__builtin_riscv_ppairo_i16x4:
    case RISCV::BI__builtin_riscv_ppairo_u16x4:
    case RISCV::BI__builtin_riscv_ppairo_i32x2:
    case RISCV::BI__builtin_riscv_ppairo_u32x2:
      IntID = Intrinsic::riscv_ppairo;
      break;
    }
    // The vector type for both operands and the result is the natural type
    // of the builtin: 32-bit-table builtins use a 32-bit vector and the
    // result is bitcast back to i32; 64-bit-table builtins use a 64-bit
    // vector and the result is bitcast back to i64.
    llvm::Type *VecTy;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_ppaire_i8x4:
    case RISCV::BI__builtin_riscv_ppaire_u8x4:
    case RISCV::BI__builtin_riscv_ppaireo_i8x4:
    case RISCV::BI__builtin_riscv_ppaireo_u8x4:
    case RISCV::BI__builtin_riscv_ppairoe_i8x4:
    case RISCV::BI__builtin_riscv_ppairoe_u8x4:
    case RISCV::BI__builtin_riscv_ppairo_i8x4:
    case RISCV::BI__builtin_riscv_ppairo_u8x4:
      VecTy = llvm::FixedVectorType::get(Int8Ty, 4);
      break;
    case RISCV::BI__builtin_riscv_ppaire_i16x2:
    case RISCV::BI__builtin_riscv_ppaire_u16x2:
    case RISCV::BI__builtin_riscv_ppaireo_i16x2:
    case RISCV::BI__builtin_riscv_ppaireo_u16x2:
    case RISCV::BI__builtin_riscv_ppairoe_i16x2:
    case RISCV::BI__builtin_riscv_ppairoe_u16x2:
    case RISCV::BI__builtin_riscv_ppairo_i16x2:
    case RISCV::BI__builtin_riscv_ppairo_u16x2:
      VecTy = llvm::FixedVectorType::get(Int16Ty, 2);
      break;
    case RISCV::BI__builtin_riscv_ppaire_i8x8:
    case RISCV::BI__builtin_riscv_ppaire_u8x8:
    case RISCV::BI__builtin_riscv_ppaireo_i8x8:
    case RISCV::BI__builtin_riscv_ppaireo_u8x8:
    case RISCV::BI__builtin_riscv_ppairoe_i8x8:
    case RISCV::BI__builtin_riscv_ppairoe_u8x8:
    case RISCV::BI__builtin_riscv_ppairo_i8x8:
    case RISCV::BI__builtin_riscv_ppairo_u8x8:
      VecTy = llvm::FixedVectorType::get(Int8Ty, 8);
      break;
    case RISCV::BI__builtin_riscv_ppaire_i16x4:
    case RISCV::BI__builtin_riscv_ppaire_u16x4:
    case RISCV::BI__builtin_riscv_ppaireo_i16x4:
    case RISCV::BI__builtin_riscv_ppaireo_u16x4:
    case RISCV::BI__builtin_riscv_ppairoe_i16x4:
    case RISCV::BI__builtin_riscv_ppairoe_u16x4:
    case RISCV::BI__builtin_riscv_ppairo_i16x4:
    case RISCV::BI__builtin_riscv_ppairo_u16x4:
      VecTy = llvm::FixedVectorType::get(Int16Ty, 4);
      break;
    case RISCV::BI__builtin_riscv_ppaire_i32x2:
    case RISCV::BI__builtin_riscv_ppaire_u32x2:
    case RISCV::BI__builtin_riscv_ppaireo_i32x2:
    case RISCV::BI__builtin_riscv_ppaireo_u32x2:
    case RISCV::BI__builtin_riscv_ppairoe_i32x2:
    case RISCV::BI__builtin_riscv_ppairoe_u32x2:
    case RISCV::BI__builtin_riscv_ppairo_i32x2:
    case RISCV::BI__builtin_riscv_ppairo_u32x2:
      VecTy = llvm::FixedVectorType::get(Int32Ty, 2);
      break;
    }
    Ops[0] = Builder.CreateBitCast(Ops[0], VecTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], VecTy);
    llvm::Function *F = CGM.getIntrinsic(IntID, VecTy);
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Narrowing Convert / Packed Unzip (RV32 only). Both map directly
  // to pnsrli with imm=0 (low half / even lanes) or imm=element-width
  // (high half / odd lanes); we delegate to the existing pnsrl IR intrinsic
  // with the matching shift amount.
  case RISCV::BI__builtin_riscv_pncvt_i8x4:
  case RISCV::BI__builtin_riscv_pncvt_u8x4:
  case RISCV::BI__builtin_riscv_pncvt_i16x2:
  case RISCV::BI__builtin_riscv_pncvt_u16x2:
  case RISCV::BI__builtin_riscv_pncvth_i8x4:
  case RISCV::BI__builtin_riscv_pncvth_u8x4:
  case RISCV::BI__builtin_riscv_pncvth_i16x2:
  case RISCV::BI__builtin_riscv_pncvth_u16x2:
  case RISCV::BI__builtin_riscv_punzipe_i8x4:
  case RISCV::BI__builtin_riscv_punzipe_u8x4:
  case RISCV::BI__builtin_riscv_punzipe_i16x2:
  case RISCV::BI__builtin_riscv_punzipe_u16x2:
  case RISCV::BI__builtin_riscv_punzipo_i8x4:
  case RISCV::BI__builtin_riscv_punzipo_u8x4:
  case RISCV::BI__builtin_riscv_punzipo_i16x2:
  case RISCV::BI__builtin_riscv_punzipo_u16x2: {
    unsigned Shamt;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pncvt_i8x4:
    case RISCV::BI__builtin_riscv_pncvt_u8x4:
    case RISCV::BI__builtin_riscv_pncvt_i16x2:
    case RISCV::BI__builtin_riscv_pncvt_u16x2:
    case RISCV::BI__builtin_riscv_punzipe_i8x4:
    case RISCV::BI__builtin_riscv_punzipe_u8x4:
    case RISCV::BI__builtin_riscv_punzipe_i16x2:
    case RISCV::BI__builtin_riscv_punzipe_u16x2:
      Shamt = 0;
      break;
    case RISCV::BI__builtin_riscv_pncvth_i8x4:
    case RISCV::BI__builtin_riscv_pncvth_u8x4:
    case RISCV::BI__builtin_riscv_punzipo_i8x4:
    case RISCV::BI__builtin_riscv_punzipo_u8x4:
      Shamt = 8;
      break;
    case RISCV::BI__builtin_riscv_pncvth_i16x2:
    case RISCV::BI__builtin_riscv_pncvth_u16x2:
    case RISCV::BI__builtin_riscv_punzipo_i16x2:
    case RISCV::BI__builtin_riscv_punzipo_u16x2:
      Shamt = 16;
      break;
    }
    Ops[0] = Builder.CreateBitCast(Ops[0], Int64Ty);
    Ops.push_back(Builder.getInt32(Shamt));
    llvm::Function *F =
        CGM.getIntrinsic(Intrinsic::riscv_pnsrl, ConvertType(E->getType()));
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Widening Convert (RV32 only). Single narrow-vector input,
  // widened i64-shaped result; bitcast result to the 64-bit vector type.
  case RISCV::BI__builtin_riscv_pwcvt_i16x4:
  case RISCV::BI__builtin_riscv_pwcvt_i32x2:
  case RISCV::BI__builtin_riscv_pwcvtu_u16x4:
  case RISCV::BI__builtin_riscv_pwcvtu_u32x2:
  case RISCV::BI__builtin_riscv_pwcvth_i16x4:
  case RISCV::BI__builtin_riscv_pwcvth_u16x4:
  case RISCV::BI__builtin_riscv_pwcvth_i32x2:
  case RISCV::BI__builtin_riscv_pwcvth_u32x2: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwcvt_i16x4:
    case RISCV::BI__builtin_riscv_pwcvt_i32x2:
      IntID = Intrinsic::riscv_pwcvt;
      break;
    case RISCV::BI__builtin_riscv_pwcvtu_u16x4:
    case RISCV::BI__builtin_riscv_pwcvtu_u32x2:
      IntID = Intrinsic::riscv_pwcvtu;
      break;
    case RISCV::BI__builtin_riscv_pwcvth_i16x4:
    case RISCV::BI__builtin_riscv_pwcvth_u16x4:
    case RISCV::BI__builtin_riscv_pwcvth_i32x2:
    case RISCV::BI__builtin_riscv_pwcvth_u32x2:
      IntID = Intrinsic::riscv_pwcvth;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[0]->getType());
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  case RISCV::BI__builtin_riscv_pm2wadda_i64:
  case RISCV::BI__builtin_riscv_pm2wadda_x_i64:
  case RISCV::BI__builtin_riscv_pm2waddau_u64:
  case RISCV::BI__builtin_riscv_pm2wsuba_i64:
  case RISCV::BI__builtin_riscv_pm2wsuba_x_i64:
  case RISCV::BI__builtin_riscv_pm2waddasu_u64: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pm2wadda_i64:
      IntID = Intrinsic::riscv_pm2wadda;
      break;
    case RISCV::BI__builtin_riscv_pm2wadda_x_i64:
      IntID = Intrinsic::riscv_pm2wadda_x;
      break;
    case RISCV::BI__builtin_riscv_pm2waddau_u64:
      IntID = Intrinsic::riscv_pm2waddau;
      break;
    case RISCV::BI__builtin_riscv_pm2wsuba_i64:
      IntID = Intrinsic::riscv_pm2wsuba;
      break;
    case RISCV::BI__builtin_riscv_pm2wsuba_x_i64:
      IntID = Intrinsic::riscv_pm2wsuba_x;
      break;
    case RISCV::BI__builtin_riscv_pm2waddasu_u64:
      IntID = Intrinsic::riscv_pm2waddasu;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[1]->getType());
    return Builder.CreateCall(F, Ops);
  }

  // Packed Widening Shift Left (RV32 only). Same i64 -> vector bitcast shape
  // as the widening add/sub/multiply family, with an i32 shift amount.
  case RISCV::BI__builtin_riscv_pwsll_s_u16x4:
  case RISCV::BI__builtin_riscv_pwsll_s_u32x2:
  case RISCV::BI__builtin_riscv_pwsla_s_i16x4:
  case RISCV::BI__builtin_riscv_pwsla_s_i32x2: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pwsll_s_u16x4:
    case RISCV::BI__builtin_riscv_pwsll_s_u32x2:
      IntID = Intrinsic::riscv_pwsll;
      break;
    case RISCV::BI__builtin_riscv_pwsla_s_i16x4:
    case RISCV::BI__builtin_riscv_pwsla_s_i32x2:
      IntID = Intrinsic::riscv_pwsla;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, Ops[0]->getType());
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Narrowing Shift Right / Narrowing Clip (RV32 only). The IR
  // intrinsic takes (i64 rs1, i32 shamt) and returns a narrow vector; the
  // builtin signature uses a 64-bit vector for rs1, so bitcast the input to
  // i64 before calling.
  case RISCV::BI__builtin_riscv_pnsrl_s_u8x4:
  case RISCV::BI__builtin_riscv_pnsrl_s_u16x2:
  case RISCV::BI__builtin_riscv_pnsra_s_i8x4:
  case RISCV::BI__builtin_riscv_pnsra_s_i16x2:
  case RISCV::BI__builtin_riscv_pnsrar_s_i8x4:
  case RISCV::BI__builtin_riscv_pnsrar_s_i16x2:
  case RISCV::BI__builtin_riscv_pnclipu_s_u8x4:
  case RISCV::BI__builtin_riscv_pnclipu_s_u16x2:
  case RISCV::BI__builtin_riscv_pnclipru_s_u8x4:
  case RISCV::BI__builtin_riscv_pnclipru_s_u16x2:
  case RISCV::BI__builtin_riscv_pnclip_s_i8x4:
  case RISCV::BI__builtin_riscv_pnclip_s_i16x2:
  case RISCV::BI__builtin_riscv_pnclipr_s_i8x4:
  case RISCV::BI__builtin_riscv_pnclipr_s_i16x2: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pnsrl_s_u8x4:
    case RISCV::BI__builtin_riscv_pnsrl_s_u16x2:
      IntID = Intrinsic::riscv_pnsrl;
      break;
    case RISCV::BI__builtin_riscv_pnsra_s_i8x4:
    case RISCV::BI__builtin_riscv_pnsra_s_i16x2:
      IntID = Intrinsic::riscv_pnsra;
      break;
    case RISCV::BI__builtin_riscv_pnsrar_s_i8x4:
    case RISCV::BI__builtin_riscv_pnsrar_s_i16x2:
      IntID = Intrinsic::riscv_pnsrar;
      break;
    case RISCV::BI__builtin_riscv_pnclipu_s_u8x4:
    case RISCV::BI__builtin_riscv_pnclipu_s_u16x2:
      IntID = Intrinsic::riscv_pnclipu;
      break;
    case RISCV::BI__builtin_riscv_pnclipru_s_u8x4:
    case RISCV::BI__builtin_riscv_pnclipru_s_u16x2:
      IntID = Intrinsic::riscv_pnclipru;
      break;
    case RISCV::BI__builtin_riscv_pnclip_s_i8x4:
    case RISCV::BI__builtin_riscv_pnclip_s_i16x2:
      IntID = Intrinsic::riscv_pnclip;
      break;
    case RISCV::BI__builtin_riscv_pnclipr_s_i8x4:
    case RISCV::BI__builtin_riscv_pnclipr_s_i16x2:
      IntID = Intrinsic::riscv_pnclipr;
      break;
    }
    Ops[0] = Builder.CreateBitCast(Ops[0], Int64Ty);
    llvm::Function *F = CGM.getIntrinsic(IntID, ConvertType(E->getType()));
    llvm::Value *Call = Builder.CreateCall(F, Ops);
    return Builder.CreateBitCast(Call, ResultType);
  }

  // Packed Element Insert and Extract. Both XLENs; idx is a constant.
  case RISCV::BI__builtin_riscv_pget_i8x4_i8:
  case RISCV::BI__builtin_riscv_pget_u8x4_u8:
  case RISCV::BI__builtin_riscv_pget_i16x2_i16:
  case RISCV::BI__builtin_riscv_pget_u16x2_u16:
  case RISCV::BI__builtin_riscv_pget_i8x8_i8:
  case RISCV::BI__builtin_riscv_pget_u8x8_u8:
  case RISCV::BI__builtin_riscv_pget_i16x4_i16:
  case RISCV::BI__builtin_riscv_pget_u16x4_u16:
  case RISCV::BI__builtin_riscv_pget_i32x2_i32:
  case RISCV::BI__builtin_riscv_pget_u32x2_u32:
    return Builder.CreateExtractElement(Ops[0], Ops[1]);

  case RISCV::BI__builtin_riscv_pset_i8_i8x4:
  case RISCV::BI__builtin_riscv_pset_u8_u8x4:
  case RISCV::BI__builtin_riscv_pset_i16_i16x2:
  case RISCV::BI__builtin_riscv_pset_u16_u16x2:
  case RISCV::BI__builtin_riscv_pset_i8_i8x8:
  case RISCV::BI__builtin_riscv_pset_u8_u8x8:
  case RISCV::BI__builtin_riscv_pset_i16_i16x4:
  case RISCV::BI__builtin_riscv_pset_u16_u16x4:
  case RISCV::BI__builtin_riscv_pset_i32_i32x2:
  case RISCV::BI__builtin_riscv_pset_u32_u32x2:
    return Builder.CreateInsertElement(Ops[0], Ops[1], Ops[2]);

  // Slide by variable element count. rs2 is masked to log2(N) bits and
  // multiplied by elt_bits to get a bit-level shift amount, then we funnel
  // shift the (rd:rs1) concatenation.
  //   pslideupx:   result = fshl(rd, rs1, (rs2 & (N-1)) * elt_bits)
  //   pslidedownx: result = fshr(rs1, rd, (rs2 & (N-1)) * elt_bits)
  case RISCV::BI__builtin_riscv_pslideupx_i8x4:
  case RISCV::BI__builtin_riscv_pslideupx_u8x4:
  case RISCV::BI__builtin_riscv_pslideupx_i16x2:
  case RISCV::BI__builtin_riscv_pslideupx_u16x2:
  case RISCV::BI__builtin_riscv_pslideupx_i8x8:
  case RISCV::BI__builtin_riscv_pslideupx_u8x8:
  case RISCV::BI__builtin_riscv_pslideupx_i16x4:
  case RISCV::BI__builtin_riscv_pslideupx_u16x4:
  case RISCV::BI__builtin_riscv_pslideupx_i32x2:
  case RISCV::BI__builtin_riscv_pslideupx_u32x2:
  case RISCV::BI__builtin_riscv_pslidedownx_i8x4:
  case RISCV::BI__builtin_riscv_pslidedownx_u8x4:
  case RISCV::BI__builtin_riscv_pslidedownx_i16x2:
  case RISCV::BI__builtin_riscv_pslidedownx_u16x2:
  case RISCV::BI__builtin_riscv_pslidedownx_i8x8:
  case RISCV::BI__builtin_riscv_pslidedownx_u8x8:
  case RISCV::BI__builtin_riscv_pslidedownx_i16x4:
  case RISCV::BI__builtin_riscv_pslidedownx_u16x4:
  case RISCV::BI__builtin_riscv_pslidedownx_i32x2:
  case RISCV::BI__builtin_riscv_pslidedownx_u32x2: {
    auto *VecTy = cast<llvm::FixedVectorType>(Ops[0]->getType());
    unsigned N = VecTy->getNumElements();
    unsigned EltBits = VecTy->getScalarSizeInBits();
    unsigned VecBits = N * EltBits;
    llvm::Type *VecIntTy = Builder.getIntNTy(VecBits);
    bool IsUp = false;
    switch (BuiltinID) {
    case RISCV::BI__builtin_riscv_pslideupx_i8x4:
    case RISCV::BI__builtin_riscv_pslideupx_u8x4:
    case RISCV::BI__builtin_riscv_pslideupx_i16x2:
    case RISCV::BI__builtin_riscv_pslideupx_u16x2:
    case RISCV::BI__builtin_riscv_pslideupx_i8x8:
    case RISCV::BI__builtin_riscv_pslideupx_u8x8:
    case RISCV::BI__builtin_riscv_pslideupx_i16x4:
    case RISCV::BI__builtin_riscv_pslideupx_u16x4:
    case RISCV::BI__builtin_riscv_pslideupx_i32x2:
    case RISCV::BI__builtin_riscv_pslideupx_u32x2:
      IsUp = true;
      break;
    default:
      break;
    }
    llvm::Value *RdInt = Builder.CreateBitCast(Ops[0], VecIntTy);
    llvm::Value *Rs1Int = Builder.CreateBitCast(Ops[1], VecIntTy);
    // Mask the element count to log2(N) bits, then multiply by elt_bits.
    llvm::Value *Rs2 = Builder.CreateAnd(
        Ops[2], llvm::ConstantInt::get(Ops[2]->getType(), N - 1));
    llvm::Value *Rs2Vec = Builder.CreateZExtOrTrunc(Rs2, VecIntTy);
    llvm::Value *Shamt = Builder.CreateMul(
        Rs2Vec, llvm::ConstantInt::get(VecIntTy, EltBits));
    llvm::Function *F = CGM.getIntrinsic(
        IsUp ? Intrinsic::fshl : Intrinsic::fshr, VecIntTy);
    llvm::Value *Result = IsUp
        ? Builder.CreateCall(F, {RdInt, Rs1Int, Shamt})
        : Builder.CreateCall(F, {Rs1Int, RdInt, Shamt});
    return Builder.CreateBitCast(Result, ConvertType(E->getType()));
  }

  // Slide 1 up / down. rd is the packed vector, rs1 is a scalar new element.
  //   slide1up:   result = (rd << elt_bits) | (rs1 & elt_mask)
  //               i.e. [rs1, rd[0], rd[1], ..., rd[N-2]]
  //   slide1down: result = (rd >> elt_bits) | (rs1 << (N-1)*elt_bits)
  //               i.e. [rd[1], rd[2], ..., rd[N-1], rs1]
  case RISCV::BI__builtin_riscv_pslide1up_i8x4:
  case RISCV::BI__builtin_riscv_pslide1up_u8x4:
  case RISCV::BI__builtin_riscv_pslide1up_i16x2:
  case RISCV::BI__builtin_riscv_pslide1up_u16x2:
  case RISCV::BI__builtin_riscv_pslide1up_i8x8:
  case RISCV::BI__builtin_riscv_pslide1up_u8x8:
  case RISCV::BI__builtin_riscv_pslide1up_i16x4:
  case RISCV::BI__builtin_riscv_pslide1up_u16x4:
  case RISCV::BI__builtin_riscv_pslide1up_i32x2:
  case RISCV::BI__builtin_riscv_pslide1up_u32x2:
  case RISCV::BI__builtin_riscv_pslide1down_i8x4:
  case RISCV::BI__builtin_riscv_pslide1down_u8x4:
  case RISCV::BI__builtin_riscv_pslide1down_i16x2:
  case RISCV::BI__builtin_riscv_pslide1down_u16x2:
  case RISCV::BI__builtin_riscv_pslide1down_i8x8:
  case RISCV::BI__builtin_riscv_pslide1down_u8x8:
  case RISCV::BI__builtin_riscv_pslide1down_i16x4:
  case RISCV::BI__builtin_riscv_pslide1down_u16x4:
  case RISCV::BI__builtin_riscv_pslide1down_i32x2:
  case RISCV::BI__builtin_riscv_pslide1down_u32x2: {
    auto *VecTy = cast<llvm::FixedVectorType>(Ops[0]->getType());
    unsigned N = VecTy->getNumElements();
    unsigned EltBits = VecTy->getScalarSizeInBits();
    unsigned VecBits = N * EltBits;
    llvm::Type *VecIntTy = Builder.getIntNTy(VecBits);
    bool IsUp = false;
    switch (BuiltinID) {
    case RISCV::BI__builtin_riscv_pslide1up_i8x4:
    case RISCV::BI__builtin_riscv_pslide1up_u8x4:
    case RISCV::BI__builtin_riscv_pslide1up_i16x2:
    case RISCV::BI__builtin_riscv_pslide1up_u16x2:
    case RISCV::BI__builtin_riscv_pslide1up_i8x8:
    case RISCV::BI__builtin_riscv_pslide1up_u8x8:
    case RISCV::BI__builtin_riscv_pslide1up_i16x4:
    case RISCV::BI__builtin_riscv_pslide1up_u16x4:
    case RISCV::BI__builtin_riscv_pslide1up_i32x2:
    case RISCV::BI__builtin_riscv_pslide1up_u32x2:
      IsUp = true;
      break;
    default:
      break;
    }
    llvm::Value *RdInt = Builder.CreateBitCast(Ops[0], VecIntTy);
    llvm::Value *Rs1Zext = Builder.CreateZExt(Ops[1], VecIntTy);
    llvm::Value *Shifted, *Filler;
    if (IsUp) {
      Shifted = Builder.CreateShl(RdInt, EltBits);
      Filler = Rs1Zext;  // rs1 lives in the low elt
    } else {
      Shifted = Builder.CreateLShr(RdInt, EltBits);
      Filler = Builder.CreateShl(Rs1Zext, (N - 1) * EltBits);
    }
    llvm::Value *Result = Builder.CreateOr(Shifted, Filler);
    return Builder.CreateBitCast(Result, ConvertType(E->getType()));
  }

  // Packed Subvector Join. Concatenate two 32-bit subvectors into a 64-bit
  // packed vector via shufflevector.
  case RISCV::BI__builtin_riscv_pjoin2_i8x8:
  case RISCV::BI__builtin_riscv_pjoin2_u8x8:
  case RISCV::BI__builtin_riscv_pjoin2_i16x4:
  case RISCV::BI__builtin_riscv_pjoin2_u16x4: {
    auto *SubVT = cast<llvm::FixedVectorType>(Ops[0]->getType());
    unsigned N = SubVT->getNumElements();
    SmallVector<int, 8> Mask;
    for (unsigned I = 0; I < 2 * N; ++I)
      Mask.push_back(static_cast<int>(I));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Mask);
  }

  // Packed Subvector Insert and Extract. Extract / insert a half-sized
  // subvector at index 0 (low) or 1 (high) of a 64-bit packed vector. idx
  // is a compile-time constant.
  case RISCV::BI__builtin_riscv_pget_i8x8_i8x4:
  case RISCV::BI__builtin_riscv_pget_u8x8_u8x4:
  case RISCV::BI__builtin_riscv_pget_i16x4_i16x2:
  case RISCV::BI__builtin_riscv_pget_u16x4_u16x2: {
    auto *WideVT = cast<llvm::FixedVectorType>(Ops[0]->getType());
    unsigned N = WideVT->getNumElements() / 2;
    uint64_t Idx = cast<llvm::ConstantInt>(Ops[1])->getZExtValue();
    SmallVector<int, 4> Mask;
    for (unsigned I = 0; I < N; ++I)
      Mask.push_back(static_cast<int>(Idx * N + I));
    return Builder.CreateShuffleVector(Ops[0], Mask);
  }
  case RISCV::BI__builtin_riscv_pset_i8x4_i8x8:
  case RISCV::BI__builtin_riscv_pset_u8x4_u8x8:
  case RISCV::BI__builtin_riscv_pset_i16x2_i16x4:
  case RISCV::BI__builtin_riscv_pset_u16x2_u16x4: {
    // Materialize via integer mask/shift/or so the backend can reduce the
    // result to a pack / mv depending on XLEN. (Element-wise shuffles tend
    // to fall apart into per-byte shifts on wide types.)
    auto *WideVT = cast<llvm::FixedVectorType>(Ops[0]->getType());
    auto *SubVT = cast<llvm::FixedVectorType>(Ops[1]->getType());
    unsigned VecBits = WideVT->getNumElements() * WideVT->getScalarSizeInBits();
    unsigned SubBits = SubVT->getNumElements() * SubVT->getScalarSizeInBits();
    llvm::Type *WideIntTy = Builder.getIntNTy(VecBits);
    llvm::Type *SubIntTy = Builder.getIntNTy(SubBits);
    uint64_t Idx = cast<llvm::ConstantInt>(Ops[2])->getZExtValue();
    uint64_t HoleMask = ((1ULL << SubBits) - 1) << (Idx * SubBits);
    llvm::Value *VInt = Builder.CreateBitCast(Ops[0], WideIntTy);
    llvm::Value *SInt = Builder.CreateBitCast(Ops[1], SubIntTy);
    llvm::Value *SExt = Builder.CreateZExt(SInt, WideIntTy);
    llvm::Value *VKept = Builder.CreateAnd(
        VInt, llvm::ConstantInt::get(WideIntTy, ~HoleMask));
    llvm::Value *SShifted =
        Idx == 0 ? SExt : Builder.CreateShl(SExt, Idx * SubBits);
    llvm::Value *Merged = Builder.CreateOr(VKept, SShifted);
    return Builder.CreateBitCast(Merged, ConvertType(E->getType()));
  }

  // Packed Element Join. Build the result vector via an insertelement chain
  // over poison; SLP / DAG combining will reduce to pack / ppaire / mv as
  // appropriate.
  case RISCV::BI__builtin_riscv_pjoin4_i8x4:
  case RISCV::BI__builtin_riscv_pjoin4_u8x4:
  case RISCV::BI__builtin_riscv_pjoin2_i16x2:
  case RISCV::BI__builtin_riscv_pjoin2_u16x2:
  case RISCV::BI__builtin_riscv_pjoin4_i16x4:
  case RISCV::BI__builtin_riscv_pjoin4_u16x4:
  case RISCV::BI__builtin_riscv_pjoin2_i32x2:
  case RISCV::BI__builtin_riscv_pjoin2_u32x2: {
    llvm::Value *V = llvm::PoisonValue::get(ConvertType(E->getType()));
    for (unsigned I = 0, N = Ops.size(); I < N; ++I)
      V = Builder.CreateInsertElement(V, Ops[I], I);
    return V;
  }

  // Packed Load and Store. The spec guarantees the pointer is aligned to
  // the element size; emit a vector load/store with that alignment so the
  // backend can pick an unaligned wider load (lw/sw or ld/sd) or fall back
  // to byte-wise loads/stores when the subtarget can't do unaligned access.
  case RISCV::BI__builtin_riscv_pld_i8x4:
  case RISCV::BI__builtin_riscv_pld_u8x4:
  case RISCV::BI__builtin_riscv_pld_i16x2:
  case RISCV::BI__builtin_riscv_pld_u16x2:
  case RISCV::BI__builtin_riscv_pld_i8x8:
  case RISCV::BI__builtin_riscv_pld_u8x8:
  case RISCV::BI__builtin_riscv_pld_i16x4:
  case RISCV::BI__builtin_riscv_pld_u16x4:
  case RISCV::BI__builtin_riscv_pld_i32x2:
  case RISCV::BI__builtin_riscv_pld_u32x2: {
    unsigned EltBytes;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pld_i8x4:
    case RISCV::BI__builtin_riscv_pld_u8x4:
    case RISCV::BI__builtin_riscv_pld_i8x8:
    case RISCV::BI__builtin_riscv_pld_u8x8:
      EltBytes = 1;
      break;
    case RISCV::BI__builtin_riscv_pld_i16x2:
    case RISCV::BI__builtin_riscv_pld_u16x2:
    case RISCV::BI__builtin_riscv_pld_i16x4:
    case RISCV::BI__builtin_riscv_pld_u16x4:
      EltBytes = 2;
      break;
    case RISCV::BI__builtin_riscv_pld_i32x2:
    case RISCV::BI__builtin_riscv_pld_u32x2:
      EltBytes = 4;
      break;
    }
    llvm::Type *VecTy = ConvertType(E->getType());
    return Builder.CreateAlignedLoad(VecTy, Ops[0], llvm::Align(EltBytes));
  }
  case RISCV::BI__builtin_riscv_pst_i8x4:
  case RISCV::BI__builtin_riscv_pst_u8x4:
  case RISCV::BI__builtin_riscv_pst_i16x2:
  case RISCV::BI__builtin_riscv_pst_u16x2:
  case RISCV::BI__builtin_riscv_pst_i8x8:
  case RISCV::BI__builtin_riscv_pst_u8x8:
  case RISCV::BI__builtin_riscv_pst_i16x4:
  case RISCV::BI__builtin_riscv_pst_u16x4:
  case RISCV::BI__builtin_riscv_pst_i32x2:
  case RISCV::BI__builtin_riscv_pst_u32x2: {
    unsigned EltBytes;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_pst_i8x4:
    case RISCV::BI__builtin_riscv_pst_u8x4:
    case RISCV::BI__builtin_riscv_pst_i8x8:
    case RISCV::BI__builtin_riscv_pst_u8x8:
      EltBytes = 1;
      break;
    case RISCV::BI__builtin_riscv_pst_i16x2:
    case RISCV::BI__builtin_riscv_pst_u16x2:
    case RISCV::BI__builtin_riscv_pst_i16x4:
    case RISCV::BI__builtin_riscv_pst_u16x4:
      EltBytes = 2;
      break;
    case RISCV::BI__builtin_riscv_pst_i32x2:
    case RISCV::BI__builtin_riscv_pst_u32x2:
      EltBytes = 4;
      break;
    }
    return Builder.CreateAlignedStore(Ops[1], Ops[0], llvm::Align(EltBytes));
  }

  // Scalar P intrinsics (C class). The underlying IR intrinsic is overloaded
  // on an integer type. On RV64 the i32-typed arguments must be widened to i64
  // (XLenVT) for the intrinsic call, and the result truncated back to i32.
  case RISCV::BI__builtin_riscv_asub_i32:
  case RISCV::BI__builtin_riscv_asubu_u32:
  case RISCV::BI__builtin_riscv_ssh1sadd_i32:
  case RISCV::BI__builtin_riscv_mulhr_i32:
  case RISCV::BI__builtin_riscv_mulhru_u32:
  case RISCV::BI__builtin_riscv_mulhrsu_i32:
  case RISCV::BI__builtin_riscv_mulq_i32:
  case RISCV::BI__builtin_riscv_mulqr_i32:
  case RISCV::BI__builtin_riscv_mseq_i32:
  case RISCV::BI__builtin_riscv_mseq_u32:
  case RISCV::BI__builtin_riscv_mslt_u32:
  case RISCV::BI__builtin_riscv_msgt_u32:
  case RISCV::BI__builtin_riscv_msltu_u32:
  case RISCV::BI__builtin_riscv_msgtu_u32: {
    unsigned IntID;
    bool SwapOps = false;
    bool Signed = true;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_asub_i32:
      IntID = Intrinsic::riscv_asub_i32; break;
    case RISCV::BI__builtin_riscv_asubu_u32:
      IntID = Intrinsic::riscv_asubu_i32; Signed = false; break;
    case RISCV::BI__builtin_riscv_ssh1sadd_i32:
      IntID = Intrinsic::riscv_ssh1sadd_i32; break;
    case RISCV::BI__builtin_riscv_mulhr_i32:
      IntID = Intrinsic::riscv_mulhr_i32; break;
    case RISCV::BI__builtin_riscv_mulhru_u32:
      IntID = Intrinsic::riscv_mulhru_i32; Signed = false; break;
    case RISCV::BI__builtin_riscv_mulhrsu_i32:
      IntID = Intrinsic::riscv_mulhrsu_i32; break;
    case RISCV::BI__builtin_riscv_mulq_i32:
      IntID = Intrinsic::riscv_mulq_i32; break;
    case RISCV::BI__builtin_riscv_mulqr_i32:
      IntID = Intrinsic::riscv_mulqr_i32; break;
    case RISCV::BI__builtin_riscv_mseq_i32:
    case RISCV::BI__builtin_riscv_mseq_u32:
      IntID = Intrinsic::riscv_mseq_i32; break;
    case RISCV::BI__builtin_riscv_mslt_u32:
      IntID = Intrinsic::riscv_mslt_i32; break;
    case RISCV::BI__builtin_riscv_msgt_u32:
      IntID = Intrinsic::riscv_mslt_i32; SwapOps = true; break;
    case RISCV::BI__builtin_riscv_msltu_u32:
      IntID = Intrinsic::riscv_msltu_i32; Signed = false; break;
    case RISCV::BI__builtin_riscv_msgtu_u32:
      IntID = Intrinsic::riscv_msltu_i32; SwapOps = true; Signed = false; break;
    }
    llvm::Value *Op0 = Ops[0];
    llvm::Value *Op1 = Ops[1];
    if (SwapOps)
      std::swap(Op0, Op1);
    // On RV64 widen i32 inputs to i64 (XLenVT), call the intrinsic, then
    // truncate the result back to i32.
    llvm::Type *XLenTy = getTarget().getTriple().isRISCV64() ? Int64Ty : Int32Ty;
    if (XLenTy != Int32Ty) {
      Op0 = Signed ? Builder.CreateSExt(Op0, XLenTy)
                   : Builder.CreateZExt(Op0, XLenTy);
      Op1 = Signed ? Builder.CreateSExt(Op1, XLenTy)
                   : Builder.CreateZExt(Op1, XLenTy);
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, {XLenTy});
    llvm::Value *Call = Builder.CreateCall(F, {Op0, Op1});
    if (Call->getType() != ResultType)
      Call = Builder.CreateTrunc(Call, ResultType);
    return Call;
  }

  // Scalar multiply-high accumulate (3 operands, all i32).
  case RISCV::BI__builtin_riscv_mhacc_i32:
  case RISCV::BI__builtin_riscv_mhracc_i32:
  case RISCV::BI__builtin_riscv_mhaccu_u32:
  case RISCV::BI__builtin_riscv_mhraccu_u32:
  case RISCV::BI__builtin_riscv_mhaccsu_i32:
  case RISCV::BI__builtin_riscv_mhraccsu_i32: {
    unsigned IntID;
    bool Op1Signed = true, Op2Signed = true;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_mhacc_i32:
      IntID = Intrinsic::riscv_mhacc_i32; break;
    case RISCV::BI__builtin_riscv_mhracc_i32:
      IntID = Intrinsic::riscv_mhracc_i32; break;
    case RISCV::BI__builtin_riscv_mhaccu_u32:
      IntID = Intrinsic::riscv_mhaccu_i32;
      Op1Signed = false; Op2Signed = false; break;
    case RISCV::BI__builtin_riscv_mhraccu_u32:
      IntID = Intrinsic::riscv_mhraccu_i32;
      Op1Signed = false; Op2Signed = false; break;
    case RISCV::BI__builtin_riscv_mhaccsu_i32:
      IntID = Intrinsic::riscv_mhaccsu_i32;
      Op2Signed = false; break;
    case RISCV::BI__builtin_riscv_mhraccsu_i32:
      IntID = Intrinsic::riscv_mhraccsu_i32;
      Op2Signed = false; break;
    }
    llvm::Value *Rd = Ops[0], *Rs1 = Ops[1], *Rs2 = Ops[2];
    llvm::Type *XLenTy = getTarget().getTriple().isRISCV64() ? Int64Ty : Int32Ty;
    if (XLenTy != Int32Ty) {
      // rd has the result's signedness; pick signed extend for signed builtins
      // (mhacc, mhracc, mhaccsu, mhraccsu) and zero extend for the unsigned
      // accumulating forms.
      bool RdSigned = (BuiltinID != RISCV::BI__builtin_riscv_mhaccu_u32 &&
                       BuiltinID != RISCV::BI__builtin_riscv_mhraccu_u32);
      Rd = RdSigned ? Builder.CreateSExt(Rd, XLenTy)
                    : Builder.CreateZExt(Rd, XLenTy);
      Rs1 = Op1Signed ? Builder.CreateSExt(Rs1, XLenTy)
                      : Builder.CreateZExt(Rs1, XLenTy);
      Rs2 = Op2Signed ? Builder.CreateSExt(Rs2, XLenTy)
                      : Builder.CreateZExt(Rs2, XLenTy);
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, {XLenTy});
    llvm::Value *Call = Builder.CreateCall(F, {Rd, Rs1, Rs2});
    if (Call->getType() != ResultType)
      Call = Builder.CreateTrunc(Call, ResultType);
    return Call;
  }

  // Saturate-immediate: (rs1, shamt). shamt is a constant.
  case RISCV::BI__builtin_riscv_sati_i32:
  case RISCV::BI__builtin_riscv_usati_u32:
  case RISCV::BI__builtin_riscv_sati_i64:
  case RISCV::BI__builtin_riscv_usati_u64: {
    unsigned IntID;
    bool Is64;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_sati_i32:
      IntID = Intrinsic::riscv_sati_i32; Is64 = false; break;
    case RISCV::BI__builtin_riscv_usati_u32:
      IntID = Intrinsic::riscv_usati_i32; Is64 = false; break;
    case RISCV::BI__builtin_riscv_sati_i64:
      IntID = Intrinsic::riscv_sati_i64; Is64 = true; break;
    case RISCV::BI__builtin_riscv_usati_u64:
      IntID = Intrinsic::riscv_usati_i64; Is64 = true; break;
    }
    llvm::Type *XLenTy = Is64 ? Int64Ty :
                        (getTarget().getTriple().isRISCV64() ? Int64Ty : Int32Ty);
    llvm::Value *Op0 = Ops[0];
    if (Op0->getType() != XLenTy)
      Op0 = Builder.CreateSExt(Op0, XLenTy);
    // The shamt must remain i32 (the IR intrinsic signature is (T, i32)).
    llvm::Value *Shamt = Builder.CreateZExtOrTrunc(Ops[1], Int32Ty);
    llvm::Function *F = CGM.getIntrinsic(IntID, {XLenTy});
    llvm::Value *Call = Builder.CreateCall(F, {Op0, Shamt});
    if (Call->getType() != ResultType)
      Call = Builder.CreateTrunc(Call, ResultType);
    return Call;
  }

  // Q-format multiply with widening accumulate: (i64 acc, i32 a, i32 b) -> i64.
  // The narrow inputs are XLenVT-typed in the intrinsic; sign-extend the i32
  // args to i64 on RV64.
  case RISCV::BI__builtin_riscv_mqwacc_i64:
  case RISCV::BI__builtin_riscv_mqrwacc_i64: {
    unsigned IntID = BuiltinID == RISCV::BI__builtin_riscv_mqwacc_i64
                         ? Intrinsic::riscv_mqwacc_i64
                         : Intrinsic::riscv_mqrwacc_i64;
    llvm::Type *XLenTy = getTarget().getTriple().isRISCV64() ? Int64Ty : Int32Ty;
    llvm::Value *A = Ops[1];
    llvm::Value *B = Ops[2];
    if (XLenTy != Int32Ty) {
      A = Builder.CreateSExt(A, XLenTy);
      B = Builder.CreateSExt(B, XLenTy);
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, {XLenTy});
    return Builder.CreateCall(F, {Ops[0], A, B});
  }

  // RV64-only: rev16. Reverse 16-bit chunks within an i64.
  case RISCV::BI__builtin_riscv_rev16_64: {
    llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_rev16, {Int64Ty});
    return Builder.CreateCall(F, Ops);
  }

  // RV64-only: 64-bit zip / unzip pairs. Direct i64 -> i64 IR intrinsics.
  case RISCV::BI__builtin_riscv_zip8p_64:
  case RISCV::BI__builtin_riscv_zip16p_64:
  case RISCV::BI__builtin_riscv_zip8hp_64:
  case RISCV::BI__builtin_riscv_zip16hp_64:
  case RISCV::BI__builtin_riscv_unzip8p_64:
  case RISCV::BI__builtin_riscv_unzip16p_64:
  case RISCV::BI__builtin_riscv_unzip8hp_64:
  case RISCV::BI__builtin_riscv_unzip16hp_64: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_zip8p_64:
      IntID = Intrinsic::riscv_zip8p_64; break;
    case RISCV::BI__builtin_riscv_zip16p_64:
      IntID = Intrinsic::riscv_zip16p_64; break;
    case RISCV::BI__builtin_riscv_zip8hp_64:
      IntID = Intrinsic::riscv_zip8hp_64; break;
    case RISCV::BI__builtin_riscv_zip16hp_64:
      IntID = Intrinsic::riscv_zip16hp_64; break;
    case RISCV::BI__builtin_riscv_unzip8p_64:
      IntID = Intrinsic::riscv_unzip8p_64; break;
    case RISCV::BI__builtin_riscv_unzip16p_64:
      IntID = Intrinsic::riscv_unzip16p_64; break;
    case RISCV::BI__builtin_riscv_unzip8hp_64:
      IntID = Intrinsic::riscv_unzip8hp_64; break;
    case RISCV::BI__builtin_riscv_unzip16hp_64:
      IntID = Intrinsic::riscv_unzip16hp_64; break;
    }
    llvm::Function *F = CGM.getIntrinsic(IntID);
    return Builder.CreateCall(F, Ops);
  }

  // Saturating compound shifts with signed shift amount. The IR intrinsic is
  // overloaded on the rs1/rs2/result integer type (XLenVT). On RV64 we
  // sign-extend the i32 rs1 / shift amount to i64 before the call and
  // truncate the result back to i32.
  case RISCV::BI__builtin_riscv_ssha_i32:
  case RISCV::BI__builtin_riscv_sshar_i32:
  case RISCV::BI__builtin_riscv_sshl_u32:
  case RISCV::BI__builtin_riscv_sshlr_u32: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_ssha_i32:
      IntID = Intrinsic::riscv_ssha_i32; break;
    case RISCV::BI__builtin_riscv_sshar_i32:
      IntID = Intrinsic::riscv_sshar_i32; break;
    case RISCV::BI__builtin_riscv_sshl_u32:
      IntID = Intrinsic::riscv_sshl_u32; break;
    case RISCV::BI__builtin_riscv_sshlr_u32:
      IntID = Intrinsic::riscv_sshlr_u32; break;
    }
    llvm::Type *XLenTy = getTarget().getTriple().isRISCV64() ? Int64Ty : Int32Ty;
    llvm::Value *Op0 = Ops[0];
    llvm::Value *Shamt = Ops[1];
    if (XLenTy != Int32Ty) {
      Op0 = Builder.CreateSExt(Op0, XLenTy);
      Shamt = Builder.CreateSExt(Shamt, XLenTy);
    }
    llvm::Function *F = CGM.getIntrinsic(IntID, {XLenTy});
    llvm::Value *Call = Builder.CreateCall(F, {Op0, Shamt});
    if (Call->getType() != ResultType)
      Call = Builder.CreateTrunc(Call, ResultType);
    return Call;
  }

  // RV64-only compound shifts with signed shift amount. Direct i64 -> i64
  // IR intrinsics; the i32 shift amount is sign-extended to i64.
  case RISCV::BI__builtin_riscv_sha_i64:
  case RISCV::BI__builtin_riscv_shar_i64:
  case RISCV::BI__builtin_riscv_shl_u64:
  case RISCV::BI__builtin_riscv_shlr_u64: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_sha_i64:
      IntID = Intrinsic::riscv_sha_i64; break;
    case RISCV::BI__builtin_riscv_shar_i64:
      IntID = Intrinsic::riscv_shar_i64; break;
    case RISCV::BI__builtin_riscv_shl_u64:
      IntID = Intrinsic::riscv_shl_u64; break;
    case RISCV::BI__builtin_riscv_shlr_u64:
      IntID = Intrinsic::riscv_shlr_u64; break;
    }
    llvm::Value *Shamt = Builder.CreateSExt(Ops[1], Int64Ty);
    llvm::Function *F = CGM.getIntrinsic(IntID);
    return Builder.CreateCall(F, {Ops[0], Shamt});
  }

  // Widening zip: 2 x u32 inputs, u64 output. Reuses int_riscv_pzip on v4i8
  // (byte zip) / v2i16 (halfword zip).
  case RISCV::BI__builtin_riscv_wzip8p_64:
  case RISCV::BI__builtin_riscv_wzip16p_64: {
    bool IsByteZip = BuiltinID == RISCV::BI__builtin_riscv_wzip8p_64;
    auto *V4I8Ty = llvm::FixedVectorType::get(Int8Ty, 4);
    auto *V2I16Ty = llvm::FixedVectorType::get(Int16Ty, 2);
    llvm::Type *VecTy = IsByteZip ? (llvm::Type *)V4I8Ty : (llvm::Type *)V2I16Ty;
    llvm::Value *Op0 = Builder.CreateBitCast(Ops[0], VecTy);
    llvm::Value *Op1 = Builder.CreateBitCast(Ops[1], VecTy);
    llvm::Function *F = CGM.getIntrinsic(Intrinsic::riscv_pzip, VecTy);
    return Builder.CreateCall(F, {Op0, Op1});
  }

  // Narrowing clip / rounding shift: i64 input, i32 output (RV32 only).
  // Reuses the existing pnclip* / pnsrar IR intrinsics; we request an i32
  // result, and LowerINTRINSIC_WO_CHAIN selects the scalar nclip / nsrar
  // form when the output type is i32.
  case RISCV::BI__builtin_riscv_nclipu_u32:
  case RISCV::BI__builtin_riscv_nclipru_u32:
  case RISCV::BI__builtin_riscv_nclip_i32:
  case RISCV::BI__builtin_riscv_nclipr_i32:
  case RISCV::BI__builtin_riscv_nsrar_i32: {
    unsigned IntID;
    switch (BuiltinID) {
    default: llvm_unreachable("unexpected builtin");
    case RISCV::BI__builtin_riscv_nclipu_u32:
      IntID = Intrinsic::riscv_pnclipu; break;
    case RISCV::BI__builtin_riscv_nclipru_u32:
      IntID = Intrinsic::riscv_pnclipru; break;
    case RISCV::BI__builtin_riscv_nclip_i32:
      IntID = Intrinsic::riscv_pnclip; break;
    case RISCV::BI__builtin_riscv_nclipr_i32:
      IntID = Intrinsic::riscv_pnclipr; break;
    case RISCV::BI__builtin_riscv_nsrar_i32:
      IntID = Intrinsic::riscv_pnsrar; break;
    }
    llvm::Value *Op1 = Builder.CreateZExtOrTrunc(Ops[1], Int32Ty);
    llvm::Function *F = CGM.getIntrinsic(IntID, Int32Ty);
    return Builder.CreateCall(F, {Ops[0], Op1});
  }

  case RISCV::BI__builtin_riscv_clz_32:
  case RISCV::BI__builtin_riscv_clz_64: {
    Function *F = CGM.getIntrinsic(Intrinsic::ctlz, Ops[0]->getType());
    Value *Result = Builder.CreateCall(F, {Ops[0], Builder.getInt1(false)});
    if (Result->getType() != ResultType)
      Result =
          Builder.CreateIntCast(Result, ResultType, /*isSigned*/ false, "cast");
    return Result;
  }
  case RISCV::BI__builtin_riscv_ctz_32:
  case RISCV::BI__builtin_riscv_ctz_64: {
    Function *F = CGM.getIntrinsic(Intrinsic::cttz, Ops[0]->getType());
    Value *Result = Builder.CreateCall(F, {Ops[0], Builder.getInt1(false)});
    if (Result->getType() != ResultType)
      Result =
          Builder.CreateIntCast(Result, ResultType, /*isSigned*/ false, "cast");
    return Result;
  }

  // Zihintntl
  case RISCV::BI__builtin_riscv_ntl_load: {
    llvm::Type *ResTy = ConvertType(E->getType());
    unsigned DomainVal = 5; // Default __RISCV_NTLH_ALL
    if (Ops.size() == 2)
      DomainVal = cast<ConstantInt>(Ops[1])->getZExtValue();

    llvm::MDNode *RISCVDomainNode = llvm::MDNode::get(
        getLLVMContext(),
        llvm::ConstantAsMetadata::get(Builder.getInt32(DomainVal)));
    llvm::MDNode *NontemporalNode = llvm::MDNode::get(
        getLLVMContext(), llvm::ConstantAsMetadata::get(Builder.getInt32(1)));

    int Width;
    if(ResTy->isScalableTy()) {
      const ScalableVectorType *SVTy = cast<ScalableVectorType>(ResTy);
      llvm::Type *ScalarTy = ResTy->getScalarType();
      Width = ScalarTy->getPrimitiveSizeInBits() *
              SVTy->getElementCount().getKnownMinValue();
    } else
      Width = ResTy->getPrimitiveSizeInBits();
    LoadInst *Load = Builder.CreateLoad(
        Address(Ops[0], ResTy, CharUnits::fromQuantity(Width / 8)));

    Load->setMetadata(llvm::LLVMContext::MD_nontemporal, NontemporalNode);
    Load->setMetadata(CGM.getModule().getMDKindID("riscv-nontemporal-domain"),
                      RISCVDomainNode);

    return Load;
  }
  case RISCV::BI__builtin_riscv_ntl_store: {
    unsigned DomainVal = 5; // Default __RISCV_NTLH_ALL
    if (Ops.size() == 3)
      DomainVal = cast<ConstantInt>(Ops[2])->getZExtValue();

    llvm::MDNode *RISCVDomainNode = llvm::MDNode::get(
        getLLVMContext(),
        llvm::ConstantAsMetadata::get(Builder.getInt32(DomainVal)));
    llvm::MDNode *NontemporalNode = llvm::MDNode::get(
        getLLVMContext(), llvm::ConstantAsMetadata::get(Builder.getInt32(1)));

    StoreInst *Store = Builder.CreateDefaultAlignedStore(Ops[1], Ops[0]);
    Store->setMetadata(llvm::LLVMContext::MD_nontemporal, NontemporalNode);
    Store->setMetadata(CGM.getModule().getMDKindID("riscv-nontemporal-domain"),
                       RISCVDomainNode);

    return Store;
  }
  // Zihintpause
  case RISCV::BI__builtin_riscv_pause: {
    llvm::Function *Fn = CGM.getIntrinsic(llvm::Intrinsic::riscv_pause);
    return Builder.CreateCall(Fn, {});
  }

  // XCValu
  case RISCV::BI__builtin_riscv_cv_alu_addN:
    ID = Intrinsic::riscv_cv_alu_addN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_addRN:
    ID = Intrinsic::riscv_cv_alu_addRN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_adduN:
    ID = Intrinsic::riscv_cv_alu_adduN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_adduRN:
    ID = Intrinsic::riscv_cv_alu_adduRN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_clip:
    ID = Intrinsic::riscv_cv_alu_clip;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_clipu:
    ID = Intrinsic::riscv_cv_alu_clipu;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_extbs:
    return Builder.CreateSExt(Builder.CreateTrunc(Ops[0], Int8Ty), Int32Ty,
                              "extbs");
  case RISCV::BI__builtin_riscv_cv_alu_extbz:
    return Builder.CreateZExt(Builder.CreateTrunc(Ops[0], Int8Ty), Int32Ty,
                              "extbz");
  case RISCV::BI__builtin_riscv_cv_alu_exths:
    return Builder.CreateSExt(Builder.CreateTrunc(Ops[0], Int16Ty), Int32Ty,
                              "exths");
  case RISCV::BI__builtin_riscv_cv_alu_exthz:
    return Builder.CreateZExt(Builder.CreateTrunc(Ops[0], Int16Ty), Int32Ty,
                              "exthz");
  case RISCV::BI__builtin_riscv_cv_alu_sle:
    return Builder.CreateZExt(Builder.CreateICmpSLE(Ops[0], Ops[1]), Int32Ty,
                              "sle");
  case RISCV::BI__builtin_riscv_cv_alu_sleu:
    return Builder.CreateZExt(Builder.CreateICmpULE(Ops[0], Ops[1]), Int32Ty,
                              "sleu");
  case RISCV::BI__builtin_riscv_cv_alu_subN:
    ID = Intrinsic::riscv_cv_alu_subN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_subRN:
    ID = Intrinsic::riscv_cv_alu_subRN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_subuN:
    ID = Intrinsic::riscv_cv_alu_subuN;
    break;
  case RISCV::BI__builtin_riscv_cv_alu_subuRN:
    ID = Intrinsic::riscv_cv_alu_subuRN;
    break;

  // XAndesPerf
  case RISCV::BI__builtin_riscv_nds_ffb_32:
  case RISCV::BI__builtin_riscv_nds_ffb_64:
    IntrinsicTypes = {ResultType};
    ID = Intrinsic::riscv_nds_ffb;
    break;
  case RISCV::BI__builtin_riscv_nds_ffzmism_32:
  case RISCV::BI__builtin_riscv_nds_ffzmism_64:
    IntrinsicTypes = {ResultType};
    ID = Intrinsic::riscv_nds_ffzmism;
    break;
  case RISCV::BI__builtin_riscv_nds_ffmism_32:
  case RISCV::BI__builtin_riscv_nds_ffmism_64:
    IntrinsicTypes = {ResultType};
    ID = Intrinsic::riscv_nds_ffmism;
    break;
  case RISCV::BI__builtin_riscv_nds_flmism_32:
  case RISCV::BI__builtin_riscv_nds_flmism_64:
    IntrinsicTypes = {ResultType};
    ID = Intrinsic::riscv_nds_flmism;
    break;

  // XAndesBFHCvt
  case RISCV::BI__builtin_riscv_nds_fcvt_s_bf16:
    return Builder.CreateFPExt(Ops[0], FloatTy);
  case RISCV::BI__builtin_riscv_nds_fcvt_bf16_s:
    return Builder.CreateFPTrunc(Ops[0], BFloatTy);

    // Vector builtins are handled from here.
#include "clang/Basic/riscv_vector_builtin_cg.inc"

    // SiFive Vector builtins are handled from here.
#include "clang/Basic/riscv_sifive_vector_builtin_cg.inc"

    // Andes Vector builtins are handled from here.
#include "clang/Basic/riscv_andes_vector_builtin_cg.inc"
  }

  assert(ID != Intrinsic::not_intrinsic);

  llvm::Function *F = CGM.getIntrinsic(ID, IntrinsicTypes);
  return Builder.CreateCall(F, Ops, "");
}
