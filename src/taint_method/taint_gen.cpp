#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <queue>
#include <utility>
#include <assert.h>
#include <unordered_map>
#include <set>
#include "taint_gen.h"

#define toStr(a) std::to_string(a)
/* TODO:
 *  1. If a slice of a word is used, how to define its _t, _r, ...?
 *  Now I just do not distinguish slice and the whole word.
 * */

/* declarations */
std::regex pModule    (to_re("^(\\s*)module (NAME)\\(.+\\);$"));
std::regex pInput     (to_re("^(\\s*)input (\\[\\d+\\:0\\] )?(NAME)(\\s*)?;$"));
std::regex pOutput    (to_re("^(\\s*)output (\\[\\d+\\:0\\] )?(NAME)(\\s*)?;$"));
std::regex pReg       (to_re("^(\\s*)reg (\\[\\d+\\:\\d+\\] )?(NAME)(\\s*)?;$"));
std::regex pRegConst  (to_re("^(\\s*)reg (\\[\\d+\\:\\d+\\] )?(NAME) = (NUM)(\\s*)?;$"));
std::regex pWire      (to_re("^(\\s*)wire (\\[\\d+\\:\\d+\\] )?(NAME)(\\s*)?;$"));
/* 2 operators */
std::regex pAdd       (to_re("^(\\s*)assign (NAME) = (NAME) \\+ (NAME)(\\s*)?;$"));
std::regex pSub       (to_re("^(\\s*)assign (NAME) = (NAME) - (NAME)(\\s*)?;$"));
std::regex pMult      (to_re("^(\\s*)assign (NAME) = (NAME) \\* (NAME)(\\s*)?;$"));
std::regex pAnd       (to_re("^(\\s*)assign (NAME) = (NAME) && (NAME)(\\s*)?;$"));
std::regex pOr        (to_re("^(\\s*)assign (NAME) = (NAME) \\|\\| (NAME)(\\s*)?;$"));
std::regex pEq        (to_re("^(\\s*)assign (NAME) = (NAME) == (NAME)(\\s*)?;$"));
std::regex pEq3       (to_re("^(\\s*)assign (NAME) = (NAME) === (NAME)(\\s*)?;$"));
std::regex pNeq       (to_re("^(\\s*)assign (NAME) = (NAME) != (NAME)(\\s*)?;$"));
std::regex pLt        (to_re("^(\\s*)assign (NAME) = (NAME) > (NAME)(\\s*)?;$"));
std::regex pLe        (to_re("^(\\s*)assign (NAME) = (NAME) >= (NAME)(\\s*)?;$"));
std::regex pSt        (to_re("^(\\s*)assign (NAME) = (NAME) < (NAME)(\\s*)?;$"));
std::regex pSe        (to_re("^(\\s*)assign (NAME) = (NAME) <= (NAME)(\\s*)?;$"));
std::regex pBitOr     (to_re("^(\\s*)assign (NAME) = (NAME) \\| (NAME)(\\s*)?;$"));
std::regex pBitExOr   (to_re("^(\\s*)assign (NAME) = (NAME) \\^ (NAME)(\\s*)?;$"));
std::regex pBitAnd    (to_re("^(\\s*)assign (NAME) = (NAME) & (NAME)(\\s*)?;$"));
//TODO: merge pConcat and pPureConcat
std::regex pConcat    (to_re("^(\\s*)assign (NAME) = \\{ (NAME), (NAME) \\}(\\s*)?;$"));
std::regex pSel1      (to_re("^(\\s*)assign (NAME) = (NAME)\\[\\$signed\\((NAME)\\) \\+\\: INT\\](\\s*)?;$"));
std::regex pSel2      (to_re("^(\\s*)assign (NAME) = (NAME)\\[(NAME) \\+\\: INT\\](\\s*)?;$"));
std::regex pSel3      (to_re("^(\\s*)assign (NAME) = (NAME)\\[\\$signed\\((NAME)\\) \\-\\: INT\\](\\s*)?;$"));
std::regex pSel4      (to_re("^(\\s*)assign (NAME) = (NAME)\\[(NAME) \\-\\: INT\\](\\s*)?;$"));

std::regex pBitOrRed2 (to_re("^(\\s*)assign (NAME) = \\| \\{ (NAME), (NAME) \\}(\\s*)?;$"));
/* 1 operator */
std::regex pNot       (to_re("^(\\s*)assign (NAME) = ! (NAME)(\\s*)?;$"));
std::regex pNone      (to_re("^(\\s*)assign (NAME) = (NAME)(\\s*)?;$"));
std::regex pInvert    (to_re("^(\\s*)assign (NAME) = \\~ (NAME)(\\s*)?;$"));
/* reduce 1 op */
std::regex pRedOr     (to_re("^(\\s*)assign (NAME) = \\| (NAME)(\\s*)?;$"));
std::regex pRedAnd    (to_re("^(\\s*)assign (NAME) = & (NAME)(\\s*)?;$"));
std::regex pRedNand   (to_re("^(\\s*)assign (NAME) = \\~& (NAME)(\\s*)?;$"));
std::regex pRedNor    (to_re("^(\\s*)assign (NAME) = \\~\\| (NAME)(\\s*)?;$"));
std::regex pRedXor    (to_re("^(\\s*)assign (NAME) = \\^ (NAME)(\\s*)?;$"));
std::regex pRedXnor   (to_re("^(\\s*)assign (NAME) = \\~\\^ (NAME)(\\s*)?;$"));
/* ite */
std::regex pIte       (to_re("^(\\s*)assign (NAME) = (NAME) \\? (NAME) \\: (NAME)(\\s*)?;$"));
/* do not add anything */
// Assume: always comes with posedge or negedge
std::regex pAlwaysClk (to_re("^(\\s*)always @\\(posedge (NAME)\\)$"));
std::regex pAlwaysClkRst  (to_re("^(\\s*)always @\\(posedge (NAME) or (?:posedge|negedge) (NAME)(\\s?)\\)$"));
std::regex pEnd       ("^(\\s*)end$");
std::regex pEndmodule ("^(\\s*)endmodule$");
/* non-blocking assignment */
std::regex pNonblock  (to_re("^(\\s*)(NAME) <= (NAME)(\\s*)?;$"));
std::regex pNonblockConcat    (to_re("^(\\s*)(NAME) <= \\{(.+)\\}(\\s*)?;$"));
/* function */
std::regex pFunctionDef   (to_re("^(\\s*)function (\\[\\d+\\:0\\] )?(NAME)(\\s*)?;$"));
std::regex pEndfunction   (to_re("^(\\s*)endfunction$"));
std::regex pFunctionCall  (to_re("^(\\s*)assign (NAME) = (NAME)\\((.*)\\)(\\s*)?;$"));
/* control keywords */
// case
std::regex pCase      (to_re("^(\\s*)case(\\S)? \\((NAME)\\)$"));
std::regex pEndcase   (to_re("^(\\s*)endcase$"));
std::regex pDefault   (to_re("^(\\s*)default\\:$"));
std::regex pBlock     (to_re("^(\\s*)(NAME) = (NAME)(\\s*)?;$"));
// if else
std::regex pIf        (to_re("^(\\s*)if \\(.*\\)$"));
std::regex pElse      (to_re("^(\\s*)else$"));
/* multiple/un-certain # of ops */
//std::regex pBitExOrConcat (to_re("^(\\s*)assign (NAME) = \\{\\} \\^ (NAME)(\\s*)?;$"));
std::regex pSrcConcat(to_re("^(\\s*)assign (NAME) = \\{ ((?:NAME(?:\\s)?, )+NAME) \\}(\\s*)?;$"));
std::regex pSrcDestBothConcat(to_re("^(\\s*)assign \\{ ((?:NAME(?:\\s)?, )+NAME) \\} = \\{ ((?:NAME(?:\\s)?, )+NAME) \\}(\\s*)?;$"));

