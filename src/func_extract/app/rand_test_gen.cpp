#include "../src/parse_fill.h"
#include "../src/helper.h"
#include "../src/util.h"
#include "../../taint_method/src/global_data.h"
#include "../src/global_data_struct.h"
#include <math.h>
#include <iostream>
#include <fstream>
#include <map>
#define toStr(a) std::to_string(a)

using namespace funcExtract;
using namespace taintGen;

std::ofstream g_output;
int InstrNum = 10;
std::regex pX("(\\d+)'[b|h][x|X]$");
std::regex pHex("(\\d+)'h([0-9a-fA-F]+)$");
std::map<std::string, std::string> regValueMap;


//std::string correct_hex(std::string hexValue) {
//  std::smatch m;
//  if(!std::regex_match(hexVal, m, pHex)) {
//    toCout("Error: input number does not match pHex");
//    abort();
//  }
//  uint32_t width = std::stoi(m.str(1));
//  std::string num = m.str(2);
//  if(width <= 4) assert(num.size() == 1);
//  else if(width == 5) {
//    if(num.substr(0, 1) != "1") 
//      num = "1"+num.substr(1);
//  }
//  else if(width == 6) {
//    uint32_t msb = std::stoi(num.substr(0, 1));
//    if(msb > 3) 
//      num = "3"+num.substr(1);
//  }
//  else if(width == 7) {
//    uint32_t msb = std::stoi(num.substr(0, 1));
//    if(msb > 3) 
//      num = "3"+num.substr(1);
//  }
//}


void to_file(std::string line) {
  g_output << line << std::endl;
}

std::string materialize_num(std::string val) {
  assert(val.find("+") == std::string::npos);
  std::smatch m;
  if(!is_x(val)) return val;
  else {
    if(!std::regex_match(val, m, pX)) {
      toCout("Error: does not match pX");
      abort();
    }
    uint32_t width = std::stoi(m.str(1));
    uint32_t randVal = rand() % (2 << width);
    std::string hexVal = dec2hex(randVal);
    return toStr(width)+"'h"+hexVal;
  }
}


// convert 4'h1+4'h2 to { 4'h1, 4'h2 }
std::string replace_x(std::string val) {
  std::smatch m;
  if(val.find("+") == std::string::npos)
    return materialize_num(val);
  else {
    std::string ret;
    remove_two_end_space(val);
    std::vector<std::string> vec;
    split_by(val, "+", vec);
    // replace x with number value
    for(auto it = vec.begin(); it != vec.end(); it++) {
      if(!std::regex_match(*it, m, pX)) continue;
      uint32_t width = std::stoi(m.str(1));
      uint32_t base = exp2(width);
      uint32_t newVal = rand() % base;
      std::string hexVal = dec2hex(newVal);
      *it = toStr(width)+"'h"+hexVal;
    }
    ret = merge_with(vec, "+");
    return ret;
  }
}


void assign_instr_value(std::string var, std::string value, bool rand) {
  value = replace_x(value);
  to_file(""+var+" = "+value);
  if(regValueMap.find(var) == regValueMap.end()) {
    regValueMap.emplace(var, value);
  }
  else {
    regValueMap[var] = value;
  }
}


void make_instr(uint32_t instrIdx) {
  auto instrInfo = g_instrInfo[instrIdx];

  // FIXME: currently assume 
  // first assign instruction encodings
  to_file("// instr: "+instrInfo.name);
  uint32_t instrLen = instrInfo.instrEncoding.begin()->second.size();
  for(uint32_t i = 0; i < instrLen; i++) {
    to_file("("+toStr(i)+")");
    for(auto pair: instrInfo.instrEncoding) {
      if(i == 0)
        assign_instr_value(pair.first, pair.second[i], true);
      else
        assign_instr_value(pair.first, regValueMap[pair.first], true);
    }
  }
}


int main(int argc, char *argv[]) {
  g_path = argv[1];
  g_verb = false;
  read_in_instructions(g_path+"/instr.txt");
  g_output.open(g_path+"/tb.txt", std::ios::out);
  
  uint32_t idx = 0;
  while(idx++ < InstrNum) {
    uint32_t instrIdx = rand() % g_instrInfo.size();
    make_instr(instrIdx);
  }
}
