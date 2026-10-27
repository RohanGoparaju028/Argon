#include "project.hpp"
#include <iostream>

namespace fs=std::filesystem;
static void usage(){
    std::cout<<"Argon 0.1.0 — C++17 compiler and integrated project builder\n"
        <<"  argonc new DIRECTORY [--cpp] [--cxx-dir DIRECTORY]\n"
        <<"  argonc init [DIRECTORY] [--cpp] [--cxx-dir DIRECTORY]\n"
        <<"  argonc build [FILE] [--debug] [-o EXECUTABLE] [--cxx-dir DIRECTORY]\n"
        <<"  argonc run [FILE] [--debug] [--cxx-dir DIRECTORY]\n"
        <<"  argonc check [FILE]             check types and global test blocks\n"
        <<"  argonc emit-cpp [FILE] [-o FILE.cpp | --stdout] [--cxx-dir DIRECTORY]\n"
        <<"  argonc FILE [-o PATH]           single-file compile shorthand\n"
        <<"  argonc doctor | --version | --help\n"
        <<"Native files: cpp/; generated output: argon-cxx/release/ (or debug/).\n"
        <<"new/init persist --cxx-dir in Argon.toml; build/run resolve it from the current directory.\n";
}
int main(int argc,char** argv){
    try{
        if(argc==1){usage();return 0;}
        std::string command=argv[1];
        if(command=="--help"||command=="-h"){usage();return 0;}
        if(command=="--version"){std::cout<<"Argon 0.1.0\n";return 0;}
        if(command=="doctor"){
            if(argc!=2)throw argon::Error("usage: argonc doctor");
            std::cout<<"C++ compiler: "<<argon::find_cpp_compiler().string()<<'\n';return 0;
        }
        if(command=="new"||command=="init"){
            fs::path path,cxx_dir="argon-cxx";bool cpp=false,custom=false;
            for(int i=2;i<argc;++i){
                std::string a=argv[i];
                if(a=="--cpp"&&!cpp)cpp=true;
                else if(a=="--cxx-dir"){
                    if(++i==argc||custom||std::string(argv[i]).empty())throw argon::Error("--cxx-dir requires one directory");
                    cxx_dir=argv[i];custom=true;
                }else if(!a.empty()&&a[0]=='-')throw argon::Error("unknown project option: "+a);
                else if(path.empty())path=a;
                else throw argon::Error("specify one project directory");
            }
            if(path.empty()){
                if(command=="new")throw argon::Error("usage: argonc new DIRECTORY [--cpp] [--cxx-dir DIRECTORY]");
                path=fs::current_path();
            }
            argon::create_project(path,command=="new",cpp,cxx_dir);return 0;
        }
        if(command=="--run")command="run";
        if(command=="--check")command="check";
        bool shorthand=command!="build"&&command!="run"&&command!="check"&&command!="emit-cpp";
        fs::path source,output,cxx_dir;bool debug=false,to_stdout=false;
        if(shorthand){
            if(!command.empty()&&command[0]=='-')throw argon::Error("unknown option: "+command);
            source=command;command="build";
        }
        for(int i=2;i<argc;++i){
            std::string a=argv[i];
            if(a=="--debug"&&!debug)debug=true;
            else if(a=="--stdout"&&!to_stdout)to_stdout=true;
            else if(a=="-o"||a=="--cxx-dir"){
                auto& destination=a=="-o"?output:cxx_dir;
                if(++i==argc||!destination.empty()||std::string(argv[i]).empty())throw argon::Error(a+" requires one path");
                destination=argv[i];
            }else if(!a.empty()&&a[0]=='-')throw argon::Error("unknown option: "+a);
            else if(source.empty())source=a;
            else throw argon::Error("compile one entry file at a time");
        }
        if(command=="check"&&(debug||!output.empty()||!cxx_dir.empty()))throw argon::Error("check does not accept output options");
        if(command=="run"&&!output.empty())throw argon::Error("run does not accept -o");
        if(command=="emit-cpp"&&debug)throw argon::Error("emit-cpp does not accept --debug");
        if(to_stdout&&(command!="emit-cpp"||!output.empty()||!cxx_dir.empty()))throw argon::Error("--stdout is only for emit-cpp without output paths");
        if(command=="emit-cpp"&&!output.empty()&&!cxx_dir.empty())throw argon::Error("emit-cpp accepts either -o or --cxx-dir");
        argon::Project project;
        if(source.empty()){project=argon::open_project(fs::current_path());source=project.entry;}
        else{
            source=fs::absolute(source);
            if(source.extension()!=".ar"&&source.extension()!=".fe")throw argon::Error("source must use .ar or .fe");
            project=argon::open_project(source.parent_path(),false);
            if(project.root.empty()){
                project.root=fs::canonical(source.parent_path());project.entry=source;project.name=source.stem().string();
                argon::prepare_project(project);
            }
        }
        if(!cxx_dir.empty())project.cxx_dir=fs::absolute(cxx_dir).lexically_normal();
        auto compiled=argon::compile(source,project.options,command!="check");
        if(command=="check"){
            std::cout<<"Checked "<<source.filename().string()<<" ("<<compiled.test_blocks<<" compile-time test blocks)\n";return 0;
        }
        if(command=="emit-cpp"){
            if(to_stdout){std::cout<<compiled.cpp;return 0;}
            if(output.empty()){
                auto directory=argon::build_directory(project,false);
                fs::create_directories(directory);output=directory/"program.cpp";
            }
            argon::validate_output(compiled,project,output);
            argon::write_file(output,compiled.cpp);
            std::cout<<"Generated "<<fs::absolute(output).string()<<'\n';return 0;
        }
        if(output.empty()){
            if(shorthand)output=fs::current_path()/source.stem();
            else{
                auto directory=argon::build_directory(project,debug);
                fs::create_directories(directory);output=directory/project.name;
            }
#ifdef _WIN32
            output+=".exe";
#endif
        }
        int status=argon::build_native(compiled,project,output,debug);if(status)return status;
        if(command=="run"){
            output=fs::absolute(output);fs::current_path(project.root);
            return argon::execute_process({output.string()});
        }
        std::cout<<"Built "<<fs::absolute(output).string()<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"argonc: "<<e.what()<<'\n';return 1;}
}
