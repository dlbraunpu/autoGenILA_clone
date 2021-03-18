#include "ast.h"
#include "parse_fill.h"
#include "helper.h"
#include "global_data_struct.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>



using namespace syntaxPatterns;
using namespace taintGen;

namespace funcExtract {

#define SV std::vector<std::string>
#define PV std::vector<astNode*>
#define toStr(a) std::to_string(a)


std::stack<std::shared_ptr<std::map<std::string, astNode*>>> g_visitedNodeStk;
std::shared_ptr<std::map<std::string, astNode*>> g_visitedNode;
astNode* s_node;
std::shared_ptr<ModuleInfo_t> g_topMod;
std::stack<std::pair<std::string, std::shared_ptr<ModuleInfo_t>>> g_instancePairStk;
/// key is submodule output: module_$output
/// vector is the inputs. 
/// This map is filled after the function's ast is constructed
std::unordered_map<std::string, std::vector<std::string>> g_funcInputMp;


// "line" is verilog line generated by Yosys
void build_ast_tree() {
  toCout("### Begin build_ast_tree");
  g_curMod = g_moduleInfoMap[g_topModule];
  g_instancePairStk.push(std::make_pair(g_topModule, g_curMod));
  g_topMod = g_curMod;
  g_visitedNode = std::make_unique<std::map<std::string, astNode*>>();
  g_visitedNodeStk.push(g_visitedNode);
  for(std::string reg: g_curMod->moduleAs) {
    if(g_curMod->reg2Slices.find(reg) == g_curMod->reg2Slices.end()) {
      build_tree_for_single_as(reg);
    }
    else { // if different slices are assigned differently
      for(std::string regAndSlice: g_curMod->reg2Slices[reg]) {
        build_tree_for_single_as(regAndSlice);
      }
    }
  }
}


void build_tree_for_single_as(std::string regAndSlice) {
  //g_visitedNode.clear();
  toCoutVerb("### Begin build: "+regAndSlice);
  uint32_t regWidth = get_var_slice_width_simp(regAndSlice);
  astNode* root;
  if(g_visitedNode->find(regAndSlice) == g_visitedNode->end())
    root = new astNode;
  else
    root = (*g_visitedNode)[regAndSlice];
  add_node(regAndSlice, 0, root, true);
}


// Attention: If different slices of var are assigned in different expressions, they should have different nodes
// Summary: for each varAndSlice, add a node for it. If varAndSlice is separately assigned(not part of assignment to var), then this function directly add a node for varAndSlice. Otherwise, add a node to var.
void add_node(std::string varAndSlice, 
              uint32_t timeIdx, 
              astNode* const node, 
              bool varIsDest) {
  //toCout("Add node for: "+varAndSlice);
  if(varAndSlice.find("aes_reg_key0_i.reg_out") != std::string::npos) {
    toCout("Found it!");
    s_node = node;
  }
  if(g_visitedNode->find(varAndSlice) != g_visitedNode->end() && !varIsDest)
    return;

  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  std::string varToAdd;
  if(g_curMod->reg2Slices.find(var) == g_curMod->reg2Slices.end()) {
    varToAdd = var;
  }
  else if(std::find(g_curMod->reg2Slices[var].begin(), 
                    g_curMod->reg2Slices[var].end(), 
                    varAndSlice) != g_curMod->reg2Slices[var].end()) {
    // the varAndSlice is separately assigned
    varToAdd = varAndSlice;
  }
  else { // var is the key to g_curMod->reg2Slices, but varAndSlice is not its value
    varToAdd = var;
  }

  g_visitedNode->emplace(varToAdd, node);
  g_curMod->varNode.emplace(varToAdd, node);
  if( g_curMod->reg2Slices.find(varToAdd) != g_curMod->reg2Slices.end() ) {
    add_sliced_node(varToAdd, timeIdx, node);
  }
  else if ( is_input(varToAdd) ) {
    add_input_node(varToAdd, timeIdx, node);
  }
  else if ( isAs(varToAdd) && !varIsDest ) {
    abort();
    add_as_node(varToAdd, timeIdx, node);    
  }
  else if( is_reg(varToAdd) ) {
    add_nb_node(varToAdd, timeIdx, node);
  }
  else if( is_number(varToAdd) ) {
    add_num_node(varToAdd, timeIdx, node);
  }
  else if( is_case_dest(varToAdd) ) {
    add_case_node(varToAdd, timeIdx, node);
  }
  else if( is_func_output(varToAdd) ) {
    abort();
    add_func_node(varToAdd, timeIdx, node);
  }
  else if( is_submod_output(varToAdd) ) {
    add_submod_node(varToAdd, timeIdx, node);
  }
  else { // it is wire
    add_ssa_node(varToAdd, timeIdx, node);
  }
}


// varAndSlice is the child variable, when the func is called, varAndSlice is directly from RHS of the expression. So it may/may not have slice, may/not have direct assignment.
// parentNode is the node of its parent
void add_child_node(std::string varAndSlice, 
                    uint32_t timeIdx, 
                    astNode* const parentNode) {
  toCoutVerb("!! Add child "+varAndSlice+" to "+parentNode->dest);
  if(varAndSlice == "state_0") {
    toCout("Found state_0!");
  }
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  // if varAndSlice is dest of one line, treat it separately
  std::string childName;
  if(varSlice.empty()) 
    childName = var;
  else if(has_direct_assignment(varAndSlice))
    childName = varAndSlice;
  else
    childName = var;
  if(g_visitedNode->find(childName) == g_visitedNode->end()) {
    astNode* nextNode = new astNode;      
    parentNode->childVec.push_back(nextNode);
    add_node(childName, timeIdx, nextNode, false);
  }
  else {
    astNode* existedNode = (*g_visitedNode)[childName];
    parentNode->childVec.push_back(existedNode);
  }
}


// attention: the slices in g_curMod->reg2Slices may not be complete
// summary: for var whose slices are assigned differently, first add node for whole var,
//   and then add node to slices that are separately assigned
void add_sliced_node(std::string varAndSlice, 
                     uint32_t timeIdx, 
                     astNode* const node) {
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  assert(varSlice.empty());
  if(var.find("u0.word") != std::string::npos) {
    toCoutVerb("Found it!");
  }

  node->type = SRC_CONCAT;
  node->dest = var;
  node->op = "";
  //node->srcVec = g_curMod->reg2Slices[var];
  node->destTime = timeIdx;
  node->done = false;

  auto srcVec = g_curMod->reg2Slices[var];
  uint32_t srcHi = get_ltr_hi(srcVec.front());
  uint32_t srcLo = get_ltr_lo(srcVec.back());
  auto idxPairs = g_curMod->varWidth.get_idx_pair(var, "add_sliced_node for:"+var);
  uint32_t destHi = idxPairs.first;
  uint32_t destLo = idxPairs.second;

  if(destHi > srcHi)
    srcVec.insert(srcVec.begin(), toStr(destHi-srcHi)+"'h0");
  if(destLo < srcLo)
    srcVec.push_back(toStr(srcLo-destLo)+"'h0");
  node->srcVec = srcVec; 
    
  for(auto srcAndSlice: srcVec) {
    add_child_node(srcAndSlice, timeIdx, node);  
  }
}


// timeIdx is time for dest, not src in the expression
void add_nb_node(std::string regAndSlice, uint32_t timeIdx, astNode* const node) {
  if(regAndSlice.find("aes_reg_key0_i.reg_out") != std::string::npos) {
    toCout("Found it!");
  }
  toCoutVerb("Add nb node for :" + regAndSlice);
  g_visitedNode->emplace(regAndSlice, node);
  if(g_curMod->nbTable.find(regAndSlice) == g_curMod->nbTable.end()) {
    toCout("Error: not in g_curMod->nbTable: "+regAndSlice);
    abort();
  }
  if(regAndSlice == "state_0") {
    toCout("Found state_0!");
  }
  std::string destAssign = g_curMod->nbTable[regAndSlice];
  std::smatch m;
  if(std::regex_match(destAssign, m, pNonblock)) {
    std::string destNext = m.str(3);
    remove_two_end_space(destNext);
    uint32_t destNextWidth = get_var_slice_width_simp(destNext);
    uint32_t destWidth = get_var_slice_width_simp(regAndSlice);

    node->type = NONBLOCK;
    node->dest = regAndSlice;
    node->op = "<=";
    node->srcVec.push_back(destNext);
    node->destTime = timeIdx;
    node->done = false;

    add_child_node(destNext, timeIdx+1, node);
  }
  else if(std::regex_match(destAssign, m, pNonblockConcat)) {
  }
  else if(std::regex_match(destAssign, m, pNonblockIf)) {
  }
  else {
    toCout("Error in add_nb_constraint: "+destAssign);
  }
}


// The var may contain slice
// (1) If varAndSlice is in g_curMod->ssaTable, then just build that node
// (2) If not, build nodes for the whole variable
// (3) If no slice, build nodes for the whole variable
void add_ssa_node(std::string varAndSlice, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Add ssa node for :" + varAndSlice);
  g_visitedNode->emplace(varAndSlice, node);
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  std::string varAssign;
  if(g_curMod->ssaTable.find(varAndSlice) != g_curMod->ssaTable.end() )
    varAssign = g_curMod->ssaTable[varAndSlice];
  else if(g_curMod->ssaTable.find(var) != g_curMod->ssaTable.end())
    varAssign = g_curMod->ssaTable[var];
  else {
    toCout("Error: not in g_curMod->ssaTable:"+varAndSlice);
    abort();
  }

  uint32_t choice = parse_verilog_line(varAssign);
  switch( choice ) {
    case INPUT:
      add_input_node(varAssign, timeIdx, node);
      break;
    case TWO_OP:
      add_two_op_node(varAssign, timeIdx, node);
      break;
    case ONE_OP:
      add_one_op_node(varAssign, timeIdx, node);
      break;
    case REDUCE1:
      add_reduce_op_node(varAssign, timeIdx, node);
      break;
    case SEL:
      add_sel_op_node(varAssign, timeIdx, node);
      break;
    case SRC_CONCAT:
      add_src_concat_op_node(varAssign, timeIdx, node);
      break;
    case ITE:
      add_ite_op_node(varAssign, timeIdx, node);
      break;
    default:
      toCout("Error in add_ssa_node for: "+var);
      break;
  }
}


/// Treat differently for top module and sub module
void add_input_node(std::string input, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process input node: "+input);
  if(g_curMod == g_topMod) {
    node->type = INPUT;
    node->dest = input;
    node->op = "";
    node->destTime = timeIdx;
    node->done = true;
  }
  else {
    node->type = SUBMODIN;
    node->dest = input;
    std::pair<std::string, std::shared_ptr<ModuleInfo_t>> pair = g_instancePairStk.top();
    /// the input is in parent-module
    node->op = "";
    node->destTime = timeIdx;
    node->done = false;
  }
}


void add_num_node(std::string num, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process num node: "+num);  
  node->type = FE_NUM;
  node->dest = num;
  node->op = "";
  node->destTime = timeIdx;
  node->done = true;
}


void add_as_node(std::string as, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process AS node: "+as);  
  node->type = FE_AS;
  node->dest = as;
  node->op = "";
  node->destTime = timeIdx;
  node->done = true;
}


void add_two_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process Two op for :"+line);
  bool isReduceOp;
  std::string op;
  std::string destAndSlice;
  std::string op1AndSlice;
  std::string op2AndSlice;
  if (!check_two_op(line, op, destAndSlice, op1AndSlice, op2AndSlice, isReduceOp)) {
    toCout("Error: check_two_op failed for: "+line);
    abort();
  }

