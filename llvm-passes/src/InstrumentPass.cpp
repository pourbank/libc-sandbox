#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

#include <string>

#include "DummySyscalls.cpp"

namespace instrument {
    class InstrumentPass : public llvm::PassInfoMixin<InstrumentPass> {
    public:
        static bool isRequired() { return true; }
        bool instrumentSyscall(llvm::Module &Mod, llvm::FunctionCallee syscallFn);
        llvm::PreservedAnalyses run(llvm::Module &Mod, llvm::AnalysisManager<llvm::Module> &mngr);
    };

    llvm::PreservedAnalyses InstrumentPass::run(llvm::Module &Mod, llvm::AnalysisManager<llvm::Module> &mngr) {
        llvm::Type *i64 = llvm::Type::getInt64Ty(Mod.getContext());
        llvm::FunctionType *sysTy = llvm::FunctionType::get(i64, {i64}, true);
        llvm::FunctionCallee syscallFn = Mod.getOrInsertFunction("syscall", sysTy);
        bool modified = instrumentSyscall(Mod, syscallFn);
        return modified ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
    }

    bool InstrumentPass::instrumentSyscall(llvm::Module &Mod, llvm::FunctionCallee syscallFn) {
        std::vector<std::pair<llvm::CallInst*, int>> targets;
        bool modified = false;

        for (llvm::Function &F : Mod) {
            if (F.isDeclaration() || F.isIntrinsic()) continue;
            for (llvm::BasicBlock &BB : F) {
                for (llvm::Instruction &I : BB) {
                    if (auto *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
                        if (CI->getMetadata("instrumented")) continue;
                        if (llvm::Function *CF = CI->getCalledFunction()) {
                            int id = libcMap(CF->getName().str());
                            if (id >= 0) targets.push_back({CI, id});
                        }
                    }
                }
            }
        }

        for (auto &[CI, id] : targets) {
            llvm::IRBuilder<> B(CI);
            llvm::Value *arg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Mod.getContext()), 470);
            llvm::Value *libcId = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Mod.getContext()), id);
            auto *call = B.CreateCall(syscallFn, {arg, libcId});
            llvm::MDNode *N = llvm::MDNode::get(Mod.getContext(), llvm::MDString::get(Mod.getContext(), "instrumented"));
            call->setMetadata("instrumented", N);
            modified = true;
        }

        return modified;
    }
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        // Use your existing plugin details
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "InstrumentPassPlugin",
        .PluginVersion = "v0.4",
        .RegisterPassBuilderCallbacks = [](llvm::PassBuilder &PB) {
            
            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel Level) { 
                    // Add your pass here
                    MPM.addPass(instrument::InstrumentPass());
                }
            );
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::ModulePassManager &MPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if(Name == "instrument-pass") {
                        MPM.addPass(instrument::InstrumentPass());
                        return true;
                    }
                    return false;
                }
            );
        },
    };
}
