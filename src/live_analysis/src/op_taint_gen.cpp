#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <utility>
#include <assert.h>
#include <unordered_map>
#include "op_taint_gen.h"
#include <math.h>
#include <assert.h>
#include "global_data.h"
#include "helper.h"

#define toStr(a) std::to_string(a)
// configurations:

using namespace syntaxPatterns;

namespace taintGen {

void input_taint_gen(std::string line, std::ofstream &output) {
  std::smatch m;
  if (!std::regex_match(line, m, pInput))
    return;
  std::string blank = m.str(1);
  std::string slice = m.str(2);
  std::string var = m.str(3);
  bool isClk = false, isRst = false;
  if(!g_hasRst) {
    if(var.compare("rst") == 0
        || var.compare("i_rst") == 0
        || var.compare("RST") == 0
        || var.compare("reset") == 0 ) {
      g_hasRst = true;
      g_rst_pos = true;
      g_recentRst = var;
      isClk = true;
    }
    else if ( var.compare("reset_n") == 0
                || var.compare("resetn") == 0
                || var.compare("rstn") == 0 
                || var.compare("RSTN") == 0 
                || var.compare("RST_N") == 0) {
      g_hasRst = true;
      g_rst_pos = false;
      g_recentRst = var;
      isRst = true;
    }
    else if ( g_clkrst_exist && var.compare(g_possibleRST) == 0 ) {
      g_hasRst = true;
      g_rst_pos = g_possibleSign;
      g_recentRst = var;
      isRst = true;
    }
    else if ( g_clkrst_exist && var.compare(g_possibleCLK) == 0 ) {
      g_recentClk = var;
    }
    else if(var.find("clk") != std::string::npos
            || var.find("clock") != std::string::npos 
            || var.find("CLOCK") != std::string::npos 
            || var.find("CLK") != std::string::npos && var.find("EN") == std::string::npos ) {
      if(var.length() <= 8 && var != "rclk" && var != "tclk") {    
        toCout("================================================  Find potential unexpected clk signal: "+var+" in module: "+moduleName);
        g_recentClk = var;
      }
    }
    else if( var.find("rstn") != std::string::npos
            || var.find("rst_n") != std::string::npos
            || var.find("resetn") != std::string::npos
            || var.find("RSTN") != std::string::npos
            || var.find("RST_N") != std::string::npos
            || var.find("RESETN") != std::string::npos
            || var.find("RESET_N") != std::string::npos
            || var.find("reset_n") != std::string::npos ) {
      toCout("================================================  Find potential unexpected rstn signal: "+var+" in module: "+moduleName);
      g_hasRst = true;
      g_rst_pos = false;
      g_recentRst = var;
      isRst = true;
    }
    else if(var.find("rst") != std::string::npos
            || var.find("reset") != std::string::npos 
            || var.find("RST") != std::string::npos 
            || var.find("RESET") != std::string::npos ) {
      toCout("================================================  Find potential unexpected rst signal: "+var+" in module: "+moduleName);
      g_hasRst = true;
      g_rst_pos = true;
      g_recentRst = var;
      isRst = true;
    }
  }
  g_clk_set.insert(g_recentClk);
  moduleInputs.push_back(var);
  if(var != g_recentClk) {
    extendInputs.push_back(var+_t);
    extendOutputs.push_back(var+_r);
  }
  if(!isTop && !g_use_value_change && var != g_recentClk)
    extendInputs.push_back(var+_sig);
  //if (var.compare( clockName) == 0)
  //  return;
  //debug_line(line);
  if(g_enable_taint) {
    if(var != g_recentClk) { 
      output << blank + "input " + slice + var + _t + " ;" << std::endl;
      output << blank + "output " + slice + var + _r + " ;" << std::endl;
    }
    if(!g_use_value_change && isTop && var != g_recentClk)
      output << blank + "wire [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;
    else if(!g_use_value_change && var != g_recentClk)
      output << blank + "input [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;

    if(isTop && !g_use_value_change)
      output << blank + "assign " + var + _sig + " = 0 ;" << std::endl;\
  }

  if (!did_clean_file) {
    bool insertDone;
    if(!slice.empty())
      insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
    else
      insertDone = varWidth.var_width_insert(var, 0, 0);
    if (!insertDone) {
      std::cout << "insert failed in input case:" + line << std::endl;
      std::cout << "m.str(2):" + m.str(2) << std::endl;
      std::cout << "m.str(3):" + m.str(3) << std::endl;
    }
  }
}


void reg_taint_gen(std::string line, std::ofstream &output) {
  std::smatch m;
  if( std::regex_match(line, m, pRegConst) ) {
    toCout("Found reg const");
  }
  if ( !std::regex_match(line, m, pReg) 
        && !std::regex_match(line, m, pRegConst) )
    return;
  std::string blank = m.str(1);
  std::string slice = m.str(2);  
  std::string var = m.str(3);
  std::string num = m.str(4);

  moduleRegs.push_back(var);

  if(g_enable_taint) {
    output << blank << "logic " + slice + " " + var + _t + " ;" << std::endl;

    if(g_use_does_keep) output <<  "  logic " + slice + " " +var + "_DOES_KEEP = 0 ;"  << std::endl;

    if(!isOutput(var)) { // maybe later declared as output
      output << blank << "logic " + slice + " " + var + _r + " ;" << std::endl;
      //output << blank << "logic " + slice + " " + var + _x + " ;" << std::endl;
      //output << blank << "logic " + slice + " " + var + _c + " ;" << std::endl;
      if(!g_use_value_change)
      output << blank << "logic [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;    
    }

    output << blank << "logic " + var + "_t_flag = 0;" << std::endl;
    output << blank << "logic " + var + "_r_flag = 0;" << std::endl;

    if(isTrueReg(var) && !g_use_value_change) {
      output << blank << "assign " + var + _sig + " = " + toStr(g_next_sig++) + " ;" << std::endl;
    }
    if(g_use_reset_taint)
      output << blank << "logic " + var + "_reset ;" << std::endl;
  }
  flagOutputs.push_back(var+"_r_flag");  

  if (!did_clean_file) {
    bool insertDone;
    if(!slice.empty())
      insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
    else
      insertDone = varWidth.var_width_insert(var, 0, 0);
    if (!insertDone) {
      std::cout << "insert failed in input case:" + line << std::endl;
      std::cout << "m.str(2):" + m.str(2) << std::endl;
      std::cout << "m.str(3):" + m.str(3) << std::endl;
    }
  }

  if(g_check_invariance == CheckOneVal || g_check_invariance == CheckTwoVal)
    output << blank << "reg " + slice + " " + var + "_PREV_VAL1 ;" << std::endl; 
  if(g_check_invariance == CheckTwoVal)
    output << blank << "reg " + slice + " " + var + "_PREV_VAL2 ;" << std::endl; 
}


// TODO: add sig for mem
void mem_taint_gen(std::string line, std::ofstream &output) {
  toCout("Warning: mem not supported for use_reset_sig!!!");  
  std::smatch m;
  if ( !std::regex_match(line, m, pMem )) 
    return;
  std::string blank = m.str(1);
  std::string slice = m.str(2);  
  std::string var = m.str(3);
  std::string sliceTop = m.str(4);
  memDims.emplace(var, std::make_pair(slice, sliceTop));
  
  uint32_t varLen = get_end(sliceTop) + 1;
  moduleMems.emplace(var, varLen);
  //assert_info(!isTop || !isOutput(var), "mem_taint_gen:var is output, line: "+line);  

  if(g_enable_taint) {
    output << blank << "logic " + slice + " " + var + _t + " " + sliceTop + " ;" << std::endl;

    output << blank << "logic " + sliceTop + " " + var + "_t_flag = 0;" << std::endl;
    output << blank << "logic " + sliceTop + " " + var + "_r_flag = 0;" << std::endl;
    output << blank << "logic " + slice + " " + var + _r + " " + sliceTop + " ;" << std::endl;
    output << blank << "logic " + var + "_r_flag_top = 0;" << std::endl;
    // _sig
    if(!g_use_value_change)
    output << blank << "logic [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;
      if(isTrueReg(var) && !g_use_value_change ) {
      output << blank << "assign " + var + _sig + " = " + toStr(g_next_sig++) + " ;" << std::endl;
    }
    //output << blank << "logic " + var + "_reset ;" << std::endl;
  }
  flagOutputs.push_back(var+"_r_flag_top");  

  if (!did_clean_file) {
    bool insertDone;
    if(!slice.empty())
      insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
    else
      insertDone = varWidth.var_width_insert(var, 0, 0);
    if (!insertDone) {
      std::cout << "insert failed in input case:" + line << std::endl;
      std::cout << "m.str(2):" + m.str(2) << std::endl;
      std::cout << "m.str(3):" + m.str(3) << std::endl;
    }
  }

  if(g_check_invariance == CheckOneVal || g_check_invariance == CheckTwoVal)
    output << blank << "reg " + slice + " " + var + "_PREV_VAL1 " + sliceTop + " ;" << std::endl;
  if(g_check_invariance == CheckTwoVal)
    output << blank << "reg " + slice + " " + var + "_PREV_VAL2 " + sliceTop + " ;" << std::endl;

}


void wire_taint_gen(std::string line, std::ofstream &output) {
  std::smatch m;
  if ( !std::regex_match(line, m, pWire) )
    return;
  std::string slice = m.str(2);  
  std::string var = m.str(3);
  std::string blank = m.str(1);
  moduleWires.insert(var);  
  //debug_line(line);  

  if(g_enable_taint) {
    output << blank + "logic " + slice + var + _t+" ;" << std::endl;
    output << blank + "logic " + slice + var + _r+" ;" << std::endl;
    // for wires named addedVar, delay declaring it until assignment
    if(var.find("addedVar") == std::string::npos && !g_use_value_change)
      output << blank + "logic [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;
  }

  if (!did_clean_file) {
    bool insertDone;
    if(!slice.empty())
      insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
    else
      insertDone = varWidth.var_width_insert(var, 0, 0);
    if (!insertDone) {
      std::cout << "insert failed in input case:" + line << std::endl;
      std::cout << "m.str(2):" + m.str(2) << std::endl;
      std::cout << "m.str(3):" + m.str(3) << std::endl;
    }
  }
}


void output_insert_map(std::string line, std::ofstream &output, std::ifstream &input) {
  std::smatch m;
  if ( !std::regex_match(line, m, pOutput) )
    return;
  std::string slice = m.str(2);  
  std::string var = m.str(3);
  std::string blank = m.str(1);
    
  if(isOutput(var)) {
    std::cout << "!! Duplicate output find: " + line << std::endl;
  }
  else 
    moduleOutputs.push_back(var);
 
  //output << blank << "logic " + slice + var + " ;" << std::endl;  

  auto currentPos = input.tellg();
  std::string nextLine;
  std::getline(input, nextLine);
  input.seekg(currentPos);

  if (!did_clean_file) {
    bool insertDone;
    if(!slice.empty())
      insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
    else
      insertDone = varWidth.var_width_insert(var, 0, 0);
    if (!insertDone) {
      std::cout << "insert failed in output case:" + line << std::endl;
      std::cout << "m.str(2):" + m.str(2) << std::endl;
      std::cout << "m.str(3):" + m.str(3) << std::endl;
    }
  }

  if(g_enable_taint) {
    output << blank << "output " + slice + var + _t + " ;" << std::endl;  
    output << blank << "logic " + slice + var + _t + " ;" << std::endl;  
    if(!isOAReg(var)) {
      output << blank << "logic " + slice + var + _r + " ;" << std::endl;
      if(!g_use_value_change)
      output << blank << "logic [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;  
    }

    bool inTaintIsNew;
    uint32_t inVerNum = find_version_num(var, inTaintIsNew, output);
    assert(inVerNum == 0);
    assert(inTaintIsNew);
    output << blank << "input " + slice + var + _r + toStr(inVerNum) + " ;" << std::endl;

    if(!isTop && !g_use_value_change)
      output << blank << "output [" + toStr(g_sig_width-1) + ":0] " + var + _sig + " ;" << std::endl;

    extendOutputs.push_back(var+_t);
    if(!isTop && !g_use_value_change)
      extendOutputs.push_back(var+_sig);
    extendInputs.push_back(var+_r+toStr(inVerNum));
  }
}


void two_op_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;  
  std::smatch m;  
  bool isReduceOp = false;
  bool isOR = false;
  bool isAnd = false;
  bool isBitAnd = false;
  bool isBitOr = false;
  bool isShift = false;
  bool isEq = false;
  bool isSub = false;
  if( std::regex_match(line, m, pOr) )
    isOR = true;
  else if( std::regex_match(line, m, pAnd) )
    isAnd = true;
  else if( std::regex_match(line, m, pBitAnd) )
    isBitAnd = true;
  else if( std::regex_match(line, m, pBitOr) )
    isBitOr = true;
  else if( std::regex_match(line, m, pEq) )
    isEq = true;
  else if( std::regex_match(line, m, pSub) )
    isSub = true;