  if(op == ">>>") {
    toCout("Found it!");
  }

  //if(destAndSlice.compare("adr_check") == 0) {
  //  toCout("Found adr_check");
  //}
  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;

  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  split_slice(op2AndSlice, op2, op2Slice);
  remove_two_end_space(op1AndSlice);
  remove_two_end_space(op2AndSlice);
  uint32_t destAndSliceWidth = get_var_slice_width_simp(destAndSlice);
  uint32_t op1AndSliceWidth = get_var_slice_width_simp(op1AndSlice);
  uint32_t op2AndSliceWidth = get_var_slice_width_simp(op2AndSlice);

  node->type = TWO_OP;
  node->dest = destAndSlice;
  node->op = op;
  node->srcVec = SV{op1AndSlice, op2AndSlice};
  node->destTime = timeIdx;
  node->isReduceOp = isReduceOp;
  node->done = false;

  add_child_node(op1AndSlice, timeIdx, node);
  add_child_node(op2AndSlice, timeIdx, node);

  if(isReduceOp)
    assert(destAndSliceWidth == 1);

  assert(!isMem(op1));
  assert(!isMem(op2));
}


void add_one_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process One op for :"+line); 
  std::string op;
  std::string destAndSlice;
  std::string op1AndSlice;
  if (!check_one_op(line, op, destAndSlice, op1AndSlice)) {
    toCout("Error: check_one_op failed for: "+line);
    abort();
  }

  std::string dest, destSlice;
  std::string op1, op1Slice;

  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  remove_two_end_space(op1AndSlice);
  std::string destAndSliceTimed = destAndSlice + "#" + toStr(timeIdx);

  node->type = ONE_OP;
  node->dest = destAndSlice;
  node->op = op;
  node->srcVec = SV{op1AndSlice};
  node->destTime = timeIdx;
  node->done = false;

  add_child_node(op1AndSlice, timeIdx, node);  
}


