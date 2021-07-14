#include "helper.h"
#include "parse_fill.h"
#include "global_data_struct.h"
#include <ctype.h>

#define context std::shared_ptr<llvm::LLVMContext>
#define toStr(a) std::to_string(a)
#define value(a) llvm::dyn_cast<llvm::Value>(a)
#define instr(a) llvm::dyn_cast<llvm::Instruction>(a)
//#define llvmWidth(a, c) llvm::IntegerType::get(c, a)
//#define llvmInt(b, a, c) llvm::ConstantInt::get(llvmWidth(a, c), b, false)


using namespace z3;
using namespace taintGen;
using namespace syntaxPatterns;

namespace funcExtract {

std::regex pHex("^(\\d+)'h([\\dabcdefx\\?]+)$");
std::regex pDec("^(\\d+)'d([\\dx\\?]+)$");
std::regex pBin("^(\\d+)'b([01x\\?]+)$");

llvm::IntegerType* llvmWidth(uint32_t width, std::shared_ptr<llvm::LLVMContext> &c) { 
  return llvm::IntegerType::get(*c, width);
}


llvm::Value* llvmInt(uint32_t value, uint32_t width, 
                     std::shared_ptr<llvm::LLVMContext> &c) {
  return llvm::ConstantInt::get(llvm::IntegerType::get(*c, width), value, false);
}


llvm::Value* zext(llvm::Value* v1, uint32_t width,
                 std::shared_ptr<llvm::LLVMContext> &c,
                 std::shared_ptr<llvm::IRBuilder<>> &b) {
  return b->CreateZExtOrTrunc(v1, llvmWidth(width, c));
}


llvm::Value* sext(llvm::Value* v1, uint32_t width,
                 std::shared_ptr<llvm::LLVMContext> &c,
                 std::shared_ptr<llvm::IRBuilder<>> &b) {
  return b->CreateSExtOrTrunc(v1, llvmWidth(width, c));
}


bool isAs(std::string var, HierCtx_t &insContextStk) {
  auto curMod = get_curMod(insContextStk);
  auto it = std::find( curMod->moduleAs.begin(), curMod->moduleAs.end(), var );
  return it != curMod->moduleAs.end();
}


bool is_formed_num(std::string num) {
  std::smatch m;
  return std::regex_match(num, m, pHex)
          || std::regex_match(num, m, pBin)
          || std::regex_match(num, m, pDec);
}


// convert a string number, in hex|decimal|binary form, into uint32_t
uint32_t hdb2int(std::string num) {
  if(num.find("x") != std::string::npos) {
    replace_with(num, "x", "0");
  }
  num = remove_signed(num);
  std::smatch m;
  if(std::regex_match(num, m, pDec)) {
    std::string pureNum = m.str(2);
    return str2int(pureNum, "input num in hdb is: "+num);
  }
  else if(std::regex_match(num, m, pHex)) {
    std::string pureNum = m.str(2); 
    return hex2int(pureNum);    
  }
  else if(std::regex_match(num, m, pBin)){
    std::string pureNum = m.str(2); 
    return bin2int(pureNum);    
  }
  else 
    return try_stoi(num);
}

uint32_t hex2int(std::string num) {
  uint32_t res = 0;
  for(auto it = num.begin(); it != num.end(); it++) {
    res = res * 16;
    if(*it == 'f')
      res += 15;
    else if(*it == 'e')
      res += 14;
    else if(*it == 'd')
      res += 13;
    else if(*it == 'c')
      res += 12;
    else if(*it == 'b')
      res += 11;
    else if(*it == 'a')
      res += 10;
    else
      res += (*it - '0');
  }
  return res;
}


std::string get_pure_num(std::string formedNum) {
  std::smatch m;
  if (std::regex_match(formedNum, m, pHex)
      || std::regex_match(formedNum, m, pBin )
      || std::regex_match(formedNum, m, pDec )) {
    return m.str(2);
  }
  else {
    toCout("Error: not expected formed number: "+formedNum);
  }
}


std::string formedHex2bin(std::string num) {
  std::smatch m;
  if(!std::regex_match(num, m, pHex)) {
    toCout("Error: input to hex2bin is not hex:" +num);
  }
  uint32_t width = try_stoi(m.str(1));
  std::string pureNum = m.str(2);
  std::string ret="";
  for(char &c: pureNum) {
    switch(c) {
      case 'f':
        ret += "1111";
        break;
      case 'e':
        ret += "1110";        
        break;
      case 'd':
        ret += "1101";        
        break;      
      case 'c':
        ret += "1100";        
        break;      
      case 'b':
        ret += "1011";        
        break;      
      case 'a':
        ret += "1010";        
        break;      
      case '9':
        ret += "1001";        
        break;      
      case '8':
        ret += "1000";        
        break;      
      case '7':
        ret += "0111";        
        break;      
      case '6':
        ret += "0110";        
        break;      
      case '5':
        ret += "0101";
        break;      
      case '4':
        ret += "0100";
        break;      
      case '3':
        ret += "0011";
        break;
      case '2':
        ret += "0010";        
        break;
      case '1':
        ret += "0001";        
        break;
      case '0':
        ret += "0000";        
        break;
    }
  }
  if(ret.length() > width)
    return m.str(1) + "'b" + ret.substr(ret.length()-width);
  else
    return m.str(1) + "'b" + ret;
}


bool is_hex(std::string num) {
  std::smatch m;
  return std::regex_match(num, m, pHex);
}


uint32_t bin2int(std::string num) {
  uint32_t res = 0;
  for(char &c: num) {
    res = (res << 1) + (c - '0');
  }
  return res;
}


std::string timed_name(std::string name, uint32_t timeIdx) {
  return name + DELIM + toStr(timeIdx);
}


void record_expr(expr varExpr) {
  expr *tmpPnt = new expr(varExpr);
  TIMED_VAR2EXPR.emplace(varExpr.decl().name().str(), tmpPnt);
}


// extend e to added_len + len(e)
//expr sext(expr const& e, uint32_t added_len) {
//  return to_expr(e.ctx(), Z3_mk_sign_ext(e.ctx(), added_len, e));
//}


// extend e to len
expr sext_full(expr const& e, uint32_t len) {
  uint32_t eLen = get_var_slice_width_simp(pure(get_name(e)));
  if(eLen == len)
    return e; 
  else
    return to_expr(e.ctx(), Z3_mk_sign_ext(e.ctx(), len - eLen, e));
}


// extend e to len
expr zext_full(expr const& e, uint32_t destWidth, uint32_t opWidth) {
  if(destWidth == opWidth)
    return e; 
  else
    return zext(e, destWidth - opWidth);
}


bool is_root(std::string var) {
  return var.compare(g_rootNode) == 0;
}


std::string pure(std::string var) {
  if(var.find("_#") == std::string::npos)
    return var;
  size_t pos = var.find_last_of("#");
  return var.substr(0, pos-3);
}


bool is_taint(std::string var) {
  return var.back() == 'T';
}


bool is_clean(std::string var) {
  return !is_taint(var) && ( is_input(pure(var)) || is_read_asv(pure(var)) );
}


std::string get_name(expr expression) {
  return expression.decl().name().str();
}


bool is_read_asv(std::string var) {
  auto curMod = get_curMod();
  return g_readASV.find(pure(var)) != g_readASV.end() || g_readASV.find(curMod->name+"."+pure(var)) != g_readASV.end();
}


// FIXME: not sure if this is true for multi-cycle word
bool has_explicit_value(std::string input) {
  uint32_t encodingSize = g_currInstrInfo.instrEncoding.begin()->second.size();
  if(g_currInstrInfo.instrEncoding.find(input) == g_currInstrInfo.instrEncoding.end())
    return false;
  for(auto it = g_currInstrInfo.instrEncoding[input].begin(); 
        it != g_currInstrInfo.instrEncoding[input].end(); 
        it++) {
    if(*it != "x")
      return true;
  }
  return false;
}


uint32_t expr_len(expr &e) {
  return get_var_slice_width_simp(pure(get_name(e)));
}


bool comparePair(const std::pair<std::string, uint32_t> &p1, 
                 const std::pair<std::string, uint32_t> &p2 ) {
  return p1.first < p2.first;
}


uint32_t get_time(std::string var) {
  if(var.find(DELIM) == std::string::npos) {
    toCout("Error: the var's Name has no time: "+var);
    abort();
  }
  uint32_t pos = var.find(DELIM);
  uint32_t len = var.length();
  if(var.back() == 'T')
    return try_stoi(var.substr(pos+4, len-2));
  else
    return try_stoi(var.substr(pos+4));
}


bool is_case_dest(std::string var) {
  auto curMod = get_curMod();
  return curMod->caseTable.find(var) != curMod->caseTable.end();
}

bool is_func_output(std::string var) {
  auto curMod = get_curMod();
  if(curMod->funcTable.find(var) != curMod->funcTable.end())
    return true;
  if(curMod->funcTable.find(var+" ") != curMod->funcTable.end())
    return true;
  return false;
}

uint32_t get_pos_of_one(std::string value) {
  if(value.compare("default") == 0) {
    toCout("Error: try to find 1 in default: "+value);
    abort();
  }
  if(value.find("?") == std::string::npos 
      || value.find("b") == std::string::npos) {
    toCout("Error: case value is not in the correct form: "+value);
    abort();
  }
  size_t posB = value.find("b");
  size_t pos = value.find("1", posB+1);
  size_t pos2 = value.find("1", pos+1);
  if(pos2 != std::string::npos) {
    toCout("Error: found 2 1 in the case value: "+value);
    abort();
  }
  return value.length() - pos - 1;
}


// Attention:
// This function is a little counter-intuitive
// get logical hi
uint32_t get_lgc_hi(std::string varAndSlice) {
  auto curMod = get_curMod();
  varAndSlice = remove_signed(varAndSlice);
  std::smatch m;
  if(is_number(varAndSlice)) {
    if(!std::regex_match(varAndSlice, m, pBin)
        && !std::regex_match(varAndSlice, m, pHex)) {
      toCout("Error: input number for get_lgc_hi is not binary or hex: "+varAndSlice);
    }
    std::string bitNum = m.str(1);
    return try_stoi(bitNum)-1;
  }
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  if(!varSlice.empty()) {
    if(has_direct_assignment(varAndSlice))
      return get_end(varSlice) - get_begin(varSlice);
    else
      return get_end(varSlice);
  }
  auto idxPairs = curMod->varWidth.get_idx_pair(var, "find_version_num for: "+var);
  return idxPairs.first;
}


// get literal hi
uint32_t get_ltr_hi(std::string varAndSlice) {
  auto curMod = get_curMod();
  varAndSlice = remove_signed(varAndSlice);  
  std::smatch m;
  if(is_number(varAndSlice)) {
    if(!std::regex_match(varAndSlice, m, pBin)
        && !std::regex_match(varAndSlice, m, pHex)) {
      toCout("Error: input number for get_ltr_hi is not binary or hex: "+varAndSlice);
    }
    std::string bitNum = m.str(1);
    return try_stoi(bitNum)-1;
  }
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  if(!varSlice.empty())
    return get_end(varSlice);
  auto idxPairs = curMod->varWidth.get_idx_pair(var, "find_version_num for: "+var);
  return idxPairs.first;
}


uint32_t get_lgc_lo(std::string varAndSlice) {
  auto curMod = get_curMod();
  varAndSlice = remove_signed(varAndSlice);  
  if(is_number(varAndSlice))
    return 0;
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);

