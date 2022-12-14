#include "parse_fill.h"
#include "check_regs.h"
#include "op_constraint.h"
#include "helper.h"
#include "global_data_struct.h"
#include "ins_context_stack.h"


using namespace taintGen;

namespace funcExtract {

#define SV std::vector<std::string>
#define toStr(a) std::to_string(a)
#define context std::shared_ptr<llvm::LLVMContext>
#define llvmWidth(a, c) llvm::IntegerType::get(*c, a)
#define llvmInt(val, width, c) llvm::ConstantInt::get(llvmWidth(width, c), val, false);
#define value(a) llvm::dyn_cast<llvm::Value>(a)
#define instr(a) llvm::dyn_cast<llvm::Instruction>(a)


//static std::shared_ptr<KaleidoscopeJIT> TheJIT;
//static std::map<std::string, std::shared_ptr<llvm::PrototypeAST>> FunctionProtos;

std::regex pTimed("^(\\S+)"+DELIM+"(\\d+)$");
std::string CURRENT_VAR;

// DIRTY_QUEUE is mainly not-current AS

std::map<std::string, expr*> INPUT_EXPR_VAL;
std::map<std::string, expr*> TIMED_VAR2EXPR;

std::map<std::string, uint32_t> g_allRegs;
std::set<std::string> g_regWithFunc;
HierCtx g_insContextStk;

//std::unordered_map<std::string, expr*> INT_EXPR_VAL;
std::set<std::string> INT_EXPR_SET;
std::set<std::string> g_readASV;
// remaining variables to be built goal for
std::queue<std::pair<std::string, uint32_t>> g_goalVars;
InstrInfo_t g_currInstrInfo;
std::string g_rootNode;
std::string g_currentModuleName;
std::string DELIM = "___#";
std::string ENDSIGN = "__(";
uint32_t g_destWidth;
bool g_seeInputs;
uint32_t g_maxDelay = 0;
uint32_t g_pushNum = 0;

    
    
// Check if this type is small enough to be passed in a register.
// BTW, we don't use floats and doubles...
static bool
isSmallType(const llvm::Type *type) {
  return (type->isIntegerTy() && type->getIntegerBitWidth() <= 64);
}



// assume ssaTable and nbTable have been filled
void check_all_regs() {
  toCout("### Begin check_all_regs");
  // clean goal file
  std::ofstream goalFile;
  goalFile.open(g_path+"/goal.txt");
  goalFile.close();
  g_insContextStk.clear();
  clean_module_inputs();
  uint32_t i = 1;
  DestInfo destInfo;
  UpdateFunctionGen UFGen;  
  UFGen.TheContext = std::make_unique<llvm::LLVMContext>();
  for(auto instrInfo : g_instrInfo) {
    if(!instrInfo.writeASVVec.empty() && !instrInfo.writeASV.empty()) {
      toCout("Error: does not support single ASV and vector together");
      abort();
    }
    toCout("---  BEGIN INSTRUCTION #"+toStr(i++)+" ---");
    destInfo.set_instr_name(instrInfo.name);
    //g_readASV = instrInfo.readASV;
    if(g_use_read_ASV && g_readASV.empty()) {
      toCout("Error: read asv is not specified in the instr file!");
      abort();
    }
    g_currInstrInfo = instrInfo;
    // declaration for llvm
    for(auto pair: instrInfo.writeASV) {
      uint32_t cycleCnt = pair.first;
      std::string oneWriteAsv = pair.second;
      auto prefixVarPair = split_prefix_var(oneWriteAsv);
      std::string prefix = prefixVarPair.first;
      std::string writeVar = prefixVarPair.second;
      std::string insName;
      destInfo.isVector = false;
      if(prefix.empty()) {
        uint32_t width = get_var_slice_width_simp(writeVar, 
                                                  g_moduleInfoMap[g_topModule]);
        destInfo.set_dest_and_slice(writeVar, width);
        destInfo.set_module_name(g_topModule);
      }
      else if(g_moduleInfoMap.find(prefix) != g_moduleInfoMap.end()) {
        uint32_t width = get_var_slice_width_simp(writeVar, 
                                                  g_moduleInfoMap[prefix]);
        destInfo.set_dest_and_slice(writeVar, width);
        destInfo.set_module_name(prefix);
        insName = ask_parent_my_ins_name(
                    prefix,
                    *(g_moduleInfoMap[prefix]->parentModVec.begin())                    
                  );
        destInfo.set_instance_name(insName);
      }
      else {
        destInfo.set_instance_name(prefix);
        auto subMod = get_mod_info(prefix, g_moduleInfoMap[g_topModule]);
        uint32_t width = get_var_slice_width_simp(writeVar, subMod);
        destInfo.set_dest_and_slice(writeVar, width);
        destInfo.set_module_name(subMod->name);
      }

      std::string fileName = UFGen.make_llvm_basename(destInfo, cycleCnt) + "_tmp.ll";
      UFGen.print_llvm_ir(destInfo, cycleCnt, i-1, fileName);
      //print_llvm_ir_without_submodules(oneWriteAsv, cycleCnt-1, i-1);
      g_maxDelay = cycleCnt;
    }
    // print ir for reg vector
    if(!instrInfo.writeASVVec.empty()) {
      destInfo.isVector = true;
      destInfo.set_dest_vec(instrInfo.writeASVVec);
      std::string firstASV = instrInfo.writeASVVec.front();
      std::string modName = get_mod_name(firstASV);
      uint32_t cycleCnt = instrInfo.writeASVVecDelay;
      if(!modName.empty())
        destInfo.set_module_name(modName);
      else
        destInfo.set_module_name(g_topModule);

      std::string fileName = UFGen.make_llvm_basename(destInfo, cycleCnt) + "_tmp.ll";
      UFGen.print_llvm_ir(destInfo, cycleCnt, i-1, fileName);
      g_maxDelay = cycleCnt;    
    }

    // if g_get_all_update=true, only do the first instruction
    if(i-1 >= g_do_instr_num) return;
  }
}


void UpdateFunctionGen::clean_data() {
  CLEAN_QUEUE.clear();
  DIRTY_QUEUE.clear();
  resetedReg.clear();
}



// To get a full filename, add something like ".ll" to this.
// Static function.
std::string
UpdateFunctionGen::make_llvm_basename(DestInfo &destInfo, 
                                     const uint32_t bound) {
  std::string destNameSimp = destInfo.get_dest_name();
  remove_front_backslash(destNameSimp);
  std::string fileName = g_path+"/"+destInfo.get_instr_name()+"_"
                         +destNameSimp+"_"+toStr(bound);
  return fileName;
}


/// bound is the number of regs from input to output
/// timeIdx start from 0, max value = bound
void UpdateFunctionGen::print_llvm_ir(DestInfo &destInfo, 
                                      const uint32_t bound, 
                                      uint32_t instrIdx,
                                      std::string fileName) {

  // Creating the wrapper function here is not so robust - the optimization tends to
  // damage it.  Instead there is code to make the wrapper function in the final
  // post-processing after optimization.
  bool makeWrapper = false;

  // FIXME: change the following model name
  std::string destName = destInfo.get_dest_name();
  std::string curModName = destInfo.get_mod_name();
  std::string curInsName = destInfo.get_ins_name();
  auto curMod = g_moduleInfoMap[curModName];
  auto curDynData = get_dyn_data(curMod);
  //std::string insName = curInsName.empty() ? curModName : curInsName;
  std::string insName = curModName == g_topModule ? curModName : curInsName;
  Context_t insCntxt(insName, "", curMod, nullptr, nullptr);
  insContextStk.clear();
  clean_all_mod_dynamic_info();
  cct_cnt = 0; 
  ext_cnt = 0;
  if(curModName == g_topModule) {
    insContextStk.push_back(insCntxt);
    print_context_info(insCntxt);
  }
  else {
    // if starts from a sub-module, the submodule and all its parent modules
    // are not treated as separate functions, instead, they are just part of
    // the function of the top module. 
    // Here all the submodules and the top module are pushed into the stack,
    // and their isFunctionedSubMod is set to false
    // Need to fill target and func in the stack later
    while(curMod->name != g_topModule) {
      if(curMod->parentModVec.size() > 1) {
        toCout("Error: module: "+curMod->name+" has multiple parents:");
        for(auto parentMod: curMod->parentModVec)
          toCout(parentMod->name);
      }
      curDynData->isFunctionedSubMod = false;
      auto parentMod = *(curMod->parentModVec.begin());
      insName = ask_parent_my_ins_name(curMod->name, parentMod);
      Context_t insCntxt(insName, "", curMod, parentMod, nullptr);  
      insContextStk.insert(insContextStk.begin(), insCntxt);
      curMod = parentMod;
      curDynData = get_dyn_data(curMod);
    }
    // curMod is the top module
    Context_t insCntxt(curMod->name, "", curMod, nullptr, nullptr);
    curDynData->isFunctionedSubMod = true;
    insContextStk.insert(insContextStk.begin(), insCntxt);    
  }
  // TODO: modify the following two lines of code
  std::string destPrefix = insContextStk.get_hier_name(false);
  if(!destPrefix.empty()) destName = destPrefix + "." + destName;
  TheModule = std::make_unique<llvm::Module>(
    "mod_;_"+curMod->name+"_;_"+destName, 
    *TheContext
  );
  Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  /// declare function
  // input types
  std::vector<llvm::Type *> argTy;
  std::shared_ptr<ModuleInfo_t> topModInfo = g_moduleInfoMap[g_topModule];
  regWidth.clear();
  collect_regs(topModInfo, "", regWidth);
  funcExtract::print_all_regs(regWidth);

  funcExtract::collect_mem_ins(topModInfo, "", memInstances);

  // push regs
  // add regs from all instances of sub-modules to the args
  uint32_t argNum = 0;
  for(auto it = regWidth.begin(); it != regWidth.end(); it++) {
    uint32_t width = it->second;
    argTy.push_back(llvm::IntegerType::get(*TheContext, width));
    argNum++;
  }

  toCout("=== Finished adding reg-type args!");

  // push output ports of memory modules
  for(auto it = memInstances.begin(); it != memInstances.end(); it++) {
    std::string pathInsName = it->first;
    std::string modName = it->second;
    auto memMod = g_moduleInfoMap[modName];
    for(uint32_t i = 0; i <= bound; i++)
      for( auto output : memMod->moduleOutputs ) {
        uint32_t width = get_var_slice_width_simp(output, memMod);
        argTy.push_back(llvm::IntegerType::get(*TheContext, width));
        argNum++;
      }
  }

  toCout("=== Finished adding memory-output args!");

  // push inputs
  // bound = (delay in instr.txt) - 1
  for(uint32_t i = 0; i <= bound; i++)  
    for(auto it = curMod->moduleInputs.begin(); 
             it != curMod->moduleInputs.end(); it++) {
      if(*it == curMod->clk) continue;
      uint32_t width = get_var_slice_width_simp(*it, curMod);
      // FIXME the start and end index may be wrong
      argTy.push_back(llvm::IntegerType::get(*TheContext, width));
      argNum++;
    }

  toCout("=== Finished adding module input args!");
  // return types
  llvm::Type* retTy = destInfo.get_ret_type(TheContext);
  if(retTy == nullptr) {
    toCout("Error: retTy is nullptr");
    abort();
  }

  // This function type definition is suitable for TheFunction and topFunction
  llvm::FunctionType *FT =
    llvm::FunctionType::get(retTy, argTy, false);

  std::string destSimpleName = funcExtract::var_name_convert(destName, true);

  std::vector<std::string> destVec = destInfo.get_no_slice_name();  

  // if target is vector, declare global array before creating
  // the top function
  // These will be non-null only for a vector return type
  llvm::Type* retArrayElementTy = nullptr;
  llvm::ArrayType* retArrayTy = nullptr;
  llvm::GlobalVariable* retArray = nullptr;

  if(destInfo.isVector && !destInfo.isMemVec) {
    retArrayElementTy = destInfo.get_ret_element_type(TheContext);

    retArrayTy = llvm::ArrayType::get(retArrayElementTy, destVec.size());

    // zero initializer
    llvm::ConstantAggregateZero* zeroInit = llvm::ConstantAggregateZero::get(retArrayTy);
    retArray = new llvm::GlobalVariable(
        *TheModule, 
        retArrayTy, false, 
        //llvm::GlobalValue::InternalLinkage,
        llvm::GlobalValue::LinkOnceAnyLinkage,
        zeroInit,
        "RET_ARRAY_PTR"
    );
  }

  // make a top function
  llvm::Function *topFunction 
    = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, 
                             "top_function", TheModule.get());

