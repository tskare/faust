/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2018 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#ifndef _FBC_LLVM_COMPILER_H
#define _FBC_LLVM_COMPILER_H

#include <map>
#include <string>
#include <vector>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LegacyPassNameParser.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/LinkAllPasses.h>

#include "fbc_interpreter.hh"
#include "interpreter_bytecode.hh"

using namespace llvm;

typedef llvm::Value* LLVMValue;

#define dumpLLVM1(val)                           \
    {                                            \
        std::string        res;                  \
        raw_string_ostream out_str(res);         \
        out_str << *val;                         \
        std::cout << out_str.str() << std::endl; \
    }

#define dispatchReturn()  \
    {                     \
        it = popAddr();   \
    }
#define saveReturn()      \
    {                     \
        pushAddr(it + 1); \
    }

// FBC LLVM compiler
template <class T>
class FBCLLVMCompiler {
    typedef void (*compiledFun)(int* int_heap, T* real_heap, T** inputs, T** outputs);

   protected:
    llvm::ExecutionEngine* fJIT;
    llvm::Module*          fModule;
    llvm::IRBuilder<>*     fBuilder;
    LLVMContext*           fContext;
    compiledFun            fCompiledFun;

    LLVMValue     fLLVMStack[512];
    InstructionIT fAddressStack[64];

    int fLLVMStackIndex;
    int fAddrStackIndex;

    LLVMValue fLLVMIntHeap;
    LLVMValue fLLVMRealHeap;
    LLVMValue fLLVMInputs;
    LLVMValue fLLVMOutputs;

    LLVMContext& getContext() { return *fContext; }

    LLVMValue genFloat(float num) { return ConstantFP::get(fModule->getContext(), APFloat(num)); }
    LLVMValue genDouble(double num) { return ConstantFP::get(fModule->getContext(), APFloat(num)); }
    LLVMValue genReal(double num) { return (sizeof(T) == sizeof(double)) ? genDouble(num) : genFloat(num); }
    LLVMValue genInt32(int num) { return ConstantInt::get(llvm::Type::getInt32Ty(fModule->getContext()), num); }
    LLVMValue genInt64(long long num) { return ConstantInt::get(llvm::Type::getInt64Ty(fModule->getContext()), num); }

    llvm::Type* getFloatTy() { return llvm::Type::getFloatTy(fModule->getContext()); }
    llvm::Type* getInt32Ty() { return llvm::Type::getInt32Ty(fModule->getContext()); }
    llvm::Type* getInt64Ty() { return llvm::Type::getInt64Ty(fModule->getContext()); }
    llvm::Type* getInt1Ty() { return llvm::Type::getInt1Ty(fModule->getContext()); }
    llvm::Type* getDoubleTy() { return llvm::Type::getDoubleTy(fModule->getContext()); }
    llvm::Type* getRealTy() { return (sizeof(T) == sizeof(double)) ? getDoubleTy() : getFloatTy(); }

    std::string getMathName(const std::string& name) { return (sizeof(T) == sizeof(float)) ? (name + "f") : name; }

    void      pushValue(LLVMValue val) { fLLVMStack[fLLVMStackIndex++] = val; }
    LLVMValue popValue() { return fLLVMStack[--fLLVMStackIndex]; }

    void          pushAddr(InstructionIT addr) { fAddressStack[fAddrStackIndex++] = addr; }
    InstructionIT popAddr() { return fAddressStack[--fAddrStackIndex]; }
    bool          emptyReturn() { return (fAddrStackIndex == 0); }
    
