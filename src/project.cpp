#include "project.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace argon {
bool within(const fs::path& root,const fs::path& path){auto r=fs::weakly_canonical(root),p=fs::weakly_canonical(path);auto a=r.begin(),b=p.begin();for(;a!=r.end();++a,++b)if(b==p.end()||*a!=*b)return false;return true;}
namespace {
std::string trim(std::string text){auto a=text.find_first_not_of(" \t\r\n"),b=text.find_last_not_of(" \t\r\n");return a==std::string::npos?"":text.substr(a,b-a+1);}
bool valid_name(const std::string& name){return name.size()<=64&&std::regex_match(name,std::regex("[A-Za-z0-9][A-Za-z0-9_-]*"));}
std::string project_name(const fs::path& path){auto name=path.filename().string();for(char& c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))c='-';return valid_name(name)?name:"app";}
std::vector<std::string> strings(const std::string& value,bool array,const Token& token){
    std::vector<std::string> out;size_t pos=0;auto skip=[&]{while(pos<value.size()&&std::isspace(static_cast<unsigned char>(value[pos])))++pos;};skip();if(array){if(pos==value.size()||value[pos++]!='[')throw Error("expected an array of quoted strings",token);skip();}
    while(pos<value.size()&&(!array||value[pos]!=']')){
        skip();if(pos>=value.size()||value[pos]!='"')throw Error("expected a quoted string",token);auto begin=pos++;bool closed=false;
        while(pos<value.size()){if(value[pos]=='\\'){pos+=2;continue;}if(value[pos++]=='"'){closed=true;break;}}
        if(!closed)throw Error("unterminated manifest string",token);Token t{value.substr(begin,pos-begin),"string",token.file,token.line,token.column};out.push_back(decode_string(t));skip();if(!array)break;if(pos<value.size()&&value[pos]==','){++pos;skip();}else break;
    }
    if(array){skip();if(pos==value.size()||value[pos++]!=']')throw Error("expected closing ]",token);}skip();if(pos!=value.size())throw Error("unexpected manifest text",token);if(!array&&out.size()!=1)throw Error("expected a quoted string",token);return out;
}
fs::path project_path(const Project& p,const std::string& name,bool directory){fs::path relative(name);if(name.empty()||name.find('\0')!=std::string::npos||relative.is_absolute()||relative.has_root_name())throw Error("manifest paths must be relative to the project");auto path=p.root/relative;if(!within(p.root,path))throw Error("manifest path escapes the project: "+name);if(directory?!fs::is_directory(path):!fs::is_regular_file(path))throw Error("manifest path does not exist: "+name);return fs::canonical(path);}
}
Project open_project(const fs::path& start,bool required){
    auto root=fs::absolute(start);while(!fs::exists(root/"Argon.toml")){auto parent=root.parent_path();if(parent==root){if(required)throw Error("no Argon.toml found; use argonc new NAME or argonc init");return {};}root=parent;}
    Project p;p.root=fs::canonical(root);p.entry="src/main.ar";std::istringstream input(read_file(root/"Argon.toml"));std::string text,section;int line=0;bool schema=false;std::set<std::string> seen;
    while(std::getline(input,text)){++line;bool quoted=false,escaped=false;for(size_t i=0;i<text.size();++i){char c=text[i];if(escaped){escaped=false;continue;}if(c=='\\'&&quoted){escaped=true;continue;}if(c=='"')quoted=!quoted;if(c=='#'&&!quoted){text.resize(i);break;}}text=trim(text);if(text.empty())continue;Token tok{text,"manifest",(root/"Argon.toml").string(),line,1};
        if(text.front()=='['&&text.back()==']'){section=text.substr(1,text.size()-2);if(section!="project"&&section!="cpp")throw Error("unknown manifest section",tok);continue;}
        auto equal=text.find('=');if(equal==std::string::npos)throw Error("expected key = value",tok);auto key=trim(text.substr(0,equal)),value=trim(text.substr(equal+1));if(!seen.insert(section+'.'+key).second)throw Error("duplicate manifest key",tok);
        if(section=="project"){
            if(key=="schema"){if(value!="1")throw Error("supported manifest schema is 1",tok);schema=true;}
            else if(key=="name"){p.name=strings(value,false,tok)[0];if(!valid_name(p.name))throw Error("project name must use 1-64 letters, digits, underscores, or hyphens",tok);}
            else if(key=="entry"){p.entry=strings(value,false,tok)[0];}
            else throw Error("unknown project key '"+key+"'",tok);
        }else if(section=="cpp"){
            auto items=strings(value,true,tok);
            for(const auto& item:items){
                if(key=="headers"){if(!std::regex_match(item,std::regex("[A-Za-z0-9_][A-Za-z0-9_./+-]*"))||item.find("..")!=std::string::npos)throw Error("invalid C++ header name",tok);p.options.headers.push_back(item);}
                else if(key=="libraries"){if(!std::regex_match(item,std::regex("[A-Za-z0-9_][A-Za-z0-9_.+-]*")))throw Error("invalid library name",tok);p.options.libraries.push_back(item);}
                else if(key=="sources"){auto path=project_path(p,item,false);auto ext=path.extension().string();if(ext!=".cpp"&&ext!=".cc"&&ext!=".cxx")throw Error("C++ sources must use .cpp, .cc, or .cxx",tok);p.options.sources.push_back(path);}
                else if(key=="include_dirs")p.options.include_dirs.push_back(project_path(p,item,true));
                else if(key=="library_dirs")p.options.library_dirs.push_back(project_path(p,item,true));
                else throw Error("unknown C++ key '"+key+"'",tok);
            }
            if(items.empty()&&key!="headers"&&key!="sources"&&key!="include_dirs"&&key!="library_dirs"&&key!="libraries")throw Error("unknown C++ key '"+key+"'",tok);
        }else throw Error("keys must appear under [project] or [cpp]",tok);
    }
    if(!schema||p.name.empty())throw Error("Argon.toml requires [project] schema = 1 and name");
    p.entry=project_path(p,p.entry.string(),false);
    if(p.entry.extension()!=".ar"&&p.entry.extension()!=".fe")throw Error("entry must use .ar (or legacy .fe)");return p;
}
void create_project(const fs::path& directory,bool fresh,bool cpp){
    auto root=fs::weakly_canonical(fs::absolute(directory));if(fresh&&fs::exists(fs::symlink_status(root)))throw Error("destination exists; use argonc init inside an existing directory");if(fs::exists(root)&&!fs::is_directory(root))throw Error("destination is not a directory");auto name=project_name(root);
    std::map<std::string,std::string> files;
    files["Argon.toml"]="[project]\nschema = 1\nname = "+quote(name)+"\nentry = \"src/main.ar\"\n";
    files["src/main.ar"]="test {\n    const answer: i32 = 6 * 7;\n    assert(answer == 42);\n}\n\nfn main() -> i32 {\n    println(\"Hello from Argon!\");\n    println(answer);\n    return 0;\n}\n";
    if(cpp){files["Argon.toml"]+="\n[cpp]\nheaders = [\"demo.hpp\"]\nsources = [\"native/demo.cpp\"]\ninclude_dirs = [\"native\"]\n";
        files["native/demo.hpp"]="#pragma once\n#include <cstdint>\nnamespace demo { std::int32_t twice(std::int32_t value); }\n";
        files["native/demo.cpp"]="#include \"demo.hpp\"\nstd::int32_t demo::twice(std::int32_t value) { return value * 2; }\n";
        files["src/main.ar"]="extern cpp fn twice(value: i32) -> i32 = \"demo::twice\";\n\nfn main() -> i32 {\n    unsafe {\n        println(twice(21));\n    }\n    return 0;\n}\n";
    }
    for(const auto& file:files){auto path=root/file.first;if(!within(root,path)||fs::exists(fs::symlink_status(path)))throw Error("refusing to overwrite "+path.string());auto parent=path.parent_path();if(fs::exists(parent)&&!fs::is_directory(parent))throw Error("project path is not a directory: "+parent.string());}
    for(const auto& file:files){auto path=root/file.first;fs::create_directories(path.parent_path());write_file(path,file.second);}
    if(!fs::exists(fs::symlink_status(root/".gitignore")))write_file(root/".gitignore",".argon/\nbuild/\n");
    std::cout<<"Created "<<name<<" in "<<root.string()<<"\nRun argonc run from that directory.\n";
}
fs::path find_cpp_compiler(){
    std::vector<std::string> candidates;if(const char* custom=std::getenv("ARGON_CXX");custom&&*custom)candidates.push_back(custom);else candidates={"clang++","g++","c++"};
    const char* path=std::getenv("PATH");std::string search=path?path:"";
#ifdef _WIN32
    char separator=';';
#else
    char separator=':';
#endif
    for(auto candidate:candidates){
#ifdef _WIN32
        if(fs::path(candidate).extension().empty())candidate+=".exe";
#endif
        if(fs::path(candidate).has_parent_path()){if(fs::is_regular_file(candidate))return fs::absolute(candidate);continue;}
        std::istringstream parts(search);std::string part;while(std::getline(parts,part,separator)){auto file=fs::path(part)/candidate;if(!fs::is_regular_file(file))continue;
#ifndef _WIN32
            if(access(file.c_str(),X_OK)!=0)continue;
#endif
            return fs::absolute(file);
        }
    }throw Error("C++17 compiler not found; install Clang/GCC or set ARGON_CXX to one executable path");
}
int execute_process(const std::vector<std::string>& args){
    if(args.empty())throw Error("empty process command");
#ifdef _WIN32
    std::wstring command;for(const auto& arg:args){if(!command.empty())command+=L' ';command+=L'"';size_t slashes=0;for(wchar_t c:fs::path(arg).wstring()){if(c==L'\\'){++slashes;continue;}command.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;command+=c;}command.append(slashes*2,L'\\');command+=L'"';}
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,TRUE,0,nullptr,nullptr,&startup,&process))throw Error("cannot start process: "+args[0]);WaitForSingleObject(process.hProcess,INFINITE);DWORD status=1;GetExitCodeProcess(process.hProcess,&status);CloseHandle(process.hThread);CloseHandle(process.hProcess);return static_cast<int>(status);
#else
    std::vector<char*> argv;for(const auto& arg:args)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);auto pid=fork();if(pid<0)throw Error("cannot start process");if(pid==0){execvp(argv[0],argv.data());_exit(127);}int status=0;while(waitpid(pid,&status,0)<0)if(errno!=EINTR)throw Error("cannot wait for process");return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