  // Create the main function
  TheFunction = llvm::Function::Create(
    FT, llvm::Function::InternalLinkage, 
    destInfo.get_instr_name()+"_"+destSimpleName, TheModule.get());
  TheFunction->addFnAttr(llvm::Attribute::NoInline);

  for(auto it = insContextStk.begin();
      it != insContextStk.end(); it++) {
    it->Target = destName;
    // FIXME: change topFunction to TheFunction
    //it->Func = topFunction;
    it->Func = TheFunction;
    initialize_min_delay(it->ModInfo, destName);
  }

  // Optionally make the wrapper function for C/C++ interfacing
  llvm::Function *wrapperFunction = nullptr;
  llvm::FunctionType *wrapperFT = nullptr;

  if (makeWrapper) {
    // Build a FunctionType for wrapperFunction:  it has pointers for
    // every arg bigger than 64 bits.  If the return value is bigger than 64 bits,
    // one more pointer arg is added for it, and the function returns void.

    std::vector<llvm::Type *> wrapperArgTy;
    for (size_t i=0; i < argTy.size(); ++i) {
      llvm::Type *type = argTy[i];
      if (isSmallType(type)) {
        wrapperArgTy.push_back(type);
      } else {
        wrapperArgTy.push_back(llvm::PointerType::getUnqual(type));
      }
    }

    // Deal with small vs large return values
    llvm::Type* wrapperRetTy = nullptr;
    if (isSmallType(retTy)) {
      wrapperRetTy = retTy;
    } else {
      // Add one more pointer argument for return value.
      // This pointer is typed - using an opaque pointer here caused a LLVM crash during optimization...
      wrapperArgTy.push_back(llvm::PointerType::getUnqual(retTy));
      wrapperRetTy = llvm::Type::getVoidTy(*TheContext);
    }

    wrapperFT = llvm::FunctionType::get(wrapperRetTy, wrapperArgTy, false);

    // Like the main function, the wrapper function must have internal linkage so
    // that the unneeded args can be optimized away.
    wrapperFunction = llvm::Function::Create(wrapperFT, llvm::Function::InternalLinkage, 
                       destInfo.get_instr_name()+"_"+destSimpleName+"_wrapper_old", TheModule.get());
    wrapperFunction->addFnAttr(llvm::Attribute::NoInline);
  }

  // set arg names for the functions
  uint32_t idx = 0;
  
  // First, do the reg-type args
  for (auto w : regWidth) {
    std::string regName = w.first;
    std::string regNameBound = regName+post_fix(bound);
    toCoutVerb("set reg-type func arg: "+regNameBound);
    TheFunction->getArg(idx)->setName(regNameBound);
    topFunction->getArg(idx)->setName(regNameBound);

    // Some wrapper function args are pointers, not values
    if (wrapperFunction) {
      if (isSmallType(TheFunction->getArg(idx)->getType())) {
        wrapperFunction->getArg(idx)->setName(regNameBound);
      } else {
        wrapperFunction->getArg(idx)->setName(regNameBound+"ptr_");
      }
    }

    argNum--;
    topFuncArgMp.emplace(regNameBound, TheFunction->args().begin()+idx);
    idx++;
  }

  toCout("=== Finished setting reg-type arg name!");

  // Now do memory-output args
  for(auto inst : memInstances) {
    std::string pathInsName = inst.first;
    std::string modName = inst.second;
    auto memMod = g_moduleInfoMap[modName];
    for(uint32_t i = 0; i <= bound; i++) {
      for( auto output : memMod->moduleOutputs ) {
        std::string portName = pathInsName+"."+output+post_fix(i);
        toCoutVerb("set mem ouput func arg, mem: "+pathInsName+", output: "+output);
        TheFunction->getArg(idx)->setName(portName);
        topFunction->getArg(idx)->setName(portName);
	
        if (wrapperFunction) {
          // Some wrapper function args are pointers, not values
          if (isSmallType(TheFunction->getArg(idx)->getType())) {
            wrapperFunction->getArg(idx)->setName(portName);
          } else {
            wrapperFunction->getArg(idx)->setName(portName+"ptr_");
          }
        }

        argNum--;
        topFuncArgMp.emplace(portName, TheFunction->args().begin()+idx);
	idx++;
      }
    }
  }

  toCout("=== Finished setting memory output arg name!");

  // Now do module-input args
  uint32_t argSize = TheFunction->arg_size();
  toCoutVerb("Function arg size is: "+toStr(argSize));
  for (uint32_t i = 0; i <= bound; i++) {
    for (std::string inp : curMod->moduleInputs) {
      if(inp == curMod->clk) continue;    
      //uint32_t width = get_var_slice_width_simp(inp, curMod);
      std::string argName = inp+post_fix(i);
      toCoutVerb("set func arg: "+argName);
      (TheFunction->args().begin()+idx)->setName(argName);
      (topFunction->args().begin()+idx)->setName(argName);

      if (wrapperFunction) {
        // Some wrapper function args are pointers, not values
        if (isSmallType(TheFunction->getArg(idx)->getType())) {
          wrapperFunction->getArg(idx)->setName(argName);
        } else {
          wrapperFunction->getArg(idx)->setName(argName+"ptr_");
        }
      }

      idx++;
      argNum--;
    }
  }

  toCout("=== Finished setting module input arg name!");

  // If it exists, set the name of the extra wrapper function arg
  if (wrapperFunction && !isSmallType(retTy)) {
    wrapperFunction->getArg(idx)->setName("_return_val_ptr_");
  }

  // basic block
  BB = llvm::BasicBlock::Create(*TheContext, "bb_;_"+destName, TheFunction);
  Builder->SetInsertPoint(BB);

  ignoreSubModules = false;
  skipCheck = true;
  clean_data();
  std::ofstream goalFile;
  goalFile.open(g_path+"/goal.txt", std::ofstream::app);
  llvm::Value* destNextExpr;

  curMod = g_moduleInfoMap[curModName];
  llvm::Instruction* retInst;
  if (!destInfo.isVector) {
    std::string dest = destVec.front();
    if (is_output(destName, curMod)) initialize_min_delay(curMod, destName);
    if (curMod->visitedNode.find(dest) == curMod->visitedNode.end()
        && curMod->reg2Slices.find(dest) == curMod->reg2Slices.end()) {
      toCout("Error: ast node is not found for this var: |"+dest+"|"
              +", curMod: "+curMod->name);
      abort();
    } 
    // The return value for the function
    else 
      destNextExpr = add_constraint(dest, 
                                    0, TheContext, Builder, bound);
    retInst = Builder->CreateRet(destNextExpr);
  }
  else {
    // if destName is a vector, return an array
    std::vector<llvm::Value*> retVec;
    std::string modName = destInfo.get_mod_name();
    std::string insName = destInfo.get_ins_name();
    if (insName.empty()) insName = modName;
    check_mod_name(modName);
    llvm::Value* destNextExpr;
    //FIXME: currently do not support vector in submodules
    for (std::string dest: destVec) {
      if (is_output(dest, curMod)) initialize_min_delay(curMod, dest);      
      if (dest == "buff10")
        toCoutVerb("Find it!");
      curMod = g_moduleInfoMap[modName];

      //toCout("Error: re-think and check the g_insContextStk");
      assert(modName == g_topModule);
      insContextStk.clear();
      Context_t insCntxt(insName, dest, curMod, nullptr, TheFunction);
      insContextStk.push_back(insCntxt);
      print_context_info(insCntxt);      
      if (curMod->visitedNode.find(dest) == curMod->visitedNode.end()
          && curMod->reg2Slices.find(dest) == curMod->reg2Slices.end()) {
        toCout("Error: ast node is not found for this var: |"+dest+"|"
                +", curMod: "+curMod->name);
        abort();
      } 
      else {
        destNextExpr = add_constraint(dest, 
                                      0, TheContext, Builder, bound);
        if (!destInfo.isMemVec)
          retVec.push_back(destNextExpr);
      }
    }

    uint32_t size = destVec.size();
    uint32_t bitNum = log2(size) + 2;

    // the arrayAlloca is removed because a global array is declared
    //llvm::AllocaInst* arrayAlloca 
    //    = Builder->CreateAlloca(
    //        //allocaTy,
    //        retTy,
    //        llvm::ConstantInt::get(llvm::IntegerType::get(*TheContext, bitNum), 
    //                                                size, false), // # elements
    //        llvm::Twine(destName)
    //      );

    // store values in retVec to memory of global array
    uint32_t i = 0;
    if (!retVec.empty()) {
      assert(retArrayElementTy);
      for (llvm::Value* val : retVec) {
        llvm::GetElementPtrInst* ptr 
          = llvm::GetElementPtrInst::Create(
              retArrayTy,
              retArray,
              std::vector<llvm::Value*>{
                llvm::ConstantInt::get(
                  llvm::IntegerType::get(*TheContext, bitNum), 
                  0, false),
                llvm::ConstantInt::get(
                  llvm::IntegerType::get(*TheContext, bitNum), 
                  i++, false) },
              llvm::Twine(val->getName().str()+"_mem"),
              BB
            );
        Builder->SetInsertPoint(BB);

        // print info
        //uint32_t valWidth = val->getType()->getIntegerBitWidth();
        //auto ty1 = val->getType();
        //auto ty2 = llvm::cast<llvm::ArrayType>(retTy)->getElementType();
        //auto ty3 = llvm::cast<llvm::PointerType>(ptr->getType())->getElementType();
        //auto ty4 = retArrPtr->getType();
	Builder->CreateStore(val, value(ptr));  
      }
    }
    else assert(destInfo.isMemVec);

    //llvm::LoadInst *retArr = Builder->CreateLoad(retTy, retArrPtr, llvm::Twine("retArr"));
    //Builder->CreateRet(value(retArr));

    if (!retVec.empty()) {
      llvm::GetElementPtrInst* retPtr 
        = llvm::GetElementPtrInst::Create(
            retArrayTy,
            retArray,
            std::vector<llvm::Value*>{
              llvm::ConstantInt::get(
                llvm::IntegerType::get(*TheContext, bitNum), 
                0, false),
              llvm::ConstantInt::get(
                llvm::IntegerType::get(*TheContext, bitNum), 
                0, false) },
            llvm::Twine("RET_PTR"),
            BB
          );

      retInst = Builder->CreateRet(value(retPtr));
    }
    else {
      assert(destInfo.isMemVec);    
      retInst = Builder->CreateRetVoid();
    }
  } // end of add vector