  if (std::regex_match(line, m, pAdd)
            || std::regex_match(line, m, pSub)
            || std::regex_match(line, m, pMult)
            || std::regex_match(line, m, pMod)
            || std::regex_match(line, m, pDvd)
            || std::regex_match(line, m, pAnd)
            || std::regex_match(line, m, pOr)
            || std::regex_match(line, m, pBitOr)
            || std::regex_match(line, m, pBitExOr)
            || std::regex_match(line, m, pBitAnd)
            || std::regex_match(line, m, pBitXnor1)
            || std::regex_match(line, m, pBitXnor2)
            || std::regex_match(line, m, pBitOrRed2) ) {}
  else if ( std::regex_match(line, m, pLeftShift)
              || std::regex_match(line, m, pRightShift)
              || std::regex_match(line, m, pSignedRightShift)
              || std::regex_match(line, m, pSignedLeftShift) ) {
    isShift = true;
  }
  else if ( std::regex_match(line, m, pEq)
              || std::regex_match(line, m, pEq3)
              || std::regex_match(line, m, pNeq)
              || std::regex_match(line, m, pLt)
              || std::regex_match(line, m, pLe)
              || std::regex_match(line, m, pSt)
              || std::regex_match(line, m, pSe) ) {
    isReduceOp = true;
  }
  else
    return;

  assert(!m.str(3).empty());
  assert(!m.str(4).empty());
  bool op1IsNum = isNum(m.str(3));
  bool op2IsNum = isNum(m.str(4));
  std::string blank = m.str(1);
  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;
  std::string destAndSlice = m.str(2);
  std::string op1AndSlice = m.str(3);
  std::string op2AndSlice = m.str(4);

  split_slice(destAndSlice, dest, destSlice);
  if(isReduceOp)
    assert(get_var_slice_width(destAndSlice) == 1);
  // !! Attention: op1 is just var name, op1Slice is its slice
  split_slice(op1AndSlice, op1, op1Slice);
  split_slice(op2AndSlice, op2, op2Slice);
  // TODO: temporarily assume mem only appears in simple statements
  assert(!isMem(op1));
  assert(!isMem(op2));

  auto op1IdxPair = varWidth.get_idx_pair(op1, line);
  auto op2IdxPair = varWidth.get_idx_pair(op2, line);
  std::string op1HighIdx  = toStr(op1IdxPair.first);
  std::string op1LowIdx   = toStr(op1IdxPair.second);
  std::string op2HighIdx  = toStr(op2IdxPair.first);
  std::string op2LowIdx   = toStr(op2IdxPair.second);

  uint32_t destWidthNum = get_var_slice_width(destAndSlice);
  uint32_t op1LocalWidthNum = get_var_slice_width(op1AndSlice);
  uint32_t op2LocalWidthNum = get_var_slice_width(op2AndSlice);

  auto op1BoundPair = varWidth.get_idx_pair(op1, line);
  auto op2BoundPair = varWidth.get_idx_pair(op2, line);

  // Below are all taint-related statements

  if (!op1IsNum && !op2IsNum) { // 2-op

    // _sig
    if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    /* Assgin new versions */
    uint32_t sndVerNum, thdVerNum;
    bool op1IsNew, op2IsNew;
    sndVerNum = find_version_num(op1AndSlice, op1IsNew, output);
    thdVerNum = find_version_num(op2AndSlice, op2IsNew, output);
  
    std::string sndVer = std::to_string(sndVerNum);
    std::string thdVer = std::to_string(thdVerNum);

    /* delcare new wires */
    if(op1IsNew) {
      output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + sndVer + " ;" << std::endl;
    }
    
    if(op2IsNew) {
      output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + thdVer + " ;" << std::endl;
    }

    /* make assignments */
    /* FIXME: the width of op1/op2 and dest are not necessarily the same */
    if(!isReduceOp && !isShift)
      if(isAnd)
        output << blank << "assign " + dest + _t + destSlice + " = ( " + op1 + _t + op1Slice + " && " + op2AndSlice + " ) | ( " + op2 + _t + op2Slice + " && " + op1AndSlice + " ) ;" << std::endl;
      else if(isOR)
        output << blank << "assign " + dest + _t + destSlice + " = ( " + op1 + _t + op1Slice + " && " + op2AndSlice + " != 1 ) | ( " + op2 + _t + op2Slice + " && " + op1AndSlice + " != 1 ) ;" << std::endl;
      else if(isBitAnd)
        output << blank << "assign " + dest + _t + destSlice + " = ( " + op1 + _t + op1Slice + " & " + op2AndSlice + " ) | ( " + op2 + _t + op2Slice + " & " + op1AndSlice + " ) ;" << std::endl;
      else if(isBitOr)
        output << blank << "assign " + dest + _t + destSlice + " = ( " + op1 + _t + op1Slice + " & ~" + op2AndSlice + " ) | ( " + op2 + _t + op2Slice + " & ~" + op1AndSlice + " ) ;" << std::endl;
      else
        output << blank << "assign " + dest + _t + destSlice + " = " + op1 + _t + op1Slice + " | " + op2 + _t + op2Slice + " ;" << std::endl;
    else if(isReduceOp)
      if (isEq && g_special_equal_taint)
        output << blank << "assign " + dest + _t + destSlice + " = ((| " + op1 + _t + op1Slice + " ) | (|" + op2 + _t + op2Slice + " )) & ( "+op1AndSlice+" == "+op2AndSlice+" ) ;" << std::endl;
      else
        output << blank << "assign " + dest + _t + destSlice + " = (| " + op1 + _t + op1Slice + " ) | (|" + op2 + _t + op2Slice + " ) ;" << std::endl;
    else if(isShift)
      output << blank << "assign " + dest + _t + destSlice + " = " + extend("(| " + op1 + _t + op1Slice + " ) | (|" + op2 + _t + op2Slice + " )", destWidthNum) + " ;" << std::endl;


    if ( isOutput(dest) && isTop ) {
      if(!isReduceOp && !isShift) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      }
      else if(isReduceOp) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1LocalWidthNum) +  " ;" << std::endl;
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend(dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
      }
      else if(isShift) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend("| "+dest+_r+destSlice, op1LocalWidthNum) +  " ;" << std::endl;
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
      }
    }
    else if (!isReduceOp && !isShift){
      if(isOR) { // NOTE, operands of OR can be more than 1 bit
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " & " + extend(op2AndSlice + " == 0", op2LocalWidthNum) + " ;" << std::endl;     
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " & " + extend(op1AndSlice + " == 0", op1LocalWidthNum) + " ;" << std::endl;
      }
      else if(isAnd) { // NOTE, operands of AND can be more than 1 bit
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " =  " + dest + _r + destSlice + " & " + extend(op2AndSlice + " != 0", op2LocalWidthNum) + " ;" << std::endl;     
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " =  " + dest + _r + destSlice + " & " + extend(op1AndSlice + " != 0", op1LocalWidthNum) + " ;" << std::endl;
      }
      else if(isBitAnd) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " & " + extend(op2AndSlice + " != 0", op2LocalWidthNum) + " ;" << std::endl;     
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " & " + extend(op1AndSlice + " != 0", op1LocalWidthNum) + " ;" << std::endl;
      }
      else if(isBitOr) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " & " + extend(op2AndSlice + " != "+max_num(op1LocalWidthNum), op2LocalWidthNum) + " ;" << std::endl;     
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " & " + extend(op1AndSlice + " != "+max_num(op2LocalWidthNum), op1LocalWidthNum) + " ;" << std::endl;
      }
      else {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;     
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      }
    }
    else if(isReduceOp){ // if dest is not output and op is reduceOp
      output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1LocalWidthNum) + " ;" << std::endl;
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend(dest+_r+destSlice, op2LocalWidthNum) + " ;" << std::endl;
    }
    else if(isShift) {
      output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend("| "+dest+_r+destSlice, op1LocalWidthNum) +  " ;" << std::endl;
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
    }
  } 
  else if (!op1IsNum && op2IsNum) { // 2-op
    assert(!isShift);
    // _sig
    if(!g_use_value_change)
      output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    bool op1IsNew;
    uint32_t sndVerNum = find_version_num(op1AndSlice, op1IsNew, output);
    std::string sndVer = std::to_string(sndVerNum);

    /* declare new wires */
    if(op1IsNew) {    
      output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + sndVer + " ;" << std::endl;
    }

    if(isReduceOp)
      if (isEq && g_special_equal_taint)
        output << blank << "assign " + dest + _t + destSlice + " = (| " + op1 + _t + op1Slice + " ) & ( "+op1AndSlice+" == "+op2AndSlice+" ) ;" << std::endl;
      else
        output << blank << "assign " + dest + _t + destSlice + " = | " + op1 + _t + op1Slice + " ;" << std::endl;
    else
      output << blank << "assign " + dest + _t + destSlice + " = " + op1 + _t + op1Slice + " ;" << std::endl;

    if ( isOutput(dest) && isTop ) {
      if(!isReduceOp) {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      }
      else {
        output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1LocalWidthNum) +  " ;" << std::endl;
      }
    }
    else if(!isReduceOp){
      output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
    }
    else {
      output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1LocalWidthNum) + " ;" << std::endl;
    }
  }
  else if (op1IsNum && !op2IsNum) { // 2-op
    //assert(!isShift);
    // _sig
    if(!g_use_value_change)
      output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    bool op2IsNew;    
    uint32_t thdVerNum = find_version_num(op2AndSlice, op2IsNew, output);
    std::string thdVer = std::to_string(thdVerNum);

    /* declare new wires */
    if(op2IsNew) {    
      output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + thdVer + " ;" << std::endl;
    }

    if(isReduceOp)    
      if (isEq && g_special_equal_taint)
        output << blank << "assign " + dest + _t + destSlice + " = (|" + op2 + _t + op2Slice + " ) & ( "+op1AndSlice+" == "+op2AndSlice+" ) ;" << std::endl;
      else
        output << blank << "assign " + dest + _t + destSlice + " = | " + op2 + _t + op2Slice + " ;" << std::endl;
    else if(isShift)
      output << blank << "assign " + dest + _t + destSlice + " = " + extend("| " + op2 + _t + op2Slice, destWidthNum) + " ;" << std::endl;
    else
      output << blank << "assign " + dest + _t + destSlice + " = " + op2 + _t + op2Slice + " ;" << std::endl;

    if ( isOutput(dest) && isTop ) {
      if(!isReduceOp && !isShift) {
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " ;" << std::endl;  
      }
      else if(isReduceOp){
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend(dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
      }
      else if(isShift) {
        output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
      }
    }
    else if(!isReduceOp && !isShift) {
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
    }
    else if(isReduceOp){
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend(dest+_r+destSlice, op2LocalWidthNum) + " ;" << std::endl;
    }
    else if(isShift) {
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2LocalWidthNum) +  " ;" << std::endl;
    }
  }
  else {
    std::cout << "!!! Warning: both two operators are constants" << std::endl;
    output << blank << "assign " + dest + _t + destSlice + " = 0 ;" << std::endl;
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;    
  }
}