#endif
}
int build_native(const Compiled& program,const Project& project,const fs::path& requested,bool debug){
    auto output=fs::absolute(requested);auto directory=project.root/".argon"/"build"/(debug?"debug":"release");
    if(!within(project.root,directory))throw Error(".argon build directory must stay inside the project");
    if(fs::is_symlink(fs::symlink_status(output)))throw Error("output cannot be a symbolic link");
    if(fs::exists(output)&&(!fs::is_regular_file(output)||fs::hard_link_count(output)>1))throw Error("output must be a regular file without hard-link aliases");
    for(const auto& source:program.sources)if(fs::weakly_canonical(source)==fs::weakly_canonical(output)||(fs::exists(output)&&fs::equivalent(source,output)))throw Error("output must not overwrite source files");
    static const std::set<std::string> source_extensions={".ar",".fe",".cpp",".cc",".cxx",".h",".hpp"};if(source_extensions.count(output.extension().string()))throw Error("executable output must not use a source-file extension");
    if(!fs::is_directory(output.parent_path()))throw Error("output directory does not exist");fs::create_directories(directory);
    static std::atomic<unsigned> sequence{0};fs::path stage;
    for(int i=0;i<100;++i){auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();stage=directory/("stage-"+std::to_string(nonce)+"-"+std::to_string(sequence++));if(fs::create_directory(stage))break;stage.clear();}if(stage.empty())throw Error("cannot create build staging directory");
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}}cleanup{stage};
    auto source=stage/"program.cpp",binary=stage/"program";
