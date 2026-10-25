#include "project.hpp"
#include <iostream>

namespace fs=std::filesystem;
static void usage(){std::cout<<"Argon 0.1.0 — C++17 compiler and integrated project builder\n"
    <<"  argonc new DIRECTORY [--cpp]     create a project (optionally with a C++ library)\n"
    <<"  argonc init [DIRECTORY] [--cpp]  initialize an existing directory\n"
    <<"  argonc build [FILE] [--debug] [-o EXECUTABLE]\n"
    <<"  argonc run [FILE] [--debug]\n"
    <<"  argonc check [FILE]             type-check and execute global test blocks\n"
    <<"  argonc emit-cpp [FILE] [-o FILE.cpp]\n"
    <<"  argonc FILE                    compile one .ar (or legacy .fe) file\n"
    <<"  argonc doctor | --version | --help\n";
}
int main(int argc,char** argv){
    try{
        if(argc==1){usage();return 0;}std::string command=argv[1];if(command=="--help"||command=="-h"){usage();return 0;}if(command=="--version"){std::cout<<"Argon 0.1.0\n";return 0;}
        if(command=="doctor"){if(argc!=2)throw argon::Error("usage: argonc doctor");std::cout<<"C++ compiler: "<<argon::find_cpp_compiler().string()<<'\n';return 0;}
        if(command=="new"||command=="init"){
            fs::path path;bool cpp=false;for(int i=2;i<argc;++i){std::string a=argv[i];if(a=="--cpp"&&!cpp)cpp=true;else if(!a.empty()&&a[0]=='-')throw argon::Error("unknown project option: "+a);else if(path.empty())path=a;else throw argon::Error("specify one project directory");}if(path.empty()){if(command=="new")throw argon::Error("usage: argonc new DIRECTORY [--cpp]");path=fs::current_path();}argon::create_project(path,command=="new",cpp);return 0;
        }
        bool shorthand=command!="build"&&command!="run"&&command!="check"&&command!="emit-cpp";if(command=="--run"){command="run";shorthand=false;}if(command=="--check"){command="check";shorthand=false;}
        fs::path source,output;bool debug=false;if(shorthand){if(!command.empty()&&command[0]=='-')throw argon::Error("unknown option: "+command);source=command;command="build";}
        for(int i=2;i<argc;++i){std::string a=argv[i];if(a=="--debug"&&!debug)debug=true;else if(a=="-o"){if(++i==argc||!output.empty())throw argon::Error("-o requires one output path");output=argv[i];}else if(!a.empty()&&a[0]=='-')throw argon::Error("unknown option: "+a);else if(source.empty())source=a;else throw argon::Error("compile one entry file at a time");}
        if(command=="check"&&(debug||!output.empty()))throw argon::Error("check does not accept output options");if(command=="run"&&!output.empty())throw argon::Error("run does not accept -o");if(command=="emit-cpp"&&debug)throw argon::Error("emit-cpp does not accept --debug");
        argon::Project project;
        if(source.empty()){project=argon::open_project(fs::current_path());source=project.entry;}
        else{source=fs::absolute(source);if(source.extension()!=".ar"&&source.extension()!=".fe")throw argon::Error("source must use .ar or .fe");project=argon::open_project(source.parent_path(),false);if(project.root.empty()){project.root=source.parent_path();project.entry=source;project.name=source.stem().string();}}
        auto compiled=argon::compile(source,project.options,command!="check");
        if(command=="check"){std::cout<<"Checked "<<source.filename().string()<<" ("<<compiled.test_blocks<<" compile-time test blocks)\n";return 0;}
        if(command=="emit-cpp"){
            if(output.empty())std::cout<<compiled.cpp;else{auto protected_files=compiled.sources;protected_files.insert(protected_files.end(),project.options.sources.begin(),project.options.sources.end());for(const auto& header:project.options.headers)for(const auto& directory:project.options.include_dirs)if(fs::is_regular_file(directory/header))protected_files.push_back(directory/header);for(const auto& file:protected_files)if(fs::weakly_canonical(file)==fs::weakly_canonical(output)||(fs::exists(output)&&fs::equivalent(file,output)))throw argon::Error("output must not overwrite source files");if(fs::is_symlink(fs::symlink_status(output))||(fs::exists(output)&&fs::hard_link_count(output)>1))throw argon::Error("output has filesystem aliases");argon::write_file(output,compiled.cpp);}return 0;
        }
        if(output.empty()){
            if(shorthand)output=fs::current_path()/source.stem();else{auto directory=project.root/".argon"/"build"/(debug?"debug":"release");if(!argon::within(project.root,directory))throw argon::Error("build directory escapes the project");fs::create_directories(directory);output=directory/project.name;}
#ifdef _WIN32
            output+=".exe";
#endif
        }
        int status=argon::build_native(compiled,project,output,debug);if(status)return status;
        if(command=="run"){fs::current_path(project.root);return argon::execute_process({fs::absolute(output).string()});}
        std::cout<<"Built "<<fs::absolute(output).string()<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"argonc: "<<e.what()<<'\n';return 1;}
}
