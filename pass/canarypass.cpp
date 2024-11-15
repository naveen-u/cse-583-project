#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace llvm {

struct CanaryPass : public PassInfoMixin<CanaryPass> {

  SmallVector<AllocaInst *> getAllocas(BasicBlock &BB) {
    SmallVector<AllocaInst *> allocas;
    for (auto &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        allocas.push_back(AI);
      }
    }
    return allocas;
  }

  /* -------------------------------------------------------------------------- */
  /*                        Create a consolidated alloca                        */
  /* -------------------------------------------------------------------------- */

  AllocaInst *
  createConsolidatedAlloca(Function &F,
                           const SmallVector<AllocaInst *> &allocas) {
    const DataLayout &dataLayout = F.getParent()->getDataLayout();
    uint64_t totalSize = 0;
    unsigned maxAlign = 1;

    // Calculate total size and maximum alignment
    for (AllocaInst *allocaInst : allocas) {
      Type *allocaType = allocaInst->getAllocatedType();
      unsigned alignment = allocaInst->getAlign().value();

      // Align the offset
      totalSize = (totalSize + alignment - 1) & ~(alignment - 1);
      totalSize += dataLayout.getTypeAllocSize(allocaType);
      totalSize += dataLayout.getTypeAllocSize(Type::getInt8Ty(F.getContext()));
      maxAlign = std::max(maxAlign, alignment);
    }

    // Create new alloca instruction
    IRBuilder<> Builder(&F.getEntryBlock().front());
    errs() << "Allocating a block with total size: " << totalSize << "\n";
    Type *i8ArrayTy =
        ArrayType::get(Type::getInt8Ty(F.getContext()), totalSize);
    AllocaInst *newAlloca =
        Builder.CreateAlloca(i8ArrayTy, nullptr, "consolidated");
    newAlloca->setAlignment(Align(maxAlign));

    return newAlloca;
  }

  /* -------------------------------------------------------------------------- */
  /*                     Replace allocas with GetElementPtr                     */
  /* -------------------------------------------------------------------------- */

  void replaceAllocaWithGEP(AllocaInst *oldAlloca,
                            AllocaInst *consolidatedAlloca, Value *offset) {
    IRBuilder<> Builder(oldAlloca);

    // Create GEP instruction
    Value *zero =
        ConstantInt::get(Type::getInt64Ty(oldAlloca->getContext()), 0);

    SmallVector<Value *> indices = {zero, offset};
    Value *gep = Builder.CreateGEP(consolidatedAlloca->getAllocatedType(),
                                   consolidatedAlloca, indices, "gep");

    // Bitcast to original type if necessary
    Value *replacement =
        Builder.CreateBitCast(gep, oldAlloca->getType(), "cast");

    // Replace all uses of the old alloca
    oldAlloca->replaceAllUsesWith(replacement);
  }

  /* -------------------------------------------------------------------------- */
  /*                         Pass to add random padding                         */
  /* -------------------------------------------------------------------------- */

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    if (F.getName().str() == "main") {
      return PreservedAnalyses::all();
    }

    BasicBlock &BB = F.getEntryBlock();
    LLVMContext &ctx = F.getContext();
    Type *i32 = IntegerType::getInt32Ty(ctx);
    const DataLayout &dataLayout = F.getParent()->getDataLayout();

    SmallVector<AllocaInst *> allocas = getAllocas(BB);

    if (allocas.empty()) {
      errs() << "No stack allocations found for " << F.getName().str() << "\n";
      return PreservedAnalyses::all();
    }

    errs() << "Updating stack allocations for " << F.getName().str() << "\n";

    // Create a consolidated alloca instruction
    AllocaInst *consolidatedAlloca = createConsolidatedAlloca(F, allocas);

    // Get a random number
    IRBuilder<> builder(allocas[0]);

    FunctionType *randType = FunctionType::get(i32, false);
    FunctionCallee randFunc =
        F.getParent()->getOrInsertFunction("get_rand32", randType);
    Value *randomVal = builder.CreateCall(randFunc);

    Value *one = ConstantInt::get(i32, 1);
    Value *remainingRandBits = randomVal;
    Value *currentOffset = ConstantInt::get(i32, 0);

    // Replace allocas with GetElementPtrs
    for (AllocaInst *allocaInst : allocas) {
      builder.SetInsertPoint(allocaInst);
      Value *randomBit = builder.CreateAnd(remainingRandBits, one, "randomBit");
      Value *alignment = ConstantInt::get(i32, allocaInst->getAlign().value());
      Value *alignMask = builder.CreateSub(alignment, one, "alignMask");

      // We do currentOffset = (currentOffset + alignment - 1) & ~(alignment - 1)
      // to ensure the variable is aligned correctly
      currentOffset =
          builder.CreateAnd(builder.CreateAdd(currentOffset, alignMask),
                            builder.CreateNot(alignMask), "alignedOffset");

      replaceAllocaWithGEP(allocaInst, consolidatedAlloca, currentOffset);

      Type *allocaType = allocaInst->getAllocatedType();
      currentOffset = builder.CreateAdd(
          currentOffset,
          ConstantInt::get(i32, dataLayout.getTypeAllocSize(allocaType)));

      currentOffset = builder.CreateAdd(currentOffset, randomBit);
      remainingRandBits = builder.CreateLShr(remainingRandBits, one);
    }

    // Clean up old allocas
    for (AllocaInst *allocaInst : allocas) {
      allocaInst->eraseFromParent();
    }

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