#ifdef _WIN32
    binary+=".exe";
#endif
    write_file(source,program.cpp);std::vector<std::string> command={find_cpp_compiler().string(),"-std=c++17",debug?"-O0":"-O2","-fno-fast-math"};if(debug)command.push_back("-g");for(const auto& inc:project.options.include_dirs){command.push_back("-I");command.push_back(inc.string());}command.push_back(source.string());for(const auto& file:project.options.sources)command.push_back(file.string());for(const auto& lib:project.options.library_dirs){command.push_back("-L");command.push_back(lib.string());}for(const auto& lib:project.options.libraries)command.push_back("-l"+lib);command.push_back("-o");command.push_back(binary.string());
    int status=execute_process(command);if(status!=0)return status;
    auto generated=directory/"program.cpp";if(fs::is_symlink(fs::symlink_status(generated))||(fs::exists(generated)&&fs::hard_link_count(generated)>1))throw Error("generated output has filesystem aliases");fs::copy_file(source,generated,fs::copy_options::overwrite_existing);
#ifdef __APPLE__
    auto staged_symbols=fs::path(binary.string()+".dSYM");
    auto symbol_output=fs::path(output.string()+".dSYM");
    auto previous_symbols=stage/"previous.dSYM";
    bool published_symbols=false,saved_symbols=false;
    if(debug){
        if(!fs::is_directory(staged_symbols)){
            status=execute_process({"dsymutil",binary.string(),"-o",staged_symbols.string()});
            if(status!=0)return status;
        }
        if(fs::exists(fs::symlink_status(symbol_output))){
            if(fs::is_symlink(fs::symlink_status(symbol_output))||!fs::is_regular_file(symbol_output/"Contents/Info.plist"))throw Error("debug symbol output is not a compiler dSYM bundle");
            fs::rename(symbol_output,previous_symbols);saved_symbols=true;
        }
        try{fs::rename(staged_symbols,symbol_output);published_symbols=true;}
        catch(...){if(saved_symbols)fs::rename(previous_symbols,symbol_output);throw;}
    }
#endif
    try{
#ifdef _WIN32
    if(!MoveFileExW(binary.c_str(),output.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw Error("cannot replace output executable");
#else
    fs::rename(binary,output);
#endif
    }catch(...){
#ifdef __APPLE__
        if(published_symbols)fs::rename(symbol_output,staged_symbols);
        if(saved_symbols)fs::rename(previous_symbols,symbol_output);
#endif
        throw;
    }
    return 0;
}
}
