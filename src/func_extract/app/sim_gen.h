#include <set>
#include <string>
#include <map>
#include <vector>
#include "../src/parse_fill.h"

using namespace funcExtract;
using namespace taintGen;


std::string asv_type(uint32_t width);

std::string func_call(std::string writeASV, std::string funcName, 
                      const std::vector<std::pair<uint32_t, std::string>> &argTy,
                      const std::map<std::string, std::vector<std::string>> &inputInstr);                      

uint32_t get_instr_by_name(std::string instrName);

void print_func_declare(struct FuncTy_t funcTy, 
                        std::string funcName, 
                        std::ofstream &header);