  // if the top module contains memories, add the additional memory
  // writes from lastMemReadAddr+1 to bound-1
  uint32_t i = 0;
  if (!curMod->moduleMems.empty()) {
    Builder->SetInsertPoint(retInst);  
    for (auto pair : curDynData->memDynInfo) {
      std::string mem = pair.first;
      toCoutVerb("check mem: "+mem);
      if (i == 48)
        toCoutVerb("Find i!");
      toCoutVerb("i: "+toStr(i++));
      if (mem.find("\\store.tensorStore.tensorFile_0_0") != std::string::npos)
        toCoutVerb("Find it!");
      astNode* node = memNodeMap[mem];
      if (!pair.second.hasBeenWritten) continue;      
      uint32_t lastWriteTimeIdx = pair.second.lastWriteTimeIdx;
      for (uint32_t i = lastWriteTimeIdx+1; i < bound; i++) {
        mem_assign_constraint(node, i, TheContext, Builder, bound);
      }
    }
  }

  llvm::verifyFunction(*TheFunction);


  // Fill in the contents of topFunction

  auto topBB = llvm::BasicBlock::Create(*TheContext, "top_bb", topFunction);
  Builder->SetInsertPoint(topBB);  


  // call TheFunction in topFunction: same args that topFunction has
  std::vector<llvm::Value*> args;
  for (size_t i=0; i < argSize; ++i) {
    args.push_back(topFunction->getArg(i));
  }

  llvm::Value *theRet = Builder->CreateCall(FT, TheFunction, args);


  if (wrapperFunction) {
    // call wrapperFunction (if any) in topFunction.
    // This prevents the wrapperFunction from being optimized away.
    // Remember: topFunction will eventually get removed.
    //
    std::vector<llvm::Value*> wrapperArgs;
    for (size_t i=0; i < argSize; ++i) {
      llvm::Type *type = argTy[i];
      if (isSmallType(type)) {
        // Pass the value
        wrapperArgs.push_back(topFunction->getArg(i));
      } else {
        // Create a pointer that points to some memory 
        llvm::Value *tmpPtr = Builder->CreateAlloca(type);
        // store the arg value into that memory
        Builder->CreateStore(topFunction->getArg(i), tmpPtr);
        //wrapperArgs.push_back(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(type)));
        wrapperArgs.push_back(tmpPtr);
      }
    }

    llvm::Value *wrapperRet = nullptr;
    if (isSmallType(retTy)) {
      // the wrapper returns by value
      wrapperRet = Builder->CreateCall(wrapperFT, wrapperFunction, wrapperArgs);
    } else {
      // Pass an additional pointer to something
      llvm::Value *retPtr = Builder->CreateAlloca(retTy);
      // In release 15 of LLVM, this will become an opaque pointer
      //llvm::Value *retOpaquePtr = Builder->CreateBitCast(retPtr, llvm::PointerType::getUnqual(*TheContext));
      wrapperArgs.push_back(retPtr);

      // Call the wrapper - it will fill in the additional pointer
      Builder->CreateCall(wrapperFT, wrapperFunction, wrapperArgs);

      // Dereference that pointer to get the return value
      wrapperRet = Builder->CreateLoad(retTy, retPtr);
    }

    // Do a calculation with both return values, so that the functions will not get optimized away.
    llvm::Value *finalRet = Builder->CreateAdd(theRet, wrapperRet);
    Builder->CreateRet(finalRet);

  } else {
    Builder->CreateRet(theRet);
  }


  llvm::verifyFunction(*topFunction);


  // Fill in the contents of wrapperFunction (if any)
  if (wrapperFunction) {
    auto wrapperBB = llvm::BasicBlock::Create(*TheContext, "wrapper_bb", wrapperFunction);
    Builder->SetInsertPoint(wrapperBB);  

    // Pass the wrapper args down to the main function. For ones that
    // are provided via pointers, dereference the pointers.
    std::vector<llvm::Value*> wrappedArgs;
    for (size_t i=0; i < argSize; ++i) {
      llvm::Value *argVal = nullptr;
      if (isSmallType(argTy[i])) {
        // Pass the arg by value
        argVal = wrapperFunction->getArg(i);
      } else {
        // Dereference the pointer
        argVal = Builder->CreateLoad(argTy[i], wrapperFunction->getArg(i));
      }
      wrappedArgs.push_back(argVal);
    }
    
    // call TheFunction from wrapperFunction
    llvm::Value *call = Builder->CreateCall(FT, TheFunction, wrappedArgs);

    if (isSmallType(retTy)) {
      Builder->CreateRet(call);
    } else {
      // Add store of return value to last pointer arg
      Builder->CreateStore(call, wrapperFunction->getArg(argSize));
      Builder->CreateRetVoid();
    }

    llvm::verifyFunction(*wrapperFunction);
  }

  llvm::verifyModule(*TheModule);
  std::string Str;
  llvm::raw_string_ostream OS(Str);
  OS << *TheModule;
  OS.flush();
  std::ofstream output(fileName);
  output << Str << std::endl;
  output.close();
  if (true) {
    std::ofstream output("./tmp.ll");
    output << Str << std::endl;
    output.close();    
  }
  toCout("** Finish update function for: "+destName);
}


// submodules are treated as another module, communicated with ports
//void simplify_goal_without_submodules(std::string destAndSlice, 
//                                      const uint32_t bound, 
//                                      uint32_t instrIdx) {
//  g_skipCheck = true;
//  g_ignoreSubModules = true;
//  clean_data();
//  std::ofstream goalFile;
//  goalFile.open(g_path+"/sub_goals.txt");
//  context c;
//  solver s(c);
//  g_goalVars.push(std::make_pair(destAndSlice, 0));
//  while(g_goalVars.size() > 0) {
//    goal g(c);
//    auto varPair = g_goalVars.front();
//    g_goalVars.pop();
//    std::string var = varPair.first;
//    remove_two_end_space(var);
//    uint32_t timeIdx = varPair.second;
//    g_regWithFunc.insert(var);
//    if(g_visitedNode.find(var) == g_visitedNode.end()
//        && g_visitedNode.find("\\"+var) == g_visitedNode.end() ) {
//      toCout("Error: the key does not exist in the map: |"+var+"|");
//      abort();
//    } 
//    expr destNextExpr = add_constraint(g_visitedNode[var], 
//                                       timeIdx, c, s, g, bound, /*isSolve=*/false);
//    expr destExpr_g = var_expr(var, timeIdx+100, c, false);
//    g.add( destExpr_g == destNextExpr ); 
//    //add_all_clean_constraints(c, s, g, bound, /*isSolve=*/false);
//    tactic t(c, "simplify");
//    apply_result r = t(g);
//    toCout("*************************  Update function for "+var);
//    std::cout << r << std::endl;
//    g_outFile << r << std::endl;
//    goalFile << "#"+toStr(instrIdx)+"#"+var+"#"+toStr(bound) << std::endl;
//    goalFile << r << std::endl;
//  }
//  goalFile.close();
//}