void add_ite_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("process Ite line for: "+line);
  std::smatch m;
  if ( !std::regex_match(line, m, pIte) ) {
    toCout("Error: does not match ite");
    abort();
  }

  assert(!m.str(3).empty());
  assert(!m.str(4).empty());
  assert(!m.str(5).empty());

  std::string dest, destSlice;
  std::string cond, condSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;
  std::string blank = m.str(1);
  std::string destAndSlice = m.str(2);
  std::string condAndSlice = m.str(3);
  std::string op1AndSlice = m.str(4);
  std::string op2AndSlice = m.str(5);
  split_slice(destAndSlice, dest, destSlice);
  split_slice(condAndSlice, cond, condSlice);
  split_slice(op1AndSlice , op1, op1Slice);
  split_slice(op2AndSlice , op2, op2Slice);
  remove_two_end_space(op1AndSlice);
  remove_two_end_space(op2AndSlice);
  remove_two_end_space(condAndSlice);

  assert(!isMem(op1));    
  assert(!isMem(op2));

  uint32_t localWidthNum;
  std::string localWidth;
  localWidthNum = get_var_slice_width_simp(destAndSlice);

  localWidth = std::to_string(localWidthNum);

  node->type = ITE;
  node->dest = destAndSlice;
  node->op = "ite";
  node->srcVec = SV{condAndSlice, op1AndSlice, op2AndSlice};
  node->destTime = timeIdx;
  node->done = false;

  add_child_node(condAndSlice, timeIdx, node);  
  add_child_node(op1AndSlice,  timeIdx, node);  
  add_child_node(op2AndSlice,  timeIdx, node);  
}