void one_op_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;
  std::smatch m;
  bool isNone = false;
  if (std::regex_match(line, m, pNone) 
        || std::regex_match(line, m, pPlus)
        || std::regex_match(line, m, pMinus)
        || std::regex_match(line, m, pInvert)){}
  else 
    return;
  
  assert(!m.str(2).empty());
  assert(!m.str(3).empty());

  std::string blank = m.str(1);
  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string op1AndSlice = m.str(3);
  uint32_t op1WidthNum = get_var_slice_width(op1AndSlice);
  //assert_info(!isTop || !isOutput(op1AndSlice), "one_op_taint_gen:op1 is output, line: "+line);  
  
  split_slice(m.str(2), dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  std::string sndVer;
  if(std::regex_match(line, m, pNone) && isNum(op1AndSlice))
    toCout("WARNING!! Number found in RHS of Non operation!!  "+line);

  // assume memory slices can only be used in simple assignment statements
  // also assume each memory slice is used only once
  if(isMem(op1)) {
    assert(std::regex_match(line, m, pNone));
    if( nextVersion.find(op1) == nextVersion.end() ) {
      sndVer = toStr(0);
      nextVersion.emplace(op1, 1);
    }
    else {
      sndVer = toStr(nextVersion[op1]);
    }
    // declare new _r, _c, _x taints
    auto slicePair = memDims[op1];
    std::string slice = slicePair.first;
    std::string sliceTop = slicePair.second;
    std::string highIdx = toStr(get_end(sliceTop));
    output << blank + "logic " + slice + " " + op1 + _r + sndVer + sliceTop + " ;" << std::endl;

    output << blank + "integer i;" << std::endl;
    output << blank + "always @(*) begin" << std::endl;
    output << blank + "  for(i = 0; i <= "+highIdx+"; i = i + 1) begin" << std::endl;
    output << blank + "    "+op1+_r+sndVer+" [i] = 0;" << std::endl;
    output << blank + "  end" << std::endl;
    output << blank + "end" << std::endl;

    output << blank + "assign " + dest + _t + destSlice + " = " + op1 + _t + op1Slice + " ;" << std::endl;
    if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = " + op1 + _sig + " ;" << std::endl;
    output << blank + "always @(*) begin" << std::endl;
    output << blank + "  " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
    output << blank + "end" << std::endl;    
    return;
  }

  if (std::regex_match(line, m, pNone)) 
    isNone = true;
  if( isNum(op1) ) {
    output << blank + "assign " + dest + _t + destSlice + " = 0 ;" << std::endl;   
    if(!g_use_value_change && destSlice.empty())
    output << blank + "assign " + dest + _sig + " = " + CONSTANT_SIG + " ;" << std::endl;
    return;
  }

  auto op1IdxPair = varWidth.get_idx_pair(op1, line);
  std::string op1HighIdx  = toStr(op1IdxPair.first);
  std::string op1LowIdx   = toStr(op1IdxPair.second);

  bool op1IsNew;
  uint32_t sndVerNum = find_version_num(op1AndSlice, op1IsNew, output);
  sndVer = std::to_string(sndVerNum);

  /* declare new wires */
  if(op1IsNew) {
    output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + sndVer + " ;" << std::endl;
  }

  output << blank << "assign " + dest + _t + destSlice + " = " + op1 + _t + op1Slice + " ;" << std::endl;
  if ( isOutput(dest) && isTop ) {
      output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      // FIXME: because output is floating, it is always changed??      
  }
  else {
    output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + dest + _r + destSlice + " ;" << std::endl;
  }

  // FIXME: destSlice.empty
  if(std::regex_match(line, m, pNone) && !g_use_value_change && destSlice.empty())
    output << blank + "assign " + dest + _sig + " = " + op1 + _sig + " ;" << std::endl;
  else if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;
}


void sel_op_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;  
  std::smatch m;
  if (std::regex_match(line, m, pSel1)
        || std::regex_match(line, m, pSel2)
        || std::regex_match(line, m, pSel3)
        || std::regex_match(line, m, pSel4)) {}
  else 
    return;

  assert(!m.str(2).empty());
  assert(!m.str(3).empty());
  assert(!m.str(4).empty());
  assert(!m.str(5).empty());
  assert(!m.str(6).empty());
 
  std::string blank = m.str(1);
  std::string destAndSlice = m.str(2);
  std::string op1AndSlice = m.str(3);
  std::string slice = m.str(4);
  std::string op2AndSlice = m.str(5);
  std::string lowIdx = m.str(6);
  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;
  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  split_slice(op2AndSlice, op2, op2Slice);
  if(!op1Slice.empty()){
    toCout("Error: op1Slice is not empty in sel_op: op1AndSlice:"+op1AndSlice+", op1Slice:"+op1Slice);
    abort();
  }

  //assert_info(!isNum(op1), "Error: the var to be selected are numbers!");
  uint32_t localWidth = get_var_slice_width(destAndSlice);
  uint32_t op2Width = get_var_slice_width(op2AndSlice);
  std::string sndVer;
 
  ////// declare taints
  if(!isMem(op1) && !isNum(op1)) {
    auto op1IdxPair = varWidth.get_idx_pair(op1, line);
    std::string op1HighIdx  = toStr(op1IdxPair.first);
    std::string op1LowIdx   = toStr(op1IdxPair.second);

    bool op1IsNew;
    uint32_t sndVerNum = find_version_num(op1AndSlice, op1IsNew, output);
    sndVer = std::to_string(sndVerNum);

    if(op1IsNew) {
      output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + sndVer + " ;" << std::endl;
    }
  }
  else if(isMem(op1)) { //op1 is mem
    if( nextVersion.find(op1) == nextVersion.end() ) {
      sndVer = toStr(0);
      nextVersion.emplace(op1, 1);
    }
    else {
      sndVer = toStr(nextVersion[op1]);
    }
    // declare new _r, _c, _x taints, and set default value to 0
    auto slicePair = memDims[op1];
    std::string slice = slicePair.first;
    std::string sliceTop = slicePair.second;
    std::string highIdx = toStr(get_end(sliceTop));
    output << blank + "logic " + slice + " " + op1 + _r + sndVer + sliceTop + " ;" << std::endl;
    output << blank + "integer i;" << std::endl;
    output << blank + "always @(*) begin" << std::endl;
    output << blank + "  for(i = 0; i < "+highIdx+"; i = i + 1) begin" << std::endl;
    output << blank + "    "+op1+_r+sndVer+" [i] = 0;" << std::endl;
    output << blank + "  end" << std::endl;
    output << blank + "end" << std::endl;
  }
  ////// end of declare taints
  
  if(!isNum(op2) && !isNum(op1)) {

    // _sig
    if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    auto op2IdxPair = varWidth.get_idx_pair(op2, line);
    std::string op2HighIdx  = toStr(op2IdxPair.first);
    std::string op2LowIdx   = toStr(op2IdxPair.second);

    bool op2IsNew;
    uint32_t thdVerNum = find_version_num(op2AndSlice, op2IsNew, output);
    std::string thdVer = std::to_string(thdVerNum);

    /* declare new wires */
    if(op2IsNew) {
      output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + thdVer + " ;" << std::endl;
    }

    output << blank + "assign " + dest + _t + destSlice + " = " + op1 + _t + slice + " | " + extend("| "+op2+_t+op2Slice, localWidth) + " ;" << std::endl;  

    if ( isOutput(dest) && isTop ) {
      output << blank << "assign " + op1 + _r + sndVer + slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2Width) + " ;" << std::endl;
      return;
    }

    // both isMem and !isMem have the same assignments
    // assume memory slices can only be used in simple assignment statements11
    // also assume each memory slice is used only once
    output << blank + "always @(*) begin" << std::endl;
    if(!isMem(op1)) {
    output << blank + "  " + op1 + _r + sndVer + " = 0 ;" << std::endl;
    }
    output << blank + "  " + op1 + _r + sndVer + slice + " = " + dest + _r + destSlice + " ;" << std::endl;
    output << blank + "  " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2Width) + " ;" << std::endl;
    output << blank + "end" << std::endl;    
  }
  else if(!isNum(op1)){ // isNum(op2)

    // _sig
    if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    // taints
    output << blank + "assign " + dest + _t + destSlice + " = " + op1 + _t + slice + " ;" << std::endl;  
    if ( isOutput(dest) && isTop ) {
      output << blank << "assign " + op1 + _r + sndVer + slice + " = " + dest + _r + destSlice + " ;" << std::endl;
      return;
    }
    output << blank + "always @(*) begin" << std::endl;
    output << blank + "  " + op1 + _r + sndVer + " = 0 ;" << std::endl;
    output << blank + "  " + op1 + _r + sndVer + slice + " = " + dest + _r + destSlice + " ;" << std::endl;
    output << blank + "end" << std::endl;    
  }
  else if(!isNum(op2)) { // isNum(op1)
    // _sig
    if(!g_use_value_change)
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;

    auto op2IdxPair = varWidth.get_idx_pair(op2, line);
    std::string op2HighIdx  = toStr(op2IdxPair.first);
    std::string op2LowIdx   = toStr(op2IdxPair.second);

    bool op2IsNew;
    uint32_t thdVerNum = find_version_num(op2AndSlice, op2IsNew, output);
    std::string thdVer = std::to_string(thdVerNum);

    /* declare new wires */
    if(op2IsNew) {
      output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + thdVer + " ;" << std::endl;
    }

    output << blank + "assign " + dest + _t + destSlice + " = " + extend("| "+op2+_t+op2Slice, localWidth) + " ;" << std::endl;  

    if ( isOutput(dest) && isTop ) {
      output << blank << "assign " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2Width) + " ;" << std::endl;
      return;
    }

    // both isMem and !isMem have the same assignments
    // assume memory slices can only be used in simple assignment statements11
    // also assume each memory slice is used only once
    output << blank + "always @(*) begin" << std::endl;
    output << blank + "  " + op2 + _r + thdVer + op2Slice + " = " + extend("| "+dest+_r+destSlice, op2Width) + " ;" << std::endl;
    output << blank + "end" << std::endl; 
  }
}


void reduce_one_op_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;
  std::smatch m;
  bool isRedOr=false, isNot=false, isRedAnd=false, isRedNand=false, isRedNor=false, isRedXor=false, isRedXnor=false;
  if ( std::regex_match(line, m, pRedOr))
    isRedOr = true;
  else if ( std::regex_match(line, m, pRedAnd))
    isRedAnd = true;
  else if ( std::regex_match(line, m, pRedNand))
    isRedNand = true;
  else if ( std::regex_match(line, m, pRedNor))
    isRedNor = true;
  else if ( std::regex_match(line, m, pRedXor))
    isRedXor = true;
  else if ( std::regex_match(line, m, pRedXnor))
    isRedXnor = true;
  else if ( std::regex_match(line, m, pNot))
    isNot = true;
  else 
    return;
  assert(!m.str(2).empty());
  assert(!m.str(3).empty());
  
  std::string blank = m.str(1);
  std::string dest, destSlice;
  std::string op1, op1Slice;
  std::string destAndSlice = m.str(2);
  std::string op1AndSlice = m.str(3);
  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  assert(get_var_slice_width(destAndSlice) == 1);
  assert(!isMem(op1));
  //assert_info(!isTop || !isOutput(op1), "reduce_one_op_taint_gen:op1 is output, line: "+line);  

  //uint32_t op1WidthNum = varWidth.get_from_var_width(op1, line);
  uint32_t op1WidthNum = get_var_slice_width(op1AndSlice);
  auto op1IdxPair = varWidth.get_idx_pair(op1, line);
  std::string op1HighIdx  = toStr(op1IdxPair.first);
  std::string op1LowIdx   = toStr(op1IdxPair.second);

  bool op1IsNew;
  uint32_t sndVerNum = find_version_num(op1AndSlice, op1IsNew, output);
  std::string sndVer = std::to_string(sndVerNum);

  /* declare new wires */
  if(op1IsNew) {
    output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + sndVer + " ;" << std::endl;
  }

  output << blank << "assign " + dest + _t + destSlice + " = | " + op1 + _t + op1Slice + " ;" << std::endl;


  // _r taint, put complex information into _r taint instead of _x and _c
  if(isRedOr)
    output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1WidthNum) + " & " + op1AndSlice + " ;" << std::endl;
  else if(isRedAnd)
    output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1WidthNum) + " & ~" + op1AndSlice + " ;" << std::endl;  
  else if(isRedNand ||isRedNor ) 
    checkCond(false, "Unsupported in reduce op taint: "+line);
  else 
    output << blank << "assign " + op1 + _r + sndVer + op1Slice + " = " + extend(dest+_r+destSlice, op1WidthNum) + " ;" << std::endl;

  if(!g_use_value_change)
  output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;
}


