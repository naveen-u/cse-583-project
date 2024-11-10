#include <unordered_set>
#include <vector>

#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicsX86.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"

using namespace llvm;

namespace llvm {

struct CanaryPass : public PassInfoMixin<CanaryPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    int numAllocas = 0;
    Instruction *secondAlloca = nullptr;
    for (auto &I : F.front()) { // Iterate all instructions in the first BB
      if (I.getOpcode() == Instruction::Alloca) {
        if (numAllocas++ > 0) {
          secondAlloca = &I;
          break;
        };
      }
    }
    if (secondAlloca == nullptr) {
      return PreservedAnalyses::all();
    }

    errs() << "Modifying the CFG\n";

    LLVMContext &ctx = F.getContext();
    Module *M = F.getParent();

    Type *i32 = IntegerType::getInt32Ty(ctx);
    Type *i16 = IntegerType::getInt16Ty(ctx);

    Type *ReturnTys[] = {i16, i32};
    StructType *RetTy = StructType::get(ctx, ReturnTys);

    // Declare the rdrand16 intrinsic
    // FunctionType *RDRandTy = FunctionType::get(RetTy, false);
    // Function *RDRand = Function::Create(RDRandTy, Function::ExternalLinkage,
    //                                     "llvm.x86.rdrand.16", M);
    Function *RDRand =
        Intrinsic::getDeclaration(F.getParent(), Intrinsic::x86_rdrand_16);

    IRBuilder<> canaryBuilder(secondAlloca);

    // Allocate space for the 64-bit random value
    // AllocaInst *ResultPtr = canaryBuilder.CreateAlloca(i16);

    // Call RDRAND
    // Value *Status = canaryBuilder.CreateCall(RDRand, ResultPtr);
    // Value *Result =
    //     canaryBuilder.CreateIntrinsic(Intrinsic::x86_rdrand_16, {RetTy}, {});

    // F.getParent()->addModuleFlag(Module::ModFlagBehavior::Warning,
    //                              "target-features", "+rdrnd");
    Value *Result = canaryBuilder.CreateCall(RDRand, {});

    // Value *Result = canaryBuilder.CreateCall(RDRand);
    errs() << "Result is: " << *Result << "\n";
    Value *RandomVal = canaryBuilder.CreateExtractValue(Result, 0);
    errs() << "Extracted value " << *RandomVal << "\n";
    // Load the random value
    // Value *RandomVal = canaryBuilder.CreateLoad(i16, ResultPtr);

    // Extract least significant bit using AND
    Value *One = ConstantInt::get(i16, 1);
    Value *RandomBit = canaryBuilder.CreateAnd(RandomVal, One);

    errs() << "Random value pointer is " << *RandomBit << "\n";

    // AllocaInst *canarySizeVar =
    //     canaryBuilder.CreateAlloca(i32, 0, 0, "canaryAlloc");
    // StoreInst *canarySizeStore = canaryBuilder.CreateStore(
    //     ConstantInt::get(i32, APInt(32, 1)), canarySizeVar);
    // LoadInst *canarySize =
    //     canaryBuilder.CreateLoad(i32, canarySizeVar, "canarySizeLoad");
    AllocaInst *canary =
        canaryBuilder.CreateAlloca(i32, 0, RandomBit, "canary");
    return PreservedAnalyses::none();
  }
};

} // namespace llvm

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CanaryPass", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "canarypass") {
                    FPM.addPass(CanaryPass());
                    return true;
                  }
                  return false;
                });
          }};
}