    void pushBinop(llvm::Instruction::BinaryOps op)
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        pushValue(fBuilder->CreateBinOp(op, v1, v2));
    }
    
    void pushRealComp(CmpInst::Predicate op)
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateFCmp(op, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, genInt32(1), genInt32(0)));
    }
        
    void pushIntComp(CmpInst::Predicate op)
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateICmp(op, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, genInt32(1), genInt32(0)));
    }
    
    // Min/max
    void pushIntMax()
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateICmp(ICmpInst::ICMP_SLT, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, v2, v1));
    }
    
    void pushIntMin()
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateICmp(ICmpInst::ICMP_SLT, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, v1, v2));
    }
    
    void pushRealMax()
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateFCmp(FCmpInst::FCMP_OLT, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, v2, v1));
    }
    
    void pushRealMin()
    {
        LLVMValue v1 = popValue();
        LLVMValue v2 = popValue();
        LLVMValue cond_value = fBuilder->CreateFCmp(FCmpInst::FCMP_OLT, v1, v2);
        pushValue(fBuilder->CreateSelect(cond_value, v1, v2));
    }
  
    void pushUnaryCall(std::string name, llvm::Type* type, bool rename)
    {
        if (rename) name = getMathName(name);
        llvm::Function* function = fModule->getFunction(name);
        if (!function) {
            // Define it
            std::vector<llvm::Type*> fun_args_type;
            fun_args_type.push_back(type);
            llvm::FunctionType* fun_type = FunctionType::get(type, makeArrayRef(fun_args_type), false);
            function                     = Function::Create(fun_type, GlobalValue::ExternalLinkage, name, fModule);
            function->setCallingConv(CallingConv::C);
        }
        // Create the function call
        std::vector<LLVMValue> fun_args;
        fun_args.push_back(popValue());
        llvm::CallInst* call_inst = fBuilder->CreateCall(function, makeArrayRef(fun_args));
        call_inst->setCallingConv(CallingConv::C);
        pushValue(call_inst);
    }

    void pushUnaryIntCall(const std::string& name, bool rename = true) { return pushUnaryCall(name, getInt32Ty(), rename); }
    void pushUnaryRealCall(const std::string& name, bool rename = true) { return pushUnaryCall(name, getRealTy(), rename); }

    void pushBinaryCall(const std::string& name_aux, llvm::Type* res_type)
    {
        std::string name = getMathName(name_aux);
        llvm::Function* function = fModule->getFunction(name);
        if (!function) {
            // Define it
            std::vector<llvm::Type*> fun_args_type;
            fun_args_type.push_back(res_type);
            fun_args_type.push_back(res_type);
            llvm::FunctionType* fun_type = FunctionType::get(res_type, makeArrayRef(fun_args_type), false);
            function                     = Function::Create(fun_type, GlobalValue::ExternalLinkage, name, fModule);
            function->setCallingConv(CallingConv::C);
        }
        // Create the function call
        std::vector<LLVMValue> fun_args;
        fun_args.push_back(popValue());
        fun_args.push_back(popValue());
        llvm::CallInst* call_inst = fBuilder->CreateCall(function, makeArrayRef(fun_args));
        call_inst->setCallingConv(CallingConv::C);
        pushValue(call_inst);
    }

    void pushBinaryIntCall(const std::string& name) { pushBinaryCall(name, getInt32Ty()); }

    void pushBinaryRealCall(const std::string& name) { pushBinaryCall(name, getRealTy()); }

    void pushLoadArray(LLVMValue array, int index) { pushLoadArray(array, genInt32(index)); }

    void pushStoreArray(LLVMValue array, int index) { pushStoreArray(array, genInt32(index)); }

    void pushLoadArray(LLVMValue array, LLVMValue index)
    {
        LLVMValue load_ptr = fBuilder->CreateInBoundsGEP(array, index);
        pushValue(fBuilder->CreateLoad(load_ptr));
    }

    void pushStoreArray(LLVMValue array, LLVMValue index)
    {
        LLVMValue store_ptr = fBuilder->CreateInBoundsGEP(array, index);
        fBuilder->CreateStore(popValue(), store_ptr);
    }

    void pushLoadInput(int index)
    {
        LLVMValue input_ptr_ptr = fBuilder->CreateInBoundsGEP(fLLVMInputs, genInt32(index));
        LLVMValue input_ptr     = fBuilder->CreateLoad(input_ptr_ptr);
        LLVMValue input         = fBuilder->CreateInBoundsGEP(input_ptr, popValue());
        pushValue(fBuilder->CreateLoad(input));
    }

    void pushStoreOutput(int index)
    {
        LLVMValue output_ptr_ptr = fBuilder->CreateInBoundsGEP(fLLVMOutputs, genInt32(index));
        LLVMValue output_ptr     = fBuilder->CreateLoad(output_ptr_ptr);
        LLVMValue output         = fBuilder->CreateInBoundsGEP(output_ptr, popValue());
        fBuilder->CreateStore(popValue(), output);
    }

    void CompileBlock(FBCBlockInstruction<T>* block, BasicBlock* code_block)
    {
        InstructionIT it  = block->fInstructions.begin();
        bool          end = false;

        // Insert in the current block
        fBuilder->SetInsertPoint(code_block);

        while ((it != block->fInstructions.end()) && !end) {
            //(*it)->write(&std::cout);

            switch ((*it)->fOpcode) {
                    // Numbers
                case FBCInstruction::kRealValue:
                    pushValue(genReal((*it)->fRealValue));
                    it++;
                    break;

                case FBCInstruction::kInt32Value:
                    pushValue(genInt32((*it)->fIntValue));
                    it++;
                    break;

                    // Memory load/store
                case FBCInstruction::kLoadReal:
                    pushLoadArray(fLLVMRealHeap, (*it)->fOffset1);
                    it++;
                    break;

                case FBCInstruction::kLoadInt:
                    pushLoadArray(fLLVMIntHeap, (*it)->fOffset1);
                    it++;
                    break;

                case FBCInstruction::kStoreReal:
                    pushStoreArray(fLLVMRealHeap, (*it)->fOffset1);
                    it++;
                    break;

                case FBCInstruction::kStoreInt:
                    pushStoreArray(fLLVMIntHeap, (*it)->fOffset1);
                    it++;
                    break;

                    // Indexed memory load/store: constant values are added at generation time by CreateBinOp...
                case FBCInstruction::kLoadIndexedReal: {
                    LLVMValue offset = fBuilder->CreateBinOp(Instruction::Add, genInt32((*it)->fOffset1), popValue());
                    pushLoadArray(fLLVMRealHeap, offset);
                    it++;
                    break;
                }

                case FBCInstruction::kLoadIndexedInt: {
                    LLVMValue offset = fBuilder->CreateBinOp(Instruction::Add, genInt32((*it)->fOffset1), popValue());
                    pushLoadArray(fLLVMIntHeap, offset);
                    it++;
                    break;
                }

                case FBCInstruction::kStoreIndexedReal: {
                    LLVMValue offset = fBuilder->CreateBinOp(Instruction::Add, genInt32((*it)->fOffset1), popValue());
                    pushStoreArray(fLLVMRealHeap, offset);
                    it++;
                    break;
                }

                case FBCInstruction::kStoreIndexedInt: {
                    LLVMValue offset = fBuilder->CreateBinOp(Instruction::Add, genInt32((*it)->fOffset1), popValue());
                    pushStoreArray(fLLVMIntHeap, offset);
                    it++;
                    break;
                }

                    // Memory shift (TODO : use memmove ?)
                case FBCInstruction::kBlockShiftReal: {
                    for (int i = (*it)->fOffset1; i > (*it)->fOffset2; i -= 1) {
                        pushLoadArray(fLLVMRealHeap, i - 1);
                        pushStoreArray(fLLVMRealHeap, i);
                    }
                    it++;
                    break;
                }

                case FBCInstruction::kBlockShiftInt: {
                    for (int i = (*it)->fOffset1; i > (*it)->fOffset2; i -= 1) {
                        pushLoadArray(fLLVMIntHeap, i - 1);
                        pushStoreArray(fLLVMIntHeap, i);
                    }
                    it++;
                    break;
                }

                    // Input/output
                case FBCInstruction::kLoadInput:
                    pushLoadInput((*it)->fOffset1);
                    it++;
                    break;

                case FBCInstruction::kStoreOutput:
                    pushStoreOutput((*it)->fOffset1);
                    it++;
                    break;

                    // Cast
                case FBCInstruction::kCastReal: {
                    LLVMValue val = popValue();
                    pushValue(fBuilder->CreateSIToFP(val, getRealTy()));
                    it++;
                    break;
                }

                case FBCInstruction::kCastInt: {
                    LLVMValue val = popValue();
                    pushValue(fBuilder->CreateFPToSI(val, getInt32Ty()));
                    it++;
                    break;
                }

                case FBCInstruction::kBitcastInt: {
                    LLVMValue val = popValue();
                    pushValue(fBuilder->CreateBitCast(val, getInt32Ty()));
                    it++;
                    break;
                }

                case FBCInstruction::kBitcastReal: {
                    LLVMValue val = popValue();
                    pushValue(fBuilder->CreateBitCast(val, getRealTy()));
                    it++;
                    break;
                }

                    // Binary math
                case FBCInstruction::kAddReal:
                    pushBinop(Instruction::FAdd);
                    it++;
                    break;

                case FBCInstruction::kAddInt:
                    pushBinop(Instruction::Add);
                    it++;
                    break;

                case FBCInstruction::kSubReal:
                    pushBinop(Instruction::FSub);
                    it++;
                    break;

                case FBCInstruction::kSubInt:
                    pushBinop(Instruction::Sub);
                    it++;
                    break;

                case FBCInstruction::kMultReal:
                    pushBinop(Instruction::FMul);
                    it++;
                    break;

                case FBCInstruction::kMultInt:
                    pushBinop(Instruction::Mul);
                    it++;
                    break;

                case FBCInstruction::kDivReal:
                    pushBinop(Instruction::FDiv);
                    it++;
                    break;

                case FBCInstruction::kDivInt:
                    pushBinop(Instruction::SDiv);
                    it++;
                    break;

                case FBCInstruction::kRemReal:
                    pushBinaryRealCall("remainder");
                    it++;
                    break;

                case FBCInstruction::kRemInt:
                    pushBinop(Instruction::SRem);
                    it++;
                    break;

                case FBCInstruction::kLshInt:
                    pushBinop(Instruction::Shl);
                    it++;
                    break;

                case FBCInstruction::kRshInt:
                    pushBinop(Instruction::LShr);
                    it++;
                    break;

                case FBCInstruction::kGTInt:
                    pushIntComp(ICmpInst::ICMP_SGT);
                    it++;
                    break;

                case FBCInstruction::kLTInt:
                    pushIntComp(ICmpInst::ICMP_SLT);
                    it++;
                    break;

                case FBCInstruction::kGEInt:
                    pushIntComp(ICmpInst::ICMP_SGE);
                    it++;
                    break;

                case FBCInstruction::kLEInt:
                    pushIntComp(ICmpInst::ICMP_SLE);
                    it++;
                    break;

                case FBCInstruction::kEQInt:
                    pushIntComp(ICmpInst::ICMP_EQ);
                    it++;
                    break;

                case FBCInstruction::kNEInt:
                    pushIntComp(ICmpInst::ICMP_NE);
                    it++;
                    break;

                case FBCInstruction::kGTReal:
                    pushRealComp(FCmpInst::FCMP_OGT);
                    it++;
                    break;

                case FBCInstruction::kLTReal:
                    pushRealComp(FCmpInst::FCMP_OLT);
                    it++;
                    break;

                case FBCInstruction::kGEReal:
                    pushRealComp(FCmpInst::FCMP_OGE);
                    it++;
                    break;

                case FBCInstruction::kLEReal:
                    pushRealComp(FCmpInst::FCMP_OLE);
                    it++;
                    break;

                case FBCInstruction::kEQReal:
                    pushRealComp(FCmpInst::FCMP_OEQ);
                    it++;
                    break;

                case FBCInstruction::kNEReal:
                    pushRealComp(FCmpInst::FCMP_ONE);
                    it++;
                    break;

                case FBCInstruction::kANDInt:
                    pushBinop(Instruction::And);
                    it++;
                    break;

                case FBCInstruction::kORInt:
                    pushBinop(Instruction::Or);
                    it++;
                    break;

                case FBCInstruction::kXORInt:
                    pushBinop(Instruction::Xor);
                    it++;
                    break;

                    // Extended unary math
                case FBCInstruction::kAbs:
                    pushUnaryIntCall("abs", false);
                    it++;
                    break;

                case FBCInstruction::kAbsf:
                    pushUnaryRealCall("fabs");
                    it++;
                    break;

                case FBCInstruction::kAcosf:
                    pushUnaryRealCall("acos");
                    it++;
                    break;

                case FBCInstruction::kAsinf:
                    pushUnaryRealCall("asin");
                    it++;
                    break;

                case FBCInstruction::kAtanf:
                    pushUnaryRealCall("atan");
                    it++;
                    break;

                case FBCInstruction::kCeilf:
                    pushUnaryRealCall("ceil");
                    it++;
                    break;

                case FBCInstruction::kCosf:
                    pushUnaryRealCall("cos");
                    it++;
                    break;

                case FBCInstruction::kCoshf:
                    pushUnaryRealCall("cosh");
                    it++;
                    break;

                case FBCInstruction::kExpf:
                    pushUnaryRealCall("exp");
                    it++;
                    break;

                case FBCInstruction::kFloorf:
                    pushUnaryRealCall("floor");
                    it++;
                    break;

                case FBCInstruction::kLogf:
                    pushUnaryRealCall("log");
                    it++;
                    break;

                case FBCInstruction::kLog10f:
                    pushUnaryRealCall("log10");
                    it++;
                    break;

                case FBCInstruction::kRoundf:
                    pushUnaryRealCall("round");
                    it++;
                    break;

                case FBCInstruction::kSinf:
                    pushUnaryRealCall("sin");
                    it++;
                    break;

                case FBCInstruction::kSinhf:
                    pushUnaryRealCall("sinh");
                    it++;
                    break;

                case FBCInstruction::kSqrtf:
                    pushUnaryRealCall("sqrt");
                    it++;
                    break;

                case FBCInstruction::kTanf:
                    pushUnaryRealCall("tan");
                    it++;
                    break;

                case FBCInstruction::kTanhf:
                    pushUnaryRealCall("tanh");
                    it++;
                    break;

                    // Extended binary math
                case FBCInstruction::kAtan2f:
                    pushBinaryRealCall("atan2");
                    it++;
                    break;

                case FBCInstruction::kFmodf:
                    pushBinaryRealCall("fmod");
                    it++;
                    break;

                case FBCInstruction::kPowf:
                    pushBinaryRealCall("pow");
                    it++;
                    break;

                case FBCInstruction::kMax: {
                    pushIntMax();
                    it++;
                    break;
                }

                case FBCInstruction::kMaxf: {
                    pushRealMax();
                    it++;
                    break;
                }

                case FBCInstruction::kMin: {
                    pushIntMin();
                    it++;
                    break;
                }

                case FBCInstruction::kMinf: {
                    pushRealMin();
                    it++;
                    break;
                }

                    // Control
                case FBCInstruction::kReturn:
                    // Empty addr stack = end of computation
                    if (emptyReturn()) {
                        end = true;
                    } else {
                        dispatchReturn();
                    }
                    break;

                case FBCInstruction::kIf: {
                    
                    saveReturn();
                    
                    // Prepare condition
                    LLVMValue cond_value = fBuilder->CreateICmpEQ(popValue(), genInt32(1), "ifcond");
                    Function* function   = fBuilder->GetInsertBlock()->getParent();

                    // Create blocks for the then and else cases.  Insert the 'then' block at the end of the function
                    BasicBlock* then_block  = BasicBlock::Create(fModule->getContext(), "then_code", function);
                    BasicBlock* else_block  = BasicBlock::Create(fModule->getContext(), "else_code");
                    BasicBlock* merge_block = BasicBlock::Create(fModule->getContext(), "merge_block");
     
                    fBuilder->CreateCondBr(cond_value, then_block, else_block);

                    // Compile then branch (= branch1)
                    CompileBlock((*it)->fBranch1, then_block);

                    fBuilder->CreateBr(merge_block);
                    // Codegen of 'Then' can change the current block, update then_block for the PHI
                    then_block = fBuilder->GetInsertBlock();

                    // Emit else block
                    function->getBasicBlockList().push_back(else_block);

                    // Compile else branch (= branch2)
                    CompileBlock((*it)->fBranch2, else_block);
   
                    fBuilder->CreateBr(merge_block);
                    // Codegen of 'Else' can change the current block, update else_block for the PHI
                    else_block = fBuilder->GetInsertBlock();

                    // Emit merge block
                    function->getBasicBlockList().push_back(merge_block);
                    fBuilder->SetInsertPoint(merge_block);
                    
                    it++;
                    break;
                }

                case FBCInstruction::kSelectReal:
                case FBCInstruction::kSelectInt: {
                    // Prepare condition
                    LLVMValue cond_value = fBuilder->CreateICmpNE(popValue(), genInt32(0), "select_cond");

                    // Compile then branch (= branch1)
                    CompileBlock((*it)->fBranch1, code_block);

                    // Compile else branch (= branch2)
                    CompileBlock((*it)->fBranch2, code_block);

                    // Create the result (= branch2)
                    LLVMValue then_value = popValue();
                    LLVMValue else_value = popValue();
                    // Inverted here
                    pushValue(fBuilder->CreateSelect(cond_value, else_value, then_value));

                    it++;
                    break;
                }

                case FBCInstruction::kCondBranch: {
                    // Prepare condition
                    LLVMValue cond_value = fBuilder->CreateTrunc(popValue(), fBuilder->getInt1Ty());

                    Function*   function   = fBuilder->GetInsertBlock()->getParent();
                    BasicBlock* next_block = BasicBlock::Create(fModule->getContext(), "next_block", function);

                    // Branch to current block
                    fBuilder->CreateCondBr(cond_value, code_block, next_block);

                    // Insert in next_block
                    fBuilder->SetInsertPoint(next_block);

                    it++;
                    break;
                }

                case FBCInstruction::kLoop: {
                    Function*   function   = fBuilder->GetInsertBlock()->getParent();
                    BasicBlock* init_block = BasicBlock::Create(fModule->getContext(), "init_block", function);
                    BasicBlock* loop_body_block = BasicBlock::Create(fModule->getContext(), "loop_body_block", function);

                    // Link previous_block and init_block
                    fBuilder->CreateBr(init_block);

                    // Compile init branch (= branch1)
                    CompileBlock((*it)->fBranch1, init_block);

                    // Link previous_block and loop_body_block
                    fBuilder->CreateBr(loop_body_block);

                    // Compile loop branch (= branch2)
                    CompileBlock((*it)->fBranch2, loop_body_block);

                    it++;
                    break;
                }

                default:
                    // Should not happen
                    (*it)->write(&std::cout);
                    faustassert(false);
                    break;
            }
        }
    }

   public:
    FBCLLVMCompiler(FBCBlockInstruction<T>* fbc_block)
    {
        fLLVMStackIndex = 0;
        fAddrStackIndex = 0;

        fContext = new LLVMContext();
        fBuilder = new IRBuilder<>(getContext());
        
        // Set fast-math flags
        FastMathFlags FMF;
    #if defined(LLVM_60) || defined(LLVM_70) || defined(LLVM_80)
        FMF.setFast();
    #else
        FMF.setUnsafeAlgebra();
    #endif
        fBuilder->setFastMathFlags(FMF);
        
        fModule = new Module(std::string(FAUSTVERSION), getContext());
        fModule->setTargetTriple(llvm::sys::getDefaultTargetTriple());

        // Compile compute function
        std::vector<llvm::Type*> execute_args;
        execute_args.push_back(PointerType::get(getInt32Ty(), 0));
        execute_args.push_back(PointerType::get(getRealTy(), 0));
        execute_args.push_back(PointerType::get(PointerType::get(getRealTy(), 0), 0));
        execute_args.push_back(PointerType::get(PointerType::get(getRealTy(), 0), 0));

        FunctionType* execute_type = FunctionType::get(fBuilder->getVoidTy(), execute_args, false);
        Function* execute = Function::Create(execute_type, GlobalValue::ExternalLinkage, "execute", fModule);
        execute->setCallingConv(CallingConv::C);

        BasicBlock* code_block = BasicBlock::Create(fModule->getContext(), "entry_block", execute);

        Function::arg_iterator execute_args_it = execute->arg_begin();
        execute_args_it->setName("int_heap");
        fLLVMIntHeap = execute_args_it++;
        execute_args_it->setName("real_heap");
        fLLVMRealHeap = execute_args_it++;
        execute_args_it->setName("inputs");
        fLLVMInputs = execute_args_it++;
        execute_args_it->setName("outputs");
        fLLVMOutputs = execute_args_it++;

        // fbc_block->write(&std::cout);

        // Compile compute body
        CompileBlock(fbc_block, code_block);

        // Get last block of post code section
        BasicBlock* last_block = fBuilder->GetInsertBlock();
        // Add return
        ReturnInst::Create(fModule->getContext(), last_block);

        // Check function
        verifyFunction(*execute);

        // dumpLLVM1(fModule);

        // For host target support
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetAsmParser();

        EngineBuilder  builder((std::unique_ptr<Module>(fModule)));
        TargetMachine* tm = builder.selectTarget();
        builder.setOptLevel(CodeGenOpt::Aggressive);
        builder.setEngineKind(EngineKind::JIT);

        fJIT = builder.create(tm);
        fModule->setDataLayout(fJIT->getDataLayout());
        
        legacy::PassManager pm;
        pm.add(createVerifierPass());
        pm.run(*fModule);
        
        // TODO : add appropriate optimization passes
        /*
        llvm::legacy::FunctionPassManager fpm(fModule);
        fpm.add(llvm::createPromoteMemoryToRegisterPass());
        fpm.add(llvm::createInstructionCombiningPass());
        fpm.add(llvm::createCFGSimplificationPass());
        fpm.add(llvm::createJumpThreadingPass());
        fpm.add(llvm::createConstantPropagationPass());
        fpm.add(llvm::createLoopVectorizePass());
        fpm.add(llvm::createSLPVectorizerPass());
        fpm.doInitialization();
        for (auto& it : *fModule) { fpm.run(it); }
        */
     
        // Get 'execute' entry point
        fCompiledFun = (compiledFun)fJIT->getFunctionAddress("execute");
    }

    virtual ~FBCLLVMCompiler()
    {
        // fModule is kept and deleted by fJIT
        delete fJIT;
        delete fBuilder;
        delete fContext;
    }

    void Execute(int* int_heap, T* real_heap, T** inputs, T** outputs)
    {
        fCompiledFun(int_heap, real_heap, inputs, outputs);
    }
};