// src_concat
void mult_op_taint_gen(std::string line, std::ofstream &output) {
  std::smatch m;
  if( !is_srcConcat(line) )
    abort();

  if(!std::regex_match(line, m, pSrcConcat)) abort();

  std::string blank = m.str(1);
  std::string destAndSlice = m.str(2);
  std::string dest, destSlice;
  split_slice(destAndSlice, dest, destSlice);
  //assert(destSlice.empty());
  std::string updateList = m.str(3);
  // assume only addedVar variable can be LHS of src concatenation statement
  //checkCond(dest.find("addedVar") != std::string::npos, "Var not name addedVar appeared in src_concat! "+line);
  bool isAddedVar = (dest.find("addedVar") != std::string::npos);

  std::vector<std::string> updateVec;
  parse_var_list(updateList, updateVec);
  uint32_t destWidthNum = 0;
  for(auto v : updateVec) {
    assert(!isMem(v));
    destWidthNum += get_var_slice_width(v);
  }
  if (destWidthNum != get_var_slice_width(destAndSlice)) {
    toCout("width mismatch in mult_op function!");
    abort();
  }

  uint32_t destHighIdx = varWidth.get_high_idx(dest, line);

  // _t
  std::string updateTList = get_rhs_taint_list(updateVec, _t);
  std::string destWidth = std::to_string(destWidthNum);
  std::string destT = get_lhs_taint_list(dest, _t, output);
  if(g_enable_taint)
    output << blank + "assign " + dest + _t + destSlice + " = { " + updateTList + " };" << std::endl;
  auto destBoundPair = varWidth.get_idx_pair(dest, line);


  uint32_t srcVecItemNum = 0;
  std::string srcSigList;
  std::string v, vSlice;
  uint32_t localWidth;
  uint32_t caseSliceWidth = 0;
  if(isAddedVar)
    caseSliceWidth = addedVarCaseSliceWidth[dest];
  uint32_t remainBits = caseSliceWidth; // remainBits are bits to be filled
  std::string currentSig = "";
  bool addElement = false;
  uint32_t overFlowBits = 0;
  // if caseSliceWidth > 0, addedVar variable is used in case statements
  if(isAddedVar && caseSliceWidth > 0) {
    //if(line.find())
    for(auto vAndSlice: updateVec) {
      addElement = false;
      checkCond(vAndSlice.find("addedVar") == std::string::npos, "addedVar found in addedVar! "+line);
      split_slice(vAndSlice, v, vSlice);
      localWidth = get_var_slice_width(vAndSlice);


      if(overFlowBits == 0) { // there are remaining bits to fill
        if(remainBits == 0)
          remainBits = caseSliceWidth;
        if(localWidth < remainBits) {
          addElement = false;          
          remainBits = remainBits - localWidth;
          overFlowBits = 0;
        }
        else if (localWidth >= remainBits) {
          addElement = true;
          overFlowBits = localWidth - remainBits; // can be 0
          remainBits = 0;          
        }
      }
      else if(overFlowBits > 0 && remainBits == 0) {
        assert(overFlowBits < caseSliceWidth);
        if(overFlowBits + localWidth < caseSliceWidth) {
          addElement = false;
          remainBits = caseSliceWidth - localWidth - overFlowBits;
          overFlowBits = 0;
        }
        else if (overFlowBits + localWidth >= caseSliceWidth) {
          addElement = true;
          remainBits = 0;
          overFlowBits = localWidth  + overFlowBits - caseSliceWidth; // can be 0
        }
      }
      else {
        checkCond(false, "Error: unexpected condition in src_concat: "+line);
      }

      // begin adding srcSigList
      if(addElement) {  // time to add one element
        srcVecItemNum++;
        if(currentSig.empty() && !isNum(vAndSlice)) {
          srcSigList = srcSigList + v + _sig + " , ";
        }
        else if(currentSig.empty() && isNum(vAndSlice)) {
          srcSigList = srcSigList + CONSTANT_SIG + ", ";
        }
        else if(isNum(vAndSlice)){
          srcSigList = srcSigList + currentSig + " , ";
        }
        else {
          if(currentSig != v+_sig) {
            toCout("WARNING: two signature found for one case assign!! First: "+currentSig+", second: "+v+_sig);
            srcSigList = srcSigList + toStr(g_sig_width) + "'b0 , ";
          }
          else
            srcSigList = srcSigList + currentSig + " , ";
        }
        currentSig.clear();
        // need to deal with too much overFlow
        if(overFlowBits >= caseSliceWidth) {
          while(overFlowBits >= caseSliceWidth) {
            overFlowBits -= caseSliceWidth;
            srcSigList = srcSigList + toStr(g_sig_width) + "'b0 , ";
            srcVecItemNum++;
          }
        }
      }
      else { // if remaining bits not 0, need to wait for next vector element
        if(currentSig.empty() && isNum(vAndSlice)) {}
        else if (currentSig.empty() && !isNum(vAndSlice)) {
          currentSig = v+_sig;
        }
        else if (!currentSig.empty() && isNum(vAndSlice)) {}
        else {  // !currentSig.empty() && !isNum(vAndSlice)
          if(currentSig == v+_sig) {
            toCout("WARNING: two signature found for one case assign!! First: "+currentSig+", second: "+v+_sig);
            // if two sig found, set overall sig to 0.
            currentSig = toStr(g_sig_width) + "'b0";
          }
        }
      }
    } // end of for loop
    if(!srcSigList.empty()) 
      srcSigList.pop_back();
    if(!srcSigList.empty()) 
      srcSigList.pop_back();
    addedVarItemNum.emplace(dest, srcVecItemNum);
    
    // _sig
    if(g_enable_taint && !g_use_value_change) {
      output << blank + "logic [" + toStr(srcVecItemNum*g_sig_width-1) + ":" + "0] " + dest + _sig + " ;" << std::endl;
      output << blank + "assign " + dest + _sig + " = { " + srcSigList + " };" << std::endl;
    }
  }
  else if(!g_use_value_change && g_enable_taint) { // if addedVar var is not used as RHS of case or the variable is not named addedVarXX
    // in this scenario, if dest is used in some select expression, we need to be careful with setting sig to 0.
    // First, we assume the above scenario does not happen.
    //if(g_selAssign.find(dest) != g_selAssign.end()) {
    //  toCout("!!! Warning: src_concat variable appear in sel op: "+ line);
    //  abort();
    //}
    output << blank + "logic [" + toStr(g_sig_width-1) + ":" + "0] " + dest + _sig + " ;" << std::endl;
    // when all the signatures of concatenation elements are 
    output << blank + "assign " + dest + _sig + " = 0 ;" << std::endl;
  }

  if(!g_enable_taint) return;  

  // other taints
  uint32_t startIdxNum;
  if(destSlice.empty()) {
    startIdxNum = destHighIdx;
  }
  else {
    startIdxNum = get_end(destSlice);
  }
  // TODO: using get_lhs_taint_list to replace this loop
  std::vector<std::string> collapsedVec;
  merge_vec(updateVec, collapsedVec);
  for (std::string updateAndSlice: collapsedVec) {
    std::string update, updateSlice;
    split_slice(updateAndSlice, update, updateSlice);
    uint32_t updateSliceWidthNum = get_var_slice_width(updateAndSlice);

    std::string startIdx = toStr(startIdxNum);
    std::string endIdx = toStr(startIdxNum - updateSliceWidthNum + 1);

    if(!isNum(update)) {
      auto updateIdxPair = varWidth.get_idx_pair(update, line);
      std::string updateHighIdx  = toStr(updateIdxPair.first);
      std::string updateLowIdx   = toStr(updateIdxPair.second);

      bool updateIsNew;
      uint32_t localVerNum = find_version_num(updateAndSlice, updateIsNew, output);
      std::string localVer = std::to_string(localVerNum); 

      if(updateIsNew) {
      output << blank + "logic [" + updateHighIdx + ":" + updateLowIdx + "] " + update + _r + localVer + " ;" << std::endl;
      }
      output << blank + "assign " + update + _r + localVer + updateSlice + " = " + dest + _r + " [" + startIdx + ":" + endIdx + "] ;" << std::endl;
    } 
    startIdxNum -= updateSliceWidthNum;
  }
}


// FIXME
// THIS IS NOT A GOOD SOLUTION:
// sig of all LHS are set to 0.
void both_concat_op_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;  
  //checkCond(!g_use_reset_sig, "both_concat not supported when use_reset_sig!!");
  std::smatch m;
  if( !std::regex_match(line, m, pSrcDestBothConcat) )
  //if(!is_srcDestConcat(line))
    abort(); //

  std::string blank = m.str(1);
  std::string destList = m.str(2);
  std::string srcList = m.str(3);

  std::vector<std::string> destVec;
  parse_var_list(destList, destVec);
  std::vector<std::string> srcVec;
  parse_var_list(srcList, srcVec);
  for(std::string src: srcVec) { 
    assert(!isMem(src));
    //assert_info(!isTop || !isOutput(src), "both_concat_taint_gen:"+src+" is output, line: "+line);  
  }

  //std::string destTList = std::regex_replace(destList, pVarNameGroup, "$1_t$3 ");
  std::string destTList = get_lhs_taint_list(destList, _t, output);
  std::string srcTList = get_rhs_taint_list(srcVec, _t);
  output << blank + "assign { " + destTList + " } = { " + srcTList + " };" << std::endl;

  if(!g_use_value_change) {
    std::string destSigList = get_lhs_taint_list_no_slice(destList, _sig, output);
    std::string srcSigList = get_rhs_taint_list(srcVec, _sig);
    output << blank + "assign { " + destSigList + " } = 0;" << std::endl;
  }
  
  // declare new taint wires for dests
  uint32_t destTotalWidthNum = 0;
  uint32_t newConcatVarIdx = NEW_VAR++;
  std::string newConcatVarIdxStr = toStr(newConcatVarIdx);
  for(std::string destAndSlice: destVec) {
    assert(!isMem(destAndSlice));  
    if(isNum(destAndSlice)) {
      toCout("Unexpected ");
      abort();
    }
    destTotalWidthNum += get_var_slice_width(destAndSlice);
  }
  std::string destTotalWidth = toStr(destTotalWidthNum);
  output << blank + "logic [" + destTotalWidth + "-1:0] newConcatVar" + newConcatVarIdxStr + _r+" ;" << std::endl;

  std::string destRList = get_rhs_taint_list(destList, _r);
  output << blank + "assign newConcatVar" + newConcatVarIdxStr + _r+" = { " + destRList + " };" << std::endl;
  
  uint32_t startIdx = destTotalWidthNum - 1;
  uint32_t endIdx;

  // check if there is number in the vec
  bool numExist = false;
  for (std::string srcAndSlice: srcVec) {
    if(isNum(srcAndSlice))
      numExist = true;
  }

  // declare taint wires
  std::string src, srcSlice;
  if(!numExist) {
    std::vector<uint32_t> verVec;
    std::vector<bool> isNewVec;
    get_ver_vec(srcVec, verVec, output);
    std::string srcRList = get_lhs_ver_taint_list(srcVec, _r, output, verVec);
    std::string srcSList = get_lhs_ver_taint_list(srcVec, _sig, output, verVec);

    output << blank + "assign { " + srcRList + " } = newConcatVar" + newConcatVarIdxStr + _r+" ;" << std::endl;
    return;
  }

  // if there is number in the src list
  for (std::string srcAndSlice: srcVec) {
    uint32_t srcLocalWidthNum = get_var_slice_width(srcAndSlice);
    endIdx = startIdx - srcLocalWidthNum + 1;
    if(!isNum(srcAndSlice)) {
      split_slice(srcAndSlice, src, srcSlice);

      auto srcIdxPair = varWidth.get_idx_pair(src, line);
      std::string srcHighIdx  = toStr(srcIdxPair.first);
      std::string srcLowIdx   = toStr(srcIdxPair.second);

      bool srcIsNew;
      std::string srcVer = toStr(find_version_num(srcAndSlice, srcIsNew, output));
      if(srcIsNew) {
      output << blank + "logic [" + srcHighIdx + ":" + srcLowIdx + "] " + src + _r + srcVer + " ;" << std::endl;
      }
      output << blank + "assign " + src + _r + srcVer + srcSlice + " = newConcatVar" + newConcatVarIdxStr + _r+" [" + toStr(startIdx) + ":" + toStr(endIdx) + "] ;" << std::endl;
    }
    startIdx = endIdx - 1;
  }
}


void dest_concat_op_taint_gen(std::string line, std::ofstream &output) { 
  if(!g_enable_taint) return;  
  //checkCond(!g_use_reset_sig, "both_concat not supported when use_reset_sig!!");
  std::smatch m;
  if( !std::regex_match(line, m, pDestConcat) )
  //if( !is_destConcat(line) )
    abort(); //

  std::string blank = m.str(1);
  std::string destList = m.str(2);
  std::string srcAndSlice = m.str(3);

  std::vector<std::string> destVec;
  parse_var_list(destList, destVec);
  std::string src, srcSlice;

  if(!isNum(srcAndSlice)) {
    split_slice(srcAndSlice, src, srcSlice);
    assert(!isMem(src));

    //std::string destTList = std::regex_replace(destList, pVarNameGroup, "$1_t$3 ");
    std::string destTList = get_lhs_taint_list(destList, _t, output);
    output << blank + "assign { " + destTList + " } = " + src + _t + srcSlice + " ;" << std::endl;

    std::string destSigList = get_lhs_taint_list_no_slice(destList, _sig, output);
    if(!g_use_value_change)
      output << blank + "assign { " + destSigList + " } = 0;" << std::endl;
    
    std::string destRList = get_rhs_taint_list(destList, _r);

    auto srcIdxPair = varWidth.get_idx_pair(src, line);
    std::string srcHighIdx = toStr(srcIdxPair.first);
    std::string srcLowIdx  = toStr(srcIdxPair.second);
    bool isNew;
    uint32_t localVerNum = find_version_num(srcAndSlice, isNew, output);
    std::string localVer = std::to_string(localVerNum);
    if(isNew) {
      output << blank + "logic [" + srcHighIdx + ":" + srcLowIdx + "] " + src + _r + localVer + " ;" << std::endl;
    }
    output << blank + "assign " + src + _r + localVer + srcSlice + " = { " + destRList + " };" << std::endl;
  }
  else {
    //std::string destTList = std::regex_replace(destList, pVarNameGroup, "$1_t$3 ");
    std::string destTList = get_lhs_taint_list(destList, _t, output);
    output << blank + "assign { " + destTList + " } = 0 ;" << std::endl;

    std::string destSigList = get_lhs_taint_list_no_slice(destList, _sig, output);
    if(!g_use_value_change)
      output << blank + "assign { " + destSigList + " } = 0;" << std::endl;
  }
}


