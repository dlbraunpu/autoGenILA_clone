#include "global_data_struct.h"
#include "clean_goal.h"
#include "parse_fill.h"
#include "helper.h"

#define toStr(a) std::to_string(a)

void clean_goal() {
  toCout("### Begin clean_goal");
  std::ifstream input(g_path+"/goal.txt");
  std::ofstream output(g_path+"/clean_goal.txt");
  std::string line;
  uint32_t instrIdx;
  std::string writeASV;
  uint32_t bound;
  uint32_t assumIdx = 0;
  while(std::getline(input, line)) {
    toCout(line);
    if(line.front() == '#') {
      size_t pos2 = line.find("#", 1);
      size_t pos3 = line.find("#", pos2+1);
      size_t pos4 = line.find("#", pos3+1);
      if(pos4 != std::string::npos) {
        toCout("Error: more than 3 # found: "+line);
        abort();
      }
      instrIdx = std::stoi(line.substr(1, pos2-1));
      writeASV = line.substr(pos2+1, pos3-pos2-1);
      bound = std::stoi(line.substr(pos3+1));
    }
    else {
      while(line.find("___#") != std::string::npos) {
        size_t pos = line.find("___#");
        size_t i = pos + 4;
        while(i != std::string::npos && std::isdigit(line[i++]));
        line = line.substr(0, pos) + line.substr(i-1);
      }
    }
    //if(line.find("MIXIEGEN") != std::string::npos) {
    //  size_t pos = line.find("MIXIEGEN");
    //  std::string width;
    //  uint32_t i = pos + 8;
    //  while(std::isdigit(line[i])) 
    //    width += line[i++];
    //  uint32_t localWidth = std::stoi(width);
    //  line = line.substr(0, pos) + get_zero(localWidth) + line.substr(i);
    //}
    output << line << std::endl;

    if(line.front() == ')') {
      std::string instrEncodings = get_encodings(g_instrInfo[instrIdx-1].instrEncoding);
      if(instrEncodings.empty())
        continue;
      std::string rstEncodings = get_encodings(g_rstVal);
      uint32_t writeDelay = get_write_delay(g_instrInfo[instrIdx-1].writeASV, writeASV);
      output << "assume -name zy_assume"+toStr(assumIdx++)+" {__START__ == 1 |-> ("+instrEncodings+" ##1 ( "+rstEncodings+" )[*"+toStr(writeDelay+1)+"] ) }" << std::endl;
    }

  }
  input.close();
  output.close();
}


std::string get_encodings( const std::unordered_map<std::string, std::string> &instrEncoding ) {
  if(instrEncoding.empty()) {
    toCout("Error: instrEncoding is empty!");
    abort();
  }
  std::string ret;
  for(auto it = instrEncoding.begin(); it != instrEncoding.end(); it++) {
    if(it->second == "x")
      continue;
    ret += it->first + " == " + it->second + " && ";
  }
  if(ret.length() > 3) {
    ret.pop_back();
    ret.pop_back();
    ret.pop_back();
  }
  return ret;
}


uint32_t get_write_delay(const std::set<std::pair<uint32_t, std::string>> &writeASV, std::string asv) {
  if(writeASV.empty()) {
    toCout("Error: writeASV is empty!");
    abort();
  }
  for(auto it = writeASV.begin(); it != writeASV.end(); it++) {
    if(it->second == asv)
      return it->first;
  }
  toCout("Error:"+asv+" is not in the writeASV set!");
  abort();
}


std::string get_zero(uint32_t width) {
  std::string ret = "#";
  if(width % 4 == 0) {
    width = width / 4;
    ret += "x";
  }
  else {
    ret += "b";
  }
  while(width-- > 0) {
    ret += "0";
  }
  return ret;
}