// should construct two things while checking
// 1. a SMT equation to be solved for correct input and AS
// 2. an AST tree for the reg related expression. The solution 
// from 1 will be used to simplify this set of expressions.
//
// However, constructions for the two are quite different. AST tree is
// constructed only one. But the SMT equation may need to be constructed
// multiple times, until a solution is obtained or a bound is reached.
//void check_single_reg_and_slice(std::string destAndSlice, 
//                                const uint32_t boundIn, 
//                                uint32_t instrIdx) {
//  g_skipCheck = false;  
//  std::ofstream goalFile;
//  goalFile.open(g_path+"/goal.txt");
//  g_rootNode = destAndSlice;
//  clean_data();
//  toCout("========== Begin check SAT for: "+destAndSlice+" ==========");
//  uint32_t regWidth = get_var_slice_width_simp(destAndSlice);
//  CURRENT_VAR = destAndSlice;
//  const uint32_t bound = boundIn > 0 ? boundIn : 0;
//  bound_limit = boundIn+2;
//  bool z3Res = false;
//  context c;
//
//  std::set<std::string> varToExpand{destAndSlice};
//  expr destExpr_t = var_expr(destAndSlice, 0, c, true);      
//  s.push();
//  bool lastHasSolution = false;
//  bool curHasSolution = false;
//  // relation between bound and timeIdx:
//  // expr wih timeIdx = bound exist in solutions.
//  // But those expr are only inputs.
//  uint32_t topTimeIdx = 0;
//  g_seeInputs = false;
//  while(bound < bound_limit) {
//    curHasSolution = false;
//    s.pop();
//    CLEAN_QUEUE.clear();
//    DIRTY_QUEUE.clear();
//    // g_existedExpr must be cleared because reg_#bound needs to be expanded
//    g_existedExpr.clear();
//
//    toCoutVerb("### Begin bound: "+ toStr(bound));
//    goal g(c);
//    for(std::string rootReg: varToExpand) {
//      if(g_visitedNode.find(pure(rootReg)) == g_visitedNode.end() ) {
//        toCout("Error: the key does not exist in the map: |"+pure(rootReg)+"|");
//        abort();
//      } 
//      astNode *root = g_visitedNode[pure(rootReg)];
//      if(root) {
//        add_constraint(root, topTimeIdx, c, s, g, bound, true);
//      }
//      else {
//        toCout(destAndSlice+" does not have its root!");
//        abort();
//      }
//    }
//    // Finished adding constraints except leaves.
//    s.push();
//    add_all_clean_constraints(c, s, g, bound, true);
//    add_all_dirty_constraints(c, s, bound);
//    add_input_values(c, s, bound);
//    add_nop(c, s, bound);
//    // save dirty and clean regs for next round
//    save_regs_for_expand(varToExpand);
//    z3Res = (s.check() == sat);
//    // exhaust all the solutions for a bound
//    while(z3Res) {
//      curHasSolution = true;
//      INPUT_EXPR_VAL.clear();
//      INT_EXPR_SET.clear();
//      g_resetedReg.clear();
//      model m = s.get_model();
//      uint32_t j = 0;
//      // only block values for varriables in CLEAN_QUEUE
//      toCout("+++++++ Solution found for "+destAndSlice
//             +", bound: "+toStr(int(bound))+" +++++++++");
//      g_outFile << "+++++++ Solution found for "+destAndSlice
//                   +", bound: "+toStr(int(bound))+" +++++++++" << std::endl;
//
//      std::vector<std::pair<std::string, uint32_t>> varIdxPairVec;
//      for (uint32_t i = 0; i < m.size(); i++)
//        varIdxPairVec.push_back(std::make_pair(m[i].name().str(), i));
//
//      std::sort(varIdxPairVec.begin(), varIdxPairVec.end(), comparePair);
//      for (auto pair : varIdxPairVec) {
//        func_decl v = m[pair.second];
//        assert(v.arity() == 0);
//        std::string var = v.name().str();
//        if(var == "instr_bne___#1_T") {
//          toCoutVerb("Find it");
//        }
//        assert(var == pair.first);
//        // check if current is reg and has reset value
//        expr tmp = m.get_const_interp(v);
//        if( g_rstVal.find(pure(var)) != g_rstVal.end() && !is_taint(var)) {
//          std::string localVal = g_rstVal[pure(var)];
//          localVal = is_number(localVal) ? localVal : "0";
//          if(tmp == c.bv_val(hdb2int(localVal), get_var_slice_width_simp(pure(var)))) {
//            g_resetedReg.insert(var);
//            expr *tmpPnt = new expr(m.get_const_interp(v));
//            INPUT_EXPR_VAL.emplace(var, tmpPnt);
//          }
//        }
//
//        if(!is_taint(var) && ( isInput(pure(var)) || isAs(pure(var)) )) {
//          std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ " 
//                    << v.name() << " = " << m.get_const_interp(v) << "\n";
//          g_outFile << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ " 
//                    << v.name() << " = " << m.get_const_interp(v) << "\n";
//          if(isInput(pure(var)) || isAs(pure(var)) ) {
//            expr *tmpPnt = new expr(m.get_const_interp(v));
//            INPUT_EXPR_VAL.emplace(var, tmpPnt);
//          }
//        }
//        else  {
//          std::cout << v.name() << " = " << m.get_const_interp(v) << "\n";
//          g_outFile << v.name() << " = " << m.get_const_interp(v) << "\n";
//        }
//      }
//
//      // after getting one solution, build a goal and simplify it with input values
//      g_existedExpr.clear();
//      if(g_visitedNode.find(destAndSlice) == g_visitedNode.end() ) {
//        toCout("Error: the key does not exist in the map: |"+destAndSlice+"|");
//        abort();
//      } 
//      expr destNextExpr = add_constraint(g_visitedNode[destAndSlice], 
//                                         0, c, s, g, bound, /*isSolve=*/false);
//      expr destExpr_g = var_expr(destAndSlice, 100, c, false);
//      g.add( destExpr_g == destNextExpr ); 
//      //add_all_clean_constraints(c, s, g, bound, /*isSolve=*/false);
//      tactic t(c, "simplify");
//      apply_result r = t(g);
//      toCout("*************************  Update function for "+destAndSlice);
//      std::cout << r << std::endl;
//      g_outFile << r << std::endl;
//      goalFile << "#"+toStr(instrIdx)+"#"+destAndSlice+"#"+toStr(bound) << std::endl;
//      goalFile << r << std::endl;
//
//      // if input's value is x, fix it to x
//      // fix value for readASV
//      uint32_t encodingSize = g_currInstrInfo.instrEncoding.begin()->second.size();
//      for(uint32_t i = 0; i < m.size(); i++) {
//        func_decl v = m[i];
//        std::string vName = v.name().str();
//        std::string pureVName = pure(vName);
//        uint32_t localTimeIdx = get_time(vName);
//        if(localTimeIdx >= bound + 2 - encodingSize) {
//          uint32_t idx = bound+1-localTimeIdx;
//          if( g_currInstrInfo.instrEncoding.find(pureVName) 
//                != g_currInstrInfo.instrEncoding.end() ) {
//            if(g_currInstrInfo.instrEncoding[pureVName][idx] == "x") {
//              toCoutVerb("Fix value for "+vName);
//              s.add( v() == m.get_const_interp(v) );
//            }
//          }
//          else if( is_read_asv(pureVName) ) {
//            toCoutVerb("Fix value for "+vName);
//            s.add( v() == m.get_const_interp(v) );
//          }
//        }
//      }
//
//      // begin block this solution for a new solution
//      while( j < m.size() ) {
//        bool res = is_clean(m[j++].name().str());
//        if(res) break;
//      }
//      if( j == m.size()) {
//        toCout("Error: does not find clean var in the model");
//        abort();
//      }
//      j--;
//      func_decl v = m[j];
//      expr block = (v() != m.get_const_interp(v));
//      for (uint32_t i = j+1; i < m.size(); i++) {
//        func_decl v = m[i];
//        std::string vName = v.name().str();
//        // block this value 
//        if( is_clean(vName) && !is_read_asv(vName) )
//          block = block || (v() != m.get_const_interp(v));
//        else if(is_read_asv(vName) && get_time(vName) == bound + 1)
//          // we do not want to iterate read-ASV values
//          s.add( v() == m.get_const_interp(v) );
//      }
//      s.add(block);
//      // Make sure the clean and dirty constraints are at the top
//      //add_all_clean_constraints(c, s, g, bound);
//      //add_all_dirty_constraints(c, s, bound);
//      //add_input_values(c, s, bound);
//      //add_nop(c, s, bound);      
//      save_regs_for_expand(varToExpand);      
//      z3Res = (s.check() == sat);
//    }
//    assert(bound <= bound_limit);
//    toCout("------- No more solution found within the bound: "
//           +toStr(bound)+" ----------");
//    g_outFile << "------- No more solution found within the bound: "
//                 +toStr(bound)+" ----------" << std::endl;
//    // terminate if has solution in lower bound but no solution in current bound
//    if(lastHasSolution && !curHasSolution) return; 
//    bound++;
//    topTimeIdx = bound; // Attention: bound has already added by 1
//    lastHasSolution = curHasSolution;
//  }
//  goalFile.close();
//}


llvm::Value*
UpdateFunctionGen::add_constraint(std::string varAndSlice, uint32_t timeIdx, context &c,
                                   std::shared_ptr<llvm::IRBuilder<>> &b,
                                   const uint32_t bound ) {
  if (varAndSlice.find("hls_target_linebuffer_1_U0_out_stream_V_value_V_write") 
       != std::string::npos)
       //&& timeIdx == 17)
    toCoutVerb("Find it!");
  auto curMod = insContextStk.get_curMod();
  if (curMod->name == "hls_target_Loop_1_proc")
    toCoutVerb("Find it!");
  llvm::Value* ret;
  bool retIsEmpty = true;
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  const std::string curTgt = insContextStk.get_target();
  //assert(curMod->visitedNode.find(curTgt) != curMod->visitedNode.end());
  if (curMod->reg2Slices.find(var) == curMod->reg2Slices.end()) {
    if (curMod->visitedNode.find(var) == curMod->visitedNode.end()) {
      toCout("Error: cannot find node for: "+varAndSlice);
      abort();
    }
    return add_constraint(curMod->visitedNode[var], timeIdx, c, b, bound);
  }
  else {
    for (std::string varSlice : curMod->reg2Slices[var]) {
      if (curMod->visitedNode.find(varSlice) == curMod->visitedNode.end()) {
        toCout("!!! Error: cannot find node for: "+varSlice);
        abort();
      }
      llvm::Value* tmpSlice = add_constraint(curMod->visitedNode[varSlice], 
                                             timeIdx, c, b, bound);
      if (retIsEmpty) {
        retIsEmpty = false;
        ret = tmpSlice;
      }
      else {
        ret = concat_func(ret, tmpSlice, c, b, timeIdx);
      }
    }
    return ret;
  }

}