/* Milicious */
/* pVarName also exists in to_re(), and parse_var_list() */
std::regex pVarName("([\aa-zA-Z0-9_\\.\\$\\\\'\\[\\]]+)(?:\\s*\\[\\d+(\\:\\d+)?\\])?");
std::regex pVarNameGroup("([\aa-zA-Z0-9_\\.\\$\\\\'\\[\\]]+)(?:(\\s*)(\\[\\d+(\\:\\d+)?\\]))?");
std::regex pNum("^(\\d+)'(h|d|b)[\\dabcdef\\?]+$");
std::regex pNumExist("(\\d+)'(h|d|b)[\\dabcdef\\?]+");


/* Global data */
std::string moduleName;
std::vector<std::string> moduleInputs;
std::vector<std::string> moduleOutputs;
std::vector<std::string> moduleRegs;
std::set<std::string> moduleWires;
std::string clockName;
std::string resetName;
std::vector<std::string> rTaints;
std::unordered_map<std::string, uint32_t> nextVersion;
std::set<std::string> g_wire2reg;
std::unordered_map<std::string, std::string> new_next;
std::unordered_map<std::string, std::string> update_reg;
VarWidth varWidth;
VarWidth funcVarWidth;
unsigned long int NEW_VAR = 0;
unsigned long int NEW_FANGYUAN = 0;
unsigned long int USELESS_VAR = 0;
bool did_clean_file = false;
std::string g_recentClk;
std::string g_recentRst;
bool g_recentRst_positive = true;
//std::vector<std::string> g_clkGroup;
std::set<std::string> g_rstGroup;


/*remove comments
  remove redundent blanks 
  extract concatenants */
void clean_file(std::string fileName) {
  std::ifstream input(fileName);
  std::ofstream output(fileName + ".clean");
  std::string line;
  std::string cleanLine;
  std::smatch match;
  std::regex pureComment("^\\s*\\(\\*.*\\*\\)$");
  std::regex partialComment("\\(\\*.*\\*\\) ");
  std::regex redundentBlank("(\\S)(\\s+)(\\S)");
  std::regex extraBlank("([a-zA-Z0-9_\\.'])(\\s)(\\[)");
  bool inFunc = false;
  while( std::getline(input, line) ) {
    if(std::regex_match(line, match, pureComment) || line.substr(0,2) == "/*" || line.empty())
      continue;
    line = std::regex_replace(line, partialComment, "");
    cleanLine = std::regex_replace(line, redundentBlank, "$1 $3");
    // store the width of wires and regs in varWidth
    uint32_t choice = parse_verilog_line(cleanLine, true);
    std::smatch m;
    switch (choice) {
      case INPUT:
        {
          std::regex_match(line, m, pInput);
          std::string slice = m.str(2);
          std::string var = m.str(3);
          bool insertDone = false;
          if(!inFunc) {
            if(!slice.empty())
              insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = varWidth.var_width_insert(var, 0, 0);
          }
          else {
            if(!slice.empty())
              insertDone = funcVarWidth.force_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = funcVarWidth.force_insert(var, 0, 0);
          }     
          if (!insertDone) {
            std::cout << "insert failed in input case:" + line << std::endl;
            std::cout << "m.str(2):" + m.str(2) << std::endl;
            std::cout << "m.str(3):" + m.str(3) << std::endl;
          }
        }
        break;
      case REG:
        {
          std::regex_match(line, m, pReg);
          std::string slice = m.str(2);
          std::string var = m.str(3);
          bool insertDone = false;
          if(!inFunc) {
            if(!slice.empty())
              insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = varWidth.var_width_insert(var, 0, 0);
          }
          else {
            if(!slice.empty())
              insertDone = funcVarWidth.force_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = funcVarWidth.force_insert(var, 0, 0);
          }
          if (!insertDone) {
            std::cout << "insert failed in reg case:" + line << std::endl;
            std::cout << "m.str(2):" + m.str(2) << std::endl;
            std::cout << "m.str(3):" + m.str(3) << std::endl;
          }
        }
        break;
      case WIRE:
        {
          std::regex_match(line, m, pWire);
          std::string slice = m.str(2);
          std::string var = m.str(3);
          bool insertDone = false;
          if(!inFunc) {
            if(!slice.empty())
              insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = varWidth.var_width_insert(var, 0, 0);
          }
          else {
            if(!slice.empty())
              insertDone = funcVarWidth.force_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = funcVarWidth.force_insert(var, 0, 0);
          }
          if (!insertDone) {
            std::cout << "insert failed in wire case:" + line << std::endl;
            std::cout << "m.str(2):" + m.str(2) << std::endl;
            std::cout << "m.str(3):" + m.str(3) << std::endl;
          }
        }
        break;
      case OUTPUT:
        {
          std::regex_match(line, m, pOutput);
          std::string slice = m.str(2);
          std::string var = m.str(3);
          bool insertDone = false;
          if(!inFunc) {
            if(!slice.empty())
              insertDone = varWidth.var_width_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = varWidth.var_width_insert(var, 0, 0);
          }
          else {
            if(!slice.empty())
              insertDone = funcVarWidth.force_insert(var, get_end(slice), get_begin(slice));
            else
              insertDone = funcVarWidth.force_insert(var, 0, 0);
          }
          if (!insertDone) {
            std::cout << "insert failed in output case:" + line << std::endl;
            std::cout << "m.str(2):" + m.str(2) << std::endl;
            std::cout << "m.str(3):" + m.str(3) << std::endl;
          }
        }
        break;
      case FUNCDEF:
        {
          inFunc = true;
        }
        break;
      case FUNCEND:
        {
          inFunc = false;
        }
        break;
      default:
        break;
    } // end of switch
    bool noConcat = extract_concat(cleanLine, output);
    if (noConcat)
      output << cleanLine << std::endl;
  }
  output.close();
  did_clean_file = true;
}


