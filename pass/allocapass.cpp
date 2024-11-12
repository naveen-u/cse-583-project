#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include  <iostream>

using namespace llvm;

namespace {

struct AllocationPass : public PassInfoMixin<AllocationPass> {

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    unsigned int allocaCount = 0;

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (I.getOpcode() == Instruction::Alloca) {
          allocaCount++;
          // Print information about the alloca instruction
          errs() << "Here there is an allocation \n";
        }
      }
    }

    if (allocaCount > 0) {
      errs() << "Function '" << F.getName() << "' has " 
             << allocaCount << " stack allocations.\n";
    }
    return PreservedAnalyses::all();
  }
};
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "AllocationPass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "alloca"){
            FPM.addPass(AllocationPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}