void add_reduce_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process Reduce One op for :"+line); 
  std::string op;
  std::string destAndSlice;
  std::string op1AndSlice;
  if (!check_reduce_one_op(line, op, destAndSlice, op1AndSlice)) {
    toCout("Error: check_reduce_one_op failed for: "+line);
    abort();
  }

  std::string dest, destSlice;
  std::string op1, op1Slice;

  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  remove_two_end_space(op1AndSlice);

  node->type = REDUCE1;
  node->dest = destAndSlice;
  node->op = op;
  node->srcVec = SV{op1AndSlice};
  node->destTime = timeIdx;
  node->done = false;

  add_child_node(op1AndSlice, timeIdx, node);  

  return;
}


void add_sel_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process Sel op for :"+line);
  bool isReduceOp;
  std::string op;
  std::string destAndSlice;
  std::string op1AndSlice;
  std::string op2AndSlice;
  std::string integer;
  if (!check_sel_op(line, op, destAndSlice, op1AndSlice, op2AndSlice, integer)) {
    toCout("Error: check_sel_op failed for: "+line);
    abort();
  }

  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;

  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  split_slice(op2AndSlice, op2, op2Slice);
  uint32_t destAndSliceWidth = get_var_slice_width_simp(destAndSlice);
  remove_two_end_space(op1AndSlice);
  remove_two_end_space(op2AndSlice);

  node->type = SEL;
  node->dest = destAndSlice;
  node->op = op;
  node->srcVec = SV{op1AndSlice, op2AndSlice, integer};
  node->destTime = timeIdx;
  node->isReduceOp = false;
  node->done = false;

  add_child_node(op1AndSlice, timeIdx, node);
  add_child_node(op2AndSlice, timeIdx, node);

  assert(!isMem(op1));
  assert(!isMem(op2));

  return;
}


void add_src_concat_op_node(std::string line, uint32_t timeIdx, astNode* const node) {
  toCoutVerb("Process concat op for :"+line); 
  std::smatch m;
  if (!std::regex_match(line, m, pSrcConcat)) {
    toCout("Error: does not match srcConcat");
    abort();
  }

  std::string op;
  std::string destAndSlice = m.str(2);
  if(destAndSlice.compare("fangyuan23") == 0) {
    toCout("Find it!");
  }
  std::string srcList = m.str(3);

  std::string dest, destSlice;
  std::vector<std::string> srcVec;

  split_slice(destAndSlice, dest, destSlice);
  parse_var_list(srcList, srcVec);

  node->type = SRC_CONCAT;
  node->dest = destAndSlice;
  node->op = "";
  node->srcVec = srcVec;
  node->destTime = timeIdx;
  node->done = false;

  for(auto src: srcVec) {
    if(destAndSlice.compare("fangyuan23") == 0) {
      toCout("Find it!");
    }
    add_child_node(src, timeIdx, node);  
  }

  return;
}