void add_line_taints(std::string line, std::ofstream &output, std::ifstream &input) {
  line = further_clean_line(line);
  int choice = parse_verilog_line(line);
  switch( choice ) {
    case INPUT:
      input_taint_gen(line, output);
      break;
    case REG:      
      reg_taint_gen(line, output);      
      break;
    case WIRE:  
      wire_taint_gen(line, output);
      break;
    case OUTPUT:
      output_insert_map(line, output, input);
      break;
    case TWO_OP:
      two_op_taint_gen(line, output);      
      break;
    case ONE_OP:
      one_op_taint_gen(line, output);      
      break;
    case REDUCE1:
      reduce_one_op_taint_gen(line, output);
      break;
    case SRC_CONCAT:
      mult_op_taint_gen(line, output);
      break;
    case BOTH_CONCAT:
      both_concat_op_taint_gen(line, output);
      break;
    case ITE:
      ite_taint_gen(line, output);      
      break;
    case NONBLOCK:
      nonblock_taint_gen(line, output);
      break;
    case NONBLOCKCONCAT:
      nonblockconcat_taint_gen(line, output);
      break;
    case ALWAYS:
      always_taint_gen(line, input, output);
      break;
    case ALWAYS_CLKRST:
      always_clkrst_taint_gen(line, input, output);
      break;
    case UNSUPPORT:
      std::cout << "!!! Unsupported operator in: "+ line + "|" << std::endl;
      break;
    default:
      break;
  }
}


// FIXME: maybe set t-taint and r-taint to clear if reg value is not changed
int parse_verilog_line(std::string line, bool ignoreWrongOp) {
  if(line.substr(0, 1) == "X") {
    toCout("begin debug");
    ignoreWrongOp = true;
  }
  std::smatch m;
  if ( std::regex_match(line, m, pModule) ) {
    moduleName = m.str(2);
    return NONE;
  }
  else if (std::regex_match(line, m, pInput)) {
    return INPUT;
  }
  else if (std::regex_match(line, m, pOutput)) {
    std::string var = m.str(3);
    if(isOutput(var)) {
      std::cout << "!! Duplicate output find: " + line << std::endl;
    }
    else 
      moduleOutputs.push_back(var);
    return OUTPUT;
  }
  else if (std::regex_match(line, m, pReg)
            || std::regex_match(line, m, pRegConst)) {
    return REG;
  }
  /* every wire type also needs _t and _r, both are wires */
  /* The reason why these wires are named _t, _r not _t_ */
  else if (std::regex_match(line, m, pWire)) {
    return WIRE;
  }
  /* 2-operator assignment */
  else if (std::regex_match(line, m, pAdd)
            || std::regex_match(line, m, pSub)
            || std::regex_match(line, m, pMult)
            || std::regex_match(line, m, pAnd)
            || std::regex_match(line, m, pEq)
            || std::regex_match(line, m, pEq3)
            || std::regex_match(line, m, pNeq)
            || std::regex_match(line, m, pLt)
            || std::regex_match(line, m, pLe)
            || std::regex_match(line, m, pSt)
            || std::regex_match(line, m, pSe)
            || std::regex_match(line, m, pOr)
            || std::regex_match(line, m, pBitOr)
            || std::regex_match(line, m, pBitExOr)
            || std::regex_match(line, m, pBitAnd)
            || std::regex_match(line, m, pSel1)
            || std::regex_match(line, m, pSel2)
            || std::regex_match(line, m, pSel3)
            || std::regex_match(line, m, pSel4) 
            || std::regex_match(line, m, pBitOrRed2) ) {
    return TWO_OP;
  } // end of 2-operator
  /* 1-operator assignment */
  else if (std::regex_match(line, m, pNot) 
            || std::regex_match(line, m, pInvert)
            || std::regex_match(line, m, pNone)) {
    return ONE_OP;
  }
  else if ( std::regex_match(line, m, pRedOr)
            || std::regex_match(line, m, pRedAnd)
            || std::regex_match(line, m, pRedNand)
            || std::regex_match(line, m, pRedNor)
            || std::regex_match(line, m, pRedXor)
            || std::regex_match(line, m, pRedXnor) ) {
    return REDUCE1;
  }
  else if (std::regex_match(line, m, pSrcConcat)) {
    return SRC_CONCAT;
  }
  else if (std::regex_match(line, m, pSrcDestBothConcat)) {
    return BOTH_CONCAT;
  }
  else if (std::regex_match(line, m, pIte)) { // if cond is rst, then does not add any taint
    return ITE;
  } // end of ite
  else if( std::regex_match(line, m, pEnd)
            || std::regex_match(line, m, pEndmodule) ){
    return NONE;
  }
  else if( std::regex_match(line, m, pAlwaysClk) ) {
    return ALWAYS;
  }
  else if( std::regex_match(line, m, pAlwaysClkRst) ) {
    return ALWAYS_CLKRST;
  }
  else if( std::regex_match(line, m, pNonblock) ) {
    return NONBLOCK;
  }
  else if( std::regex_match(line, m, pNonblockConcat) ) {
    return NONBLOCKCONCAT;
  }
  else if( std::regex_match(line, m, pCase) ) {
    return CASE;
  }
  else if( std::regex_match(line, m, pFunctionDef) ) {
    return FUNCDEF;
  }
  else if( std::regex_match(line, m, pEndfunction) ) {
    return FUNCEND;
  }
  else {
    if(!ignoreWrongOp) {
      std::cout << "!! Unsupported operator:" + line << std::endl;
      abort();
    }
    return NONE;
  }
}


void read_in_clkrst(std::string fileName) {
  // set default name for these two variables
  clockName = "clk";
  resetName = "rst";
  g_recentClk = clockName;
  g_recentRst = resetName;
  g_rstGroup.insert(g_recentRst);  
  std::ifstream in(fileName);
  std::string line;
  std::smatch match;
  std::regex pClk("clock\\:([a-zA-Z0-9_:'\\[\\]]+)");
  std::regex pRst("reset\\:([a-zA-Z0-9_:'\\[\\]]+)");
  while( std::getline(in, line) ) {
    if ( std::regex_match(line, match, pClk) )
       clockName = match.str(1);
    if ( std::regex_match(line, match, pRst) )
      resetName = match.str(1);
  }
  in.close();
}