void ite_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;  
  std::smatch m;
  if ( !std::regex_match(line, m, pIte) )
    return;
  assert(!m.str(3).empty());
  assert(!m.str(4).empty());
  assert(!m.str(5).empty());

  std::string blank = m.str(1);
  std::string destAndSlice = m.str(2);
  std::string dest, destSlice;
  std::string cond, condSlice;
  std::string op1, op1Slice;
  std::string op2, op2Slice;

  uint32_t localWidthNum;
  std::string localWidth;
  localWidthNum = get_var_slice_width(destAndSlice);
  //if(split_slice(m.str(2), dest, destSlice)) {
  //  localWidthNum = get_width(destSlice);
  //}
  //// if no slice found, use the vector whole width
  //else {
  //  localWidthNum = varWidth.get_from_var_width(dest, line);
  //}
  localWidth = std::to_string(localWidthNum);

  std::string condAndSlice = m.str(3);
  std::string op1AndSlice = m.str(4);
  std::string op2AndSlice = m.str(5);
  split_slice(destAndSlice, dest, destSlice);
  split_slice(condAndSlice, cond, condSlice);
  split_slice(op1AndSlice , op1, op1Slice);
  split_slice(op2AndSlice , op2, op2Slice);
  assert(!isMem(op1));    
  assert(!isMem(op2));

  auto destIdxPair = varWidth.get_idx_pair(dest, line);
  auto condIdxPair = varWidth.get_idx_pair(cond, line);
  auto op1IdxPair = varWidth.get_idx_pair(op1, line);
  auto op2IdxPair = varWidth.get_idx_pair(op2, line);
  std::string destHighIdx = toStr(destIdxPair.first);
  std::string destLowIdx  = toStr(destIdxPair.second);
  std::string condHighIdx = toStr(condIdxPair.first);
  std::string condLowIdx = toStr(condIdxPair.second);
  std::string op1HighIdx  = toStr(op1IdxPair.first);
  std::string op1LowIdx   = toStr(op1IdxPair.second);
  std::string op2HighIdx  = toStr(op2IdxPair.first);
  std::string op2LowIdx   = toStr(op2IdxPair.second);


  bool condIsNum = isNum(condAndSlice);
  bool op1IsNum = isNum(op1AndSlice);
  bool op2IsNum = isNum(op2AndSlice);

  bool condIsNew;
  uint32_t condVerNum;
  std::string condVer;
  if(!condIsNum) {
    condVerNum = find_version_num(condAndSlice, condIsNew, output);
    condVer = std::to_string(condVerNum);
  }

  if(condIsNew && !condIsNum) {
    output << blank << "logic [" + condHighIdx + ":" + condLowIdx + "] " + cond + _r + condVer + " ;" << std::endl;
  }
  
  uint32_t condSliceWidth = get_width(condSlice);

  // FIXME: the following may be wrong. The best way is to add this part to pass_info
  bool printSig = true;
  if(!condIsNum) output << blank << "assign " + cond + _r + condVer + condSlice + " = ( | "+dest+_r+destSlice+" ) && " + op1AndSlice + " != " + op2AndSlice + " ;" << std::endl;

  if (!op1IsNum && !op2IsNum) { // ite
    /* Assgin new versions */
    if(!condIsNum)
      output << blank << "assign " + dest + _t + destSlice + " = " + condAndSlice + " ? ( " + extend(cond+_t+" "+condSlice, localWidthNum) + " | " + op1 + _t + op1Slice + " ) : ( " + extend(cond+_t+" "+condSlice, localWidthNum) + " | " + op2 + _t + op2Slice + " );" << std::endl;
    else
      output << blank << "assign " + dest + _t + destSlice + " = " + condAndSlice + " ? ( " + op1 + _t + op1Slice + " ) : ( " + op2 + _t + op2Slice + " );" << std::endl;


    if(printSig && !g_use_value_change) 
      output << blank << "assign " + dest + _sig + " = " + condAndSlice + " ? " + op1 + _sig + " : " + op2 + _sig + " ;" << std::endl;


    uint32_t thdVerNum, fthVerNum;
    bool op1IsNew, op2IsNew;
    thdVerNum = find_version_num(op1AndSlice, op1IsNew, output);
    fthVerNum = find_version_num(op2AndSlice, op2IsNew, output);
    std::string thdVer = std::to_string(thdVerNum);        
    std::string fthVer = std::to_string(fthVerNum);

    if(op1IsNew) {
    output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + thdVer + " ;" << std::endl;
    }
    output << blank << "assign " + op1 + _r + thdVer + op1Slice + " = " + extend(condAndSlice, localWidthNum) + " & ( " + dest + _r + destSlice + " );" << std::endl;

    if(op2IsNew) {
    output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + fthVer + " ;" << std::endl;
    }

    output << blank << "assign " + op2 + _r + fthVer + op2Slice + " = " + extend("!"+condAndSlice, localWidthNum) + " & ( " + dest + _r + destSlice + " );" << std::endl; 

  } 
  else if (!op1IsNum && op2IsNum) { // ite
    assert(!condIsNum);
    bool op1IsNew;
    uint32_t thdVerNum = find_version_num(op1AndSlice, op1IsNew, output);
    std::string thdVer = std::to_string(thdVerNum);
    /* declare new wires */
    if(op1IsNew) {
    output << blank << "logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + thdVer + " ;" << std::endl;
    }
    output << blank << "assign " + op1 + _r + thdVer + op1Slice + " = " + extend(condAndSlice, localWidthNum) + " & ( " + dest + _r + destSlice + " );" << std::endl;    

    output << blank << "assign " + dest + _t + destSlice + " = " + condAndSlice + " ? ( " + extend("| "+cond+_t+" "+condSlice, localWidthNum) + " | " + op1 + _t + op1Slice + " ) : " + extend("| "+cond+_t+" "+condSlice, localWidthNum) + ";" << std::endl;

    if(std::stoi(op2) == 0 && !g_use_value_change) {
      if(printSig) output << blank + "assign " + dest + _sig + " = " + condAndSlice + " ? " + op1 + _sig + " : " + cond + _sig + " ;" << std::endl;
    }
    else if(!g_use_value_change){
      if(printSig) output << blank + "assign " + dest + _sig + " = " + condAndSlice + " ? " + op1 + _sig + " : 0 ;" << std::endl;
    }
  }
  else if (op1IsNum && !op2IsNum) { // ite
    assert(!condIsNum);    
    bool op2IsNew;    
    uint32_t fthVerNum = find_version_num(op2AndSlice, op2IsNew, output);
    std::string fthVer = std::to_string(fthVerNum);

    if(op2IsNew) {
    output << blank << "logic [" + op2HighIdx + ":" + op2LowIdx + "] " + op2 + _r + fthVer + " ;" << std::endl;
    }
    output << blank << "assign " + op2 + _r + fthVer + op2Slice + " = " + extend("!"+condAndSlice, localWidthNum) + " & ( " + dest + _r + destSlice + " );" << std::endl;


    output << blank << "assign " + dest + _t + destSlice + " = " + condAndSlice + " ? " + extend("| "+cond+_t+" "+condSlice, localWidthNum) + " : ( " + extend("| "+cond+_t+" "+condSlice, localWidthNum) + " | " + op2 + _t + op2Slice + " );" << std::endl;  
   
    if(std::stoi(op1) == 1 && !g_use_value_change) {    
      if(printSig) output << blank << "assign " + dest + _sig + " = " + condAndSlice + " ? " + cond + _sig + " : " + op2 + _sig + " ;" << std::endl;
    }
    else if(!g_use_value_change) {
      if(printSig) output << blank << "assign " + dest + _sig + " = " + condAndSlice + " ? 0 : " + op2 + _sig + " ;" << std::endl;
    }
  }
  else {
    assert(!condIsNum);    
    /* when both inputs are constants */
    output << blank << "assign " + dest + _t + destSlice + " = " + extend(cond+_t+" "+condSlice, localWidthNum) + " ;" << std::endl;
    if(printSig && !g_use_value_change) {
      uint32_t op1Num = std::stoi(op1);
      uint32_t op2Num = std::stoi(op2);
      if(op1Num == 1 && op2Num == 0)
        output << blank << "assign " + dest + _sig + " = " + cond + _sig + " ;" << std::endl;
      else if(op1Num == 1 && op2Num != 0)
        output << blank << "assign " + dest + _sig + " = " + condAndSlice + " ? " + cond + _sig + " : 0 ;" << std::endl; 
      else if(op1Num != 1 && op2Num == 0)
        output << blank << "assign " + dest + _sig + " = " + condAndSlice + " ? 0 : " + cond + _sig + " ;" << std::endl;
      else
        output << blank << "assign " + dest + _sig + " = 0 ;" << std::endl;
    }
  }
}


void nonblock_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;
  std::smatch m;
  if ( !std::regex_match(line, m, pNonblock) )
    return;
  std::string blank = m.str(1);
  std::string op1AndSlice = m.str(3);
  std::string destAndSlice = m.str(2);
  //assert_info(!isTop || !isOutput(op1AndSlice), "nonblock_taint_gen:"+op1AndSlice+" is output, line: "+line);  
  
  std::string dest, destSlice;
  std::string op1, op1Slice;
  split_slice(op1AndSlice, op1, op1Slice);
  split_slice(destAndSlice, dest, destSlice);

  if(isNum(op1AndSlice)) {
    toCout("Warning: constant number found in RHS of nonblocking: "+line);
    output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " ) begin" << std::endl;
    output << blank + dest + _t + " \t\t<= 0 ;" << std::endl;
    output << blank + dest + "_t_flag \t\t<= 0 ;" << std::endl;
    output << blank + dest + "_r_flag \t\t<= 0 ;" << std::endl;
    output << blank + "end" << std::endl;
    return;
  }

  // some designs have separate assignment to different bits of a reg
  // FIXME: the best way is to split dest[1:0] into two separate regs: \dest[1] and \dest[0]
  bool destSliceNotEmpty = false;
  if(!destSlice.empty()) {
    toCout("Warning: dest in nonblock has slice: "+line);
    destSliceNotEmpty = true;
  }
  assert(!isMem(dest));    
  assert(!isMem(op1));    

  uint32_t localWidthNum;
  std::string localWidth;
  localWidthNum = get_var_slice_width(destAndSlice);
  localWidth = std::to_string(localWidthNum);
  bool op1IsNew;
  uint32_t op1VerNum = find_version_num(op1AndSlice, op1IsNew, output);
  std::string op1Ver = toStr(op1VerNum);

  auto op1IdxPair = varWidth.get_idx_pair(op1);
  if(op1IsNew) {
  output << blank.substr(0, blank.length()-4) + "logic [" + toStr(op1IdxPair.first) + ":" + toStr(op1IdxPair.second) + "] " + op1 + _r + op1Ver + " ;" << std::endl;
  }

  std::string repeatCond;
  bool replaceSig = (g_regCondMap.find(dest) != g_regCondMap.end());
  if(replaceSig) {
    toCout("===================== WoW!!!! replaceSig is true for: "+dest);
  }
  if(replaceSig) {
    repeatCond = g_regCondMap[dest];
  }

  bool canKeepValue=true;
  // _r
  if(g_iteDest.find(op1) != g_iteDest.end())
    output << blank.substr(0, blank.length()-4) + "assign " + op1 + _r + op1Ver + op1Slice + " = " + op1 + _t + op1Slice + " & " + extend(dest+_sig+" != "+op1+_sig, localWidthNum) + " ;" << std::endl;
  else {
    canKeepValue = false;
    output << blank.substr(0, blank.length()-4) + "assign " + op1 + _r + op1Ver + op1Slice + " = 0 ;" << std::endl;
    std::ofstream notASVOutput;
    notASVOutput.open("./not_asv.txt", std::fstream::app);
    notASVOutput << dest << std::endl;
    notASVOutput.close();
  }

  // _t
  std::string taintRst;
  if(g_use_taint_rst) taintRst = "|| "+TAINT_RST+" ";
  std::string dest_t_Or = "";
  if(g_wt_keeped) dest_t_Or = "( INSTR_IN_ZY ? 0 :" + dest + _t + destSlice + " ) | ";
  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;
  if(!replaceSig) {
    output << blank + dest + _t + destSlice + " \t\t<= rst_zy " + taintRst + "? 0 : ( " + op1 + _t + " & ( " + dest_t_Or + extend(dest+_sig+" != "+op1+_sig, localWidthNum) + " ));" << std::endl;
  }
  else {
    output << blank + dest + _t + destSlice + " \t\t<= rst_zy " + taintRst + "? 0 : ( " + op1 + _t + " & ( " + dest_t_Or + extend( "!("+repeatCond+")", localWidthNum ) + " ));" << std::endl;
  }


  // _t_flag
  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;
  if(!replaceSig) {
    if(!g_use_zy_count)
      output << blank + dest + "_t_flag \t<= rst_zy " + taintRst + "? 0 : " + dest + "_t_flag ? 1 : | " + op1 + _t + " & ( " + dest+_sig+" != "+op1+_sig + " );" << std::endl;
    else
      output << blank + dest + "_t_flag \t<= ( rst_zy "+taintRst+"|| zy_count < 50 ) ? 0 : " + dest + "_t_flag ? 1 : | " + op1 + _t + " & ( " + dest+_sig+" != "+op1+_sig + " );" << std::endl;
  }
  else {
    if(!g_use_zy_count)
      output << blank + dest + "_t_flag \t<= rst_zy "+taintRst+"? 0 : " + dest + "_t_flag ? 1 : | " + op1 + _t + " & !( "+repeatCond+" );" << std::endl;
    else
      output << blank + dest + "_t_flag \t<= ( rst_zy "+taintRst+"|| zy_count < 50 ) ? 0 : " + dest + "_t_flag ? 1 : | " + op1 + _t + " & !( "+repeatCond+" );" << std::endl;
  }


  // _r_flag
  std::string neqRst = "";
  if(g_set_rflag_if_not_rst_val) {
    if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
      toCout("Warning: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
      neqRst = " && ("+dest+" != 0";
    }
    else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
      neqRst = " && ("+dest+" != 0";
    else
      neqRst = " && ("+dest+" != "+g_rstValMap[moduleName][dest];
    if(g_set_rflag_if_not_norm_val) {
      if(g_normValMap[moduleName].find(dest) != g_normValMap[moduleName].end())
        neqRst = neqRst + " && "+dest+" != "+g_normValMap[moduleName][dest];
    }
    neqRst = neqRst + " )";
  }
  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;
  if(!g_use_zy_count)
    output << blank + dest + "_r_flag \t<= rst_zy " + taintRst + "? 0 : " + dest + "_r_flag ? 1 : " + dest + "_t_flag ? 0 : ( | " + dest + _r + neqRst + " ) ;" << std::endl;
  else
    output << blank + dest + "_r_flag \t<= ( rst_zy "+taintRst+"|| zy_count < 50 ) ? 0 : " + dest + "_r_flag ? 1 : " + dest + "_t_flag ? 0 : ( | " + dest + _r + neqRst + " ) ;" << std::endl;

  // _dest is clear either write taint arrives or the value for dest is changed.
  if(g_use_reset_taint) {
    output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;    
    // FIXME: replace get_recent_rst() with rst_zy?    
    if (g_hasRst)
      output << blank + dest + "_reset \t<= " + get_recent_rst() + " ? 1 : " + destAndSlice+" != "+op1AndSlice + " ? 0 : " + dest + "_reset ;" << std::endl;
    else
      output << blank + dest + "_reset \t<= rst_zy ? 1 : " + destAndSlice+" != "+op1AndSlice + " ? 0 : " + dest + "_reset ;" << std::endl;
  }

  // assign NEVER_KEEP
  if(g_use_does_keep) {
    output << "  always @(posedge " + g_recentClk +")" << std::endl;
    output << "    " + dest + "_DOES_KEEP <= rst_zy ? 0 : " + dest + "_DOES_KEEP || (INSTR_IN_ZY && " + dest + _sig + " == " + op1 + _sig + ") ;" << std::endl;
  }

}