void add_case_node(std::string var, uint32_t timeIdx, astNode* const node) {
  if(g_curMod->caseTable.find(var) == g_curMod->caseTable.end()) {
    toCout("Error: not found in g_curMod->caseTable: "+var);
    abort();
  }
  auto localPair = g_curMod->caseTable[var];
  std::string caseVar = localPair.first;
  auto caseAssignPairs = localPair.second;

  node->type = CASE;
  node->dest = var;
  node->op = "";
  //node->srcVec = SV{op1AndSlice, op2AndSlice, integer};
  // srcVec must follow this format:
  // 1st is the sAndSlice(case variable)
  // then followed are N pairs of (caseValue, assignVariable).
  // So the total number is 2N+1
  node->srcVec.push_back(caseVar);
  add_child_node(caseVar, timeIdx, node);
  add_child_node(caseAssignPairs.front().second, timeIdx, node);
  add_child_node(caseAssignPairs.back().second, timeIdx, node);
  for(std::pair<std::string, std::string> caseAssign : caseAssignPairs) {
    node->srcVec.push_back(caseAssign.first);
    node->srcVec.push_back(caseAssign.second);
  }
  node->destTime = timeIdx;
  node->isReduceOp = false;
  node->done = false;
  return;
}


void add_func_node(std::string var, uint32_t timeIdx, astNode* const node) {
  if(g_curMod->funcTable.find(var) == g_curMod->funcTable.end()) {
    toCout("Error: not found in g_curMod->caseTable: "+var);
    abort();
  }
  auto funcInfo = g_curMod->funcTable[var];
  node->type = FUNC;
  node->dest = var;
  node->op = funcInfo.instanceName;
  node->srcVec = funcInfo.inputs;
  for(std::string &input: funcInfo.inputs) {
    add_child_node(input, timeIdx, node);
  }
  node->destTime = timeIdx;
  node->isReduceOp = false;
  node->done = false;
}


void add_submod_node(std::string var, uint32_t timeIdx, astNode* const node) {
  node->type = SUBMODOUT;
  node->dest = var;
  std::string insName = g_curMod->wire2InsPortMp[var].first;
  std::string output = g_curMod->wire2InsPortMp[var].second;
  node->op = insName;
  node->srcVec = std::vector<std::string>{};
  node->destTime = timeIdx;
  node->isReduceOp = false;
  node->done = false;
  // switch module before adding child node
  std::string modName = g_curMod->ins2modMap[insName];
  g_curMod = g_moduleInfoMap[modName];
  auto subMod = g_curMod;
  // treate differently for new or seen submodule output
  if(g_curMod->out2RootNodeMp.find(output) == g_curMod->out2RootNodeMp.end()) {
    g_visitedNode = std::make_unique<std::map<std::string, astNode*>>();
    g_visitedNodeStk.push(g_visitedNode);
    g_instancePairStk.push(std::make_pair(insName, g_curMod));
    astNode* nextNode = new astNode;
    add_node(output, timeIdx, nextNode, false);

    g_instancePairStk.pop();
    g_visitedNodeStk.pop();
    g_curMod->out2RootNodeMp.emplace(output, nextNode);
  }
  g_curMod = g_instancePairStk.top().second;
  //add_child_node(output, timeIdx, node);
  for(std::string input : subMod->moduleInputs) {
    std::string connectWire = g_curMod->insPort2wireMp[insName][input];
    node->srcVec.push_back(connectWire);
    add_child_node(connectWire, timeIdx, node);
  }
}