  if(!varSlice.empty()) {
    if(has_direct_assignment(varAndSlice))
      return 0;
    else
      return get_begin(varSlice);
  }
  auto idxPairs = curMod->varWidth.get_idx_pair(var, "find_version_num for: "+var);
  return idxPairs.second;
}


uint32_t get_ltr_lo(std::string varAndSlice) {
  auto curMod = get_curMod();
  varAndSlice = remove_signed(varAndSlice);  
  if(is_number(varAndSlice))
    return 0;
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);

  if(!varSlice.empty())
    return get_begin(varSlice);
  auto idxPairs = curMod->varWidth.get_idx_pair(var, "find_version_num for: "+var);
  return idxPairs.second;
}


// summary: check if a variable's slice is assigned directly
// input: varAndSlice must have slice
bool has_direct_assignment(std::string varAndSlice) {
  auto curMod = get_curMod();
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  bool withinReg2Slices = curMod->reg2Slices.find(var) != curMod->reg2Slices.end();
  if(varSlice.empty()) {
    toCout("Error: expecting slice for input: "+varAndSlice);
    abort();
  }
  return withinReg2Slices 
         && std::find(curMod->reg2Slices[var].begin(), curMod->reg2Slices[var].end(), varAndSlice) 
            != curMod->reg2Slices[var].end();
}


