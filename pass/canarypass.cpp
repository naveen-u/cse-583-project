#include <map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace llvm {

struct CanaryPass : public PassInfoMixin<CanaryPass> {
  // No. of random bits required to decide padding
  const uint64_t paddingBits = 4;
  // Maximum possible padding blocks per variable (where each block is alignment sized)
  const uint64_t maxPadding = (1 << paddingBits) - 1;

  /* -------------------------------------------------------------------------- */
  /*                          Find alloca instructions                          */
  /* -------------------------------------------------------------------------- */

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

      // Add maximum padding
      totalSize += maxPadding * alignment;

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

    // Ensure 'offset' is of type i64
    Value *offset64 = Builder.CreateIntCast(
        offset, Type::getInt64Ty(oldAlloca->getContext()), false);

    // Create GEP instruction
    Value *zero =
        ConstantInt::get(Type::getInt64Ty(oldAlloca->getContext()), 0);
    SmallVector<Value *> indices = {zero, offset64};
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
    Type *i16 = IntegerType::getInt16Ty(ctx);
    Type *i32 = IntegerType::getInt32Ty(ctx);
    Type *i64 = IntegerType::getInt64Ty(ctx);
    const DataLayout &dataLayout = F.getParent()->getDataLayout();

    SmallVector<AllocaInst *> allocas = getAllocas(BB);

    if (allocas.size() < 2) {
      errs() << allocas.size() << " stack allocations found for "
             << F.getName().str() << "\n";
      return PreservedAnalyses::all();
    }

    errs() << "Updating stack allocations for " << F.getName().str() << "\n";

    // Create a consolidated alloca instruction
    AllocaInst *consolidatedAlloca = createConsolidatedAlloca(F, allocas);

    IRBuilder<> builder(allocas[0]);
    Value *currentOffset = ConstantInt::get(i64, 0);
    Value *randBits = nullptr;
    Type *randBitsType;

    bool firstAlloca = true;
    int randBitsAvailable = 0;
    int allocasRemaining = allocas.size();

    // Replace allocas with GetElementPtrs
    for (AllocaInst *allocaInst : allocas) {
      builder.SetInsertPoint(allocaInst);

      Value *alignment = ConstantInt::get(i64, allocaInst->getAlign().value());
      Value *one64 = ConstantInt::get(i64, 1);
      Value *alignMask = builder.CreateSub(alignment, one64, "alignMask");

      // currentOffset = (currentOffset + alignment -1) & ~(alignment -1)
      currentOffset =
          builder.CreateAnd(builder.CreateAdd(currentOffset, alignMask),
                            builder.CreateNot(alignMask), "alignedOffset");

      if (firstAlloca) {
        firstAlloca = false;
      } else {
        // Generate a random number if we don't have enough random bits available
        if (randBitsAvailable < paddingBits) {
          std::string randFunctionName;
          std::map<int, Type *> options = {{16, i16}, {32, i32}, {64, i64}};

          for (const auto &pair : options) {
            int bits = pair.first;
            Type *type = pair.second;
            if ((allocasRemaining < bits / paddingBits) || (bits == 64)) {
              randFunctionName = "get_rand" + std::to_string(bits);
              randBitsAvailable = bits;
              randBitsType = type;
              errs() << "Created rand call for " << bits << " bits\n";
              break;
            }
          }

          FunctionType *randType = FunctionType::get(randBitsType, false);
          FunctionCallee randFunc =
              F.getParent()->getOrInsertFunction(randFunctionName, randType);
          Value *randVal = builder.CreateCall(randFunc);
          randBits = randVal;
        }

        // Compute random padding
        Value *randPadding = builder.CreateAnd(
            randBits, ConstantInt::get(randBitsType, maxPadding),
            "randPadding");
        randBits = builder.CreateLShr(randBits, paddingBits, "randBits");
        Value *randPadding64 = builder.CreateIntCast(randPadding, i64, false);
        randBitsAvailable -= paddingBits;
        Value *randPaddingAligned =
            builder.CreateMul(randPadding64, alignment, "randPaddingAligned");

        // Add random padding to currentOffset
        currentOffset = builder.CreateAdd(currentOffset, randPaddingAligned,
                                          "offsetWithPadding");
      }

      // Replace alloca with GEP at currentOffset
      replaceAllocaWithGEP(allocaInst, consolidatedAlloca, currentOffset);

      // Update currentOffset
      Type *allocaType = allocaInst->getAllocatedType();
      uint64_t typeSize = dataLayout.getTypeAllocSize(allocaType);
      Value *typeSizeVal = ConstantInt::get(i64, typeSize);
      currentOffset =
          builder.CreateAdd(currentOffset, typeSizeVal, "nextOffset");

      allocasRemaining--;
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