/// return: if node->dest is a slice, which means the slice is directly assigned, 
//            it just returns expr for the slice. Otherwise, it returns expr for
//            the whole var.
llvm::Value* 
UpdateFunctionGen::add_constraint(astNode* const node, uint32_t timeIdx, context &c,
                            std::shared_ptr<llvm::IRBuilder<>> &b,
                            const uint32_t bound ) {
  // Attention: varAndSlice might have a slice, a directly-assigned varAndSlice
  std::string varAndSlice = node->dest;
  if (varAndSlice.find("\\fetch.xsize") 
       != std::string::npos)
    toCoutVerb("Find it!");

  auto curMod = insContextStk.get_curMod();
  if (curMod->name == "hls_target_Loop_1_proc")
    toCoutVerb("Find it!");

  std::shared_ptr<ModuleDynInfo_t> curDynData = get_dyn_data(curMod);
  toCoutVerb("add_constraint for: "+varAndSlice+", timeIdx: "+toStr(timeIdx)
             +", bound: "+toStr(bound));
  if (varAndSlice == "64'hxxxxxxxxxxxxxxxx" && timeIdx == 1) {
    toCoutVerb("find it");
  }

  if (varAndSlice.find("compute.loadUop.sIdx") != std::string::npos
       && timeIdx == 4) {
    toCoutVerb("Find it!");
  }

  if (timeIdx > bound) {
    uint32_t width = insContextStk.get_var_slice_width_simp(varAndSlice);
    return llvmInt(0, width, c);
  }

  const std::string curTgt = insContextStk.get_target();
  if (curDynData->existedExpr.find(curTgt) == curDynData->existedExpr.end() )
    curDynData->existedExpr.emplace(curTgt, 
                                    std::map<std::string, llvm::Value*>() );

  if (curDynData->existedExpr[curTgt].find(timed_name(varAndSlice, timeIdx)) 
      != curDynData->existedExpr[curTgt].end() ) {
    return curDynData->existedExpr[curTgt][timed_name(varAndSlice, timeIdx)];
  }

  llvm::Value* retExpr;
  if ( is_input(varAndSlice, curMod) ) { // input_t is always 0
    retExpr = input_constraint(node, timeIdx, c, b, bound);
  }
  else if ( is_reg_in_curMod(varAndSlice, curMod) 
             && node->type != SRC_CONCAT && node->type != SWITCH) { 
    // AS case is moved to add_nb_constraint
    retExpr = add_nb_constraint(node, timeIdx, c, b, bound);
  }
  else if ( is_number(varAndSlice) ) { // num_t is always 0
    //if (varAndSlice.find("x") != std::string::npos) {
    //  toCout("!! Warning: number in add_constraint should not have x: "
    //           +varAndSlice);
    //  //abort();
    //}
    retExpr = num_constraint(node, timeIdx, c, b);
  }
  else if ( is_case_dest(varAndSlice, curMod) ) {
    retExpr = case_constraint(node, timeIdx, c, b, bound);
  }
  else if ( is_switch_dest(varAndSlice, curMod) ) {
    retExpr = switch_constraint(node, timeIdx, c, b, bound);
  }

  //else if ( is_func_output(varAndSlice) ) {
  //  retExpr = func_constraint(node, timeIdx, c, s, g, bound, isSolve);
  //}
  else if ( is_submod_output(varAndSlice, curMod) ) {
    auto pair = curMod->wire2InsPortMp[varAndSlice];
    std::string insName = pair.first;
    auto subMod = get_mod_info(insName, curMod);
    std::string modName = subMod->name;
    if (modName == "aes_128")
      toCoutVerb("Find it!");
    if (is_mem_module(modName))
      retExpr = memMod_constraint(node, timeIdx, c, b, bound);
    else if (g_blackBoxModSet.find(modName) != g_blackBoxModSet.end())
      retExpr = bbMod_constraint(node, timeIdx, c, b, bound);
    else
      retExpr = submod_constraint(node, timeIdx, c, b, bound);
  }
  else if ( node->type == MEM_IF_ASSIGN ) {
    //toCout("Error: mem_if_assign should not appear in add_constraint:"
    //        +node->dest);
    //abort();

    std::string mem = node->dest;
    if (curDynData->memDynInfo.find(mem) != curDynData->memDynInfo.end()) {
      toCout("Error: the mem is already in curDynData->memDynInfo: "+mem);
      abort();
    }
    uint32_t lineWidth = curMod->moduleMems[mem].first;
    uint32_t lineNum = curMod->moduleMems[mem].second;

    llvm::Type* elementTy = llvmWidth(lineWidth, c);
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementTy, lineNum);
    std::vector<llvm::Constant*> initList;  
    for (uint32_t i = 0; i < lineNum; i++) {
      initList.push_back(
        llvm::ConstantInt::get(llvm::IntegerType::get(*c, lineWidth), 
                               0, false)
      );
    }
    // initialize the MemDynInfo_t
    llvm::Value* memValue;    
    llvm::GlobalVariable* memArr = new llvm::GlobalVariable(
      *TheModule,
      arrayType, false,
      // llvm::GlobalValue::InternalLinkage,
      llvm::GlobalValue::LinkOnceAnyLinkage,
      llvm::ConstantArray::get(arrayType, 
                              llvm::ArrayRef<llvm::Constant*>(initList)),
      mem+"_ARR"
    );
    memValue = memArr;
    curDynData->memDynInfo.emplace(mem, MemDynInfo_t{0, false, memValue});
    mem_assign_constraint(node, timeIdx, c, b, bound);
    return nullptr;
  }
  else if ( node->type == DYNSEL ) {
    retExpr = dyn_sel_constraint(node, timeIdx, c, b, bound);
  }
  else { // it is wire
    retExpr = add_ssa_constraint(node, timeIdx, c, b, bound);
  }
  curDynData->existedExpr[curTgt].emplace(timed_name(varAndSlice, timeIdx), 
                                          retExpr);
  if (curMod->name == "T" && curTgt == "out" 
       && varAndSlice == "in" && timeIdx == 1)
    toCoutVerb("push into expr!");
  return retExpr;
}


llvm::Value* 
UpdateFunctionGen::add_nb_constraint(astNode* const node, 
                               uint32_t timeIdx, context &c, 
                               std::shared_ptr<llvm::IRBuilder<>> &b,
                               const uint32_t bound ) {
  const auto curFunc = insContextStk.get_func();
  const auto curMod = insContextStk.get_curMod();
  auto curDynData = get_dyn_data(curMod);
  std::string destAndSlice = node->dest;
  std::string dest, destSlice;
  split_slice(destAndSlice, dest, destSlice);
  if (destAndSlice.find("compute.loadUop.sIdx_reg[0]") != std::string::npos){
       //&& timeIdx == 8) {
    //toCoutVerb("target reg found! time: "+toStr(timeIdx));
    toCoutVerb("Find it");
  }
  llvm::Value* destNextExpr;
  // assuming RHS of nonblocking is not number
  if ( node->childVec.front()->dest.empty() ) {
    toCout("Error: first child's dest is empty: "+node->dest);
    abort();
  }

  // why: when inputs havn't been seen, more unrolling is necessary
  //if (!g_seeInputs && timeIdx == bound) bound++;

  if (timeIdx < bound) {
    toCoutVerb("Add nb constraint for: " + destAndSlice
               +" ------  time: "+toStr(timeIdx));
    if (destAndSlice == "decoder_trigger_q" && timeIdx == 3)
      toCoutVerb("Find it!");
    std::string destNext = node->srcVec.front();

    auto srcExpr = add_constraint(node->childVec.front(), timeIdx+1, c, b, bound);
    std::string srcAndSlice = node->srcVec[0];
    std::string src, srcSlice;
    split_slice(srcAndSlice, src, srcSlice);
    uint32_t srcHi = get_lgc_hi(srcAndSlice, curMod);
    uint32_t srcLo = get_lgc_lo(srcAndSlice, curMod);
    if (srcSlice.empty() || has_direct_assignment(srcAndSlice, curMod))
      destNextExpr = srcExpr;
    else
      destNextExpr = extract_func(srcExpr, srcHi, srcLo, 
                                  c, b, timeIdx+1, llvm::Twine(srcAndSlice));
    return destNextExpr;
  }
  else if ( is_clean(dest, insContextStk.get_curMod()) ){ 
    // the bound has been reached, do not expand its assignment
    push_clean_queue(node, timeIdx);
  }
  else {
    push_dirty_queue(node, timeIdx);
  }
  // end of if

  // if timeIdx = bound, then return function input or rst/norm value
  // TODO: adjust the following condition for different designs

  //if (curMod->invarRegs.find(dest) == curMod->invarRegs.end()) {
  if (dest.find("internal_empty_n") != std::string::npos) {
    toCoutVerb("Find it!");
  }
  if (g_use_read_ASV) {
    if (is_read_asv(dest, insContextStk.get_curMod())) {
        //&& curMod->moduleAs.find(dest) != curMod->moduleAs.end())
      //std::string prefix = get_hier_name(false);
      //if (!prefix.empty()) prefix += ".";
      if (curDynData->isFunctionedSubMod == false) {
        std::string prefix = insContextStk.get_hier_name(false);
        if (!prefix.empty()) prefix += ".";
        dest = prefix + dest;
      }
      std::string destTimed = timed_name(dest, timeIdx);
      auto destExpr = get_arg(destTimed, curFunc);
      if (destSlice.empty())
        return destExpr;
      else {
        uint32_t hi = get_end(destSlice);
        uint32_t lo = get_begin(destSlice);
        return extract_func(destExpr, hi, lo, 
                            c, b, timeIdx, llvm::Twine(destTimed), true);
      }
    }
    else {
      uint32_t width = insContextStk.get_var_slice_width_simp(destAndSlice);    
      std::string rstVal = get_rst_value(destAndSlice, timeIdx, width);
      return var_expr(rstVal, timeIdx, c, b, false, width);    
    }
  }
  else {
    // if do not use read asv, then for invariant registers, return their
    // normal value(not rst value), and for other registers, set them symbolic
    std::string modName = curMod->name;
    if (g_invarRegs.find(modName) == g_invarRegs.end()) {
      return get_arg(timed_name(dest, timeIdx), curFunc);
    }
    else {
      if (g_invarRegs[modName].find(dest) == g_invarRegs[modName].end()) {
        if (is_top_module(curMod))
          return get_arg(timed_name(dest, timeIdx), curFunc);
        else {
          if (curDynData->isFunctionedSubMod)
            return get_arg(timed_name(dest, timeIdx), curFunc);
          else {
            std::string prefix = insContextStk.get_hier_name(false);
            dest = prefix + "." + dest;
            return get_arg(timed_name(dest, timeIdx), curFunc);
          }
        }
      }
      else {
        // if is invariant register, then return its norm value, which is
        // stored in g_rstVal
        uint32_t width = insContextStk.get_var_slice_width_simp(dest);    
        std::string rstVal = get_rst_value(dest, timeIdx, width);
        return var_expr(rstVal, timeIdx, c, b, false, width);
      }
    }
  }
}


llvm::Value* 
UpdateFunctionGen::add_ssa_constraint(astNode* const node, uint32_t timeIdx, context &c,
                                std::shared_ptr<llvm::IRBuilder<>> &b, const uint32_t bound) {
  toCoutVerb("Add ssa constraint for: " + node->dest+" ------  time: "+toStr(timeIdx));
  std::string var = node->dest;

  switch( node->type ) {
    case TWO_OP:
      return two_op_constraint(node, timeIdx, c, b, bound);
      break;
    case ONE_OP:
      return one_op_constraint(node, timeIdx, c, b, bound);
      break;
    case REDUCE1:
      return reduce_one_op_constraint(node, timeIdx, c, b, bound);
      break;
    case SEL:
      return sel_op_constraint(node, timeIdx, c, b, bound);
      break;
    case SRC_CONCAT:
      return src_concat_op_constraint(node, timeIdx, c, b, bound);
      break;
    case ITE:
      return ite_op_constraint(node, timeIdx, c, b, bound);
      break;
    default:
      toCout("Error no ssa_constraint found for: "+var);
      abort();
      break;
  }
}


//void 
//UpdateFunctionGen::add_child_constraint(astNode* const parentNode, uint32_t timeIdx, context &c,
//                          std::shared_ptr<llvm::IRBuilder<>> &b, const uint32_t bound) {
//  abort();
//  for( astNode* node: parentNode->childVec ) {
//    add_constraint(node, timeIdx, c, b, bound);
//  }
//}


// dest may be: input, AS or num
//void add_clean_constraint(astNode* const node, uint32_t timeIdx, context &c, 
//                          solver &s, goal &g, const uint32_t bound, bool isSolve) {
//  //toCoutVerb("Add clean constraint for: " + node->dest);
//  std::string dest = node->dest;
//  // add clean taint
//  expr destExpr_t(c);
//  if(isSolve) {
//    destExpr_t = var_expr(dest, timeIdx, c, true);
//    s.add( destExpr_t == 0 );
//    if(g_print_solver) {
//      toCout("Add-Solver: "+get_name(destExpr_t)+" == 0");
//    }
//  }
//  // add val expr
//  //if( is_number(dest) ) {
//  //  std::smatch m;
//  //  std::regex_match(dest, m, pNum);
//  //  std::string timedInt = timed_name(dest, timeIdx);
//  //  //if(INT_EXPR_SET.find(timedInt) != INT_EXPR_SET.end() )
//  //  //  return;
//  //  //else
//  //  //  INT_EXPR_SET.insert(timedInt);
//  //  if(isSolve) s.add( destExpr == hdb2int(node->dest) );
//  //  else        g.add( destExpr_g = bv_val(dest, c) );
//  //}
//}