bool check_two_op(std::string line, 
                  std::string &op, 
                  std::string &dest, 
                  std::string &op1, 
                  std::string &op2, 
                  bool &isReduceOp) {
  line = remove_signed(line);
  isReduceOp = false;
  std::smatch m;
  if ( std::regex_match(line, m, pAdd)) {
    op = "+";
  }
  else if ( std::regex_match(line, m, pSub)) {
    op = "-";
  }
  else if ( std::regex_match(line, m, pMult)) {
    op = "*";
  }
  else if ( std::regex_match(line, m, pAnd)) {
    op = "&&";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pOr)) {
    op = "||";
  }
  else if ( std::regex_match(line, m, pBitOr)) {
    op = "|";
  }
  else if ( std::regex_match(line, m, pBitExOr)) {
    op = "^";
  }
  else if ( std::regex_match(line, m, pBitAnd)) {
    op = "&";
  }
  else if ( std::regex_match(line, m, pLeftShift) ) {
    op = "<<";
  }
  else if ( std::regex_match(line, m, pRightShift) ) {
    op = ">>";
  }
  else if ( std::regex_match(line, m, pSignedRightShift) ) {
    op = ">>>";
  }
  else if ( std::regex_match(line, m, pSignedLeftShift) ) {
    op = "<<<";
  }
  else if ( std::regex_match(line, m, pBitOrRed2) ) {
    abort(); // needs to add support in two_op_constraint
    //since this op is same as pBitOr. 
    //needs to separate them with the isReduceOp
    op = "|";
    isReduceOp = true;
  } 
  else if ( std::regex_match(line, m, pEq) ) {
    op = "==";
    isReduceOp = true;
  } 
  else if ( std::regex_match(line, m, pEq3) ) {
    op = "===";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pNeq) ) {
    op = "!=";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pLt) ) {
    op = ">";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pLe) ) {
    op = ">=";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSt) ) {
    op = "<";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSe) ) {
    op = "<=";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSignedLt) ) {
    op = "$>";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSignedLe) ) {
    op = "$>=";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSignedSt) ) {
    op = "$<";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pSignedSe) ) {
    op = "$<=";
    isReduceOp = true;
  }
  else if ( std::regex_match(line, m, pEq)
              || std::regex_match(line, m, pEq3)
              || std::regex_match(line, m, pNeq)
              || std::regex_match(line, m, pLt)
              || std::regex_match(line, m, pLe)
              || std::regex_match(line, m, pSt)
              || std::regex_match(line, m, pSe) ) {
    isReduceOp = true;
    return true;
  }
  else
    return false;

  dest = m.str(2);
  op1 = m.str(3);
  op2 = m.str(4);  
  return true;
}


bool check_sel_op(std::string line, 
                  std::string &op, 
                  std::string &dest, 
                  std::string &op1,
                  std::string &op2, 
                  std::string &integer) {
  std::smatch m;
  if ( std::regex_match(line, m, pSel1)) {
    op = "sel1";
  }
  else if ( std::regex_match(line, m, pSel2)) {
    op = "sel2";
  }
  else if ( std::regex_match(line, m, pSel3)) {
    op = "sel3";
  }
  else if ( std::regex_match(line, m, pSel4)) {
    op = "sel4";
  }
  else if ( std::regex_match(line, m, pSel5)) {
    op = "sel5";
  }
  else {
    return false;
  }
  dest = m.str(2);
  op1 = m.str(3);
  op2 = m.str(5);
  integer = m.str(6);
  return true;
}


bool check_one_op(std::string line, 
                  std::string &op, 
                  std::string &dest,
                  std::string &op1) {
  std::smatch m;
  if ( std::regex_match(line, m, pNone)) {
    op = "";
  }
  else if ( std::regex_match(line, m, pInvert)) {
    op = "~";
  }
  else if ( std::regex_match(line, m, pPlus)) {
    op = "+";
  } 
  else if ( std::regex_match(line, m, pMinus)) {
    op = "-";
  }
  else {
    toCout("Unsupported expressions: "+line);
    abort();
    return false;
  }
  dest = m.str(2);
  op1 = m.str(3);
  return true;
}


bool check_reduce_one_op(std::string line, 
                         std::string &op, 
                         std::string &dest, 
                         std::string &op1) {
  std::smatch m;
  if ( std::regex_match(line, m, pNot)) {
    op = "!";
  }
  else if ( std::regex_match(line, m, pRedOr)) {
    op = "|";
  }
  else if ( std::regex_match(line, m, pRedAnd)) {
    op = "&";
  }
  else if ( std::regex_match(line, m, pRedNand)) {
    op = "~&";
  }
  else if ( std::regex_match(line, m, pRedNor)) {
    op = "~|";
  }
  else if ( std::regex_match(line, m, pRedXor)) {
    op = "^";
  }
  else if ( std::regex_match(line, m, pRedXnor)) {
    op = "~^";
  }
  else {
    toCout("Unsupported expressions for reduce op: "+line);
    abort();
    return false;
  }
  dest = m.str(2);
  op1 = m.str(3);
  return true;
}

} // end of namespace funcExtract