uint32_t get_num_len(std::string num) {
  num = remove_signed(num);  
  std::smatch m;
  if(std::regex_match(num, m, pHex)
      || std::regex_match(num, m, pDec)
      || std::regex_match(num, m, pBin))
    return try_stoi(m.str(1));
  else if(is_number(num) && is_bin(num)) {
    return num.length();
  }
  else {
    toCout("Error: input to get_num_len is not num:"+ num);
    abort();
  }
}


std::string zero_extend_num(std::string num) {
  std::regex pBin("^(\\d+)'b([01x\\?]+)$"); 
  std::smatch m;
  if(std::regex_match(num, m, pBin)) {
    int width = try_stoi(m.str(1));
    std::string pureNum = m.str(2);
    return m.str(1) + "'b" + std::string(width - pureNum.length(), '0') + pureNum;
  }
  else {
    toCout("Error: unsupported form of number:"+ num);
    abort();
  }
}


bool is_bin(std::string bv) {
  for(char &c : bv) {
    if(c != '0' && c != '1')
      return false;
  }
  return true;
}


bool is_all_zero(std::string str) {
  for(auto &c: str) {
    if(c != '0')
      return false;
  }
  return true;
}


bool replace(std::string& str, const std::string& from, const std::string& to) {
  size_t start_pos = str.find(from);
  if(start_pos == std::string::npos)
      return false;
  str.replace(start_pos, from.length(), to);
  return true;
}