void UpdateFunctionGen::push_clean_queue(astNode* node, uint32_t timeIdx) {
  if(CLEAN_QUEUE.find(node) != CLEAN_QUEUE.end())
    return;
  CLEAN_QUEUE.emplace(node, timeIdx);
}


//void add_all_clean_constraints(context &c, solver &s, goal &g, 
//                               const uint32_t bound, bool isSolve) {
//  toCout("-------- Begin Add all clean ----------");
//  for(auto it = CLEAN_QUEUE.begin(); it != CLEAN_QUEUE.end(); it++) {
//    add_clean_constraint(it->first, it->second, c, s, g, bound, isSolve);
//  }
//}


//void add_dirty_constraint(astNode* const node, uint32_t timeIdx, 
//                          context &c, solver &s, const uint32_t bound) {
//  toCoutVerb("Add dirty constraint for: "+node->dest+" ------  time: "+toStr(timeIdx));  
//  std::string destAndSlice = node->dest;
//  if(destAndSlice.compare("wack") == 0) {
//    //toCoutVerb("Found it!");
//  }
//  std::string dest, destSlice;
//  split_slice(destAndSlice, dest, destSlice);
//  uint32_t destWidth = get_var_slice_width_simp(destAndSlice);
//  expr destExpr_t = var_expr(destAndSlice, timeIdx, c, true);  
//  expr destExpr = var_expr(destAndSlice, timeIdx, c, false);
//  //assert(g_rstVal.find(dest) != g_rstVal.end());
//  if( g_rstVal.find(dest) != g_rstVal.end() && !is_root(dest) ) {
//    std::string rstVal = g_rstVal[dest];
//    expr rstValExpr = var_expr(rstVal, timeIdx, c, false, destWidth);
//    uint32_t allOne = std::pow(2, destWidth)-1;
//    s.add( destExpr_t == ite( destExpr == rstValExpr, 
//           c.bv_val(0, destWidth), 
//           c.bv_val(uint32_t(allOne), destWidth) ) );
//    if(g_print_solver) {
//      toCout("Add-Solver: "+get_name(destExpr_t)
//             +" == "+toStr(uint32_t(std::pow(2, destWidth)-1)));
//    }
//  }
//  else {
//    uint32_t allOne = std::pow(2, destWidth)-1;
//    s.add( destExpr_t == c.bv_val(uint32_t(allOne), destWidth) );
//    if(g_print_solver) {
//      toCout("Add-Solver: "+get_name(destExpr_t)
//             +" == "+toStr(uint32_t(std::pow(2, destWidth)-1)));
//    }
//  }
//
//  //s.add( destExpr == 0 );
//};


void UpdateFunctionGen::push_dirty_queue(astNode* node, uint32_t timeIdx) {
  if(DIRTY_QUEUE.find(node) != DIRTY_QUEUE.end())
    return;
  DIRTY_QUEUE.emplace(node, timeIdx);
  toCoutVerb("push into dirty queue: "+node->dest);
}


//void add_all_dirty_constraints(context &c, solver &s, const uint32_t bound) {
//  toCout("-------- Begin Add all dirty ----------");  
//  for(auto pair: DIRTY_QUEUE) {
//    add_dirty_constraint(pair.first, pair.second, c, s, bound);
//  }
//}


// value at bound+1 should be from instrInfo
// values in other cycles should all be NOP instructions
//void add_input_values(context &c, solver &s, const uint32_t bound) {
//  toCout("-------- Begin add input vals ----------");
//  uint32_t encodingSize = g_currInstrInfo.instrEncoding.begin()->second.size();
//  for(auto pair: g_currInstrInfo.instrEncoding) {
//    for(uint32_t i = 0; i < encodingSize; i++) {
//      expr singleInput = var_expr(pair.first, bound+1-i, c, false);
//      uint32_t width = get_var_slice_width_simp(pair.first);
//      expr localVal(c);
//      if(pair.second[i] == "x")
//        // FIXME
//        localVal = var_expr(toStr(width)+"'d0", bound+1-i, c, false, width);
//      else
//        localVal = var_expr(pair.second[i], bound+1-i, c, false, width);
//      s.add( singleInput == localVal );
//      toCoutVerb("Assign "+pair.second[i]+" to "+timed_name(pair.first, bound+1-i));
//      // input in later cycles should be filled with NOP
//    }
//  }
//}


//void add_nop(context &c, solver &s, const uint32_t bound) {
//  std::vector<uint32_t> bVec{0};
//  for(uint32_t b = 1; b <= bound; b++)
//  //for(uint32_t b: bVec)
//    for(auto it = g_nopInstr.begin(); it != g_nopInstr.end(); it++) {
//      expr singleInput = var_expr(it->first, b, c, false);     
//      expr singleInput_t = var_expr(it->first, b, c, true);     
//      uint32_t width = get_var_slice_width_simp(it->first);   
//      if(!is_number(it->second)){
//        toCout("!!!!!!!!!!!!!!!  Warning: non-number value found for NOP instruction!");
//      }
//      expr localVal = var_expr(it->second, b, c, false, width);
//      s.add( singleInput == localVal );
//      // FIXME: is this correct or necessary?
//      expr localZero = var_expr("1'd0", b, c, false, width);
//      s.add( singleInput_t == localZero );
//    }
//}


void UpdateFunctionGen::save_regs_for_expand(std::set<std::string> &varToExpand) {
  varToExpand.clear();
  for(auto pair: DIRTY_QUEUE) {
    varToExpand.insert(pair.first->dest);
  }
  for(auto pair: CLEAN_QUEUE) {
    varToExpand.insert(pair.first->dest);
  }
}


bool UpdateFunctionGen::is_in_clean_queue(std::string var) {
  if(var.back() == 'T')
    return false;
  std::string cleanVar;
  uint32_t len;
  len = var.length();
  cleanVar = var.substr(0, len - 5);
  for(auto it = CLEAN_QUEUE.begin(); it != CLEAN_QUEUE.end(); it++) {
    if((it->first->dest).compare(cleanVar) == 0)
      return true;
  }
  return false;
}


bool UpdateFunctionGen::is_in_dirty_queue(std::string var) {
  if(var.back() == 'T')
    return false;
  std::string cleanVar;
  uint32_t len;
  for(auto pair : DIRTY_QUEUE) {
    len = var.length();
    cleanVar = var.substr(0, len - 5);
    if((pair.first->dest).compare(cleanVar) == 0)
      return true;
  }
  return false;
}


void print_time() {
  time_t my_time = time(NULL); 
  
  // ctime() used to give the present time 
  printf("%s", ctime(&my_time)); 
}


void clean_module_inputs() {
  for(auto pair : g_moduleInfoMap) {
    auto curMod = pair.second;
    std::string modName = pair.first;
    //std::set<std::string> toErase;
    for(auto it = curMod->moduleInputs.begin(); 
        it != curMod->moduleInputs.end(); it++) {
      if(*it == curMod->clk ) {
        curMod->moduleInputs.erase(it);
        toCoutVerb("Erase "+*it+" from "+modName);
        return;
      }
    }
  }
}


void print_mem_info(std::vector<std::string> &mems) {
  std::ofstream output("./mem_info.txt");
  for(auto it = mems.begin(); it != mems.end(); it++) {
    std::string memName = *it;
    output << memName  << std::endl;
  }
}



std::string DestInfo::get_dest_name() {
  if(!isVector) {
    return destAndSlice;
  }
  else {
    return destVec.front()+"_Arr";
  }
}



// if is vector, return a pointer of the array-element type
// In release 15 of LLVM, this will become an opaque pointer
llvm::Type* DestInfo::get_ret_type(std::shared_ptr<llvm::LLVMContext> TheContext) {
  if(!isVector) {
    return llvm::IntegerType::get(*TheContext, 
                                  destWidth);
  }
  else if(isVector && !isMemVec){
    // if is reg vector, return a pointer type
    // first, check if every reg is of the same size
    uint32_t size = get_var_slice_width_cmplx(destVec.front());
    for(auto dest: destVec) {
      uint32_t elmtSize = get_var_slice_width_cmplx(dest);
      assert(size == elmtSize);
    }
    llvm::Type* I = llvm::IntegerType::get(*TheContext, size);    
    llvm::PointerType* pointerTy = llvm::PointerType::getUnqual(I);
    return pointerTy;
  }
  // TODO: implement the else case: the destVar is either single memory
  // or an array of memory
  else { // isVector && isMemVec
    return nullptr;
  }
}


// if is vector, return the array-element type
// Otherwise return null;
llvm::Type* DestInfo::get_ret_element_type(std::shared_ptr<llvm::LLVMContext> TheContext) {
  if(isVector && !isMemVec){
    // if is reg vector, return an array element type 
    // first, check if every reg is of the same size
    uint32_t size = get_var_slice_width_cmplx(destVec.front());
    for(auto dest: destVec) {
      uint32_t elmtSize = get_var_slice_width_cmplx(dest);
      assert(size == elmtSize);
    }
    return llvm::IntegerType::get(*TheContext, size);    
  }
  // TODO: implement the else case: the destVar is either single memory
  // or an array of memory
  else { 
    return nullptr;
  }
}


llvm::Type* DestInfo::get_arr_type(std::shared_ptr<llvm::LLVMContext> TheContext) {
  assert(isVector);
  // if is reg vector, return an array type pointer
  // first, check if every reg is of the same size
  uint32_t size = get_var_slice_width_cmplx(destVec.front());
  for(auto dest: destVec) {
    uint32_t elmtSize = get_var_slice_width_cmplx(dest);
    assert(size = elmtSize);
  }
  llvm::Type* I = llvm::IntegerType::get(*TheContext, size);
  llvm::ArrayType* arrayType = llvm::ArrayType::get(I, destVec.size());
  return arrayType;
}


std::vector<std::string> DestInfo::get_no_slice_name() {
  if(!isVector) {
    std::string dest, destSlice;
    split_slice(destAndSlice, dest, destSlice);
    return std::vector<std::string>{dest};
  }
  else {
    std::string var, varSlice;
    std::vector<std::string> retVec;
    for(std::string varAndSlice: destVec) {
      std::string pureVar = get_var_name(varAndSlice);   
      split_slice(pureVar, var, varSlice);
      retVec.push_back(var);
    }
    return retVec;
  }
}