void add_file_taints(std::string fileName) {
  long long int lineNo = 0;
  std::ifstream input(fileName);
  std::ofstream output(fileName + ".tainted");
  std::string line;
  std::smatch match;
  // Reserve first line for module declaration
  while( std::getline(input, line) ) {
    lineNo++;
    //if(lineN0)
    if ( !std::regex_match(line, match, pModule) 
        && !std::regex_match(line, match, pEndmodule) )
      output << line << std::endl;
    if ( std::regex_match(line, match, pFunctionDef) ) {
      add_func_taints_limited(input, output, line);
    }
    else if ( std::regex_match(line, match, pCase) ) {
      add_case_taints(input, output, line, "trxc");
    }
    else if ( std::regex_match(line, match, pFunctionCall) 
                && match.str(3).compare("$signed") != 0 ) {
      add_func_taints_call_limited(line, output);
    }
    else
      add_line_taints(line, output, input);  
  }
  input.close();
  output.close();
}


/* merge _c, _r, _x */
void merge_taints(std::string fileName) {
  std::ofstream output(fileName, std::fstream::app);

  // assign _t_1bit  
  for(auto reg : moduleRegs) {
    output << "  assign " + reg + "_t_1bit = | " + reg + "_t ;" << std::endl;
  }

  // assign _c, _x
  std::vector<std::string> appendix{"_c", "_x"};
  for (std::string app : appendix) {  
    for ( auto it = nextVersion.begin(); it != nextVersion.end(); ++it ) {
      if ( isInput(it->first) || isNum(it->first) ) continue;
      if ( isReg(it->first) && app == "_x" )
        output << "  assign " + it->first + app + " = | (( ";
      else
        output << "  assign " + it->first + app + " = ( ";

      for (int i = 0; i < it->second - 1; i++)
        output << it->first + app + std::to_string(i) + " ) | ( ";

      if ( isReg(it->first) && app == "_x")
        output << it->first + app + std::to_string(it->second - 1) + " ));" << std::endl;
      else
        output << it->first + app + std::to_string(it->second - 1) + " );" << std::endl;
    }
  }

  for ( auto it = nextVersion.begin(); it != nextVersion.end(); ++it ) {
    if ( isInput(it->first) ) continue;    
    output << "  assign " + it->first + "_r = ( ";
    for (int i = 0; i < it->second - 1; i++) {
      output << it->first + "_x" + std::to_string(i) + " & ";
      output << it->first + "_r" + std::to_string(i) + " ) | ( ";
    }
    output << it->first + "_x" + std::to_string(it->second - 1) + " & ";
    output << it->first + "_r" + std::to_string(it->second - 1) + " );" << std::endl;
  }

  // ground taints for floating regs
  for (auto reg : moduleRegs) {
    if ( isNum(reg) ) {
      std::cout << "find num in nextVersion: " + reg << std::endl;
      continue;
    }
    if ( nextVersion.find(reg) == nextVersion.end() ) {
      output << "  assign " + reg + "_r = 0;" << std::endl;  
      output << "  assign " + reg + "_c = 0;" << std::endl;  
      output << "  assign " + reg + "_x = 0;" << std::endl;  
    }
  }

  for (auto wire : moduleWires) {
    if ( isNum(wire) ) {
      std::cout << "find num in nextVersion: " + wire << std::endl;
      continue;
    }
    if ( nextVersion.find(wire) == nextVersion.end() ) {
      output << "  assign " + wire + "_r = 0;" << std::endl;  
      output << "  assign " + wire + "_c = 0;" << std::endl;
      if( g_wire2reg.find(wire) == g_wire2reg.end() )
        output << "  assign " + wire + "_x = 0;" << std::endl;
    }
  }

  // wires giving value to regs should have taints grounded.
  for (auto it = g_wire2reg.begin(); it != g_wire2reg.end(); ++it) {
    if ( isNum(*it) || nextVersion.find(*it) != nextVersion.end() ) {
      continue;
    }
    std::string reg, regSlice;
    split_slice(*it, reg, regSlice);
    output << "  assign " + reg + "_r" + regSlice + " = 0;" << std::endl;
    output << "  assign " + reg + "_c" + regSlice + " = 0;" << std::endl;
  }

  // declare _r taints for wire-type outputs
  // Assume: the original outputs do not end with "_r_flag"
  //for(std::string out: moduleOutputs) {
  //  if( !isReg(out) && !isRFlag(out) ) {
  //    auto idxPair = varWidth.get_idx_pair(out);
  //    output << "  input [" + toStr(idxPair.first) + ":" + toStr(idxPair.second) + "] " + out + "_r ;" << std::endl;
  //    moduleInputs.push_back(out+"_r");
  //  }
  //}

  output << "endmodule";
  output.close();
}


void add_module_name(std::string fileName) {
  std::ifstream in(fileName);
  std::ofstream out(fileName + ".final");
  std::string line;
  std::smatch match;
  out << "module " + moduleName + "( ";
  for (auto it = moduleInputs.begin(); it != moduleInputs.end(); ++it) {
    out << *it + " , ";
  }
  for (auto it = moduleOutputs.begin(); it != moduleOutputs.end() - 1; ++it) {
    out << *it + " , ";
  }
  out << moduleOutputs.back() + " );" << std::endl;
  while( std::getline(in, line) ) {
    out << line << std::endl;
  }
  in.close();
  out.close();
}


void fill_update(std::string fileName) {
  std::ifstream in(fileName);
  std::string line;
  std::smatch m;
  // push all the vars on RHS of nonblocking to new_reg
  while( std::getline(in, line) ) {
    if ( std::regex_match(line, m, pNonblock) )
      g_wire2reg.insert( m.str(3) );
    else if ( std::regex_match(line, m, pNonblockConcat) ) {
      std::vector<std::string> updateVec;
      parse_var_list(m.str(3), updateVec);
      for (std::string update: updateVec) {
        g_wire2reg.insert( update );
      }
    }
  }
}


