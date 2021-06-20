#include "util.h"
#include "parse_fill.h"
#include "global_data_struct.h"
#include "helper.h"
#include "../../taint_method/src/global_data.h"
#define toCout(a) std::cout << a << std::endl;

using namespace taintGen;
using namespace syntaxPatterns;

namespace funcExtract {

void read_to_do_instr(std::string fileName, 
                      std::vector<uint32_t> &toDoList) {
  // each line is an instruction idx
  std::ifstream input(fileName);
  std::string line;
  while(std::getline(input, line)) {
    uint32_t idx = std::stoi(line);
    toDoList.push_back(idx);
  }
}


void read_asv_info(std::string fileName) {
  std::ifstream input(fileName);
  std::string line;
  while(std::getline(input, line)) {
    if(line.find(":") == std::string::npos) continue;
    size_t pos = line.find(":");
    std::string asv = line.substr(0, pos);
    uint32_t width = std::stoi(line.substr(pos+1));
    if(width == 0) {
      toCout("Warning: asv width is 0: "+asv);
    }
    g_asv.emplace(asv, width);
  }
}


void read_func_info(std::string fileName) {
  std::ifstream input(fileName);
  std::string instrName, target;
  std::string line;
  uint32_t idx;
  while(std::getline(input, line)) {
    if(line.substr(0, 6) == "Instr:") {
      instrName = line.substr(6);
      idx = get_instr_by_name(instrName);
    }
    else if(line.substr(0, 7) == "Target:") {
      target = line.substr(7);
      uint32_t targetWidth = g_asv[target];
      if(g_instrInfo[idx].funcTypes.find(target) 
           != g_instrInfo[idx].funcTypes.end()) {
        toCout("Error: target: "+target+" already existed!");
        abort();
      }
      struct FuncTy_t tmp = { targetWidth, std::vector<std::pair<uint32_t, std::string>>{} };
      g_instrInfo[idx].funcTypes.emplace(target, tmp);
    }
    else if(line.find(":") != std::string::npos) {
      size_t pos = line.find(":");
      std::string asv = line.substr(0, pos);
      uint32_t width = std::stoi(line.substr(pos+1));
      g_instrInfo[idx].funcTypes[target].argTy.push_back(std::make_pair(width, asv));
    }
  }
}


uint32_t get_instr_by_name(std::string instrName) {
  uint32_t idx = 0;
  for(auto instrInfo : g_instrInfo) {
    if(instrInfo.name == instrName) return idx;
    idx++;
  }
  toCout("Error: cannot find instrInfo for: "+instrName);
  abort();
}

}