void vec2str(std::vector<std::string> &vec, std::string &ret) {
  ret.clear();
  for(auto &var : vec) {
    ret = ret + var + ", ";
  }
  ret.pop_back();
  ret.pop_back();
}


bool is_written_ASV(const std::string &reg) {
  // currently does not support multiple instructions
  assert(g_instrInfo.size() == 1);
  for(auto it = g_instrInfo.back().writeASV.begin(); 
      it != g_instrInfo.back().writeASV.end(); 
      it++) {
    if(it->second == reg)
      return true;
  }
  return false;
}


bool is_all_x(std::string strIn) {
  for(char &c: strIn) {
    if(c != 'x')
      return false;
  }
  return true;
}


void add_front_backslash(std::string &line) {
  if(line.substr(0, 1) == "\\" && line.substr(0, 2) != "\\\\") {
    line = "\\"+line;
  }
}


void remove_front_backslash(std::string &line) {
  while(line.substr(0, 1) == "\\" ) {
    line = line.substr(1);
  }
}



std::string purify_var_name(std::string name) {
  remove_two_end_space(name);
  if(name.substr(0, 1) != "\\")
    return name;
  std::string ret = "";
  bool isBegin = true;
  for(uint32_t i = 0; i < name.length(); i++ ) {
    char c = name[i];
    //std::cout << c << std::endl;
    if(c == '\\' && isBegin) {
      continue;
    }
    else {
      isBegin = false;
      if(std::isdigit(c) || isLetter(c) || c == '_') {
        ret += std::string(1, c);
      }
      else if(c == '$')
        ret += "_DOLR_";
      else if( c == '.')
        ret += "_DOT_";
      else if(c == '\\')
        ret += "_BKSLSH_";
      else {
        toCout("Warning: not matched char: "+std::string(1, c));
      }
    }
  }
  return ret;
}