void add_func_taints(std::ifstream &input, std::ofstream &output, std::string funcDefinition) {
  std::cout << "DO: add_func_taints" << std::endl;
  std::smatch m;
  if ( !std::regex_match(funcDefinition, m, pFunctionDef) ) {
    std::cout << "!!! Error when deal with function !!!" << std::endl;
  }
  // function related data
  std::string funcBlank = m.str(1);
  std::string funcSlice = m.str(2);
  std::string funcName = m.str(3);
  std::vector<std::pair<std::string, std::string>> funcInputs;
  std::string line;

  // first, print out the definition of function
  auto funcBegin = input.tellg();
  while ( std::getline(input, line) && !std::regex_match(line, m, pEndfunction) ) {
    output << line << std::endl;
  }
  std::getline(input, line); // print endfunction
  output << line << std::endl << std::endl;

  // next, contruct the _t , _r and _x function
  output << funcBlank + "function " + funcSlice + funcName + "_t;" << std::endl;
  input.seekg(funcBegin);
  std::unordered_map<std::string, uint32_t> localVersionMap;  
  while ( std::getline(input, line) && !std::regex_match(line, m, pEndfunction) ) {
    int choice = parse_verilog_line(line);
    std::smatch m;
    switch (choice) {
      case INPUT:
        input_taint_gen(line, output);
        std::regex_match(line, m, pInput);
        funcInputs.push_back(std::make_pair(m.str(3), m.str(2)));
        break;
      case REG:
        reg_taint_gen_func(line, output, "t");
        break;
      case WIRE: 
        wire_taint_gen_func(line, output, "t");
        break;
      case TWO_OP:
        two_op_taint_gen_func(line, output, localVersionMap, "t");      
        break;
      case ONE_OP:
        one_op_taint_gen_func(line, output, localVersionMap, "t");
        break;
      case ITE:
        ite_taint_gen_func(line, output, localVersionMap, "t"); 
        break;
      case NONBLOCK:
        output << "!!! ERROR: function contains blocking !!!" << std::endl;     
        break;
      case NONBLOCKCONCAT:
        output << "!!! ERROR: function contains blocking !!!" << std::endl;
        break;
      case CASE:
        add_case_taints(input, output, line, "t");
        break;
      case UNSUPPORT:
        output << "!!! Unsupported operator in func:" + line << std::endl;
        break;
      default:
        break;
    }
  }
  output << line << std::endl << std::endl;

  std::vector<std::string> localTaints{"x", "r", "c"};
  //_x taint
  for (auto funcIn: funcInputs) {
    for (std::string taintBit: localTaints) {
      std::unordered_map<std::string, uint32_t> localVersionMap;
      output << funcBlank + "function " + funcIn.second + funcIn.first + "_"+ taintBit + ";" << std::endl;
      output << funcBlank + "  input " + funcSlice + funcName + "_" + taintBit + ";" << std::endl;
      input.seekg(funcBegin);
      while ( std::getline(input, line) && !std::regex_match(line, m, pEndfunction) ) { // the matched line must contain the input
        int choice = parse_verilog_line(line);
        switch (choice) {
          case INPUT:
            break;
          case REG:
            reg_taint_gen_func(line, output, taintBit);
            break;
          case WIRE: 
            wire_taint_gen_func(line, output, taintBit);
            break;
          case TWO_OP:
            two_op_taint_gen_func(line, output, localVersionMap, taintBit);
            break;
          case ONE_OP:
            one_op_taint_gen_func(line, output, localVersionMap, taintBit);
            break;
          case ITE:
            ite_taint_gen_func(line, output, localVersionMap, taintBit);
            break;
          case NONBLOCK:
            output << "!!! ERROR: function contains blocking !!!" << std::endl;     
            break;
          case NONBLOCKCONCAT:
            output << "!!! ERROR: function contains blocking !!!" << std::endl;        
            break;
          case CASE:
            add_case_taints(input, output, line, taintBit);
            break;
          case UNSUPPORT:
            output << "!!! Unsupported operator in func:" + line << std::endl;
            break;
          default:
            break;
        }
      }
    }
    output << line << std::endl << std::endl;
  }
}


/* assume func only contains case, case has only the 3rd input, the case condtion is non-numerical 
 * Print the original func, and also _t, _r, _x, _c taint functions */