void nonblock_gate_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;  
  std::smatch m;
  if ( !std::regex_match(line, m, pNonblock) )
    return;
  // assume the LHS is always gate signal
  std::string gate = m.str(2);
  std::string gateUpdate = m.str(3);
  // because the gate is only used to be anded with clk, its write taint does not matter.
  output << "  assign " + gate + _t + " = 0;" << std::endl;
  // also gateUpdate_c0 == 0, _r0 == 0, _x0 == 0, all are disregarded
  // the gate should never be ASV. That is because I assume there is no instruction to directly set the gate.
  //Or in another word, all gate are set and read in the same instruction.
  output << "  assign " + gate + "_r_flag = 0;" << std::endl;
  output << "  assign " + gate + "_t_flag = 0;" << std::endl;
}


void nonblockconcat_taint_gen(std::string line, std::ofstream &output) {
  if(!g_enable_taint) return;
  checkCond(!g_use_reset_sig, "nonblockconcat not supported for use_reset_sig!!");
  std::smatch m;
  if ( !std::regex_match(line, m, pNonblockConcat) )
    return;
  std::string blank = m.str(1);
  std::string updateList = m.str(3);
  std::string destAndSlice = m.str(2);

  std::string dest, destSlice;
  split_slice(destAndSlice, dest, destSlice);
  checkCond(destSlice.empty(), "dest has slice in nonblockconcat: "+line);
  assert(!isMem(dest));
  std::vector<std::string> updateVec;
  parse_var_list(updateList, updateVec);
  for(auto update: updateVec) {
    assert(!isMem(update));
    //assert_info(!isTop || !isOutput(update), "nonblockconcat_taint_gen:"+update+" is output, line: "+line);
  }

  uint32_t localWidthNum = get_var_slice_width(destAndSlice);
  std::string localWidth;
  localWidth = std::to_string(localWidthNum);  

  std::string updateRListLeft   = get_lhs_taint_list(updateList, _r, output);
  //std::string updateXListLeft   = get_lhs_taint_list(updateList, _x, output);
  //std::string updateCListLeft   = get_lhs_taint_list(updateList, _c, output);
  std::string updateTList = get_rhs_taint_list(updateList, _t);

  output << blank.substr(0, blank.length()-4) + "assign { " + updateRListLeft + " } = 0 ;" << std::endl;

  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;
  // TODO: assume in this case, it is impossible that all the RHS has the same sig as dest
  std::string sigCheck = "";
  std::string taintRst;  
  if(g_use_taint_rst) taintRst = "|| "+TAINT_RST+" ";  
  std::string dest_t_Or = "";
  //if(g_wt_keeped) dest_t_Or = dest + _t + " | ";
  if(g_wt_keeped) dest_t_Or = "( INSTR_IN_ZY ? 0 :" + dest + _t + " ) | ";
  if(!g_use_value_change)
    sigCheck = dest+_sig+" != { "+updateList+" } | ";

  output << blank + dest + _t + " \t\t<= rst_zy "+taintRst+"? 0 : ({ " + updateTList +" } & ( " + dest_t_Or + extend(sigCheck+destAndSlice+" != "+updateList, localWidthNum) + " ));" << std::endl;

  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;
  output << blank + dest + "_t_flag \t<= rst_zy "+taintRst+"? 0 : " + dest + "_t_flag ? 1 : | { " + updateTList + " } & ( " + sigCheck +destAndSlice+" != "+updateList + ");" << std::endl;    

  output << blank.substr(0, blank.length()-4) + "always @( posedge " + g_recentClk + " )" << std::endl;

  // r_flag
  std::string neqRst = "";
  if(g_set_rflag_if_not_rst_val) {
    if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
      toCout("Warning: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
      neqRst = " && ("+dest+" != 0";
    }
    else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
      neqRst = " && ("+dest+" != 0";
    else
      neqRst = " && ("+dest+" != "+g_rstValMap[moduleName][dest];
    if(g_set_rflag_if_not_norm_val) {
      if(g_normValMap[moduleName].find(dest) != g_normValMap[moduleName].end())
        neqRst = neqRst + " && "+dest+" != "+g_normValMap[moduleName][dest];
    }
    neqRst = neqRst + " )";
  }
  if(!g_use_zy_count)
    output << blank + dest + "_r_flag \t<= rst_zy "+taintRst+"? 0 : " + dest + "_r_flag ? 1 : " + dest + "_t_flag ? 0 : ( | " + dest + _r + neqRst + " ) ;" << std::endl;
  else
    output << blank + dest + "_r_flag \t<= ( rst_zy || zy_count < 50 ) ? 0 : " + dest + "_r_flag ? 1 : " + dest + "_t_flag ? 0 : ( | " + dest + _r + neqRst + " ) ;" << std::endl;
}


