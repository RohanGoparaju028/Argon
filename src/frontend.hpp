#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace argon {
namespace fs = std::filesystem;
struct Token { std::string text, kind, file; int line=1, column=1; };
struct Error : std::runtime_error {
    explicit Error(const std::string& message):std::runtime_error(message){}
    Error(const std::string& message,const Token& t):std::runtime_error(t.file+":"+std::to_string(t.line)+":"+std::to_string(t.column)+": "+message){}
};
std::string read_file(const fs::path&);
void write_file(const fs::path&,const std::string&);
std::string quote(const std::string&);
std::string decode_string(const Token&);
struct Type {
    std::string name; std::vector<Type> args;
    bool operator==(const Type& b)const{return name==b.name&&args==b.args;}
    bool operator!=(const Type& b)const{return !(*this==b);}
};
inline const Type ti32{"i32",{}},ti64{"i64",{}},tf64{"f64",{}},tbool{"bool",{}},tstring{"string",{}},tvoid{"void",{}};
std::string spelling(const Type&);
struct Node; struct Decl; struct Unit; struct Variable;
using N=std::shared_ptr<Node>;
using Value=std::variant<std::monostate,int64_t,double,bool,std::string>;
struct Node {
    std::string kind,text; Token token;
    std::vector<N> children;
    Type annotation,type;
    bool constant=false;
    Variable* variable=nullptr; Decl* declaration=nullptr;
    int index=-1;
    std::vector<Variable*> bindings;
    std::vector<Type> argument_types;
    Value value;
};
N node(const std::string&,const Token&,const std::string& text="");
struct Variable { std::string name,cpp;Type type;bool constant=false; };
struct Decl {
    std::string kind,name,cpp,mode,foreign;
    Token token;Unit* unit=nullptr;bool public_=false,test_export=false;
    Type result;
    std::vector<std::pair<std::string,Type>> parameters,fields;
    std::map<std::string,std::vector<std::pair<std::string,Type>>> variants;
    std::vector<std::string> variant_order;
    std::vector<Variable*> arguments;
    N body;
    Value value;
    int state=0;
};
struct Unit {
    fs::path file;int id=0;
    std::vector<std::string> imports;
    std::map<std::string,Unit*> dependencies;
    std::vector<std::unique_ptr<Decl>> declarations;
    std::map<std::string,Decl*> names;
    std::vector<N> tests;
};
void parse(Unit&,const std::string&);
struct BuildOptions {
    std::vector<std::string> headers,libraries;
    std::vector<fs::path> sources,include_dirs,library_dirs;
};
struct Compiled { std::string cpp; std::vector<fs::path> sources;size_t test_blocks=0; };
Compiled compile(const fs::path&,const BuildOptions& = {},bool require_main=true);
}