// FBC compiler
template <class T>
class FBCCompiler : public FBCInterpreter<T, 0> {
   public:
    typedef typename std::map<FBCBlockInstruction<T>*, FBCLLVMCompiler<T>*>           CompiledBlocksType;
    typedef typename std::map<FBCBlockInstruction<T>*, FBCLLVMCompiler<T>*>::iterator CompiledBlocksTypeIT;

    FBCCompiler(interpreter_dsp_factory_aux<T, 0>* factory, CompiledBlocksType* map)
        : FBCInterpreter<T, 0>(factory)
    {
        fCompiledBlocks = map;
     
        // FBC blocks compilation
        // CompileBlock(factory->fComputeBlock);
        CompileBlock(factory->fComputeDSPBlock);
    }
    
    virtual ~FBCCompiler()
    {}

    void ExecuteBlock(FBCBlockInstruction<T>* block, bool compile)
    {
        if (compile && fCompiledBlocks->find(block) == fCompiledBlocks->end()) {
            CompileBlock(block);
        }
        
        // The 'DSP' compute block only is compiled..
        if (fCompiledBlocks->find(block) != fCompiledBlocks->end()) {
            ((*fCompiledBlocks)[block])->Execute(this->fIntHeap, this->fRealHeap, this->fInputs, this->fOutputs);
        } else {
            FBCInterpreter<T, 0>::ExecuteBlock(block);
        }
    }

   protected:
    CompiledBlocksType* fCompiledBlocks;

    void CompileBlock(FBCBlockInstruction<T>* block)
    {
        if (fCompiledBlocks->find(block) == fCompiledBlocks->end()) {
            (*fCompiledBlocks)[block] = new FBCLLVMCompiler<T>(block);
        } else {
            //std::cout << "FBCCompiler: reuse compiled block" << std::endl;
        }
    }
};

#endif