void add_func_taints_limited(std::ifstream &input, std::ofstream &output, std::string funcDefinition) {
  std::cout << "DO: add_func_taints_limited" << std::endl;  
  std::smatch m;
  if ( !std::regex_match(funcDefinition, m, pFunctionDef) )
    return;
  std::string blank = m.str(1);
  std::string funcSlice = m.str(2);
  std::string funcName = m.str(3);
  uint32_t condWidthNum;
  
  // extract the (caseValue, caseAssign) pair from the case
  std::vector<std::pair<std::string, std::string>> caseAssignPairs;
  std::vector<std::string> inputSlice;
  parse_func_statements(caseAssignPairs, inputSlice, input);

  // print the original function
  std::string line;
  uint32_t lineNo = 0;
  while ( std::getline(input, line) && !std::regex_match(line, m, pEndfunction) ) {
    lineNo++;
    output << line << std::endl;
    if(lineNo == 3) {
      if (!std::regex_match(line, m, pInput))
        std::cout << "!! Error: Wrong " << std::endl;
      std::string slice = m.str(2);
      condWidthNum = get_width(slice);
    }
  }
  output << line << std::endl << std::endl;

  std::string rhs, rhsSlice;
  uint32_t destWidth = get_width(inputSlice[0]);
  assert( destWidth == get_width(funcSlice) );
  uint32_t aWidth = destWidth;
  uint32_t bWidth = get_width(inputSlice[1]);
  uint32_t sWidth = get_width(inputSlice[2]);
  uint32_t allInputWidth = destWidth + bWidth + sWidth;
  // print _t function
  output << blank + "function " + funcSlice + funcName + "_t ;" << std::endl;
  output << blank + "  input " + inputSlice[0] + " a_t ;" << std::endl;
  output << blank + "  input " + inputSlice[1] + " b_t ;" << std::endl;
  output << blank + "  input " + inputSlice[2] + " s_t ;" << std::endl;
  output << blank + "  input " + inputSlice[2] + " s ;" << std::endl;
  output << blank + "  reg s_t_1bit = 0;" << std::endl;
  output << blank + "  reg " + inputSlice[0] + " s_t_full = 0;"<< std::endl;
  output << blank + "  begin" << std::endl;
  output << blank + "    s_t_1bit = | s_t ;" << std::endl;  
  output << blank + "    s_t_full = " + extend("s_t_1bit", destWidth) + " ;" << std::endl;
  output << blank + "  end" << std::endl;
  output << blank + "  casez (s) " << std::endl;
  for(auto localPair: caseAssignPairs) {
    split_slice(localPair.second, rhs, rhsSlice);
    output << blank + "  " + localPair.first + " :" << std::endl;
    output << blank + "    " + funcName + "_t = s_t_full | " + rhs + "_t " + rhsSlice + ";" << std::endl;
  }
  output << blank + "  endcase" << std::endl;
  output << blank + "endfunction" << std::endl << std::endl;

  // TODO: needs to be modified
  // print _r function, output is (a_r, b_r, s_r), input is (dest_r, s)
  std::string taintString = pairVec2taintString(caseAssignPairs, "no_reg_is_excluded_2333", "_r", output);
  output << blank + "function [" + toStr(allInputWidth-1) + ":0] " + funcName + "_r ;" << std::endl;
  output << blank + "  input " + funcSlice + "dest_r ;" << std::endl;
  output << blank + "  input " + inputSlice[2] + " s ;" << std::endl;  
  output << blank + "  reg dest_r_1bit = 0;" << std::endl;
  output << blank + "  reg " + inputSlice[0] + " a_r = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[1] + " b_r = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[2] + " s_r = 0;" << std::endl;
  output << blank + "  reg " + inputSlice[2] + " s_r_tmp  = 0;" << std::endl;
  output << blank + "  begin" << std::endl;
  output << blank + "    dest_r_1bit = | dest_r ;" << std::endl;  
  output << blank + "    s_r = " + extend("dest_r_1bit", sWidth) + " | s_r_tmp;" << std::endl;
  output << blank + "    " + funcName + "_r = {a_r, b_r, s_r};" << std::endl;
  output << blank + "    " + taintString + " = 0 ;" << std::endl;
  output << blank + "  end" << std::endl;
  output << blank + "  casez (s) " << std::endl;
  uint32_t i = 0;
  for(auto localPair: caseAssignPairs) {
    split_slice(localPair.second, rhs, rhsSlice);
    output << blank + "  " + localPair.first + " : begin" << std::endl;
    output << blank + "    " + rhs + "_r " + rhsSlice + " = dest_r ;" << std::endl;
    output << blank + "    s_r_tmp [" + toStr(i) + ":0] = " + extend("|(dest_r | "+rhs+"_r"+rhsSlice+")", i+1) + " ;" << std::endl;
    if(i+1 <= sWidth - 1)
    output << blank + "    s_r_tmp [" + toStr(sWidth-1) + ":" + toStr(i+1) + "] = 0 ;" << std::endl;
    output << blank + "  end" << std::endl;
    i = std::min(i + 1, sWidth-1);
  }
  output << blank + "  endcase" << std::endl;
  output << blank + "endfunction" << std::endl << std::endl;

  // print _x function, output is (a_x, b_x, s_x), input is (dest_x, s)
  taintString = pairVec2taintString(caseAssignPairs, "no_reg_is_excluded_2333", "_x", output);  
  output << blank + "function [" + toStr(allInputWidth-1) + ":0] " + funcName + "_x ;" << std::endl;
  output << blank + "  input " + funcSlice + "dest_x ;" << std::endl;
  output << blank + "  input " + inputSlice[2] + " s ;" << std::endl;  
  output << blank + "  reg dest_x_1bit = 0;" << std::endl;
  output << blank + "  reg " + inputSlice[0] + " a_x = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[1] + " b_x = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[2] + " s_x = 0;" << std::endl; 
  output << blank + "  begin" << std::endl;
  output << blank + "    dest_x_1bit = | dest_x ;" << std::endl;  
  output << blank + "    s_x = " + extend("dest_x_1bit", sWidth) + " ;" << std::endl;
  output << blank + "    " + funcName + "_x = {a_x, b_x, s_x};" << std::endl;
  output << blank + "    " + taintString + " = 0 ;" << std::endl;
  output << blank + "  end" << std::endl;
  output << blank + "  casez (s) " << std::endl;
  for(auto localPair: caseAssignPairs) {
    split_slice(localPair.second, rhs, rhsSlice);
    output << blank + "  " + localPair.first + " : begin" << std::endl;
    output << blank + "    " + rhs + "_x " + rhsSlice + " = dest_x ;" << std::endl;
    output << blank + "  end" << std::endl;    
  }
  output << blank + "  endcase" << std::endl;
  output << blank + "endfunction" << std::endl << std::endl;

  // print _c function, output is (a_c, b_c, s_c), input is (dest_c, s)
  taintString = pairVec2taintString(caseAssignPairs, "no_reg_is_excluded_2333", "_c", output);
  output << blank + "function [" + toStr(allInputWidth-1) + ":0] " + funcName + "_c ;" << std::endl;
  output << blank + "  input " + funcSlice + "dest_c ;" << std::endl;
  output << blank + "  input " + inputSlice[2] + " s ;" << std::endl;  
  output << blank + "  reg " + inputSlice[0] + " a_c = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[1] + " b_c = 0;" << std::endl; 
  output << blank + "  reg " + inputSlice[2] + " s_c = 0;" << std::endl; 
  output << blank + "  begin" << std::endl;
  output << blank + "    s_c = " + extend("1'b1", sWidth) + " ;" << std::endl;
  output << blank + "    " + funcName + "_c = {a_c, b_c, s_c};" << std::endl;
  output << blank + "    " + taintString + " = 0 ;" << std::endl;
  output << blank + "  end" << std::endl;
  output << blank + "  casez (s) " << std::endl;
  for(auto localPair: caseAssignPairs) {
    split_slice(localPair.second, rhs, rhsSlice);
    output << blank + "  " + localPair.first + " : begin" << std::endl;
    output << blank + "    " + rhs + "_c " + rhsSlice + " = dest_c ;" << std::endl;
    output << blank + "  end" << std::endl;    
  }
  output << blank + "  endcase" << std::endl;
  output << blank + "endfunction" << std::endl << std::endl;
}


