#include "read_instr.h"
#include "helper.h"
#include "global_data_struct.h"

namespace funcExtract {

// parse instr.txt file
// parsed result is in g_instrInfo
void read_in_instructions(std::string fileName) {
  toCout("### Begin read in instr info: "+fileName);
  g_instrInfo.clear();
  std::ifstream input(fileName);
  if(!input.is_open()) {
    toCout("Error: cannot open "+fileName);
    abort();
  }
  std::string line;
  std::smatch m;
  std::regex pIdx("^#\\d+:([a-z0-9A-Z_]*)$");
  enum State {FirstSignal, OtherSignal, WriteASV, ReadASV, 
              ReadNOP, ReadMEM, ResetVal, InvarRegs, TopMod, 
              DelayBound, ReadFIFO};
  enum State state;
  bool firstWord = true;
  bool firstSignalSeen = false;
  std::string lastMemReadAddr;
  while(std::getline(input, line)) {
    toCoutVerb(line);
    if(line.empty())
      continue;
    if(line.substr(0, 2) == "//")
      continue;
    if(line.substr(0, 4) == "#CLK") {
      std::string clk = line.substr(5);
      remove_two_end_space(clk);
      g_recentClk = clk;
      continue;
    }
    if(line.substr(0, 4) == "#RST") {
      std::string rst = line.substr(5);
      remove_two_end_space(rst);
      g_recentRst = rst;
      continue;
    }
    if(line.substr(0, 4) == "#valid") {
      std::string var = line.substr(5);
      remove_two_end_space(var);
      g_inputValid = var;
      continue;
    }
    if(line.substr(0,6) == "#delay") {
      if(line.find("(") == std::string::npos) {
        std::string delay = line.substr(7);
        remove_two_end_space(delay);
        g_instrInfo.back().delayBound = std::stoi(delay);
        continue;
      }
      else {
        std::string delayPart = line.substr(7);
        size_t openBracePos = delayPart.find("(");
        std::string delay = delayPart.substr(0, openBracePos);
        remove_two_end_space(delay);
        g_instrInfo.back().delayBound = std::stoi(delay);
        std::string bracePart = delayPart.substr(openBracePos+1);
        bracePart.pop_back();
        size_t colonPos = bracePart.find(":");
        std::string exceptionVar = bracePart.substr(0, colonPos);
        uint32_t exceptionDelay = std::stoi(bracePart.substr(colonPos+1));
        // FIXME: currently only support one exception
        g_instrInfo.back().delayExceptions.emplace(exceptionVar, exceptionDelay);
        continue;
      }
    }
    if(line.substr(0,11) == "#extraDelay") {
      std::string delay = line.substr(12);
      remove_two_end_space(delay);
      g_instrInfo.back().extraDelay = std::stoi(delay);
      continue;
    }
    if(line.substr(0, 10) == "#instrAddr") {
      std::string instrAddr = line.substr(11);
      remove_two_end_space(instrAddr);
      g_instrInfo.back().instrAddr = instrAddr;
      continue;
    }
    if(line.substr(0, 8) == "#memAddr") {
      std::string instrAddr = line.substr(9);
      remove_two_end_space(instrAddr);
      g_instrInfo.back().instrAddr = instrAddr;
      continue;
    }
    if(line.substr(0, 9) == "#dataAddr") {
      std::string dataAddr = line.substr(10);
      if(dataAddr.find("(array)") == std::string::npos) {
        remove_two_end_space(dataAddr);
      }
      else {
        size_t pos = dataAddr.find("(array)");
        dataAddr = dataAddr.substr(0, pos);
        remove_two_end_space(dataAddr);
        dataAddr = var_name_convert(dataAddr, true); 
        dataAddr += "_Arr"; 
      }
      lastMemReadAddr = dataAddr;
      g_instrInfo.back().memReadAddr2TgtMap.emplace(dataAddr, 
                                std::vector<std::string>{});
      continue;
    }
    //if(line.substr(0, 9) == "#fromMemData") {

    //}
    if(line.substr(0, 12) == "#needDataTgt") {
      std::string varName = line.substr(13);
      size_t pos = varName.find("{");
      varName = varName.substr(0, pos);
      remove_two_end_space(varName);
      if(g_instrInfo.back().loadDataInfo.find(varName) 
           != g_instrInfo.back().loadDataInfo.end()) {
        toCout("Error: target is already in loadDataInfo: "+varName);
        abort();
      }
      g_instrInfo.back().loadDataInfo.emplace(varName, std::make_pair("", 0));
      std::string newLine;
      std::getline(input, newLine);
      while(newLine != "}") {
        remove_two_end_space(newLine);
        if(newLine.substr(0, 2) == "//") {}
        else if(newLine.substr(0, 7) == "#dataIn") {
          std::string dataIn = newLine.substr(8);
          size_t pos = dataIn.find("(");
          std::string idx;
          if(pos != std::string::npos) {
            idx = dataIn.substr(pos+1);      
            dataIn = dataIn.substr(0, pos);
            remove_two_end_space(idx);
            idx.pop_back();
            remove_two_end_space(idx);
          }
          remove_two_end_space(dataIn);
          uint32_t idxNum;
          if(idx.empty()) idxNum = 0;
          else idxNum = std::stoi(idx);
          g_instrInfo.back().loadDataInfo[varName] = std::make_pair(dataIn, idxNum);
        }
        else {
          toCout("Unexpected line: "+newLine);
          abort();
        }
        std::getline(input, newLine);
      }
      continue;
    }
    if(line.substr(0, 7) == "#VarMap") {
      std::string newLine;
      std::getline(input, newLine);      
      while(newLine != "}") {
        remove_two_end_space(newLine);
        if(newLine.substr(0, 2) == "//") {}
        else if(newLine.find(":") != std::string::npos) {

          size_t pos = newLine.find(":");
          std::string mappedVar = newLine.substr(0, pos);
          remove_two_end_space(mappedVar);
          std::string srcVar = newLine.substr(pos+1);
          remove_two_end_space(srcVar);
          if(g_instrInfo.back().funcTgtMap.find(srcVar)
               == g_instrInfo.back().funcTgtMap.end()) {
            g_instrInfo.back().funcTgtMap.emplace(srcVar,
                                                  std::set<std::string>{mappedVar});
          }
          else {
            g_instrInfo.back().funcTgtMap[srcVar].insert(mappedVar);
          }
        }
        else {
          toCout("Unexpected line: "+newLine);
          abort();
        }
        std::getline(input, newLine);
      }
      continue;
    }
    if(line.back() == ' ')
      line.pop_back();
    if(line.front() == '#') { // a new instr begins
      std::smatch m;
      if(!std::regex_match(line, m, pIdx)) {
        toCout("Error: does not match pIdx: "+line);
        abort();
      }
      g_instrName = m.str(1); 
      state = FirstSignal;
      firstSignalSeen = false;
      firstWord = true;
    }
    else if(line.front() == '(') { // (n) is used to express instr word in cycle n
      if(!is_number(line.substr(1, line.length()-2))) {
        toCout("Error: parse instr.txt failed! # is not followed by intruction ID: "+line);
        abort();
      }
      state = FirstSignal;
    }
    else if(line == "W:") {
      state = WriteASV;
    }
    else if(line == "R:") {
      state = ReadASV;
    }
    else if(line == "$NOP:") {
      state = ReadNOP;
    }
    else if(line == "*RESET:") {
      state = ResetVal;
    }
    else if(line == "$INVAR:") {
      state = InvarRegs;
    }
    else if(line == "$TOP:") {
      state = TopMod;
    }
    else if(line == "$MEM:") {
      state = ReadMEM;
    }
    else if(line == "$FIFO:") {
      state = ReadFIFO;
    }
    else { // still the old instruction
      switch(state) {
        case FirstSignal:
          {
            if(firstSignalSeen)
              firstWord = false;
            auto pos = line.find("=");
            std::string signalName = line.substr(0, pos-1);
            remove_two_end_space(signalName);
            std::string encoding = line.substr(pos+2);
            remove_two_end_space(encoding);
            if(!check_input_val(encoding)) {
              toCout("Encoding is not x or number[1], line is: "+line);
              abort();
            }
            if(firstWord) {
              struct InstrInfo_t info = { {{signalName, std::vector<std::string>{encoding}}}, 
                                        //std::set<std::string>{}, 
                                        std::set<std::pair<uint32_t, std::string>>{}, 
                                        std::set<std::string>{},
                                        std::vector<std::string>{}, 0, 0, 0, 1, g_instrName};
              //info.instrEncoding.emplace(signalName, encoding);
              g_instrInfo.push_back(info);
              toCout("Reading instruction "+info.name);
            }
            else {
              g_instrInfo.back().instrEncoding[signalName].push_back(encoding);
              g_instrInfo.back().instrLen++;
              toCout("Reading data for cycle " +
                  std::to_string(g_instrInfo.back().instrLen) +
                  " of instruction " +
                  g_instrInfo.back().name);
            }
            state = OtherSignal;
            firstSignalSeen = true;
          }
          break;
        case OtherSignal:
          {
            auto pos = line.find("=");
            std::string signalName = line.substr(0, pos-1);
            remove_two_end_space(signalName);            
            std::string encoding = line.substr(pos+2);
            remove_two_end_space(encoding);            
            if(!check_input_val(encoding)) {
              toCout("Encoding is not x or number[2], line is: "+line);
              abort();
            }
            if(firstWord)
              g_instrInfo.back().instrEncoding.emplace(signalName, 
                                                       std::vector<std::string>{encoding});
            else
              g_instrInfo.back().instrEncoding[signalName].push_back(encoding);
          }
          break;
        case WriteASV:
          {
            // if space exists, solve process is skipped and delay for writing ASV is specified
            if(line != "[") { // if is not a vector of reg
              if(line.find(" ") == std::string::npos) {
                g_instrInfo.back().writeASV.insert(std::make_pair(0, line));
                moduleAs.insert(line);
              }
              else {
                size_t pos = line.find(" ");
                if(pos == line.length() - 1) {
                  line.pop_back();
                  g_instrInfo.back().writeASV.insert(std::make_pair(0, line));
                  moduleAs.insert(line);
                }
                std::string cycleCnt = line.substr(pos+1);
                if(!is_number(cycleCnt)) {
                  toCout("Error: cycle count is not number: "+line);
                  abort();
                }
                std::string asName = line.substr(0, pos);
                std::string subLine = line.substr(0, pos);
                if(subLine.find("(skip)") != std::string::npos) {
                  size_t bracePos = subLine.find("(skip)");
                  asName = line.substr(0, bracePos);
                  g_instrInfo.back().skipWriteASV.insert(asName);                
                }
                // TODO
                // even if no "skip" is found, still treat it as a skipWriteASV
                //else {

                //}
                g_instrInfo.back().writeASV.insert(std::make_pair(uint32_t(std::stoi(cycleCnt)), 
                                                                  asName));
                moduleAs.insert(asName);
              }
            }
            else { // if is a vector of regs
              std::getline(input, line);              
              while(line[0] != ']') {
                if(line.substr(0, 2) != "//") {
                  g_instrInfo.back().writeASVVec.push_back(line);
                  g_instrInfo.back().skipWriteASV.insert(line);
                  moduleAs.insert(line);
                }
                std::getline(input, line);
              }
              // line begins with "]"
              size_t pos = line.find(" ");
              std::string cycleCnt = line.substr(pos+1);
              g_instrInfo.back().writeASVVecDelay = std::stoi(cycleCnt);
            }
          }
          break;
        case ReadASV:
          {
            size_t pos = line.find(".");
            g_readASV.insert(line);
            moduleAs.insert(line);
          }
          break;
        case ReadMEM:
          {
            remove_two_end_space(line);
            g_mem.insert(line);
          }
          break;
        case ReadFIFO:
          {
            remove_two_end_space(line);
            uint32_t colonPos = line.find(":");
            std::string fifoName = line.substr(0, colonPos);
            std::string bound = line.substr(colonPos+1);
            g_fifo.emplace(fifoName, std::stoi(bound));
          }
          break;
        case ReadNOP:
          {
            auto pos = line.find("=");
            std::string signalName = line.substr(0, pos-1);
            remove_two_end_space(signalName);            
            std::string encoding = line.substr(pos+2);
            remove_two_end_space(encoding);            
            if(!check_input_val(encoding)) {
              toCout("Encoding is not x or number[3], line is: "+line);
              abort();
            }
            if(encoding == "x") toCout("Warning: find value X in NOP: "+signalName);
            g_nopInstr.emplace(signalName, encoding);
          }
          break;
        case ResetVal:
          {
            auto pos = line.find("=");
            std::string signalName = line.substr(0, pos-1);
            remove_two_end_space(signalName);            
            std::string encoding = line.substr(pos+2);
            remove_two_end_space(encoding);
            if(!check_input_val(encoding)) {
              toCout("Encoding is not x or number[4], line is: "+line);
              abort();
            }
            g_rstVal.emplace(signalName, encoding);         
          }
          break;
        case InvarRegs:
          {
            if(line.find("_$_") != std::string::npos) {
              toCout("Error: the format for invariant regs are no longer supported!");
              abort();
            }
            auto pair = split_prefix_var(line);
            std::string modName, regName;
            modName = pair.first;
            regName = pair.second;
            if(modName.empty()) modName = g_topModule;
            if(g_invarRegs.find(modName) == g_invarRegs.end())
              g_invarRegs.emplace(modName, std::set<std::string>{regName});
            else
              g_invarRegs[modName].insert(regName);
          }
          break;
        case TopMod:
          {
            remove_two_end_space(line);
            g_topModule = line;
          }
          break;
      }
    }
  }
  if(g_recentClk.empty()) {
    toCout("Error: clock of top module is not set!");
    abort();
  }
  if(g_topModule.empty()) {
    toCout("Error: name of top module is not set!");
    abort();
  }
  toCout("### End read in instr info");
}


} // end of namespace
