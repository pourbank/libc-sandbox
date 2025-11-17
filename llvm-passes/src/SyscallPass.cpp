#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Path.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

#include <map>
#include <string>
#include <utility>
#include <fstream>
#include <vector>
#include <set>
#include <queue>

#include "CallNames.cpp"
#include "FSM.cpp"

namespace icfg {
    class syscallCFGPass : public llvm::PassInfoMixin<syscallCFGPass> {
        public:
            static bool isRequired() {return true;}
            llvm::PreservedAnalyses run(llvm::Module &Mod, llvm::AnalysisManager<llvm::Module> &mngr);
        private:
            fsm::nfaNode* startNode;
            uint64_t nodeCounter;
            std::map<llvm::Function*, fsm::nfaNode*> funcExitNode;
            std::map<std::pair<llvm::Function*, llvm::BasicBlock*>, fsm::nfaNode*> bbId;

            fsm::nfaNode* createNode();
            void dumpGraph(llvm::Module &Mod, fsm::nfaNode* startNode);
            fsm::nfaNode* scanCallInstructions(llvm::BasicBlock &bb, llvm::Function &func);
    };

    fsm::nfaNode* syscallCFGPass::createNode() {
        fsm::nfaNode* newNode = new fsm::nfaNode(nodeCounter++, false);
        return newNode;
    }