// FIXME: the condition var should have statements for _R !!!
void nonblockif_taint_gen(std::string line, std::string always_line, std::ifstream &input, std::ofstream &output) {
  toCout("Error: nonblockif_taint_gen is not supported any more! should've been convertd to ite");
  if(!g_enable_taint) return;  
  checkCond(!g_use_reset_sig, "nonblockif not supported for use_reset_sig!!");  
  std::smatch m;
  if ( !std::regex_match(line, m, pNonblockIf) )
    return;

  if(g_use_zy_count)
    checkCond(false, "ERROR: encounter nonblockif whiling use zy_count! "+line);

  if(line.find("ap_CS_fsm") != std::string::npos)
    toCoutVerb("Find it!");

  //output << always_line << std::endl;
  bool hasRst = false;
  std::string rst;
  std::string blank;
  std::string condAndSlice;
  std::string destAndSlice;
  std::string srcAndSlice;
  std::string dest, destSlice;
  std::string src, srcSlice;
  std::string cond, condSlice;
  bool isFirstLine = true;
  std::vector<std::pair<std::string, std::string>> condAndSrcVec;
  std::string taintRst;
  if(g_use_taint_rst) taintRst = "|| "+TAINT_RST+" ";
  std::string dest_t_Or = "";

  std::streampos checkPoint;
  do {
    checkPoint = input.tellg();  
    if (isFirstLine)
      isFirstLine = false;
    else
      output << line << std::endl;
    blank = m.str(1);
    condAndSlice = m.str(2);
    destAndSlice = m.str(3);
    srcAndSlice = m.str(4);
    //assert_info(!isTop || !isOutput(srcAndSlice), "nonblockif_taint_gen:"+srcAndSlice+" is output, line: "+line);    

    split_slice(destAndSlice, dest, destSlice);
    split_slice(srcAndSlice, src, srcSlice);
    split_slice(condAndSlice, cond, condSlice);
    //assert_info(isMem(dest), "dest is: "+dest);

    uint32_t localWidthNum = get_var_slice_width(destAndSlice);

    // assume: if src is num, the cond must be rst.
    if(g_wt_keeped) dest_t_Or = "( INSTR_IN_ZY ? 0 :" + dest + _t + " " + destSlice + " ) | ";

    if(isNum(src)) {
      hasRst = true;
      rst = condAndSlice;
      // FIXME: may need to add assignment to _t here
      //output << always_line << std::endl;
      //output << blank + "if (" + condAndSlice + ") " + dest + "_t_flag " + destSlice + " <= 0 ;" << std::endl;
      //output << always_line << std::endl;
      //output << blank + "if (" + condAndSlice + ") " + dest + "_r_flag " + destSlice + " <= 0 ;" << std::endl;

      uint32_t localWidthNum = get_var_slice_width(srcAndSlice);
      // FIXME: replace get_recent_rst() with rst_zy?
      if(g_use_value_change) {
        output << always_line << std::endl;      
        output << blank + "if (" + get_recent_rst() + taintRst + ") " + dest + _t + " " + destSlice + " <= 0 ;" << std::endl;

        output << always_line << std::endl;
        output << blank + "if ( " + condAndSlice + " ) " + dest + _t + " " + destSlice + " <= ( " + extend(cond+_t+" "+condSlice, localWidthNum) + " ) & ( " + extend(destAndSlice+" != "+srcAndSlice, localWidthNum) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;        
        output << blank + "else if ( " + condAndSlice + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + cond+_t+" "+condSlice + " ) & ( " + destAndSlice + " != " + srcAndSlice + " );" << std::endl;
      }
      else {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;
        output << blank + "else if ( " + condAndSlice + " ) " + dest + _t + " " + destSlice + " <= ( " + extend(cond+_t+" "+condSlice, localWidthNum) + " ) & ( " + dest_t_Or + extend( dest+_sig+" != 0", localWidthNum ) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;        
        output << blank + "else if ( " + condAndSlice + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + cond+_t+" "+condSlice + " ) & ( " + dest+_sig+" != 0 );" << std::endl;
      }

      std::string neqRst = "";
      std::smatch m;      
      if(g_set_rflag_if_not_rst_val) {
        if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
          toCout("Error: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
          neqRst = " && "+dest+" "+destSlice+" != 0";          
        }
        else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
          neqRst = " && "+dest+" "+destSlice+" != 0";          
        else
          neqRst = " && "+dest+" "+destSlice+" != "+g_rstValMap[moduleName][dest];
      }
      output << always_line << std::endl;  
      output << blank + "if(rst_zy) " + dest + "_r_flag <= 0;" << std::endl;
      output << blank + "else if ( " + condAndSlice + " ) " + dest + "_r_flag " + destSlice + " <= " + dest + "_r_flag " + destSlice + " ? 1 : " + dest + "_t_flag " + destSlice + " ? 0 : ( |" + dest + _r + " " + destSlice + neqRst + " ) ;" << std::endl;
    }
    else {
      condAndSrcVec.push_back(std::make_pair(condAndSlice, srcAndSlice));
      uint32_t localWidthNum = get_var_slice_width(srcAndSlice);
      // FIXME: replace get_recent_rst() with rst_zy?
      output << always_line << std::endl;      
      output << blank + "if (" + get_recent_rst() + taintRst + ") " + dest + _t + " " + destSlice + " <= 0 ;" << std::endl;

      if(g_use_value_change) {
        output << always_line << std::endl;        
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if (" + condAndSlice + ") " + dest + _t + " " + destSlice + " <= ( " + src + _t+" " + srcSlice + " | " + extend(cond+_t+" "+condSlice, localWidthNum) + " ) & ( " + extend(destAndSlice+" != "+srcAndSlice, localWidthNum) + " );" << std::endl;

        output << always_line << std::endl;        
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;        
        output << blank + "else if (" + condAndSlice + ") " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + src + _t+" " + srcSlice + " | " + extend(cond+_t+" "+condSlice, localWidthNum) + " ) & ( " + destAndSlice + " != " + srcAndSlice + " );" << std::endl;
      }
      else {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if (" + condAndSlice + ") " + dest + _t + " " + destSlice + " <= ( " + src + _t+" " + srcSlice + " | " + extend(cond+_t+" "+condSlice, localWidthNum) + " ) & ( " + dest_t_Or + extend( dest+_sig+" != "+src+_sig, localWidthNum ) + " );" << std::endl;

        output << always_line << std::endl; 
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;        
        output << blank + "else if (" + condAndSlice + ") " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : ( ( |" + src + _t + " " + srcSlice + " ) || " + cond+_t+" "+condSlice + " ) & ( " + dest+_sig+" != "+src+_sig + " );" << std::endl;
      }

      std::string neqRst = "";
      std::smatch m;      
      if(g_set_rflag_if_not_rst_val) {
        if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
          toCout("Error: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
          neqRst = " && "+dest+" "+destSlice+" != 0";          
        }
        else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
          neqRst = " && "+dest+" "+destSlice+" != 0";
        else
          neqRst = " && "+dest+" "+destSlice+" != "+g_rstValMap[moduleName][dest];
      }
      output << always_line << std::endl;
      output << blank + "if(rst_zy) " + dest + "_r_flag <= 0;" << std::endl;      
      output << blank + "else if (" + condAndSlice + ") " + dest + "_r_flag " + destSlice + " <= " + dest + "_r_flag " + destSlice + " ? 1 : " + dest + "_t_flag " + destSlice + " ? 0 : ( |" + dest + _r + " " + destSlice + neqRst + " ) ;" << std::endl;
    }
  } while( std::getline(input, line) && std::regex_match(line, m, pNonblockIf) );

  //input.seekg(checkPoint);

  // deal with the next line
  std::string elseDestAndSlice;
  std::string elseSrcAndSlice;
  std::string elseCondAndSlice;  
  std::string elseCond, elseCondSlice;  
  bool isElse, isElseIf;
  isElse = std::regex_match(line, m, pNonblockElse);
  uint32_t newVarIdx = NEW_FANGYUAN++;  
  if(!isElse) isElseIf = std::regex_match(line, m, pNBElseIf);
  if( isElse || isElseIf ) {
    output << "  wire NewCond"+toStr(newVarIdx)+";" << std::endl;
    output << "  wire NewCond"+toStr(newVarIdx)+_t+";" << std::endl;
    if(condAndSlice.front() != '!') {
      output << "  assign NewCond"+toStr(newVarIdx)+" = ! "+condAndSlice+" ;" << std::endl;
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = "+cond+_t+condSlice+" ;" << std::endl;        
    }
    else if(condAndSlice.substr(0, 2) != "!!") {
      output << "  assign NewCond"+toStr(newVarIdx)+" = "+condAndSlice.substr(1)+" ;" << std::endl;
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = "+cond.substr(1)+_t+condSlice+" ;" << std::endl;        
    }
    else {
      output << "  assign NewCond"+toStr(newVarIdx)+" = "+condAndSlice.substr(1)+" ;" << std::endl;
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = "+cond.substr(2)+_t+condSlice+" ;" << std::endl;  
    }

    if(isElse) {
      elseDestAndSlice = m.str(2);
      elseSrcAndSlice = m.str(3);
      split_slice(elseSrcAndSlice, src, srcSlice);
      assert(elseDestAndSlice == destAndSlice);
      elseCondAndSlice = "!"+condAndSlice;
      split_slice(condAndSlice, cond, condSlice);
    }
    else {
      elseDestAndSlice = m.str(3);
      elseSrcAndSlice = m.str(4);
      elseCondAndSlice = m.str(2);
    }
    split_slice(elseSrcAndSlice, src, srcSlice);
    split_slice(elseCondAndSlice, elseCond, elseCondSlice);

    newVarIdx = NEW_FANGYUAN++;
    output << "  wire NewCond"+toStr(newVarIdx)+";" << std::endl;
    output << "  assign NewCond"+toStr(newVarIdx)+" = NewCond"+toStr(newVarIdx-1)+" && "+elseCondAndSlice+" ;" << std::endl;
    output << "  wire NewCond"+toStr(newVarIdx)+_t+";" << std::endl;
    if(elseCond.front() != '!')
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = NewCond"+toStr(newVarIdx-1)+_t+" | "+elseCond+_t+elseCondSlice+" ;" << std::endl;
    else if(elseCond.substr(0, 2) != "!!")
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = NewCond"+toStr(newVarIdx-1)+_t+" | "+elseCond.substr(1)+_t+elseCondSlice+" ;" << std::endl;
    else
      output << "  assign NewCond"+toStr(newVarIdx)+_t+" = NewCond"+toStr(newVarIdx-1)+_t+" | "+elseCond.substr(2)+_t+elseCondSlice+" ;" << std::endl;
    output << always_line << std::endl;
    output << blank + "if ( " + "NewCond"+toStr(newVarIdx) + " ) " + destAndSlice + " <= " + elseSrcAndSlice + " ;" << std::endl;

    if( isNum(src) ) {
      hasRst = true;
      toCout("Warning: assignment for else branch is num: "+line);
      //abort();
      //output << blank + "else " + dest + "_t_flag " + destSlice + " <= 0 ;" << std::endl;
      //output << blank + "else " + dest + "_r_flag " + destSlice + " <= 0 ;" << std::endl;

      uint32_t localWidthNum = get_var_slice_width(elseSrcAndSlice);
      // FIXME: replace get_recent_rst() with rst_zy?
      if(g_use_value_change) {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + _t + " " + destSlice + " <= ( " + extend("NewCond"+toStr(newVarIdx)+_t, localWidthNum) + " ) & ( " + extend(destAndSlice+" != "+elseSrcAndSlice, localWidthNum) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;        
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + "NewCond"+toStr(newVarIdx)+_t + " ) & ( " + destAndSlice + " != " + elseSrcAndSlice + " );" << std::endl;
      }
      else {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + _t + " " + destSlice + " <= ( " + extend("NewCond"+toStr(newVarIdx)+_t, localWidthNum) + " ) & ( " + dest_t_Or + extend( dest+_sig+" != 0", localWidthNum ) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;                
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + "NewCond"+toStr(newVarIdx)+_t + " ) & ( " + dest+_sig+" != 0 );" << std::endl;
      }

      std::string neqRst = "";
      std::smatch m;      
      if(g_set_rflag_if_not_rst_val) {
        if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
          toCout("Error: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
          neqRst = " && "+dest+" "+destSlice+" != 0";          
        }
        else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
          neqRst = " && "+dest+" "+destSlice+" != 0";
        else
          neqRst = " && "+dest+" "+destSlice+" != "+g_rstValMap[moduleName][dest];
      }
      output << always_line << std::endl; 
      output << blank + "if(rst_zy) " + dest + "_r_flag <= 0;" << std::endl;      
      output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_r_flag " + destSlice + " <= " + dest + "_r_flag " + destSlice + " ? 1 : " + dest + "_t_flag " + destSlice + " ? 0 : ( |" + dest + _r + " " + destSlice + neqRst + " ) ;" << std::endl;


    }
    else {
      condAndSrcVec.push_back(std::make_pair("NewCond"+toStr(newVarIdx), elseSrcAndSlice));      
      uint32_t localWidthNum = get_var_slice_width(elseSrcAndSlice);
      // FIXME: replace get_recent_rst() with rst_zy?
      if(g_use_value_change) {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + _t + " " + destSlice + " <= ( " + src + _t +" " + srcSlice + " | " + extend("NewCond"+toStr(newVarIdx)+_t, localWidthNum) + " ) & ( " + extend(destAndSlice+" != "+elseSrcAndSlice, localWidthNum) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;                
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : ( ( |" + src + _t +" " + srcSlice + " ) || " + "NewCond"+toStr(newVarIdx)+_t + " ) & ( " + destAndSlice+" != "+elseSrcAndSlice + " );" << std::endl;
      }
      else {
        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + _t + " " + destSlice + " <= 0;" << std::endl;        
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + _t + " " + destSlice + " <= ( " + src + _t+" " + srcSlice + " | " + extend("NewCond"+toStr(newVarIdx)+_t, localWidthNum) + " ) & ( " + dest_t_Or + extend( dest+_sig+" != "+src+_sig, localWidthNum ) + " );" << std::endl;

        output << always_line << std::endl;
        output << blank + "if(rst_zy) " + dest + "_t_flag <= 0;" << std::endl;                
        output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : ( ( |" + src + _t + " " + srcSlice + " ) || " + "NewCond"+toStr(newVarIdx)+_t + " ) & ( " + dest+_sig+" != "+src+_sig + " );" << std::endl;
      }

      std::string neqRst = "";
      std::smatch m;        
      if(g_set_rflag_if_not_rst_val) {
        if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
          toCout("Error: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
          neqRst = " && "+dest+" "+destSlice+" != 0";          
        }
        else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
          neqRst = " && "+dest+" "+destSlice+" != 0"; 
        else
          neqRst = " && "+dest+" "+destSlice+" != "+g_rstValMap[moduleName][dest];
      }
      output << always_line << std::endl;
      output << blank + "if(rst_zy) " + dest + "_r_flag <= 0;" << std::endl;      
      output << blank + "else if ( NewCond"+toStr(newVarIdx) + " ) " + dest + "_r_flag " + destSlice + " <= " + dest + "_r_flag " + destSlice + " ? 1 : " + dest + "_t_flag " + destSlice + " ? 0 : ( |" + dest + _r + " " + destSlice + neqRst + " ) ;" << std::endl;
    }
  }
  else if( std::regex_match(line, m, pEnd) ) {
    output << line << std::endl; // this is "end"  
  }
  // if the next line is not else/elsif/end, 
  // then recover the seek point
  else input.seekg(checkPoint);


  //if(hasRst)
  //  output << blank + "if (" + rst + taintRst + ") " + dest + "_r_flag_top <= 0;" << std::endl;
  for(auto pair: condAndSrcVec) {
    //  FIXME: add option for pNonblockElse
    condAndSlice = pair.first;
    srcAndSlice = pair.second;
    split_slice(srcAndSlice, src, srcSlice);
    split_slice(condAndSlice, cond, condSlice);

    
    uint32_t localWidthNum = get_var_slice_width(srcAndSlice);
    bool srcIsNew;
    std::string srcVer = toStr(find_version_num(srcAndSlice, srcIsNew, output));
    if(srcIsNew) {
      auto srcIdxPair = varWidth.get_idx_pair(src, line);
      output << "  logic [" + toStr(srcIdxPair.first) + ":" + toStr(srcIdxPair.second) + "] " + src + _r + srcVer + " ;" << std::endl;
    }
    output << "  assign " + src + _r + srcVer + srcSlice + " = " + extend(condAndSlice, localWidthNum) + " & " + src + _t + srcSlice + " ;" << std::endl; 
  }
}


