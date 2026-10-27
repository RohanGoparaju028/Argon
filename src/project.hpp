#pragma once
#include "frontend.hpp"
namespace argon {
struct Project {fs::path root,entry,cxx_dir;std::string name;BuildOptions options;};
Project open_project(const fs::path& start,bool required=true);
void create_project(const fs::path&,bool fresh,bool cpp,const fs::path& cxx_dir="argon-cxx");
void prepare_project(Project&);
fs::path build_directory(const Project&,bool debug);
void validate_output(const Compiled&,const Project&,const fs::path&);
fs::path find_cpp_compiler();
int execute_process(const std::vector<std::string>&);
int build_native(const Compiled&,const Project&,const fs::path& output,bool debug);
bool within(const fs::path&,const fs::path&);
}
