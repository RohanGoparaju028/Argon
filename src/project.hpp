#pragma once
#include "frontend.hpp"
namespace argon {
struct Project {fs::path root,entry;std::string name;BuildOptions options;};
Project open_project(const fs::path& start,bool required=true);
void create_project(const fs::path&,bool fresh,bool cpp);
fs::path find_cpp_compiler();
int execute_process(const std::vector<std::string>&);
int build_native(const Compiled&,const Project&,const fs::path& output,bool debug);
bool within(const fs::path&,const fs::path&);
}