bool isLetter(const char &c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


std::string purify_line(const std::string &line) {
  if(line.find("|") == std::string::npos)
    return line;
  uint32_t pos = line.find("|");
  uint32_t pos2 = line.find("|", pos+1);
  std::string firstPart = line.substr(0, pos);
  std::string var = line.substr(pos+1, pos2-pos-1);
  std::string lastPart = line.substr(pos2+1);
  var = purify_var_name(var);
  lastPart = purify_line(lastPart);
  return firstPart + "|" + var + "|" + lastPart;
}


int try_stoi(std::string num) {
  int ret;
  try {
    ret = std::stoi(num);
  } catch(const std::exception& e) {
    toCout("Error for stoi, input is: "+num);
  }
  return ret;
}


// ATTENTION: for func_extract, you can only use get_var_slice_width_simp
// get_var_slice_width cannot be used!!
uint32_t get_var_slice_width_simp(std::string varAndSlice, 
                                  const std::shared_ptr<ModuleInfo_t> &mod) {
  return get_var_slice_width( varAndSlice, mod->varWidth);
}


// varAndSlice might have a module name prefix. This function
// parse the module name and get corresponding module information
uint32_t get_var_slice_width_cmplx(std::string varAndSlice) {
  if(varAndSlice.find(".") == std::string::npos 
     || varAndSlice.substr(0, 1) == "\\")
    return get_var_slice_width_simp(varAndSlice);
  else {
    size_t pos = varAndSlice.find(".");
    std::string modName = varAndSlice.substr(0, pos);
    std::string varName = varAndSlice.substr(pos+1);
    if(g_moduleInfoMap.find(modName) == g_moduleInfoMap.end()) {
      return get_var_slice_width_simp(varAndSlice);
    }
    auto subMod = g_moduleInfoMap[modName];
    return get_var_slice_width_simp(varName, subMod);
  }
}


bool is_comment_line(std::string &line) {
  uint32_t pos = line.find_first_not_of(" ", 0);
  return line.substr(pos, 2) == "//";
}


StrPair_t split_module_asv(const std::string &writeAsvLine) {
  std::regex pModuleAs  ("^(\\s*)([a-zA-Z0-9\\\\\\$\\#]+)\\.([a-zA-Z0-9\\\\\\$\\#]+)$");
  if(writeAsvLine.find(".") == std::string::npos)
    return std::make_pair(g_topModule, writeAsvLine);
  
  uint32_t dotPos = writeAsvLine.find(".");
  if(writeAsvLine.find(".", dotPos+1) != std::string::npos) {
    toCout("Error: unexpected extra dot for write ASV's name: "+writeAsvLine);
    abort();
  }
  std::string moduleName = writeAsvLine.substr(0, dotPos);
  std::string asvName = writeAsvLine.substr(dotPos+1);
  return std::make_pair(moduleName, asvName);
}


std::string remove_prefix_module(const std::string &writeAsvLine) {
  auto pair = split_module_asv(writeAsvLine);
  return pair.second;
}


llvm::Value* bit_mask(llvm::Value* in, uint32_t high, uint32_t low, 
                      std::shared_ptr<llvm::LLVMContext> &c, 
                      std::shared_ptr<llvm::IRBuilder<>> &b) {

  uint32_t len = high - low + 1;
  auto IntTy = llvm::IntegerType::get(*c, high+2);
  auto constOne = llvm::ConstantInt::get(IntTy, 1, false);
  //constOne->print(llvm::errs());
  auto s1 = b->CreateShl(constOne, len);
  //s1->print(llvm::errs());
  auto s2 = b->CreateSub( s1, constOne );
  //s2->print(llvm::errs());
  llvm::Value* mask = b->CreateShl(s2, low);
  //mask->print(llvm::errs());
  return b->CreateAnd(in, mask);
}


//llvm::Value* extract_func(llvm::Value* in, uint32_t high, uint32_t low, 
//                      std::shared_ptr<llvm::LLVMContext> &c, 
//                      std::shared_ptr<llvm::IRBuilder<>> &b, 
//                      const std::string &name, bool noinline) {
//
//  return extract_func(in, high, low, c, b, llvm::Twine(name), noinline);
//}






bool is_x(const std::string &var) {
  size_t quotePos = var.find("'");
  return quotePos != std::string::npos && var.substr(quotePos+2, 1) == "x";
}


bool is_input(const std::string &var, const std::shared_ptr<ModuleInfo_t> &modInfo) {
  auto it = std::find( modInfo->moduleInputs.begin(), modInfo->moduleInputs.end(), var );
  return it != modInfo->moduleInputs.end();
}


bool is_output(const std::string &var, std::shared_ptr<ModuleInfo_t> curMod) {
  auto it = std::find( curMod->moduleOutputs.begin(), curMod->moduleOutputs.end(), var );
  return it != curMod->moduleOutputs.end();
}


bool is_reg(std::string var) {
  remove_two_end_space(var);
  return g_allRegs.find(var) != g_allRegs.end();
}


bool is_reg_in_curMod(std::string varAndSlice) {
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  auto curMod = get_curMod();
  if(var.back() == ' ')
    var.pop_back();
  auto it = std::find( curMod->moduleTrueRegs.begin(), curMod->moduleTrueRegs.end(), var );
  return it != curMod->moduleTrueRegs.end();
}


bool is_submod_output(const std::string &var) {
  auto curMod = get_curMod();
  if( curMod->wire2InsPortMp.find(var) == curMod->wire2InsPortMp.end() )
    return false;
  auto tmpPair = curMod->wire2InsPortMp[var];
  std::string insName = tmpPair.first;
  std::string subModName = curMod->ins2modMap[insName];
  auto subMod = g_moduleInfoMap[subModName];
  std::string port = tmpPair.second;
  return subMod->moduleOutputs.find(port) != subMod->moduleOutputs.end();
}


std::shared_ptr<ModuleInfo_t> get_mod_info(std::string insName, 
                                           std::shared_ptr<ModuleInfo_t> curMod) {
  if(curMod->ins2modMap.find(insName) == curMod->ins2modMap.end()) {
    toCout("Error: cannot find submod instance: "+insName+" for module: "+curMod->name);
    abort();
  }
  std::string modName = curMod->ins2modMap[insName];
  return g_moduleInfoMap[modName];
}


std::string get_hier_name(std::vector<Context_t> &insContextStk,
                          bool includeTopModule) {
  std::string ret;
  if(includeTopModule)
    for(auto it = insContextStk.begin(); 
          it != insContextStk.end(); it++) {
      ret = ret + "." + it->InsName;
    }
  else {
    if(insContextStk.size() == 1) return "";
    for(auto it = insContextStk.begin()+1; 
          it != insContextStk.end(); it++) {
      ret = ret + "." + it->InsName;
    }
  }
  ret = ret.substr(1);
  return ret;
}


bool is_top_module() {
  auto curMod = get_curMod();
  return curMod->name == g_topModule;
}


bool is_sub_module() {
  return !is_top_module();
}


void collect_regs(std::shared_ptr<ModuleInfo_t> &curMod,
                  std::string regPrefix, 
                  RegWidthVec_t &regWidth ) {
  std::string modName = curMod->name;
  toCoutVerb("Collecting regs for: "+modName);
  collect_regs_iter(curMod, regPrefix, regWidth);
}

// Do not collect regs in mem module
void collect_regs_iter(std::shared_ptr<ModuleInfo_t> &curMod,
                       std::string regPrefix, 
                       RegWidthVec_t &regWidth ) {
  if(!regPrefix.empty())
    regPrefix = regPrefix + ".";
  for(std::string reg : curMod->moduleTrueRegs) {
    uint32_t width = get_var_slice_width_simp(reg, curMod);
    regWidth.push_back(std::make_pair(regPrefix+reg, width));
    std::string fullRegName = regPrefix+reg;
    toCoutVerb("Collect reg: "+fullRegName);
    if(fullRegName == "mOutPtr") {
      toCout("Find it!");
    }
  }

  for(auto pair : curMod->ins2modMap) {
    std::string insName = pair.first;
    std::string modName = pair.second;
    if(is_mem_module(modName)) continue;
    auto childMod = g_moduleInfoMap[modName];
    collect_regs_iter(childMod, regPrefix+insName, regWidth);
  }
}


void collect_mems(std::shared_ptr<ModuleInfo_t> &curMod,
                  std::string regPrefix, 
                  std::vector<std::string> &mems) {
  if(!regPrefix.empty())
    regPrefix = regPrefix + ".";
  for(std::string mem : curMod->moduleMems) {
    mems.push_back(regPrefix+mem);
    toCout("Collect mem: "+mem);
  }

  for(auto pair : curMod->ins2modMap) {
    std::string insName = pair.first;
    std::string modName = pair.second;
    auto childMod = g_moduleInfoMap[modName];
    collect_mems(childMod, regPrefix+insName, mems);
  }
}


void collect_mem_ins(std::shared_ptr<ModuleInfo_t> &curMod,
                     std::string regPrefix, 
                     // first is path+instance name, second is module name
                     std::vector<std::pair<std::string, 
                                        std::string>> &mems) {
  if(!regPrefix.empty())
    regPrefix = regPrefix + ".";

  for(auto pair : curMod->ins2modMap) {
    std::string insName = pair.first;
    std::string modName = pair.second;
    if(is_mem_module(modName)) {
      mems.push_back(std::make_pair(regPrefix+insName, modName));
    }
    auto childMod = g_moduleInfoMap[modName];
    collect_mem_ins(childMod, regPrefix+insName, mems);
  }
}


void check_mod_name(std::string modName) {
  if(g_moduleInfoMap.find(modName) == g_moduleInfoMap.end()) {
    toCout("Error: module is not in g_moduleInfoMap: "+modName);
    abort();
  }
}


std::string get_mod_name(std::string name) {
  size_t pos = name.find(".");
  if(pos == std::string::npos) {
    return "";
  }
  std::string modName = name.substr(0, pos);
  if(g_moduleInfoMap.find(modName) != g_moduleInfoMap.end())
    return modName;
  else
    return "";
}


std::string get_var_name(std::string name) {
  size_t pos = name.find(".");
  if(pos == std::string::npos) {
    return name;
  }
  std::string modName = name.substr(0, pos);
  if(g_moduleInfoMap.find(modName) != g_moduleInfoMap.end())
    return name.substr(pos+1);
  else
    return name;
}


std::pair<std::string, std::string> split_prefix_var(std::string var) {
  size_t pos = var.find(".");
  if(var.substr(0, 1) == "\\" || pos == std::string::npos) {
    return std::make_pair("", var);
  }
  else {
    std::string modName = var.substr(0, pos);
    std::string varName = var.substr(pos+1);
    return std::make_pair(modName, varName);
  }
}



//template <typename T>
std::vector<std::string> print_map_keys(std::map<std::string, astNode*> &map) {
  std::vector<std::string> ret;
  for(auto pair : map)
    ret.push_back(pair.first);
  return ret;
}


std::string ask_for_my_ins_name() {
  std::string insName = get_insName();
  if(!insName.empty()) return insName;
  const auto curMod = get_curMod();
  std::string myModName = curMod->name;
  if(get_parentMod() == nullptr) return myModName;
  auto parentMod = get_parentMod();
  insName = ask_parent_my_ins_name(myModName, parentMod);
  return insName;
}


std::string ask_parent_my_ins_name(std::string myModName, 
                                   std::shared_ptr<ModuleInfo_t> parentMod) {
  std::string insName = "";
  for(auto pair : parentMod->ins2modMap) {
    if(insName.empty() && pair.second == myModName)
      insName = pair.first;
    else if(!insName.empty() && pair.second == myModName) {
      toCout("Error: more than one instance matches!");
      abort();
    }
  }
  return insName;
}


void check_no_slice(std::string varAndSlice) {
  if(varAndSlice.empty()) return;
  std::string var, varSlice;
  split_slice(varAndSlice, var, varSlice);
  assert(varSlice.empty());
}


std::string get_insName(HierCtx_t &insContextStk) {
  return insContextStk.back().InsName;
}


std::string get_target(HierCtx_t &insContextStk) {
  return insContextStk.back().Target;
}


void set_target(const std::string &tgtIn, HierCtx_t &insContextStk) {
  if(!insContextStk.back().Target.empty()) {
    toCout("Warning: target has already been set: "
             +insContextStk.back().Target);
  }
  insContextStk.back().Target = tgtIn;
}


// find the first module with True isFunctionedSubMod
std::shared_ptr<ModuleInfo_t> get_curMod(HierCtx_t &insContextStk) {
  if(insContextStk.size() == 0) {
    toCout("Error: insContextStk is empty!");
    abort();
  }
  return insContextStk.back().ModInfo;
  //for(auto it = insContextStk.rbegin();
  //    it != insContextStk.rend(); it++) {
  //  if(it->ModInfo->isFunctionedSubMod) return it->ModInfo;
  //}
  //assert(insContextStk.front().InsName == g_topModule);
  //return insContextStk.front().ModInfo;
}


// used in ast
std::shared_ptr<ModuleInfo_t> get_parentMod(HierCtx_t &insContextStk) {
  auto parentInfo = insContextStk.back().ParentModInfo;
  uint32_t depth = get_stk_depth(insContextStk);
  if( depth > 1)
    assert(parentInfo == insContextStk[depth-2].ModInfo);
  return parentInfo;
}


llvm::Function* get_func(HierCtx_t &insContextStk) {
  return insContextStk.back().Func;
}


uint32_t get_stk_depth(HierCtx_t &insContextStk) {
  return insContextStk.size();
}


std::shared_ptr<ModuleInfo_t> get_real_parentMod(HierCtx_t &insContextStk) {
  auto parentMod = get_parentMod();
  if(parentMod != nullptr) return parentMod;
  else {
    assert(get_stk_depth(insContextStk) == 1);
    auto curMod = get_curMod();
    if(curMod->name == g_topModule) return nullptr;
    else {
      assert(curMod->parentModVec.size() == 1);
      return *(curMod->parentModVec.begin());
    }
  }
}


std::string remove_paramod(std::string modName) {
  remove_two_end_space(modName);
  std::smatch m;
  if(modName.find("paramod") != std::string::npos) {
    std::regex pParamod1("^\\\\\\$paramod\\\\(\\S+)(\\\\(\\S+)=(\\d+))+$");
    std::regex pParamod2("^\\\\\\$paramod\\$(?:[0-9a-z]+)(\\S+)$");
    if(std::regex_match(modName, m, pParamod1)
       || std::regex_match(modName, m, pParamod2) ) {
      modName = m.str(1);
    }
    else {
      toCout("Error: paramod module name is not matched: "+modName);
      abort();
    }
  }
  return modName;
}


bool is_mem_module(std::string modName) {
  modName = remove_paramod(modName);
  if(g_mem.find(modName) != g_mem.end()) return true;
  else return false;
}


uint32_t get_value_width(llvm::Value* in) {
  llvm::Type* inputTy = in->getType();
  uint32_t inputWidth = llvm::dyn_cast<llvm::IntegerType>(inputTy)->getBitWidth();
  return inputWidth;
}


void set_clk_rst(std::shared_ptr<ModuleInfo_t> &modInfo) {
  assert(!modInfo->clk.empty());
  assert(!modInfo->rst.empty());
  for(auto pair : modInfo->insPort2wireMp) {
    std::string insName = pair.first;
    std::string subModName = modInfo->ins2modMap[insName];
    auto subModInfo = g_moduleInfoMap[subModName];
    if(!subModInfo->clk.empty() && !subModInfo->rst.empty()) continue;
    bool clkSet = false;
    bool rstSet = false;
    for(auto portWire: pair.second) {
      std::string inPort = portWire.first;
      std::string wire = portWire.second;
      if(wire == modInfo->clk) {
        subModInfo->clk = inPort;
        clkSet = true;
        if(rstSet) {
          set_clk_rst(subModInfo);
          break;          
        }
      }
      else if(wire == modInfo->rst){
        subModInfo->rst = inPort;
        rstSet = true;
        if(clkSet) {
          set_clk_rst(subModInfo);
          break;          
        }
      }
    }
  }
}



std::string get_rst_value(const std::string &destAndSlice, uint32_t timeIdx) {
  uint32_t width = get_var_slice_width_simp(destAndSlice);
  std::string dest, destSlice;
  split_slice(destAndSlice, dest, destSlice);
  std::string rstVal;
  if(g_rstVal.find(dest) != g_rstVal.end()) {
    rstVal = g_rstVal[dest];  
    if(!destSlice.empty()) {
      uint32_t hi = get_end(destSlice);
      uint32_t lo = get_begin(destSlice);
      std::smatch m;
      if(!std::regex_match(rstVal, m, pBinX)) {
        toCout("Error: rstVal does not match pBinX: "+rstVal);
        abort();
      }
      rstVal = m.str(2);
      rstVal = get_bits(rstVal, hi, lo);
      rstVal = toStr(width)+"'b"+rstVal;
    }
  }
  else
    rstVal = toStr(width)+"'b0";

    // the following is for debug
  if(dest == "decoder_trigger"
     || dest == "latched_store"
     || dest == "latched_stalu" )
    rstVal = toStr(width)+"'b0";

  toCoutVerb("Replace "+timed_name(dest, timeIdx)+" with "+rstVal);
  g_outFile << "Replace "+timed_name(dest, timeIdx)+" with "+rstVal << std::endl;
  g_regValueFile << "Replace "+timed_name(dest, timeIdx)+" with "+rstVal << std::endl;
  if(dest == "buff1" && timeIdx == 15)
    toCoutVerb("Find it!");

  return rstVal;
}


bool is_fifo_output(std::string wire) {
  auto topMod = g_moduleInfoMap[g_topModule];
  if(topMod->wire2InsPortMp.find(wire) == topMod->wire2InsPortMp.end())
    return false;
  auto pair = topMod->wire2InsPortMp[wire];
  std::string insName = pair.first;
  std::string port = pair.second;
  std::string modName = topMod->ins2modMap[insName];
  if(is_fifo_module(modName) 
    && is_output(port, g_moduleInfoMap[modName]))
    return true;
  else return false;
}


bool is_fifo_module(std::string modName) {
  if(g_fifo.find(modName) != g_fifo.end()) return true;
  if(modName.find("\\$paramod\\mem_fifo\\WIDTH=") != std::string::npos)
    return true;
  return false;
}


void print_all_regs(const std::vector<std::pair<std::string, uint32_t>> &regWidth) {
  std::ofstream output(g_path+"/all_regs.txt", std::ios::app);
  for(auto pair : regWidth) {
    output << pair.first+":"+toStr(pair.second) << std::endl;
  }
  output.close();
}

std::string var_name_convert(std::string varName, bool replaceSlash) {
  std::string ret;
  for(char c : varName) {
    if(std::isdigit(c)
       || is_letter(c))
       ret += c;
    else if(c == '\\') {
      if(replaceSlash) ret += '_';
      else continue;
    }
    else {
      // if is special character
      ret += '_';
    }
  }
  return ret;
}


bool is_letter(char c) {
  if(c <= 'z' && c >= 'a'
     || c <= 'Z' && c >= 'A'
     || c == '_')
     return true;
  else return false;
}


void print_reg_info(RegWidthVec_t &regWidth) {
  uint32_t totalWidth = 0;
  std::ofstream output("./reg_info.txt");
  for(auto it = regWidth.begin(); it != regWidth.end(); it++) {
    std::string regName = it->first;
    uint32_t width = it->second;
    output << regName + ":" + toStr(width) << std::endl;
    totalWidth += width;
  }
  output << "Total width: "+toStr(totalWidth) << std::endl;
}


// return if var is purely number -- no x
bool is_pure_num(std::string var) {
  if(!is_number(var)) return false;
  if(var.find("x") == std::string::npos) return true;
  else return false;
}


void replace_with(std::string str, std::string subStr, std::string newSubStr) {
  size_t index = 0;
  uint32_t len = subStr.size();
  while (true) {
    /* Locate the substring to replace. */
    index = str.find(subStr, index);
    if (index == std::string::npos) break;
  
    /* Make the replacement. */
    str.replace(index, len, newSubStr);
  
    /* Advance index forward so the next iteration doesn't pick it up as well. */
    index += len;
  }
}

} // end of namespace funcExtract