std::string DestInfo::get_mod_name() {
  if(modName.empty()) {
    toCout("Warning: modName is not set");
    return g_topModule;
  }
  return modName;
}


std::string DestInfo::get_ins_name() {
  if(insName.empty()) {
    toCout("Warning: insName is not set");
    return "";
  }
  return insName;
}


std::string DestInfo::get_instr_name() {
  if(instrName.empty()) {
    toCout("Error: instrName is not set");
    abort();
    //return "";
  }
  return instrName;
}


void DestInfo::set_dest_and_slice(std::string var, uint32_t width) {
  destAndSlice = var;
  destWidth = width;
}


void DestInfo::set_module_name(std::string var) {
  modName = var;
}


void DestInfo::set_instr_name(std::string var) {
  instrName = var;
}


void DestInfo::set_instance_name(std::string var) {
  insName = var;
}


void DestInfo::set_dest_vec(const std::vector<std::string> &vec) {
  destVec = vec;
}


void UpdateFunctionGen::clean_all_mod_dynamic_info() {
  for(auto pair : g_moduleInfoMap) {
    auto dynData = get_dyn_data(pair.second);
    dynData->clean_ir_data();
  }
}


void ModuleDynInfo_t::clean_ir_data() {
  existedExpr.clear();
  minInOutDelay.clear();
  out2FuncMp.clear();
  out2InDelayMp.clear();
}


std::shared_ptr<ModuleDynInfo_t>
UpdateFunctionGen::get_dyn_data(std::shared_ptr<ModuleInfo_t> curMod) {
  if(dynDataMp.find(curMod->name) == dynDataMp.end()) {
    auto newDynData = std::make_shared<ModuleDynInfo_t>();
    dynDataMp.emplace(curMod->name, newDynData);
  }
  return dynDataMp[curMod->name];  
}


void 
UpdateFunctionGen::initialize_min_delay(std::shared_ptr<ModuleInfo_t> &modInfo, 
                                        std::string outPort) {
  auto dynData = get_dyn_data(modInfo);
  if(dynData->minInOutDelay.find(outPort) == dynData->minInOutDelay.end()) {
    std::map<std::string, uint32_t> toInputDelay;
    for(std::string input : modInfo->moduleInputs) {
      toInputDelay.emplace(input, DELAY_MAX);
    }
    dynData->minInOutDelay.emplace(outPort, toInputDelay);
  }
}



llvm::Value* 
UpdateFunctionGen::extract_func(llvm::Value* in, uint32_t high, uint32_t low,
                      std::shared_ptr<llvm::LLVMContext> &c, 
                      std::shared_ptr<llvm::IRBuilder<>> &b, uint32_t timeIdx, 
                      const llvm::Twine &name, bool noinline) {
  
  uint32_t len = high-low+1;  
  if(llvm::isa<llvm::ConstantInt>(in)) {
    auto lshrVal = b->CreateLShr( in, low, timed_name("CONST_LSHR", timeIdx));
    llvm::Value* ret = b->CreateTrunc(lshrVal, llvmWidth(len, c), 
                                      timed_name("CONST_TRUNC", timeIdx) );
    return ret;
  }

  std::string destName = in->getName().str();
  std::string dest, destSlice;
  auto curMod = insContextStk.get_curMod();
  auto curDynData = get_dyn_data(curMod);
  std::string prefix = "";
  if(!curDynData->isFunctionedSubMod) prefix = insContextStk.get_hier_name(false);

  toCoutVerb("extract for: "+destName);

  std::string extValName;
  std::string lshrName;
  if(g_use_simple_func_name)
    extValName = "ext_"+toStr(ext_cnt++);
  else if(false){
    uint32_t extIdx;
    std::string origExtName = destName+"_["+toStr(high)+":"+toStr(low)+"]"; 
    if(extNameIdxMap.find(origExtName) == extNameIdxMap.end()) {
      extIdx = ext_cnt;
      extNameIdxMap.emplace(origExtName, ext_cnt++);
    }
    else extIdx = extNameIdxMap[origExtName];
    extValName = "ext_"+toStr(extIdx);
  }
  else {
    extValName = destName+"_["+toStr(high)+":"+toStr(low)+"]";
  }
  
  bool timeIdxExist = (destName.find("___#") != std::string::npos);
  if(!timeIdxExist) extValName = timed_name(extValName, timeIdx);
  lshrName = prefix+destName+"_LSHR_"+toStr(low)+ENDSIGN;

  const std::string curTgt = insContextStk.get_target();
  if(curDynData->existedExpr[curTgt].find(extValName) 
      != curDynData->existedExpr[curTgt].end() ) {
    return curDynData->existedExpr[curTgt][extValName];
  }

  if(!g_use_concat_extract_func) {
    llvm::Value* lshrVal;
    if(curDynData->existedExpr[curTgt].find(lshrName) 
        != curDynData->existedExpr[curTgt].end() ) {
      lshrVal = curDynData->existedExpr[curTgt][lshrName];
    }
    else {
      lshrVal = b->CreateLShr(
        in, low, 
        llvm::Twine(lshrName)
      );
      curDynData->existedExpr[curTgt].emplace(lshrName, lshrVal);
    }

    llvm::Value* ret = b->CreateTrunc(lshrVal, llvmWidth(len, c), llvm::Twine(extValName));
    curDynData->existedExpr[curTgt].emplace(extValName, ret);
    return ret;
  }

  //llvm::Type* inputTy = in->getType();
  //uint32_t inputWidth = llvm::dyn_cast<llvm::IntegerType>(inputTy)->getBitWidth();
  //llvm::Function *func;
  //llvm::FunctionType *FT;  
  //if(!destName.empty()) {
  //  split_slice(destName, dest, destSlice);
  //  noinline = false;
  //}
  //else if(is_read_asv(dest, insContextStk.get_curMod()) 
  //        || (is_top_module(curMod) && is_input(dest, insContextStk.get_curMod())))
  //  noinline = true;
  //else noinline = false;
  //std::string app = "";
  //if(!noinline) app = "_in";
  //std::string funcName = "ext_"+toStr(inputWidth)+"_"+toStr(high)+"_"+toStr(low)+app;
  //if(extractFunc.find(funcName) != extractFunc.end()) {
  //  func = extractFunc[funcName];
  //  FT = func->getFunctionType();
  //}
  //else {
  //  abort(); // not supported anymore
  //  auto retTy = llvm::IntegerType::get(*c, len);
  //  std::vector<llvm::Type *> argTy;  
  //  argTy.push_back(llvm::IntegerType::get(*c, inputWidth));
  //  FT = llvm::FunctionType::get(retTy, argTy, false);
  //  func = llvm::Function::Create(FT, llvm::Function::InternalLinkage, 
  //                                      funcName, TheModule.get());
  //  if(noinline) func->addFnAttr(llvm::Attribute::NoInline);
  //  func->args().begin()->setName("input");
  //  llvm::Value* inArg = value(func->args().begin()  );

  //  llvm::BasicBlock *localBB 
  //    = llvm::BasicBlock::Create(*c, "entry", func);

  //  std::unique_ptr<llvm::IRBuilder<>> build;
  //  build = std::make_unique<llvm::IRBuilder<>>(*c);
  //  build->SetInsertPoint(localBB);
  //  auto s1 = build->CreateLShr(inArg, low, llvm::Twine("lshr"));
  //  llvm::Value* ret = build->CreateTrunc(s1, llvmWidth(len, c), llvm::Twine("trunc"));
  //  build->CreateRet(ret);
  //  extractFunc.emplace(funcName, func);
  //}
  //
  //std::vector<llvm::Value*> args;
  //args.push_back(in);
  //return b->CreateCall(FT, func, args, 
  //                     llvm::Twine(timed_name(extValName, timeIdx)));
  return nullptr;  // Not reached?
}


//llvm::Value* 
//UpdateFunctionGen::extract(llvm::Value* in, uint32_t high, uint32_t low,
//                      std::shared_ptr<llvm::LLVMContext> &c, 
//                      std::shared_ptr<llvm::IRBuilder<>> &b, 
//                      const llvm::Twine &name) {
//  abort(); // not supported anymore
//  uint32_t inWidth = llvm::dyn_cast<llvm::IntegerType>(in->getType())->getBitWidth();
//  if(inWidth < high+1) {
//    toCout("Error: input value width for extract is less than high index");
//    toCout("wdith: "+toStr(inWidth)+", high: "+toStr(high));
//    std::string tmpName = in->getName().str();
//    toCout("Input value name: "+tmpName);
//    abort();
//  }
//  uint32_t len = high - low + 1;
//  auto s1 = b->CreateLShr(in, low);
//  if(name.isTriviallyEmpty()) {
//    llvm::Value* ret = b->CreateTrunc(s1, llvmWidth(len, c), llvm::Twine(""));
//    return ret;
//  }
//  else {
//    llvm::Value* ret = b->CreateTrunc(s1, llvmWidth(len, c), 
//                          llvm::Twine(name.str()+" ["+toStr(high)+":"+toStr(low)+"]"));
//    ret->setName(name.str()+" ["+toStr(high)+":"+toStr(low)+"]");
//    return ret;
//  }
//}
//
//
//llvm::Value* 
//UpdateFunctionGen::extract(llvm::Value* in, uint32_t high, uint32_t low, 
//                      std::shared_ptr<llvm::LLVMContext> &c, 
//                      std::shared_ptr<llvm::IRBuilder<>> &b, 
//                      const std::string &name) {
//
//  return extract(in, high, low, c, b, llvm::Twine(name));
//}