// add _t, _x, _r, _c taint for function
void add_case_taints(std::ifstream &input, std::ofstream &output, std::string firstLine, std::string taintBits) {
  std::smatch m;
  bool tExist = false;
  bool rExist = false;
  bool xExist = false;
  bool cExist = false;
  parse_taintBits(taintBits, tExist, rExist, xExist, cExist);
  if ( !std::regex_match(firstLine, m, pCase) )
    return;
  std::string blank = m.str(1);
  std::string postfix = m.str(2);
  std::string condName = m.str(3);
  uint32_t condWidthNum = varWidth.get_from_var_width(condName, firstLine);
  
  /* read the whole case and get all the RHS */
  std::string line;  
  std::vector<std::string> allRhs;
  auto funcBegin = input.tellg();
  bool readSwitchValue = true;
  std::unordered_map<std::string, std::string> localVer;
  if ( cExist | rExist | xExist ) {
    while( std::getline(input, line) && !std::regex_match(line, m, pDefault) ) {
      if( readSwitchValue ) {
        readSwitchValue = false;
        continue;
      }
      else {
        readSwitchValue = true;
        if( !std::regex_match(line, m, pBlock) ) {
          std::cout << "!! Error in parsing case !!" << std::endl;
        }
        allRhs.push_back(m.str(3));
      }
    }
    std::getline(input, line);
    if( !std::regex_match(line, m, pBlock) ) {
      std::cout << "!! Error in parsing case !!" << std::endl;
    }
    allRhs.push_back(m.str(3));
    input.seekg(funcBegin);

    for( std::string rhs: allRhs ) {
      std::string rhsName, rhsSlice;
      split_slice(rhs, rhsName, rhsSlice);
      auto verNum = find_version_num(rhsName);
      std::string ver = std::to_string(verNum);
      localVer.insert( std::make_pair(rhsName, ver) );
    }
  }

  // taint wire declaration
  if (tExist) {
    output << blank + "wire " + condName + "_t_1bit = | " + condName + "_t ;" << std::endl;
  }

  if (rExist) {
    /* for each rhs, declare a new version */
    for( std::string rhs: allRhs ) {
      std::string rhsName, rhsSlice;
      split_slice(rhs, rhsName, rhsSlice);
      output << blank + "wire [" + toStr(varWidth.get_from_var_width(rhsName, firstLine)) + "-1:0] " + rhsName + "_r" + localVer[rhsName] + " ;" << std::endl;
    }
  }

  if (xExist) {
    for( std::string rhs: allRhs ) {
      std::string rhsName, rhsSlice;
      split_slice(rhs, rhsName, rhsSlice);
      output << blank + "wire [" + toStr(varWidth.get_from_var_width(rhsName, firstLine)) + "-1:0] " + rhsName + "_x" + localVer[rhsName] + " ;" << std::endl;
    }
  }

  if (cExist) {
    for( std::string rhs: allRhs ) {
      std::string rhsName, rhsSlice;
      split_slice(rhs, rhsName, rhsSlice);
      output << blank + "wire [" + toStr(varWidth.get_from_var_width(rhsName, firstLine)) + "-1:0] " + rhsName + "_c" + localVer[rhsName] + " ;" << std::endl;
    }
  }

  output << blank + "case" + postfix + " (" + condName + ")" << std::endl;

  readSwitchValue = true;
  while( std::getline(input, line) && !std::regex_match(line, m, pEndcase) ) {
    if ( readSwitchValue ) {
      output << line << std::endl;
      readSwitchValue = false;
    }
    else {
      std::regex_match(line, m, pBlock);
      std::string blank = m.str(1);
      std::string dest, destSlice;
      std::string src, srcSlice;
      uint32_t localWidthNum;
      split_slice(m.str(3), src, srcSlice);
      
      if ( split_slice(m.str(2), dest, destSlice) ) {
        localWidthNum = get_width(destSlice);
      }
      else {
        localWidthNum = varWidth.get_from_var_width(dest, line);
      }

      if( tExist ) {
        output << blank + dest + "_t" + destSlice + " = " + src + "_t" + srcSlice + " | " + extend(condName+"_t_1bit", localWidthNum) + " ;" << std::endl;
      }

      /* some notes:
       * For the _r taint, in each case, only part of the taint will be assigned. To deal with the remaining part, 
       * how to deal with floating wires? */
      if( rExist ) {
        output << blank + src + "_r" + localVer[src] + srcSlice + " = " + dest + "_r" + destSlice + " | " + extend(condName+"_t_1bit", localWidthNum) + " ;" << std::endl;
        ground_wires(src+"_r"+localVer[src], varWidth.get_idx_pair(src, line), srcSlice, blank, output);
      }

      if( xExist ) {
        output << blank + src + "_x" + localVer[src] + srcSlice + " = " + dest + "_x" + destSlice + " ;" << std::endl;
        ground_wires(src+"_x"+localVer[src], varWidth.get_idx_pair(src, line), srcSlice, blank, output);
      }

      if( cExist ) {
        output << blank + src + "_c" + localVer[src] + srcSlice + " = " + extend("1'b1", localWidthNum) + " ;" << std::endl;
        ground_wires(src+"_c"+localVer[src], varWidth.get_idx_pair(src, line), srcSlice, blank, output);
      }
      readSwitchValue = true;        
    }
  } // end of while
  // print endcase
  output << line << std::endl;
}


/* first print this func call, then print all the taint calls */
void add_func_taints_call(std::string line, std::ofstream &output) {
  std::smatch m;
  if( !std::regex_match(line, m, pFunctionCall) )
    return;
  std::string blank = m.str(1);
  std::string returnValue = m.str(2);
  std::string funcName = m.str(3);
  std::string arguments = m.str(4);

  // print the func call
  output << line << std::endl;

  // print taint func calls
  //// first parse the func arguments
  ////// preprocss the arguments string
  int inBracket = 0;
  for (size_t i = 0; i < arguments.length(); ++i) {
    if ( arguments.substr(i, 1).compare("{") == 0 )
      inBracket++;
    if ( arguments.substr(i, 1).compare("}") == 0 )
      inBracket--;
    if ( arguments.substr(i, 1).compare(",") == 0 && inBracket > 0 )
      arguments[i] = '`';
  }
  size_t previous, current;
  previous = -1;
  char delim = ',';
  std::vector<std::string> args;
  std::string arg;
  // collect all non-numerical args in vector args
  while( current != std::string::npos ) {
    current = arguments.find(delim, previous + 1);
    arg = arguments.substr(previous+1, current-1);
    if ( !isNum(arg) )
      args.push_back(arg);
    previous = current;
  }
  // Note: at most one 
  for (std::string arg: args) {
    std::regex pBracket("^\\{\\}$");
  }
  // TODO:
}


void add_func_taints_call_limited(std::string line, std::ofstream &output) {
  std::smatch m;
  if( !std::regex_match(line, m, pFunctionCall) )
    return;
  std::string blank = m.str(1);
  std::string returnArg = m.str(2);
  std::string funcName = m.str(3);
  std::string arguments = m.str(4);

  std::regex pArgComb("\\(.*\\)");
  if( !std::regex_search(line, m, pArgComb) ) {
    std::cout << "!! Error in parsing func args !!" << std::endl;
    abort();
  }
  std::string varArgs = m.str(0);
  std::string condArgForFunc = get_nth_var_in_list(varArgs, 3);

  std::string returnArgT = get_lhs_taint_list(returnArg, "_t", output);
  std::string returnArgR = get_rhs_taint_list(returnArg, "_r");
  std::string returnArgX = get_rhs_taint_list(returnArg, "_x");
  std::string returnArgC = get_rhs_taint_list(returnArg, "_c");

  std::string argTList = get_rhs_taint_list(varArgs, "_t");

  std::vector<uint32_t> verVec;
  get_ver_vec(varArgs, verVec);
  std::string argRList = get_lhs_ver_taint_list(varArgs, "_r", output, verVec);
  std::string argXList = get_lhs_ver_taint_list(varArgs, "_x", output, verVec);
  std::string argCList = get_lhs_ver_taint_list(varArgs, "_c", output, verVec);

  output << blank + "assign " + returnArgT + " = " + funcName + "_t(" + argTList + ", " + condArgForFunc + ");" << std::endl;
  output << blank + "assign { " + argRList + " } = " + funcName + "_r(" + returnArgR + ", " + condArgForFunc + ");" << std::endl;
  output << blank + "assign { " + argXList + " } = " + funcName + "_x(" + returnArgX + ", " + condArgForFunc + ");" << std::endl;
  output << blank + "assign { " + argCList + " } = " + funcName + "_c(" + returnArgC + ", " + condArgForFunc + ");" << std::endl;
}