    fsm::nfaNode* syscallCFGPass::scanCallInstructions(llvm::BasicBlock &bb, llvm::Function &func) {
        auto bbKey = std::make_pair(&func, &bb);
        if(bbId.find(bbKey) == bbId.end()) {
            bbId[bbKey] = createNode();
        }
        fsm::nfaNode* currentNode = bbId[bbKey];
        fsm::nfaNode* funcEntryNode = nullptr;
        if (!func.isDeclaration() && !func.empty()) {
            auto entryKey = std::make_pair(&func, &func.getEntryBlock());
            if(bbId.find(entryKey) == bbId.end()) {
                bbId[entryKey] = createNode();
            }
            funcEntryNode = bbId[entryKey];
        }
        for(llvm::Instruction &inst : bb) {
            if(auto *callInst = llvm::dyn_cast<llvm::CallInst>(&inst)) {
                if (auto *inlineAsm = llvm::dyn_cast<llvm::InlineAsm>(callInst->getCalledOperand())) {
                    std::string asmStr = inlineAsm->getAsmString();
                    if (asmStr.find("syscall") != std::string::npos) {
                        std::string label = "syscall(470)";
                        if (callInst->getNumOperands() > 0) {
                            if (auto *constInt = llvm::dyn_cast<llvm::ConstantInt>(callInst->getArgOperand(0))) {
                                label += " : " + std::to_string(constInt->getZExtValue());
                            }
                        }
                        fsm::nfaNode *nextNode = createNode();
                        currentNode->edges.push_back({nextNode, label});
                        currentNode = nextNode;
                        continue;
                    }
                }
                if(llvm::Function *calledFunc = callInst->getCalledFunction()) {
                    std::string funcName = calledFunc->getName().str();
                    if(funcName == func.getName().str()) {
                        if (funcEntryNode)
                            currentNode->edges.push_back({funcEntryNode, "ε"});
                    } else if (funcName == "syscall") {
                        std::string label;
                        std::string syscallNum = "";
                        std::string syscallArg = "";
                        if (callInst->getNumOperands() > 0) {
                            if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(callInst->getArgOperand(0))) {
                                syscallNum = std::to_string(constInt->getZExtValue());
                            }
                        }
                        if (callInst->getNumOperands() > 1) {
                            if (auto* constArg = llvm::dyn_cast<llvm::ConstantInt>(callInst->getArgOperand(1))) {
                                syscallArg = std::to_string(constArg->getZExtValue());
                            }
                        }
                        if (!syscallArg.empty()) {
                            label += syscallArg;
                        }
                        fsm::nfaNode* nextNode = createNode();
                        currentNode->edges.push_back({nextNode, label});
                        currentNode = nextNode;
                    } else if (calledFunc->isDeclaration() || calledFunc->empty()) {
                        fsm::nfaNode* nextNode = createNode();
                        currentNode->edges.push_back({nextNode, "ε"});
                        currentNode = nextNode;
                    } else {
                        if (!calledFunc->isDeclaration() && !calledFunc->empty()) {
                            llvm::BasicBlock &calledFuncEntryBB = calledFunc->getEntryBlock();
                            if(bbId.find({calledFunc, &calledFuncEntryBB}) == bbId.end()) {
                                for(llvm::BasicBlock &calleeBB : *calledFunc) {
                                    bbId[{calledFunc, &calleeBB}] = createNode();
                                }
                            }
                            fsm::nfaNode* calledFuncEntryNode = bbId.at({calledFunc, &calledFuncEntryBB});
                            currentNode->edges.push_back({calledFuncEntryNode, "ε"});
                            fsm::nfaNode* nextNode = createNode();
                            if (funcExitNode.count(calledFunc)) {
                                funcExitNode.at(calledFunc)->edges.push_back({nextNode, "ε"});
                            }
                            currentNode = nextNode;
                        } else {
                            fsm::nfaNode* nextNode = createNode();
                            currentNode->edges.push_back({nextNode, "ε"});
                            currentNode = nextNode;
                        }
                    }
                }
            }
        }
        return currentNode;
    }

    void syscallCFGPass::dumpGraph(llvm::Module &Mod,fsm::nfaNode* startNode) {
        std::set<fsm::nfaNode*> visited;
        std::queue<fsm::nfaNode*> q;
        q.push(startNode);
        visited.insert(startNode);

        std::string filename = "syscall_cfg.dot";
        std::ofstream outfile(filename);
        outfile << "digraph CFG {\n";
        outfile << "    rankdir=LR;\n";
        outfile << "    node [shape=circle];\n";

        while(!q.empty()) {
            fsm::nfaNode* currentNode = q.front();
            q.pop();
            for(auto const& edge : currentNode->edges) {
                outfile << "    " << currentNode->nodeId << " -> " << edge.first->nodeId << " [label=\"" << edge.second << "\"];" << std::endl;
                fsm::nfaNode* neighbor = edge.first;
                if(visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }

        outfile << "}\n";
        outfile.close();
    }

    llvm::PreservedAnalyses syscallCFGPass::run(llvm::Module &Mod, llvm::AnalysisManager<llvm::Module> &mngr) {
        nodeCounter = 0;
        startNode = createNode();
        std::vector<llvm::Function*> funcList;
        for(llvm::Function &func : Mod) {
            if(func.isDeclaration() || func.empty()) continue;
            funcList.push_back(&func);
        }
        for(llvm::Function *fptr : funcList) {
            llvm::Function &func = *fptr;
            auto entryKey = std::make_pair(&func, &func.getEntryBlock());
            bbId[entryKey] = createNode();
        }
        for(llvm::Function *fptr : funcList){
            llvm::Function &func = *fptr;
            fsm::nfaNode* exitNode = createNode();
            funcExitNode[&func] = exitNode;
        }

        llvm::Function *mainFunc = Mod.getFunction("main");
        std::vector<fsm::nfaNode*> entryNodes;
        if (mainFunc && bbId.count({mainFunc, &mainFunc->getEntryBlock()})) {
            entryNodes.push_back(bbId.at({mainFunc, &mainFunc->getEntryBlock()}));
        } else if (mainFunc && !mainFunc->isDeclaration() && !mainFunc->empty()) {
            auto ek = std::make_pair(mainFunc, &mainFunc->getEntryBlock());
            if (bbId.find(ek) == bbId.end()) bbId[ek] = createNode();
            entryNodes.push_back(bbId[ek]);
        } else {
            for (llvm::Function *fptr : funcList) {
                llvm::Function &func = *fptr;
                auto ek = std::make_pair(&func, &func.getEntryBlock());
                if (bbId.find(ek) == bbId.end()) bbId[ek] = createNode();
                entryNodes.push_back(bbId[ek]);
            }
        }
        for (fsm::nfaNode* entryNode : entryNodes) {
            startNode->edges.push_back({entryNode, "ε"});
        }
        if (mainFunc && funcExitNode.count(mainFunc)) {
            funcExitNode.at(mainFunc)->isFinalState = true;
        } else {
            for (auto &pair : funcExitNode) {
                pair.second->isFinalState = true;
            }
        }

        for(llvm::Function *fptr : funcList){
            llvm::Function &func = *fptr;
            std::vector<llvm::BasicBlock*> bbs;
            for(llvm::BasicBlock &bb : func) bbs.push_back(&bb);
            for(llvm::BasicBlock *bbptr : bbs) {
                llvm::BasicBlock &bb = *bbptr;
                fsm::nfaNode* lastNode = scanCallInstructions(bb, func);
                if (!lastNode) continue;
                llvm::Instruction *terminator = bb.getTerminator();
                if(!terminator) continue;
                if (llvm::isa<llvm::ReturnInst>(terminator)) {
                    if (funcExitNode.count(&func))
                        lastNode->edges.push_back({funcExitNode.at(&func), "ε"});
                }
                for(unsigned i = 0; i < terminator->getNumSuccessors(); i++) {
                    llvm::BasicBlock *successor = terminator->getSuccessor(i);
                    auto successorKey = std::make_pair(&func, successor);
                    if(bbId.find(successorKey) == bbId.end())
                        bbId[successorKey] = createNode();
                    fsm::nfaNode* successorNode = bbId.at(successorKey);
                    lastNode->edges.push_back({successorNode, "ε"});
                }
            }
        }
        fsm::removeEpsilonTransitions(startNode);
        fsm::nfaNode* mergedStartNode = fsm::mergeEquivalentStates(startNode);
        dumpGraph(Mod, mergedStartNode);
        fsm::clearGraph(mergedStartNode);
        return llvm::PreservedAnalyses::all();
    }
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "syscallCFGPlugin",
        .PluginVersion = "v0.5",
        .RegisterPassBuilderCallbacks = [](llvm::PassBuilder &PB) {
            
            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel Level) { 
                    MPM.addPass(icfg::syscallCFGPass());
                }
            );

            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::ModulePassManager &MPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if(Name == "syscall-cfg-pass") {
                        MPM.addPass(icfg::syscallCFGPass());
                        return true;
                    }
                    return false;
                }
            );
        },
    };
}