llvm::Value* 
UpdateFunctionGen::concat_func(llvm::Value* val1, llvm::Value* val2, 
                         std::shared_ptr<llvm::LLVMContext> &c,
                         std::shared_ptr<llvm::IRBuilder<>> &b,
                         uint32_t timeIdx,
                         bool noinline) {
  // TODO: use memorization for values generated here:
  // note that, names for constant op should be generated in a different way!

  llvm::Type* val1Ty = val1->getType();
  llvm::Type* val2Ty = val2->getType();
  uint32_t val1Width = llvm::dyn_cast<llvm::IntegerType>(val1Ty)->getBitWidth();
  uint32_t val2Width = llvm::dyn_cast<llvm::IntegerType>(val2Ty)->getBitWidth();
  uint32_t len = val1Width + val2Width;
  auto newIntTy = llvm::IntegerType::get(*c, len);
  std::string name1 = val1->getName().str();
  std::string name2 = val2->getName().str();
  auto pair1 = parse_name_idx(name1);
  auto pair2 = parse_name_idx(name2);
  //uint32_t timeIdx1 = pair1.second.first;
  //uint32_t timeIdx2 = pair2.second.first;
  //uint32_t timeIdx = (timeIdx1 > timeIdx2) ? timeIdx2 : timeIdx1;
  std::string pureName1 = pair1.first + pair1.second.second;
  std::string pureName2 = pair2.first + pair2.second.second;
  toCoutVerb("concat with "+name1+" and "+name2+", total width: "+toStr(val1Width+val2Width));  

  std::string cctValName;
  if(g_use_simple_func_name)
    cctValName = "cct_"+toStr(cct_cnt++);
  else if(false) {
    uint32_t cctIdx;
    std::string origCctName = "cct_"+pureName1+"_"+pureName2; 
    if(cctNameIdxMap.find(origCctName) == cctNameIdxMap.end()) {
      cctIdx = cct_cnt;
      cctNameIdxMap.emplace(origCctName, cct_cnt++);
    }
    else cctIdx = cctNameIdxMap[origCctName];
    cctValName = "cct_"+toStr(cctIdx);
  }
  else {
    cctValName = "cct_"+pureName1+"_"+pureName2;
  }

  if(!g_use_concat_extract_func) {
    llvm::Value* longVal1 = b->CreateZExtOrBitCast(val1, newIntTy, 
                                                   llvm::Twine(name1+"_zext"));
    llvm::Value* ret = b->CreateAdd(b->CreateShl(longVal1, val2Width, 
                                      llvm::Twine(name1+"_zext_SHL_"+toStr(val2Width)+ENDSIGN)), 
                                    zext(val2, len, c, b), 
                                    llvm::Twine(timed_name(cctValName, timeIdx)));
    return ret;
  }

  //std::string n1, n1Slice;
  //std::string n2, n2Slice;
  //auto curMod = insContextStk.get_curMod();
  //if(!name1.empty()) split_slice(name1, n1, n1Slice);
  //if(!name2.empty()) split_slice(name2, n2, n2Slice);
  //if(name1.empty() || name2.empty()) noinline = false;
  //else if( (is_read_asv(n1, curMod) || (is_top_module(curMod) && is_input(n1, curMod)))
  //      && (is_read_asv(n2, curMod) || (is_top_module(curMod) && is_input(n2, curMod))) ) 
  //  noinline = true;
  //else noinline = false;

  //std::string app = "";
  //if(!noinline) app = "_in";
  //std::string funcName = "cct_"+toStr(val1Width)+"_"+toStr(val2Width)+app;
  //llvm::Function *func;
  //llvm::FunctionType *FT;  
  //if(concatFunc.find(funcName) != concatFunc.end()) {
  //  func = concatFunc[funcName];
  //  FT = func->getFunctionType();    
  //}
  //else {
  //  auto retTy = llvm::IntegerType::get(*c, len);
  //  std::vector<llvm::Type*> argTy;
  //  llvm::Type* ty1 = llvm::IntegerType::get(*c, val1Width);
  //  llvm::Type* ty2 = llvm::IntegerType::get(*c, val2Width);
  //  assert(ty1 == val1->getType());
  //  assert(ty2 == val2->getType());
  //  argTy.push_back(ty1);
  //  argTy.push_back(ty2);
  //  FT = llvm::FunctionType::get(retTy, argTy, false);
  //  func = llvm::Function::Create(FT, llvm::Function::InternalLinkage, 
  //                                      funcName, TheModule.get());
  //  //func->addFnAttr(llvm::Attribute::NoInline);
  //  func->args().begin()->setName("val1");
  //  (func->args().begin()+1)->setName("val2");

  //  llvm::Value* val1Arg = value(func->args().begin()  );
  //  llvm::Value* val2Arg = value(func->args().begin()+1); 
  //  assert(val1Arg->getType() == val1->getType());
  //  assert(val2Arg->getType() == val2->getType());

  //  llvm::BasicBlock *localBB 
  //    = llvm::BasicBlock::Create(*c, "entry", func);

  //  std::shared_ptr<llvm::IRBuilder<>> build;
  //  build = std::make_shared<llvm::IRBuilder<>>(*c);
  //  build->SetInsertPoint(localBB);

  //  llvm::Value* longVal1 = build->CreateZExtOrBitCast(val1Arg, newIntTy);

  //  llvm::Value* ret = build->CreateAdd(build->CreateShl(longVal1, val2Width, llvm::Twine(name1+"")), 
  //                                      zext(val2Arg, len, c, build));
  //  build->CreateRet(ret);
  //  concatFunc.emplace(funcName, func);
  //}

  //std::vector<llvm::Value*> args;
  //args.push_back(val1);
  //args.push_back(val2);
  //return b->CreateCall(FT, func, args, llvm::Twine(timed_name(cctValName, timeIdx)));
  return nullptr;  // Not reached?
}


llvm::Value* 
UpdateFunctionGen::concat_value(llvm::Value* val1, llvm::Value* val2, 
                          std::shared_ptr<llvm::LLVMContext> &c,
                          std::shared_ptr<llvm::IRBuilder<>> &b) {
  uint32_t val1Width = llvm::dyn_cast<llvm::IntegerType>(val1->getType())->getBitWidth();
  uint32_t val2Width = llvm::dyn_cast<llvm::IntegerType>(val2->getType())->getBitWidth();
  std::string name1 = val1->getName().str();
  std::string name2 = val2->getName().str();
  toCoutVerb("concat "+name1+", len: "+toStr(val1Width));
  toCoutVerb("and "+name2+", len: "+toStr(val2Width));

  uint32_t newLen = val1Width+val2Width;
  auto newIntTy = llvm::IntegerType::get(*c, newLen);
  llvm::Value* longVal1 = b->CreateZExtOrBitCast(val1, newIntTy);
  return b->CreateAdd(b->CreateShl(longVal1, val2Width, 
                                   llvm::Twine(name1+"_SHL_"+toStr(val2Width))), 
                      zext(val2, newLen, c, b));
}


llvm::Value* 
UpdateFunctionGen::long_bv_val(std::string formedBinVar, context &c,
                         std::shared_ptr<llvm::IRBuilder<>> &b ) {
  assert(is_number(formedBinVar));
  if(!is_formed_num(formedBinVar)) {
    toCout("Error: input to long_bv_val is not well-formed number: "+formedBinVar);
    abort();
  }
  uint32_t width = get_num_len(formedBinVar);
  if(width <= 32) 
    return llvm::ConstantInt::get(llvmWidth(width, c), hdb2int(formedBinVar), false);

  if(is_hex(formedBinVar)) formedBinVar = formedHex2bin(formedBinVar);
  formedBinVar = zero_extend_num(formedBinVar);
  std::string pureNum = get_pure_num(formedBinVar);

  llvm::Value* ret = llvm::ConstantInt::get(llvmWidth(32, c), bin2int(pureNum.substr(0, 32)), false);
  width -= 32;
  size_t pos = 32;  
  while(width > 32) {
    std::string subVar = pureNum.substr(pos, 32);
    pos += 32;
    width -= 32;
    llvm::Value* nextNum = llvm::ConstantInt::get(llvmWidth(32, c), bin2int(subVar), false);
    ret = concat_value(ret, nextNum, c, b);
  }

  // deal with the remaining bits
  std::string subVar = pureNum.substr(pos);
  llvm::Value* nextNum = llvmInt(bin2int(subVar), width, c);
  ret = concat_value(ret, nextNum, c, b);
  return ret;
}



llvm::Value* UpdateFunctionGen::get_arg(std::string regName) {
  auto func = insContextStk.get_func();
  return get_arg(regName, func);
}


llvm::Value* UpdateFunctionGen::get_arg(std::string regName, llvm::Function *func) {
  auto curMod = insContextStk.get_curMod();
  if(regName.find("hls_target_call_Loop_LB2D_buf_proc_buffer_0_value_V_ram_U.q0___#0") 
       != std::string::npos)
    toCout("Find the arg!");
  for(auto it = func->arg_begin(); it != func->arg_end(); it++) {
    std::string funcName = llvm::dyn_cast<llvm::Value>(func)->getName().str();
    std::string argName = it->getName().str();
    //toCout("func name: "+funcName);
    //toCout("arg name: "+argName);
    if(argName.find("ata_fifo.r0___#25") != std::string::npos)
      toCoutVerb("Find it!!");
    if(it->getName().str() == regName) return it;
  }
  toCout("Error: function input is not found: "+regName
         +", modName: "+curMod->name+", func: "+func->getName().str());
  abort();
}


llvm::Value* UpdateFunctionGen::zext(llvm::Value* v1, uint32_t width,
                                     std::shared_ptr<llvm::LLVMContext> &c,
                                     std::shared_ptr<llvm::IRBuilder<>> &b) {

  if(llvm::isa<llvm::Constant>(v1)) return b->CreateZExtOrTrunc(v1, llvmWidth(width, c));

  std::string name = v1->getName().str()+"_ZEXT_TO_"+toStr(width);

  const std::string curTgt = insContextStk.get_target();
  const auto curMod = insContextStk.get_curMod();  
  auto curDynData = get_dyn_data(curMod);
  
  if(curDynData->existedExpr[curTgt].find(name) 
      != curDynData->existedExpr[curTgt].end() ) {
    return curDynData->existedExpr[curTgt][name];
  }

  llvm::Value* val = b->CreateZExtOrTrunc(v1, llvmWidth(width, c), llvm::Twine(name));
  curDynData->existedExpr[curTgt].emplace(name, val);
  return val;
}


llvm::Value* UpdateFunctionGen::sext(llvm::Value* v1, uint32_t width,
                                     std::shared_ptr<llvm::LLVMContext> &c,
                                     std::shared_ptr<llvm::IRBuilder<>> &b) {

  if(llvm::isa<llvm::Constant>(v1)) return b->CreateSExtOrTrunc(v1, llvmWidth(width, c));

  std::string name = v1->getName().str()+"_SEXT_TO_"+toStr(width);

  const std::string curTgt = insContextStk.get_target();
  const auto curMod = insContextStk.get_curMod();  
  auto curDynData = get_dyn_data(curMod);

  if(curDynData->existedExpr[curTgt].find(name) 
      != curDynData->existedExpr[curTgt].end() ) {
    return curDynData->existedExpr[curTgt][name];
  }

  llvm::Value* val = b->CreateSExtOrTrunc(v1, llvmWidth(width, c), llvm::Twine(name));
  curDynData->existedExpr[curTgt].emplace(name, val);
  return val;
}




void print_context_info(Context_t& insCntxt) {
  g_pushNum++;
  if(g_pushNum == 21)
    toCoutVerb("Find it!");
  std::string modName = insCntxt.ModInfo->name;
  std::string funcName;
  if(insCntxt.Func != nullptr)
    funcName = insCntxt.Func->getName().str();
  else
    funcName = "Not set yet!";

  toCout("push mod("+toStr(g_pushNum)+"): "+modName+", func: "+funcName);
  if(modName == "hls_target_Loop_1_proc" && funcName == "top_function")
    toCoutVerb("Find it!");
}

} // end of namespace funcExtract