/* if a basic operator contains concatenated input, 
 * declare a new variable representing the concatenated input*/
bool extract_concat(std::string line, std::ofstream &output) {
  std::smatch m;
  int blankNo = line.find('a', 0);  
  std::regex pAssign("assign ");
  std::regex pBraces(to_re("\\{ ((?:NAME(?:\\s)?, )+NAME) \\}"));
  std::regex pSlice("\\[(\\d+)(:)?(\\d+)?\\]");

  std::regex_token_iterator<std::string::iterator> rend;
  std::regex_token_iterator<std::string::iterator> it(line.begin(), line.end(), pBraces, 1);

  std::string varList;
  std::string newLine;
  std::vector<std::string> allVarList;
  std::queue<std::string> newVarQueue;
  if (std::regex_search(line, m, pAssign)
      && !std::regex_match(line, m, pSrcConcat)
      && !std::regex_match(line, m, pSrcDestBothConcat)
      && std::regex_search(line, m, pBraces)) {
    // iterate over all matches
    while( it != rend ) {
      varList = *it++;
      allVarList.push_back(varList);
      int localIdxNum = NEW_FANGYUAN++;
      std::string localIdx = std::to_string(localIdxNum);
      uint32_t totalWidth = 0;
      std::vector<std::string> varVec;
      parse_var_list(varList, varVec);
      for(std::string var: varVec) {
        uint32_t localWidth = get_var_slice_width(var);
        if(localWidth == 0) {
          toCout("0 width found!!");
          abort();
        }
        totalWidth += localWidth;
      }
      if(totalWidth > 4294967290) {
        std::cout << "!! Error in getting total width for this line:" << std::endl;
        std::cout << line << std::endl;
        abort();
      }
      bool insertDone = varWidth.var_width_insert("fangyuan"+localIdx, totalWidth-1, 0);
      if (!insertDone) {
        std::cout << "insert failed for this line:" + line << std::endl;
        std::cout << "m.str(2):" + m.str(2) << std::endl;
        std::cout << "m.str(3):" + m.str(3) << std::endl;
      }
      output << std::string(blankNo, ' ') + "wire [" + toStr(totalWidth-1) + ":0] fangyuan" + localIdx + ";" << std::endl;
      output << std::string(blankNo, ' ') + "assign fangyuan" + localIdx + " = { " + varList + " };" << std::endl;
      newVarQueue.push("fangyuan"+localIdx);
    }
    char openBrace = '{';
    char closeBrace = '}';
    int openBracePos, closeBracePos = -1;
    /* if state=0, searching for openBrace
     * if state=1, searching for closeBrace */
    int state = 0;
    std::string part;
    // Assumption: the last search must be for openBrace
    while( openBracePos != std::string::npos ) {
      if (state == 0) {
        openBracePos = line.find(openBrace, closeBracePos+1);
        part = line.substr(closeBracePos+1, openBracePos - closeBracePos - 1);
        state = 1;
      }
      else if (state == 1) {
        closeBracePos = line.find(closeBrace, openBracePos+1);
        part = line.substr(openBracePos, closeBracePos - openBracePos + 1);
        state = 0;
      }
      else {
        toCout("Error!");
        abort();
      }
      if(state == 1) {// just find openBrace
        output << part;
      }
      else { // just find closeBrace
        auto newVar = newVarQueue.front();
        output << newVar;
        newVarQueue.pop();
      }
    }
    output << std::endl;
    return false;
  } // end of if
  return true;
}


void gen_assert_property(std::string fileName) {
  std::ofstream output(fileName);
  std::regex pRFlag("r_flag");
  std::smatch m;
  for(std::string out: moduleOutputs) {
    if(std::regex_search(out, m, pRFlag))
      output << "  assert property(disable iff(" + resetName + ") " + out + " == 0 );" << std::endl;
  }
  output.close();
}


void gen_reg_output(std::string fileName) {
  std::ofstream output(fileName);
  for( std::string out: moduleOutputs ) {
    if( isReg(out) && !isRFlag(out) ) 
      output << out << std::endl;
  }
}


void gen_wire_output(std::string fileName) {
  std::ofstream output(fileName);
  for( std::string out: moduleOutputs ) {
    if( !isReg(out) && !isRFlag(out) ) 
      output << out << std::endl;
  }
}


int main(int argc, char* argv[]) {
  std::string fileName = argv[1];
  if(argc <= 2 ) {
    std::cout << "Not enough arguments!" << std::endl;
    abort();
  }
  int stage = str2int(argv[2], "main: stage");
  if (stage <= 0) {
    std::cout << "Begin cleaning!" << std::endl; //0
    clean_file(fileName);
  }
  if (stage <= 1) {  
    std::cout << "Begin read in clkrst!" << std::endl; //1
    read_in_clkrst(fileName + ".clkrst");
  }
  if (stage <= 2) {  
    std::cout << "Begin fill update!" << std::endl; //2
    fill_update(fileName + ".clean");
  }
  if (stage <= 3) {
    std::cout << "Begin add file taints!" << std::endl; //3
    add_file_taints(fileName + ".clean");
  }
  if (stage <= 4) {  
    std::cout << "Begin merge taints!" << std::endl; //4
    merge_taints(fileName + ".clean.tainted");
  }
  if (stage <= 5) {  
    std::cout << "Begin add module name!" << std::endl; //6
    add_module_name(fileName + ".clean.tainted");
  }
  if (stage <= 6) {
    std::cout << "Generate the assert property!" << std::endl; //7
    gen_assert_property(fileName + ".assert.property");
  }
  if (stage <= 7) {
    std::cout << "Generate info about outputs!" << std::endl; //7
    gen_reg_output(fileName + ".reg_output");
    gen_wire_output(fileName + ".wire_output");
  }
}
