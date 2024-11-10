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
    if (F.getName().str() == "main") {
      return PreservedAnalyses::all();
    }

    BasicBlock &BB = F.front();
    int numAllocas = 0;
    Instruction *secondAlloca = nullptr;

    for (auto &I : BB) {
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

    errs() << "Updating CFG for " << F.getName().str() << "\n";

    LLVMContext &ctx = F.getContext();
    Module *M = F.getParent();

    Type *i16 = IntegerType::getInt16Ty(ctx);
    Type *i32 = IntegerType::getInt32Ty(ctx);
    Type *i64 = IntegerType::getInt64Ty(ctx);

    BasicBlock *merge_block = BB.splitBasicBlock(secondAlloca);
    BasicBlock *canary_block =
        BasicBlock::Create(ctx, "canary", &F, merge_block);

    // Remove the existing block terminator (unconditional branch) so that we
    // can add our own branch
    BB.getTerminator()->eraseFromParent();

    // Get a random number and branch to canary block if odd
    IRBuilder<> builder(&BB);

    FunctionType *randType = FunctionType::get(i32, false);
    FunctionCallee randFunc =
        F.getParent()->getOrInsertFunction("get_rand32", randType);
    Value *RandomVal = builder.CreateCall(randFunc);
    Value *One = ConstantInt::get(i32, 1);
    Value *RandomBit = builder.CreateAnd(RandomVal, One);

    Value *cmp = builder.CreateICmpEQ(RandomBit, One, "one_check");
    builder.CreateCondBr(cmp, canary_block, merge_block);

    // Insert canary in the canary block
    builder.SetInsertPoint(canary_block);
    AllocaInst *canary = builder.CreateAlloca(i32, nullptr, "Foo");
    StoreInst *store = builder.CreateStore(One, canary);

    // Add a terminator for the canary block
    builder.CreateBr(merge_block);

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