void nonblockif2_taint_gen(std::string line, std::string always_line, std::ifstream &input, std::ofstream &output) {
  if(!g_enable_taint) return;  
  checkCond(!g_use_reset_sig, "nonblockif not supported for use_reset_sig!!");  
  std::smatch m;
  if ( !std::regex_match(line, m, pNonblockIf2) )
    return;

  if(g_use_zy_count)
    checkCond(false, "ERROR: encounter nonblockif whiling use zy_count! "+line);
  //output << always_line << std::endl;
  bool hasRst = false;
  std::string rst;
  std::string blank;
  std::string condAndSlice;
  std::string dest;
  std::string destSlice;
  std::string idxVar;
  std::string idxVarBit;
  std::string cond, condSlice;
  std::string srcAndSlice;
  std::string src, srcSlice;
  bool isFirstLine = true;
  std::vector<std::string> nonConstAssign;
  std::string taintRst;  
  if(g_use_taint_rst) taintRst = "|| "+TAINT_RST+" ";

  do{
    if (isFirstLine)
      isFirstLine = false;
    else
      output << line << std::endl;
    blank = m.str(1);
    condAndSlice = m.str(2);
    dest = m.str(3);
    destSlice = m.str(4);
    idxVar = m.str(5);
    idxVarBit = m.str(6);
    srcAndSlice = m.str(7);
    //assert_info(!isTop || !isOutput(srcAndSlice), "nonblockif_taint_gen:"+srcAndSlice+" is output, line: "+line);    

    split_slice(srcAndSlice, src, srcSlice);
    split_slice(condAndSlice, cond, condSlice);
    assert_info(isMem(dest), "dest is: "+dest);

    nonConstAssign.push_back(line);
    uint32_t localWidthNum = get_var_slice_width(srcAndSlice);
    // FIXME: replace get_recent_rst() with rst_zy?    
    output << blank + "if (" + get_recent_rst() + taintRst + ") " + dest + _t + " <= 0 ;" << std::endl;
    // for mem always use value change
    output << blank + "if (" + condAndSlice + ") " + dest + _t + " " + destSlice + " <= ( " + src + _t + " " + srcSlice + " | " + extend(cond+_t+" "+condSlice, localWidthNum) + " | " + extend(idxVar+_t, localWidthNum) + " ) & ( " + dest + destSlice + " != " + srcAndSlice + " );" << std::endl;
    output << blank + "if (" + condAndSlice + ") " + dest + "_t_flag " + destSlice + " <= " + dest + "_t_flag " + destSlice + " ? 1 : (" + src + _t+" " + srcSlice + " | " + extend(cond+_t+" "+condSlice, localWidthNum) + " | " + extend(idxVar+_t, localWidthNum) + " ) & ( " + dest + destSlice + " != " + srcAndSlice + " );" << std::endl;

    std::string neqRst = "";
    std::smatch m;
    if(g_set_rflag_if_not_rst_val) {
      if(g_rstValMap[moduleName].find(dest) == g_rstValMap[moduleName].end()) {
        toCout("Error: cannot find in g_rstValMap for module: "+moduleName+", var: "+dest);
        neqRst = " && "+dest+" "+destSlice+" != 0";          
      }
      else if(std::regex_match(g_rstValMap[moduleName][dest], m, pX))
        neqRst = " && "+dest+" "+destSlice+" != 0"; 
      else
        neqRst = " && "+dest+" "+destSlice+" != "+g_rstValMap[moduleName][dest];
    }
    output << blank + "if (" + condAndSlice + ") " + dest + "_r_flag " + destSlice + " <= " + dest + "_r_flag " + destSlice + " ? 1 : " + dest + "_t_flag " + destSlice + " ? 0 : ( |" + dest + _r + " " + destSlice + neqRst + " ) ;" << std::endl;
  } while( std::getline(input, line) && std::regex_match(line, m, pNonblockIf2) );
  assert( std::regex_match(line, m, pEnd) );
  if(hasRst)
    output << blank + "if (" + rst + taintRst + ") " + dest + "_r_flag_top <= 0;" << std::endl;
  output << line << std::endl; // this is "end"
  for(std::string line: nonConstAssign) {
    std::regex_match(line, m, pNonblockIf2);
    condAndSlice = m.str(2);
    dest = m.str(3);
    destSlice = m.str(4);
    idxVar = m.str(5);
    idxVarBit = m.str(6);
    srcAndSlice = m.str(7);

    split_slice(srcAndSlice, src, srcSlice);
    split_slice(condAndSlice, cond, condSlice);
    
    uint32_t localWidthNum = get_var_slice_width(srcAndSlice);
    bool srcIsNew;
    std::string srcVer = toStr(find_version_num(srcAndSlice, srcIsNew, output));
    if(srcIsNew) {
      auto srcIdxPair = varWidth.get_idx_pair(src, line);
      output << "  logic [" + toStr(srcIdxPair.first) + ":" + toStr(srcIdxPair.second) + "] " + src + _r + srcVer + " ;" << std::endl;
    }
    output << "  assign " + src + _r + srcVer + srcSlice + " = " + extend(condAndSlice, localWidthNum) + " & " + src + _t + srcSlice + " ;" << std::endl; 
  }
}


void always_fake_taint_gen(std::string firstLine, std::ifstream &input, std::ofstream &output) {
  std::string line;
  std::smatch m;
  std::getline(input, line);
  if(!g_enable_taint) return;    
  
  assert_info( std::regex_match(line, m, pEnd), "Error: expected end for always_fake" );
  output << line << std::endl;
}


// this expression is usually for mem with gated clock. The gated clock is the signal in if()
// Therefore, we do not consider cond_t here. Because cond is gated clock signal, those variables
// that composing it should not be treated as "read"(FIXME)
void always_star_taint_gen(std::string firstLine, std::ifstream &input, std::ofstream &output) {
  if(g_enable_taint) {
    toCout("Error: gated clock not supported yet: "+firstLine);
    abort();
  }
  std::string ifLine;
  std::string assignLine;
  std::getline(input, ifLine);
  std::getline(input, assignLine);
  if(!g_enable_taint) return;    
  
  std::smatch m;
  // process ifLine
  if(!std::regex_match(ifLine, m, pIf)) {
    toCout("Error: the if line does not match pIf: "+ifLine);
    abort();
  }
  std::string condAndSlice = m.str(2);
  std::string cond, condSlice;
  split_slice(condAndSlice, cond, condSlice);
  // cond_r
  bool condIsNew;
  uint32_t condVerNum = find_version_num(condAndSlice, condIsNew, output);
  std::string condVer = toStr(condVerNum);
  auto condIdxPair = varWidth.get_idx_pair(cond, assignLine);
  std::string condHighIdx  = toStr(condIdxPair.first);
  std::string condLowIdx   = toStr(condIdxPair.second);


  // process assignLine
  if(!std::regex_match(assignLine, m, pNoneNoAssign)) {
    toCout("Error: the assign line does not match pNoneNoAssign: "+assignLine);
    abort();
  }
  std::string destAndSlice = m.str(2);
  std::string op1AndSlice = m.str(3);
  std::string dest, destSlice;
  std::string op1, op1Slice;
  split_slice(destAndSlice, dest, destSlice);
  split_slice(op1AndSlice, op1, op1Slice);
  uint32_t destWidth = get_var_slice_width(destAndSlice);

  //_r
  bool op1IsNew;
  uint32_t op1VerNum = find_version_num(op1AndSlice, op1IsNew, output);
  std::string op1Ver = toStr(op1VerNum);
  auto op1IdxPair = varWidth.get_idx_pair(op1, assignLine);
  std::string op1HighIdx  = toStr(op1IdxPair.first);
  std::string op1LowIdx   = toStr(op1IdxPair.second);

  if(op1IsNew) {
    output << "  logic [" + op1HighIdx + ":" + op1LowIdx + "] " + op1 + _r + op1Ver + " ;" << std::endl;
  }

  output << "  always @* begin" << std::endl;
  output << "    " + dest + _t + destSlice + " = 0 ;" << std::endl;  
  output << "    " + op1 + _r + op1Ver + " = 0 ;" << std::endl;
  output << ifLine + " begin" << std::endl;
  output << assignLine << std::endl;
  output << "      " + dest + _t + destSlice + " = " + op1 + _t + op1Slice + " ;" << std::endl;
  output << "      " + op1 + _r + op1Ver + " = " + dest + _r + destSlice + " ;" << std::endl;
  output << "    end" << std::endl;
  output << "  end" << std::endl;
  //TODO: taint for cond!!
} 


void always_taint_gen(std::string firstLine, std::ifstream &input, std::ofstream &output) {
  if(!g_enable_taint) return;  
  std::smatch m;
  if( !std::regex_match(firstLine, m, pAlwaysClk) ) {
    std::cout << "!! Error in parsing always with clk & rst !!" << std::endl;
    abort();
  }
  g_recentClk = m.str(2);
  g_clk_set.insert(g_recentClk);
  std::string line;

  // parse first assignment
  std::getline(input, line);
  output << line << std::endl;
  if(line.find("_lsu.u_lsu_request.ram_q") != std::string::npos)
    toCoutVerb("Find it!");
  if( std::regex_match(line, m, pNonblock) ) {
    nonblock_taint_gen(line, output);  
  }
  else if( std::regex_match(line, m, pNonblockConcat) ) {
    nonblockconcat_taint_gen(line, output);
  }
  else if( std::regex_match(line, m, pNonblockIf) ) {
    nonblockif_taint_gen(line, firstLine, input, output);
  }
  else if( std::regex_match(line, m, pNonblockIf2) ) {
    nonblockif2_taint_gen(line, firstLine, input, output);
  }
  else if( std::regex_match(line, m, pEnd) ) {
    output << line << std::endl;
  }
  else {
    toCout("Error in parsing nonblocking: "+line);
    abort();
  }
}


void always_neg_taint_gen(std::string firstLine, std::ifstream &input, std::ofstream &output) {
  if(!g_enable_taint) return;    
  std::smatch m;
  if( !std::regex_match(firstLine, m, pAlwaysNeg) ) {
    std::cout << "!! Error in parsing always_neg !!" << std::endl;
    abort();
  }
  // here I do not update g_recentClk with the gated clk because it will not be directly used in always statement
  //g_recentClk = m.str(2);
  std::string gatedClk = m.str(2);
  std::ofstream pathOutput(g_path+"/"+g_gatedClkFileName);
  pathOutput << moduleName + " : " + gatedClk << std::endl;
  pathOutput.close();

  std::string line;
  // parse first assignment
  std::getline(input, line);
  output << line << std::endl;
  if( std::regex_match(line, m, pNonblock) ) {
    nonblock_gate_taint_gen(line, output);  
  }
  else {
    toCout("Error in parsing nonblocking: "+line);
    abort();
  }
}


// print the modified firstLine in this function
void always_clkrst_taint_gen(std::string firstLine, std::ifstream &input, std::ofstream &output) {
  if(!g_enable_taint) return;  
  g_clkrst_exist = true;  
  std::smatch m;
  if( !std::regex_match(firstLine, m, pAlwaysClkRst) ) {
    std::cout << "!! Error in parsing always with clk & rst !!" << std::endl;
    abort();
  }
  g_recentClk = m.str(2);
  g_recentRst = m.str(3);
  g_clk_set.insert(g_recentClk);

  std::regex pNeg("negedge");
  // assume clock is always positive edge
  if( std::regex_search(firstLine, m, pNeg) ) 
    g_recentRst_positive = false;
  else
    g_recentRst_positive = true;


  // parse if
  std::string line;
  std::getline(input, line);
  output << line << std::endl;
  if( !std::regex_match(line, m, pIf) ) {
    std::cout << "!! Error in parsing if condition: "+line << std::endl;
    abort();
  }

  // parse first assignment
  std::getline(input, line);
  output << line << std::endl;
  if( !std::regex_match(line, m, pNonblock) && !std::regex_match(line, m, pNonblockConcat) ) {
    std::cout << "!! Error in parsing first assignment: "+line << std::endl;
    abort();
  }
  std::string destName = m.str(2);
  std::string src1Name = m.str(3);
  //// Assume the source 1 must all be numbers
  if( !isNum(src1Name) ) {
    std::cout << "!! Error: the first src should be pure numbers !!" << std::endl;
    abort();
  }

  // parse else
  std::getline(input, line);
  output << line << std::endl;
  if( !std::regex_match(line, m, pElse) ) {
    std::cout << "!! Error in parsing else: "+line << std::endl;
    abort();
  }

  // parse second assignment
  std::getline(input, line);
  std::string sndAssign = line;
  output << line << std::endl;
  if( !std::regex_match(line, m, pNonblock) && !std::regex_match(line, m, pNonblockConcat) ) {
    std::cout << "!! Error in parsing second assignment: "+line << std::endl;
    abort();
  }
  std::string dest2Name = m.str(2);
  std::string src2Name = m.str(3);
  if(destName.compare(dest2Name) != 0) {
    std::cout << "!! The second dest var does not equal the first !!" << std::endl;
  }

  // generate taints 
  nonblock_taint_gen(sndAssign, output);
}


void print_function_lines(std::ifstream &input, std::ofstream &output, std::string firstLine) {
  output << firstLine << std::endl;
  std::string line;
  std::getline(input, line);
  while(line.find("endfunction") == std::string::npos) {
    output << line << std::endl;
    std::getline(input, line);
  }
  output << line << std::endl;
}

} // end of namespace taintGen
