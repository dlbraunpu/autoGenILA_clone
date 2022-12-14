#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <utility>
#include <stack>
#include <vector>
#include <unordered_map>
//#include "global_data_struct.h"

void hierarchical_vcd_parser(std::string fileName);

bool same_module(const std::string& name1, const std::string& name2);

bool all_are_digits(const std::string& s);

bool equal_maps( std::unordered_map<std::string, std::string>& mp1, std::unordered_map<std::string, std::string>& mp2 );

bool check_rst_value(std::map<std::string, std::unordered_map<std::string, std::string>>& rstValMap);

bool is_zero(std::string s);


//class InstanceTreeNode {
//private:
//  std::string instanceName;
//  std::string moduleName;
//  std::vector<InstanceTreeNode*> childInstance;
//
//public:
//  void append_child(InstanceTreeNode *child);
//  void set_instance_name(std::string instanceName);
//  void set_module_name(std::string moduleName);
//  std::string get_module_name();
//};
