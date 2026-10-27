#include "frontend.hpp"
#include "embedded.hpp"
#include "runtime.hpp"
#include <algorithm>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>

namespace argon {
namespace {
bool numeric(const Type& t){return t==ti32||t==ti64||t==tf64;}
bool integer(const Type& t){return t==ti32||t==ti64;}
bool scalar(const Type& t){return numeric(t)||t==tbool||t==tstring;}
bool collection(const Type& t){static const std::set<std::string> names={"Array","LinkedList","Deque","Stack","Queue","HashMap","HashSet"};return names.count(t.name)!=0;}
bool indexed(const Type& t){return t.name=="Array"||t.name=="LinkedList"||t.name=="Deque"||t.name=="HashMap";}
bool compatible(const Type& to,const Type& from){return to==from||(to==ti64&&from==ti32)||(to==tf64&&integer(from));}
void require(const Type& to,const Type& from,const Token& tok){if(!compatible(to,from))throw Error("expected "+spelling(to)+", got "+spelling(from),tok);}
Type common(const Type& a,const Type& b,const Token& tok){if(a==b)return a;if(numeric(a)&&numeric(b))return a==tf64||b==tf64?tf64:ti64;throw Error("incompatible expression types",tok);}
std::string path(N n){if(n->kind=="name")return n->text;if(n->kind=="member"){auto p=path(n->children[0]);return p.empty()?"":p+'.'+n->text;}return "";}
struct Return{Value value;};struct Break{};struct Continue{};
class Compiler {
    std::vector<std::unique_ptr<Unit>> units;
    std::vector<std::unique_ptr<Variable>> variables;
    std::vector<Decl*> types,functions,constants,type_order;
    Unit* entry=nullptr;Unit* current_unit=nullptr;Decl* current_function=nullptr;
    std::vector<std::map<std::string,Variable*>> scopes;
    std::map<std::string,Unit*> loaded;
    std::map<std::string,Decl*> type_names;
    std::set<Decl*> checked,checking;
    int loops=0,switches=0,unsafe_depth=0;
    bool in_test=false;
    size_t steps=0;int eval_depth=0;
    std::map<Variable*,Value> eval_variables;
    Unit* load(const fs::path& filename,std::set<std::string>& stack){
        auto file=fs::canonical(filename);auto key=file.string();if(stack.count(key))throw Error("circular import: "+key);if(loaded.count(key))return loaded.at(key);if(units.size()>=256)throw Error("too many imported files");
        auto unit=std::make_unique<Unit>();auto* u=unit.get();u->file=file;u->id=int(units.size());units.push_back(std::move(unit));loaded[key]=u;parse(*u,read_file(file));stack.insert(key);
        for(const auto& name:u->imports){
            if(name=="std"||name=="std.io"||name=="std.core"||name=="std.collections"||name=="std.math"||name=="std.memory")continue;
            auto relative=name;std::replace(relative.begin(),relative.end(),'.','/');fs::path found;
            for(const auto& base:{file.parent_path(),units.front()->file.parent_path()})for(const auto& extension:{".ar",".fe"})if(found.empty()&&fs::is_regular_file(base/(relative+extension)))found=base/(relative+extension);
            if(found.empty())throw Error("cannot find import '"+name+"' from "+file.string());u->dependencies[name]=load(found,stack);
        }stack.erase(key);return u;
    }
    Decl* lookup(Unit* u,const std::string& name,const Token& t,bool required=true){
        if(u->names.count(name))return u->names.at(name);Decl* found=nullptr;
        for(const auto& dep:u->dependencies)for(const auto& item:dep.second->names){auto* d=item.second;if(!d->public_)continue;auto dot=dep.first.find_last_of('.');auto shortname=dot==std::string::npos?dep.first:dep.first.substr(dot+1);
            if(name==item.first||name==dep.first+'.'+item.first||name==shortname+'.'+item.first){if(found&&found!=d)throw Error("ambiguous imported name '"+name+"'",t);found=d;}}
        if(!found&&required)throw Error("unknown or private name '"+name+"'",t);return found;
    }
    bool class_type(const Type& t){return type_names.count(t.name)&&type_names.at(t.name)->kind=="class";}
    bool owner(const Type& t){return class_type(t)||collection(t)||t.name=="Ptr";}
    Type owning(const Type& t){return class_type(t)||collection(t)?t:Type{"Ptr",{t}};}
    Type resolve(Type t,Unit* u,const Token& tok,bool void_ok=false){
        if(collection(t)||t.name=="Ptr"||t.name=="Weak"){
            size_t count=t.name=="HashMap"?2:1;if(t.args.size()!=count)throw Error(t.name+" requires "+std::to_string(count)+" type arguments",tok);for(auto& arg:t.args)arg=resolve(arg,u,tok);
            if((t.name=="HashMap"||t.name=="HashSet")&&!integer(t.args[0])&&t.args[0]!=tstring&&t.args[0]!=tbool)throw Error("hash keys must be integers, bool, or string",tok);
            if(t.name=="Ptr"&&(class_type(t.args[0])||collection(t.args[0])))return t.args[0];
            return t;
        }
        if(!t.args.empty())throw Error("generic user types are not implemented",tok);
        if(t==tvoid){if(!void_ok)throw Error("void cannot be stored as a value",tok);return t;}
        if(scalar(t))return t;auto* d=lookup(u,t.name,tok);if(d->kind!="struct"&&d->kind!="class"&&d->kind!="enum")throw Error("expected a type",tok);return {d->cpp,{}};
    }
    Variable* local(const std::string& name){for(auto i=scopes.rbegin();i!=scopes.rend();++i)if(i->count(name))return i->at(name);return nullptr;}
    Variable* variable(const std::string& name,Type type,bool constant){auto v=std::make_unique<Variable>();v->name=name;v->type=std::move(type);v->constant=constant;v->cpp="ag_v"+std::to_string(variables.size())+"_"+name;auto* p=v.get();variables.push_back(std::move(v));return p;}
    bool writable(N n){if(n->kind=="name")return n->variable&&!n->variable->constant;if(n->kind=="member"||n->kind=="index"||(n->kind=="unary"&&n->text=="*"&&n->children[0]->type.name=="Ptr"))return writable(n->children[0]);return false;}
    void number(N n){
        auto text=n->text;Type type;std::string suffix=text.size()>3?text.substr(text.size()-3):"";
        if(suffix=="i32"||suffix=="i64"||suffix=="f64"){type={suffix,{}};text.resize(text.size()-3);}else type=text.find_first_of(".eE")!=std::string::npos?tf64:ti32;
        try{
            size_t used=0;if(type==tf64){double x=std::stod(text,&used);if(!std::isfinite(x)||used!=text.size())throw std::out_of_range("number");n->value=x;}
            else{auto x=std::stoll(text,&used);if(used!=text.size())throw std::out_of_range("number");if(suffix.empty()&&(x<INT32_MIN||x>INT32_MAX))type=ti64;if(type==ti32&&(x<INT32_MIN||x>INT32_MAX))throw std::out_of_range("number");n->value=int64_t(x);}
        }catch(const std::exception&){throw Error("invalid or out-of-range numeric literal",n->token);}n->type=type;
    }
    Type global_type(Decl* d){
        if(d->test_export)return d->result;
        if(d->state==1)throw Error("circular constant definition",d->token);if(d->state==2)return d->result;d->state=1;
        auto* old=current_unit;auto oldscopes=scopes;scopes.clear();current_unit=d->unit;
        auto actual=expression(d->body);
        std::function<void(N)> validate=[&](N n){if(n->kind=="call"&&n->declaration&&n->declaration->kind=="function")throw Error("function-driven metaprogramming belongs in a global test block",n->token);for(const auto& child:n->children)validate(child);};validate(d->body);
        if(d->result.name.empty())d->result=actual;else require(d->result,actual,d->token);
        if(d->result==tvoid)throw Error("constant needs a value",d->token);d->state=2;current_unit=old;scopes=std::move(oldscopes);return d->result;
    }
    std::vector<N> args(N n){return {n->children.begin()+1,n->children.end()};}
    void arguments(const std::vector<std::pair<std::string,Type>>& params,const std::vector<N>& actual,const Token& t){if(params.size()!=actual.size())throw Error("wrong argument count",t);for(size_t i=0;i<actual.size();++i)require(params[i].second,expression(actual[i]),actual[i]->token);}
    void method_args(N n,const std::vector<std::pair<std::string,Type>>& params,const std::vector<N>& actual){arguments(params,actual,n->token);n->argument_types.clear();for(const auto& p:params)n->argument_types.push_back(p.second);}
    Type expression(N n){
        auto k=n->kind;if(k=="number"){number(n);return n->type;}if(k=="string"){n->value=n->text;return n->type=tstring;}if(k=="boolean"){n->value=n->text=="true";return n->type=tbool;}
        if(k=="name"||k=="member"){
            if(k=="name")if(auto* v=local(n->text)){n->variable=v;return n->type=v->type;}
            auto q=path(n);bool rooted=!q.empty()&&local(q.substr(0,q.find('.')));
            if(!q.empty()&&!rooted)if(auto* d=lookup(current_unit,q,n->token,false)){if(d->kind!="constant")throw Error("function or type name needs a call",n->token);n->declaration=d;return n->type=global_type(d);}
            if(k=="member"){
                auto type=expression(n->children[0]);if(type.name=="Ptr")type=type.args[0];auto found=type_names.find(type.name);if(found==type_names.end()||(found->second->kind!="struct"&&found->second->kind!="class"))throw Error("field access requires a struct, class, or owning pointer to one",n->token);
                auto* d=found->second;for(size_t i=0;i<d->fields.size();++i)if(d->fields[i].first==n->text){n->index=int(i);return n->type=d->fields[i].second;}throw Error("unknown field '"+n->text+"'",n->token);
            }throw Error("unknown value '"+n->text+"'",n->token);
        }
        if(k=="variant"){
            auto* d=lookup(current_unit,path(n->children[0]),n->token);if(d->kind!="enum"||!d->variants.count(n->text))throw Error("unknown enum variant",n->token);if(!d->variants.at(n->text).empty())throw Error("variant payload requires a call",n->token);n->declaration=d;return n->type={d->cpp,{}};
        }
        if(k=="call"){
            auto target=n->children[0];auto actual=args(n);auto q=path(target);bool rooted=!q.empty()&&local(q.substr(0,q.find('.')));
            if(target->kind=="name"&&!target->annotation.name.empty()){
                auto type=resolve(target->annotation,current_unit,n->token);auto kind=target->annotation.name;
                if(kind=="Ptr"){
                    if(actual.size()>1)throw Error("Ptr takes zero or one value",n->token);n->text=actual.empty()?"pointer_empty":type.name=="Ptr"?"pointer":"retain";
                    if(!actual.empty())method_args(n,{{"value",type.name=="Ptr"?type.args[0]:type}},actual);return n->type=type;
                }
                if(kind=="Weak"){if(actual.size()>1)throw Error("Weak takes zero or one owner",n->token);if(!actual.empty())method_args(n,{{"owner",owning(type.args[0])}},actual);n->text="weak_ctor";return n->type=type;}
                if(!actual.empty())throw Error("collection constructors take no arguments",n->token);n->text="container";return n->type=type;
            }
            if(target->kind=="variant"){
                auto* d=lookup(current_unit,path(target->children[0]),n->token);if(d->kind!="enum"||!d->variants.count(target->text))throw Error("unknown enum variant",n->token);arguments(d->variants.at(target->text),actual,n->token);n->text="variant";n->declaration=d;n->index=int(std::find(d->variant_order.begin(),d->variant_order.end(),target->text)-d->variant_order.begin());return n->type={d->cpp,{}};
            }
            if(!q.empty()&&!rooted)if(auto* d=lookup(current_unit,q,n->token,false)){
                n->declaration=d;if(d->kind=="function"){
                    if(d->mode=="foreign"&&!unsafe_depth)throw Error("C++ calls require an unsafe block",n->token);
                    arguments(d->parameters,actual,n->token);return n->type=d->result;
                }if(d->kind=="struct"||d->kind=="class"){arguments(d->fields,actual,n->token);n->text=d->kind;return n->type={d->cpp,{}};}throw Error("value is not callable",n->token);
            }
            if(!rooted&&!q.empty()){
                if(q.rfind("std.io.",0)==0)q=q.substr(7);else if(q.rfind("std.core.",0)==0)q=q.substr(9);else if(q.rfind("std.math.",0)==0)q=q.substr(9);else if(q.rfind("std.memory.",0)==0)q=q.substr(11);
                n->text=q;
                std::vector<Type> ts;for(auto a:actual)ts.push_back(expression(a));
                if(q=="i32"||q=="i64"||q=="f64"){if(ts.size()!=1||!numeric(ts[0]))throw Error("numeric cast requires one numeric argument",n->token);return n->type={q,{}};}
                if(q=="print"||q=="println"){if(ts.size()>1||(q=="print"&&ts.empty())||(!ts.empty()&&!scalar(ts[0])))throw Error("printing takes one scalar value (println also accepts none)",n->token);return n->type=tvoid;}
                if(q=="read_line"){if(!ts.empty())throw Error("read_line takes no arguments",n->token);return n->type=tstring;}
                if(q=="assert"){if(ts.size()!=1||ts[0]!=tbool)throw Error("assert takes one bool",n->token);return n->type=tvoid;}
                if(q=="str"||q=="string"){if(ts.size()!=1||!scalar(ts[0]))throw Error("str takes one scalar value",n->token);n->text="str";return n->type=tstring;}
                if(q=="length"){if(ts.size()!=1||(ts[0]!=tstring&&!collection(ts[0])))throw Error("length requires a string or collection",n->token);return n->type=ti64;}
                if(q=="weak"){if(ts.size()!=1||!owner(ts[0]))throw Error("weak requires one owning handle",n->token);return n->type={"Weak",{ts[0].name=="Ptr"?ts[0].args[0]:ts[0]}};}
                if(q=="upgrade"){if(ts.size()!=1||ts[0].name!="Weak")throw Error("upgrade requires one weak pointer",n->token);return n->type=owning(ts[0].args[0]);}
                if(q=="is_expired"){if(ts.size()!=1||ts[0].name!="Weak")throw Error("is_expired requires one weak pointer",n->token);return n->type=tbool;}
                if(q=="is_empty"){if(ts.size()!=1||!owner(ts[0]))throw Error("is_empty requires one owning handle",n->token);return n->type=tbool;}
                if(q=="strong_count"){if(ts.size()!=1||(!owner(ts[0])&&ts[0].name!="Weak"))throw Error("strong_count requires one owning or weak handle",n->token);return n->type=ti64;}
            }
            if(target->kind=="member"){
                auto receiver=target->children[0];auto type=expression(receiver);auto method=target->text;n->text="method."+method;
                std::vector<std::pair<std::string,Type>> params;Type result;bool mutate=false,valid=true;
                if((owner(type)||type.name=="Weak")&&method=="use_count")result=ti64;
                else if((owner(type)||type.name=="Weak")&&method=="reset"){result=tvoid;mutate=true;}
                else if(type.name=="Ptr"&&method=="get")result=type.args[0];
                else if(type.name=="Weak"&&method=="upgrade")result=owning(type.args[0]);
                else if(type.name=="Weak"&&method=="is_expired")result=tbool;
                else if(collection(type)){
                    auto item=type.args[0];
                    if(method=="length")result=ti64;
                    else if(method=="is_empty")result=tbool;
                    else if(method=="clone")result=type;
                    else if(method=="clear"){result=tvoid;mutate=true;}
                    else if(type.name=="HashMap"){
                        if(method=="get"){params={{"key",item}};result=type.args[1];}
                        else if(method=="set"){params={{"key",item},{"value",type.args[1]}};result=tvoid;mutate=true;}
                        else if(method=="contains"||method=="remove"){params={{"key",item}};result=tbool;mutate=method=="remove";}
                        else if(method=="keys")result={"Array",{item}};else valid=false;
                    }else if(type.name=="HashSet"){
                        if(method=="insert"||method=="contains"||method=="remove"){params={{"key",item}};result=tbool;mutate=method!="contains";}
                        else if(method=="items")result={"Array",{item}};else valid=false;
                    }else if(method=="push"&&type.name!="Queue"){params={{"value",item}};result=tvoid;mutate=true;}
                    else if(method=="pop"&&type.name!="Queue"){result=item;mutate=true;}
                    else if(method=="enqueue"&&type.name=="Queue"){params={{"value",item}};result=tvoid;mutate=true;}
                    else if(method=="dequeue"&&type.name=="Queue"){result=item;mutate=true;}
                    else if(method=="top"&&type.name=="Stack")result=item;
                    else if(method=="front"&&(type.name=="Queue"||indexed(type)))result=item;
                    else if(method=="back"&&indexed(type))result=item;
                    else if(method=="push_front"&&(type.name=="LinkedList"||type.name=="Deque")){params={{"value",item}};result=tvoid;mutate=true;}
                    else if(method=="pop_front"&&(type.name=="LinkedList"||type.name=="Deque")){result=item;mutate=true;}
                    else if(method=="get"&&indexed(type)){params={{"index",ti64}};result=item;}
                    else if(method=="set"&&indexed(type)){params={{"index",ti64},{"value",item}};result=tvoid;mutate=true;}
                    else valid=false;
                }else valid=false;
                if(valid){if(mutate&&!writable(receiver))throw Error("cannot mutate an immutable or temporary handle",n->token);method_args(n,params,actual);return n->type=result;}
            }throw Error("unknown callable",n->token);
        }
        if(k=="index"){auto a=expression(n->children[0]);if(!indexed(a))throw Error("indexing requires Array, LinkedList, Deque, or HashMap",n->token);require(a.name=="HashMap"?a.args[0]:ti64,expression(n->children[1]),n->token);return n->type=a.name=="HashMap"?a.args[1]:a.args[0];}
        if(k=="unary"||k=="update"){auto t=expression(n->children[0]);if(n->text=="*"){if(t.name=="Ptr")return n->type=t.args[0];if(class_type(t))return n->type=t;throw Error("dereference requires an owning pointer",n->token);}if(n->text=="!"){require(tbool,t,n->token);return n->type=tbool;}if(!numeric(t))throw Error("numeric operator requires a number",n->token);if(k=="update"&&!writable(n->children[0]))throw Error("cannot update an immutable or temporary value",n->token);return n->type=t;}
        if(k=="binary"){
            auto op=n->text;if(op=="|>"){auto call=node("call",n->token);call->children={n->children[1],n->children[0]};*n=*call;return expression(n);}
            auto a=expression(n->children[0]),b=expression(n->children[1]);
            if(op=="="||op=="+="||op=="-="||op=="*="||op=="/="||op=="%="){
                if(!writable(n->children[0]))throw Error("cannot assign to an immutable or temporary value",n->token);require(a,b,n->token);if(op!="="&&!numeric(a)&&!(op=="+="&&a==tstring))throw Error("invalid compound assignment",n->token);if(op=="%="&&!integer(a))throw Error("remainder needs integers",n->token);return n->type=a;
            }
            if(op=="&&"||op=="||"){require(tbool,a,n->token);require(tbool,b,n->token);return n->type=tbool;}
            if(op=="=="||op=="!="||op=="<"||op==">"||op=="<="||op==">="){if((op=="=="||op=="!=")&&owner(a)&&a==b){n->annotation=a;return n->type=tbool;}if(!scalar(a)||!scalar(b))throw Error("comparison requires scalar values or matching owning handles",n->token);n->annotation=common(a,b,n->token);return n->type=tbool;}
            if(op=="+"&&a==tstring&&b==tstring)return n->type=tstring;
            if(!numeric(a)||!numeric(b))throw Error("arithmetic requires numbers",n->token);if(op=="%"&&(!integer(a)||!integer(b)))throw Error("remainder needs integers",n->token);return n->type=common(a,b,n->token);
        }
        if(k=="match"){
            auto type=expression(n->children[0]);Decl* en=type_names.count(type.name)?type_names.at(type.name):nullptr;
            if((!en||en->kind!="enum")&&!scalar(type))throw Error("match requires an enum or scalar",n->token);
            bool wildcard=false;std::set<std::string> seen;Type result;
            for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];if(wildcard)throw Error("unreachable match arm after wildcard",arm->token);scopes.emplace_back();
                if(arm->constant){if(arm->children.size()!=1)throw Error("wildcard has no payload",arm->token);wildcard=true;}
                else if(en){auto sep=arm->text.find("::");if(sep==std::string::npos||lookup(current_unit,arm->text.substr(0,sep),arm->token)!=en)throw Error("pattern must name a variant of the matched enum",arm->token);auto variant=arm->text.substr(sep+2);if(!en->variants.count(variant))throw Error("unknown variant in pattern",arm->token);auto& fields=en->variants.at(variant);if(fields.size()+1!=arm->children.size())throw Error("wrong pattern payload count",arm->token);arm->index=int(std::find(en->variant_order.begin(),en->variant_order.end(),variant)-en->variant_order.begin());
                    for(size_t j=0;j<fields.size();++j){auto name=arm->children[j]->text;if(scopes.back().count(name))throw Error("duplicate pattern binding",arm->token);auto* v=variable(name,fields[j].second,true);arm->bindings.push_back(v);scopes.back()[name]=v;}
                }else{
                    if(arm->children.size()!=1)throw Error("scalar pattern cannot bind a payload",arm->token);auto label=node(type==tbool?"boolean":type==tstring?"string":"number",arm->token,arm->text);
                    if(type==tbool&&arm->text!="true"&&arm->text!="false")throw Error("bool pattern must be true or false",arm->token);
                    if(type==tstring&&arm->token.kind!="string")throw Error("string pattern needs a string literal",arm->token);
                    if(numeric(type)&&arm->token.kind!="number")throw Error("numeric pattern needs a literal",arm->token);
                    require(type,expression(label),arm->token);arm->value=convert(label->value,type,arm->token);arm->annotation=type;
                }
                auto key=arm->constant?"wildcard":en?"enum:"+std::to_string(arm->index):"value:"+value_string(arm->value);
                if(auto* f=std::get_if<double>(&arm->value);f&&*f==0&&!arm->constant)key="value:0";
                if(!seen.insert(key).second)throw Error("duplicate match pattern",arm->token);
                auto t=expression(arm->children.back());if(t==tvoid)throw Error("match arms need values",arm->token);result=result.name.empty()?t:common(result,t,arm->token);scopes.pop_back();
            }
            if(result.name.empty())throw Error("match needs at least one arm",n->token);
            if(!wildcard&&!(en&&seen.size()==en->variants.size())&&!(type==tbool&&seen.count("value:true")&&seen.count("value:false")))throw Error("non-exhaustive match; cover every variant or add _",n->token);return n->type=result;
        }
        throw Error("unsupported expression",n->token);
    }
    bool statement(N n){
        auto k=n->kind;
        if(k=="block"){scopes.emplace_back();bool ended=false;for(auto s:n->children)ended=statement(s)||ended;scopes.pop_back();return ended;}
        if(k=="binding"){
            if(scopes.back().count(n->text))throw Error("duplicate local binding",n->token);auto actual=expression(n->children[0]);auto type=n->annotation.name.empty()?actual:resolve(n->annotation,current_unit,n->token);require(type,actual,n->token);if(type==tvoid)throw Error("binding requires a value",n->token);
            n->type=type;n->variable=variable(n->text,type,n->constant);scopes.back()[n->text]=n->variable;return false;
        }
        if(k=="return"){if(!current_function)throw Error("return is not allowed in a global test",n->token);require(current_function->result,n->children.empty()?tvoid:expression(n->children[0]),n->token);return true;}
        if(k=="expression"){if(!n->children.empty())expression(n->children[0]);return false;}
        if(k=="unsafe"){if(in_test)throw Error("unsafe is not permitted in compile-time test blocks",n->token);++unsafe_depth;auto result=statement(n->children[0]);--unsafe_depth;return result;}
        if(k=="if"){require(tbool,expression(n->children[0]),n->token);bool yes=statement(n->children[1]),no=n->children.size()==3&&statement(n->children[2]);return yes&&no;}
        if(k=="while"||k=="for"){
            scopes.emplace_back();if(k=="for")statement(n->children[0]);require(tbool,expression(n->children[k=="for"?1:0]),n->token);if(k=="for")expression(n->children[2]);++loops;statement(n->children.back());--loops;scopes.pop_back();return false;
        }
        if(k=="switch"){
            auto type=expression(n->children[0]);if(!integer(type)&&type!=tbool&&type!=tstring)throw Error("switch requires an integer, bool, or string",n->token);bool fallback=false,all=true;std::set<std::string> labels;++switches;
            for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];if(arm->constant){if(fallback)throw Error("duplicate default",arm->token);fallback=true;}
                else{auto label=arm->children[0];require(type,expression(label),arm->token);if(label->kind!="number"&&label->kind!="boolean"&&label->kind!="string")throw Error("case requires a literal",arm->token);if(!labels.insert(value_string(convert(label->value,type,arm->token))).second)throw Error("duplicate case label",arm->token);}
                all=statement(arm->children.back())&&all;
            }--switches;return fallback&&all;
        }
        if(k=="break"){if(!loops&&!switches)throw Error("break requires a loop or switch",n->token);return false;}
        if(k=="continue"){if(!loops)throw Error("continue requires a loop",n->token);return false;}
        return false;
    }
    int flow(N n){
        constexpr int normal=1,returned=2,broken=4,continued=8;
        if(n->kind=="return")return returned;if(n->kind=="break")return broken;if(n->kind=="continue")return continued;
        if(n->kind=="block"){int out=normal;for(auto s:n->children)if(out&normal)out=(out&~normal)|flow(s);return out;}
        if(n->kind=="if")return flow(n->children[1])|(n->children.size()==3?flow(n->children[2]):normal);
        if(n->kind=="unsafe")return flow(n->children[0]);
        if(n->kind=="while"||n->kind=="for")return normal|(flow(n->children.back())&returned);
        if(n->kind=="switch"){int out=0;bool fallback=false;for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];fallback=fallback||arm->constant;auto f=flow(arm->children.back());out|=(f&~broken)|((f&broken)?normal:0);}return out|(fallback?0:normal);}
        return normal;
    }
    void check_function(Decl* d){
        if(checked.count(d)||d->mode=="foreign")return;checked.insert(d);auto* oldunit=current_unit;auto* oldfn=current_function;auto oldscopes=scopes;int oldloops=loops,oldswitches=switches,oldunsafe=unsafe_depth;bool oldtest=in_test;
        current_unit=d->unit;current_function=d;scopes.clear();scopes.emplace_back();loops=switches=unsafe_depth=0;in_test=false;
        for(const auto& a:d->parameters){if(scopes.back().count(a.first))throw Error("duplicate parameter",d->token);auto* v=variable(a.first,a.second,false);d->arguments.push_back(v);scopes.back()[a.first]=v;}
        statement(d->body);if(d->result!=tvoid&&(flow(d->body)&1))throw Error("function must return a value on every path",d->token);
        current_unit=oldunit;current_function=oldfn;scopes=std::move(oldscopes);loops=oldloops;switches=oldswitches;unsafe_depth=oldunsafe;in_test=oldtest;
    }
    void tick(const Token& t){if(++steps>100000)throw Error("compile-time evaluation exceeded 100000 steps",t);}
    Value convert(Value v,const Type& type,const Token& tok){
        try{
            if(type==ti32){if(auto* x=std::get_if<int64_t>(&v))return int64_t(ag_runtime::cast<int32_t>(*x));if(auto* x=std::get_if<double>(&v))return int64_t(ag_runtime::cast<int32_t>(*x));}
            if(type==ti64){if(std::holds_alternative<int64_t>(v))return v;if(auto* x=std::get_if<double>(&v))return ag_runtime::cast<int64_t>(*x);}
            if(type==tf64){if(auto* x=std::get_if<int64_t>(&v))return double(*x);if(std::holds_alternative<double>(v))return v;}
            if(type==tbool&&std::holds_alternative<bool>(v))return v;if(type==tstring&&std::holds_alternative<std::string>(v))return v;if(type==tvoid&&std::holds_alternative<std::monostate>(v))return v;
        }catch(const std::exception& e){throw Error(e.what(),tok);}throw Error("this value is not supported in compile-time evaluation",tok);
    }
    std::string value_string(const Value& v){if(auto* x=std::get_if<std::string>(&v))return *x;if(auto* x=std::get_if<bool>(&v))return ag_runtime::str(*x);if(auto* x=std::get_if<int64_t>(&v))return ag_runtime::str(*x);if(auto* x=std::get_if<double>(&v))return ag_runtime::str(*x);return "";}
    std::set<Decl*> evaluating;
    Value global_value(Decl* d){
        if(!std::holds_alternative<std::monostate>(d->value))return d->value;
        if(d->test_export)throw Error("test constant '"+d->name+"' is unavailable before its block executes",d->token);
        if(!evaluating.insert(d).second)throw Error("circular compile-time constant",d->token);
        d->value=convert(evaluate(d->body),d->result,d->token);evaluating.erase(d);return d->value;
    }
    Value arithmetic(const std::string& op,Value a,Value b,const Type& type,const Token& tok){
        a=convert(a,type,tok);b=convert(b,type,tok);
        try{
            if(type==tstring&&op=="+")return std::get<std::string>(a)+std::get<std::string>(b);
            if(integer(type)){auto x=std::get<int64_t>(a),y=std::get<int64_t>(b);int64_t z=0;if(op=="+")z=ag_runtime::add(x,y);else if(op=="-")z=ag_runtime::sub(x,y);else if(op=="*")z=ag_runtime::mul(x,y);else if(op=="/")z=ag_runtime::divide(x,y);else if(op=="%")z=ag_runtime::rem(x,y);else throw Error("invalid compile-time operator",tok);return convert(z,type,tok);}
            if(type==tf64){auto x=std::get<double>(a),y=std::get<double>(b);if(op=="+")return ag_runtime::add(x,y);if(op=="-")return ag_runtime::sub(x,y);if(op=="*")return ag_runtime::mul(x,y);if(op=="/")return ag_runtime::divide(x,y);}
        }catch(const Error&){throw;}catch(const std::exception& e){throw Error(e.what(),tok);}throw Error("unsupported compile-time arithmetic",tok);
    }
    bool compare(const std::string& op,const Value& a,const Value& b){if(op=="==")return a==b;if(op=="!=")return a!=b;if(op=="<")return a<b;if(op==">")return a>b;if(op=="<=")return a<=b;return a>=b;}
    Value evaluate(N n){
        tick(n->token);auto k=n->kind;
        if(k=="number"||k=="boolean"||k=="string")return n->value;
        if(k=="name"){if(n->declaration)return global_value(n->declaration);if(n->variable&&eval_variables.count(n->variable))return eval_variables.at(n->variable);throw Error("compile-time evaluation cannot read a runtime binding",n->token);}
        if(k=="member"&&n->declaration)return global_value(n->declaration);
        if(k=="unary"){auto v=evaluate(n->children[0]);if(n->text=="!")return !std::get<bool>(v);if(n->text=="+")return v;return arithmetic("-",integer(n->type)?Value(int64_t(0)):Value(0.0),v,n->type,n->token);}
        if(k=="update"){auto target=n->children[0];if(!target->variable)throw Error("compile-time mutation requires a local scalar",n->token);auto old=evaluate(target);auto one=integer(n->type)?Value(int64_t(1)):Value(1.0);eval_variables[target->variable]=arithmetic(n->text=="++"?"+":"-",old,one,n->type,n->token);return old;}
        if(k=="binary"){
            auto op=n->text;
            if(op=="="||op=="+="||op=="-="||op=="*="||op=="/="||op=="%="){
                auto target=n->children[0];if(!target->variable)throw Error("compile-time mutation requires a local scalar",n->token);auto old=op=="="?Value{}:evaluate(target);auto right=evaluate(n->children[1]);auto result=op=="="?convert(right,n->type,n->token):arithmetic(op.substr(0,1),old,right,n->type,n->token);eval_variables[target->variable]=result;return result;
            }
            auto a=evaluate(n->children[0]);if(op=="&&"&&!std::get<bool>(a))return false;if(op=="||"&&std::get<bool>(a))return true;auto b=evaluate(n->children[1]);if(op=="&&"||op=="||")return std::get<bool>(b);
            if(!n->annotation.name.empty())return compare(op,convert(a,n->annotation,n->token),convert(b,n->annotation,n->token));return arithmetic(op,a,b,n->type,n->token);
        }
        if(k=="call"){
            std::vector<Value> values;for(size_t i=1;i<n->children.size();++i)values.push_back(evaluate(n->children[i]));
            if(n->declaration){auto* d=n->declaration;if(d->kind!="function"||d->mode=="foreign")throw Error("compile-time calls cannot construct aggregate values or call C++",n->token);if(++eval_depth>64)throw Error("compile-time recursion exceeds 64 calls",n->token);struct Guard{int& depth;~Guard(){--depth;}}guard{eval_depth};auto old=eval_variables;for(size_t i=0;i<values.size();++i)eval_variables[d->arguments[i]]=convert(values[i],d->parameters[i].second,n->token);Value result;try{execute(d->body);}catch(const Return& r){result=r.value;}eval_variables=std::move(old);return convert(result,d->result,n->token);}
            if(n->text=="i32"||n->text=="i64"||n->text=="f64")return convert(values.at(0),n->type,n->token);
            if(n->text=="str")return value_string(values.at(0));
            if(n->text=="length"&&std::holds_alternative<std::string>(values.at(0)))return int64_t(std::get<std::string>(values[0]).size());
            if(n->text=="assert"){if(!std::get<bool>(values.at(0)))throw Error("compile-time assertion failed",n->token);return {};}
            throw Error("I/O, arrays, and foreign calls are unavailable in compile-time test blocks",n->token);
        }
        if(k=="match"){
            auto value=evaluate(n->children[0]);for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];if(arm->constant||(!arm->annotation.name.empty()&&value==convert(arm->value,n->children[0]->type,arm->token)))return convert(evaluate(arm->children.back()),n->type,n->token);}throw Error("aggregate match is not supported at compile time",n->token);
        }
        throw Error("aggregate values are not supported at compile time",n->token);
    }
    void execute(N n){
        tick(n->token);auto k=n->kind;
        if(k=="block"){for(auto s:n->children)execute(s);return;}
        if(k=="binding"){auto v=convert(evaluate(n->children[0]),n->type,n->token);eval_variables[n->variable]=v;if(n->declaration)n->declaration->value=v;return;}
        if(k=="expression"){if(!n->children.empty())evaluate(n->children[0]);return;}
        if(k=="return")throw Return{n->children.empty()?Value{}:evaluate(n->children[0])};
        if(k=="if"){if(std::get<bool>(evaluate(n->children[0])))execute(n->children[1]);else if(n->children.size()==3)execute(n->children[2]);return;}
        if(k=="while"||k=="for"){
            if(k=="for")execute(n->children[0]);while(std::get<bool>(evaluate(n->children[k=="for"?1:0]))){try{execute(n->children.back());}catch(const Break&){break;}catch(const Continue&){}if(k=="for")evaluate(n->children[2]);}return;
        }
        if(k=="switch"){auto value=evaluate(n->children[0]);N fallback;for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];if(arm->constant){fallback=arm->children.back();continue;}if(value==convert(evaluate(arm->children[0]),n->children[0]->type,n->token)){try{execute(arm->children.back());}catch(const Break&){}return;}}if(fallback)try{execute(fallback);}catch(const Break&){}return;}
        if(k=="break")throw Break{};if(k=="continue")throw Continue{};
        if(k=="unsafe")throw Error("unsafe cannot execute at compile time",n->token);
    }
    std::string cpp_type(const Type& t){if(t==ti32)return "std::int32_t";if(t==ti64)return "std::int64_t";if(t==tf64)return "double";if(t==tbool)return "bool";if(t==tstring)return "std::string";if(t==tvoid)return "void";if(class_type(t))return "ag_runtime::Ptr<"+t.name+">";if(t.name=="Weak")return "ag_runtime::Weak<"+cpp_type(owning(t.args[0]))+">";if(collection(t)||t.name=="Ptr"){std::string out="ag_runtime::"+t.name+"<";for(size_t i=0;i<t.args.size();++i){if(i)out+=',';out+=cpp_type(t.args[i]);}return out+">";}return t.name;}
    std::string literal(const Value& v,const Type& t){
        if(auto* x=std::get_if<int64_t>(&v)){if(*x==INT64_MIN)return "std::numeric_limits<std::int64_t>::min()";return "static_cast<"+cpp_type(t)+">("+std::to_string(*x)+"LL)";}
        if(auto* x=std::get_if<double>(&v)){std::ostringstream s;s<<std::setprecision(17)<<*x;auto text=s.str();if(text.find_first_of(".eE")==std::string::npos)text+=".0";return "static_cast<double>("+text+")";}
        if(auto* x=std::get_if<bool>(&v))return *x?"true":"false";
        if(auto* x=std::get_if<std::string>(&v))return "std::string("+quote(*x)+","+std::to_string(x->size())+")";
        throw Error("constant has no compile-time value");
    }
    std::string coerce(const Type& to,const Type& from,const std::string& x){return to==from?x:"static_cast<"+cpp_type(to)+">("+x+")";}
    int temporary=0,label_id=0;
    std::string temp(){return "ag_tmp"+std::to_string(temporary++);}
    std::string operation(const std::string& op,const Type& t,const std::string& a,const std::string& b){
        if(t==tstring&&op=="+")return "("+a+"+"+b+")";
        static const std::map<std::string,std::string> names={{"+","add"},{"-","sub"},{"*","mul"},{"/","divide"},{"%","rem"}};
        if(names.count(op))return "ag_runtime::"+names.at(op)+"<"+cpp_type(t)+">("+a+","+b+")";return "("+a+op+b+")";
    }
    struct Location{std::string setup,access;};
    bool addressable(N n){return (n->kind=="name"&&n->variable)||((n->kind=="member"||n->kind=="index"||(n->kind=="unary"&&n->text=="*"&&n->children[0]->type.name=="Ptr"))&&addressable(n->children[0]));}
    Location location(N n){
        if(n->kind=="name")return {"",n->variable->cpp};
        auto out=location(n->children[0]);
        if(n->kind=="unary")out.access+=".get()";
        else if(n->kind=="member"){if(n->children[0]->type.name=="Ptr"||class_type(n->children[0]->type))out.access+=".get()";out.access+=".ag_f"+std::to_string(n->index);}
        else{auto i=temp();out.setup+="auto "+i+"="+emit_expr(n->children[1])+";";out.access+=".at("+i+")";}
        return out;
    }
    std::string emit_expr(N n){
        auto k=n->kind;if(k=="number"||k=="string"||k=="boolean")return literal(n->value,n->type);
        if(k=="name"||k=="member"){
            if(n->declaration)return n->declaration->cpp;if(n->variable)return n->variable->cpp;
            auto base="("+emit_expr(n->children[0])+")";if(n->children[0]->type.name=="Ptr"||class_type(n->children[0]->type))base+=".get()";return base+".ag_f"+std::to_string(n->index);
        }
        if(k=="variant"){auto* d=n->declaration;auto i=std::find(d->variant_order.begin(),d->variant_order.end(),n->text)-d->variant_order.begin();return d->cpp+"{"+d->cpp+"_v"+std::to_string(i)+"{}}";}
        if(k=="index"){auto result="([&]()->"+cpp_type(n->type)+"{";if(addressable(n)){auto loc=location(n);return result+loc.setup+"return "+loc.access+";}())";}auto a=temp(),i=temp();return result+"auto "+a+"="+emit_expr(n->children[0])+";auto "+i+"="+emit_expr(n->children[1])+";return "+a+".at("+i+");}())";}
        if(k=="unary"){auto x=emit_expr(n->children[0]);if(n->text=="*"){auto p=temp();return "([&]()->"+cpp_type(n->type)+"{auto "+p+"="+x+";"+(class_type(n->children[0]->type)?"(void)"+p+".get();return "+p:"return "+p+".get()")+";}())";}if(n->text=="-")return "ag_runtime::neg<"+cpp_type(n->type)+">("+x+")";return "("+n->text+x+")";}
        if(k=="update"){
            auto loc=location(n->children[0]);auto old=temp();return "([&](){"+loc.setup+"auto "+old+"="+loc.access+";"+loc.access+"="+operation(n->text=="++"?"+":"-",n->type,old,"1")+";return "+old+";}())";
        }
        if(k=="binary"){
            auto op=n->text;
            if(op=="="||op=="+="||op=="-="||op=="*="||op=="/="||op=="%="){
                auto loc=location(n->children[0]);auto old=temp(),rhs=temp();std::string body=loc.setup;if(op!="=")body+="auto "+old+"="+loc.access+";";
                body+="auto "+rhs+"="+coerce(n->type,n->children[1]->type,emit_expr(n->children[1]))+";";auto value=op=="="?rhs:operation(op.substr(0,1),n->type,old,rhs);return "([&](){"+body+loc.access+"="+value+";return "+loc.access+";}())";
            }
            if(op=="&&"||op=="||")return "("+emit_expr(n->children[0])+op+emit_expr(n->children[1])+")";
            auto type=n->annotation.name.empty()?n->type:n->annotation;auto a=temp(),b=temp();return "([&](){auto "+a+"="+coerce(type,n->children[0]->type,emit_expr(n->children[0]))+";auto "+b+"="+coerce(type,n->children[1]->type,emit_expr(n->children[1]))+";return "+operation(op,type,a,b)+";}())";
        }
        if(k=="call"){
            if(n->text=="container")return cpp_type(n->type)+"{}";
            if(n->text=="pointer_empty")return cpp_type(n->type)+"::from_shared({})";
            if(n->text=="strong_count")return "ag_runtime::strong_count("+emit_expr(n->children[1])+")";
            std::string body;std::vector<std::string> values;
            auto target=n->children[0];bool array=n->text.rfind("method.",0)==0;Location loc;std::string receiver;
            if(array){if(addressable(target->children[0])){loc=location(target->children[0]);body+=loc.setup;receiver=loc.access;}else{receiver=temp();body+="auto "+receiver+"="+emit_expr(target->children[0])+";";}}
            for(size_t i=1;i<n->children.size();++i){auto v=temp();auto value=emit_expr(n->children[i]);Type wanted=n->children[i]->type;
                if(n->declaration){const auto& params=(n->text=="struct"||n->text=="class")?n->declaration->fields:n->text=="variant"?n->declaration->variants.at(target->text):n->declaration->parameters;wanted=params[i-1].second;}
                if(!n->argument_types.empty())wanted=n->argument_types[i-1];
                body+="auto "+v+"="+coerce(wanted,n->children[i]->type,value)+";";values.push_back(v);
            }
            std::string joined;for(size_t i=0;i<values.size();++i){if(i)joined+=',';joined+=values[i];}
            std::string call;
            if(n->text=="struct")call=n->declaration->cpp+"{"+joined+"}";
            else if(n->text=="class")call=cpp_type(n->type)+"::make("+n->declaration->cpp+"{"+joined+"})";
            else if(n->text=="pointer")call=cpp_type(n->type)+"::make("+joined+")";
            else if(n->text=="retain")call=joined;
            else if(n->text=="weak_ctor")call=cpp_type(n->type)+"("+joined+")";
            else if(n->text=="variant")call=n->declaration->cpp+"{"+n->declaration->cpp+"_v"+std::to_string(n->index)+"{"+joined+"}}";
            else if(n->declaration)call=n->declaration->cpp+"("+joined+")";
            else if(array)call=receiver+'.'+n->text.substr(7)+"("+joined+")";
            else if(n->text=="i32"||n->text=="i64"||n->text=="f64")call="ag_runtime::cast<"+cpp_type(n->type)+">("+joined+")";
            else call="ag_runtime::"+(n->text=="assert"?"assertion":n->text)+"("+joined+")";
            return "([&]()->"+cpp_type(n->type)+"{"+body+(n->type==tvoid?call+";":"return "+call+";")+"}())";
        }
        if(k=="match"){
            auto value=temp();std::string out="([&]()->"+cpp_type(n->type)+"{auto "+value+"="+emit_expr(n->children[0])+";";
            auto type=n->children[0]->type;bool en=type_names.count(type.name);
            for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];std::string condition="true";if(!arm->constant)condition=en?value+".ag_data.index()=="+std::to_string(arm->index):value+"=="+literal(arm->value,type);
                out+="if("+condition+"){";if(en&&!arm->constant){auto payload=temp();out+="const auto "+payload+"=std::get<"+std::to_string(arm->index)+">("+value+".ag_data);";for(size_t j=0;j<arm->bindings.size();++j)out+="const auto "+arm->bindings[j]->cpp+"="+payload+".ag_f"+std::to_string(j)+";";}
                out+="return "+coerce(n->type,arm->children.back()->type,emit_expr(arm->children.back()))+";}";
            }return out+"throw std::runtime_error(\"unreachable match\");}())";
        }
        throw Error("cannot emit expression",n->token);
    }
    std::vector<std::string> break_targets;
    std::string line(const Token& t){return "\n#line "+std::to_string(t.line)+" "+quote(t.file)+"\n";}
    std::string emit_statement(N n){
        auto k=n->kind;std::string out=line(n->token);
        if(k=="block"){out+="{\n";for(auto s:n->children)out+=emit_statement(s);return out+"}\n";}
        if(k=="binding")return out+(n->constant?"const ":"")+cpp_type(n->type)+" "+n->variable->cpp+"="+coerce(n->type,n->children[0]->type,emit_expr(n->children[0]))+";\n";
        if(k=="expression")return out+(n->children.empty()?";":"(void)("+emit_expr(n->children[0])+");")+"\n";
        if(k=="return")return out+"return"+(n->children.empty()?"":" "+coerce(current_function->result,n->children[0]->type,emit_expr(n->children[0])))+";\n";
        if(k=="if"){out+="if("+emit_expr(n->children[0])+")"+emit_statement(n->children[1]);if(n->children.size()==3)out+="else "+emit_statement(n->children[2]);return out;}
        if(k=="while"||k=="for"){
            break_targets.push_back("");if(k=="for")out+="{\n"+emit_statement(n->children[0])+"for(;"+emit_expr(n->children[1])+";"+emit_expr(n->children[2])+")"+emit_statement(n->children[3])+"}\n";else out+="while("+emit_expr(n->children[0])+")"+emit_statement(n->children[1]);break_targets.pop_back();return out;
        }
        if(k=="switch"){
            auto label="ag_break"+std::to_string(label_id++),value=temp();break_targets.push_back(label);out+="{auto "+value+"="+emit_expr(n->children[0])+";\n";N fallback;bool first=true;
            for(size_t i=1;i<n->children.size();++i){auto arm=n->children[i];if(arm->constant){fallback=arm->children.back();continue;}out+=(first?"if(":"else if(")+value+"=="+emit_expr(arm->children[0])+")"+emit_statement(arm->children.back());first=false;}
            if(fallback)out+=(first?"":"else ")+emit_statement(fallback);out+=label+":;\n}\n";break_targets.pop_back();return out;
        }
        if(k=="break")return out+(break_targets.back().empty()?"break;":"goto "+break_targets.back()+";")+"\n";
        if(k=="continue")return out+"continue;\n";
        if(k=="unsafe")return emit_statement(n->children[0]);
        return out+";\n";
    }
    std::string signature(Decl* d,bool names){std::string out=cpp_type(d->result)+" "+d->cpp+"(";for(size_t i=0;i<d->parameters.size();++i){if(i)out+=',';out+=cpp_type(d->parameters[i].second);if(names)out+=' '+(d->mode=="foreign"?"ag_arg"+std::to_string(i):d->arguments[i]->cpp);}return out+")";}
    std::string generate(const BuildOptions& options,Decl* main){
        std::string out=runtime_source;if(out.rfind("#pragma once\n",0)==0)out.erase(0,13);for(const auto& header:options.headers)out+="\n#include <"+header+">\n";
        for(auto* d:type_order)out+="\nstruct "+d->cpp+";\n";
        for(auto* d:type_order){
            if(d->kind=="enum"){for(size_t i=0;i<d->variant_order.size();++i){out+="\nstruct "+d->cpp+"_v"+std::to_string(i)+"{";auto& fields=d->variants.at(d->variant_order[i]);for(size_t j=0;j<fields.size();++j)out+=cpp_type(fields[j].second)+" ag_f"+std::to_string(j)+";";out+="};\n";}
                out+="struct "+d->cpp+"{std::variant<";for(size_t i=0;i<d->variant_order.size();++i){if(i)out+=',';out+=d->cpp+"_v"+std::to_string(i);}out+="> ag_data;};\n";
            }else{out+="\nstruct "+d->cpp+"{";for(size_t j=0;j<d->fields.size();++j)out+=cpp_type(d->fields[j].second)+" ag_f"+std::to_string(j)+";";out+="};\n";}
        }
        for(auto* d:constants)out+=line(d->token)+"const "+cpp_type(d->result)+" "+d->cpp+"="+literal(d->value,d->result)+";\n";
        for(auto* d:functions)out+=signature(d,false)+";\n";
        for(auto* d:functions){out+=line(d->token)+signature(d,true);current_function=d;
            if(d->mode=="foreign"){
                std::string params,values;for(size_t i=0;i<d->parameters.size();++i){if(i){params+=',';values+=',';}params+=cpp_type(d->parameters[i].second);values+="ag_arg"+std::to_string(i);}
                out+="{using ag_signature="+cpp_type(d->result)+"(*)("+params+");auto ag_target=static_cast<ag_signature>(&"+d->foreign+");";
                auto call="ag_target("+values+")";if(d->result==tf64)call="ag_runtime::finite("+call+")";out+=(d->result==tvoid?call+";":"return "+call+";")+"}\n";
            }else out+=emit_statement(d->body);
        }
        if(main)out+="\n#line 1 \"<argon entry>\"\nint main(){try{return "+main->cpp+"();}catch(const std::exception& e){std::cerr<<\"Argon runtime error: \"<<e.what()<<'\\n';return 1;}}\n";return out;
    }
public:
    Compiled run(const fs::path& source,const BuildOptions& options,bool require_main){
        std::set<std::string> stack;entry=load(source,stack);int id=0;
        for(auto& u:units)for(auto& d:u->declarations){auto prefix=d->kind=="function"?"ag_fn":"ag_d";d->cpp=prefix+std::to_string(id++);if(d->kind=="function")d->cpp+='_'+d->name;if(d->kind=="struct"||d->kind=="class"||d->kind=="enum"){types.push_back(d.get());type_names[d->cpp]=d.get();}else if(d->kind=="function")functions.push_back(d.get());else constants.push_back(d.get());}
        auto fields=[&](std::vector<std::pair<std::string,Type>>& items,Decl* d){std::set<std::string> seen;for(auto& f:items){if(!seen.insert(f.first).second)throw Error("duplicate field or parameter",d->token);f.second=resolve(f.second,d->unit,d->token);}};
        for(auto* d:types){fields(d->fields,d);for(auto& v:d->variants)fields(v.second,d);}
        std::map<Decl*,int> state;
        std::function<void(Decl*)> visit=[&](Decl* d){if(state[d]==1)throw Error("recursive value types require a Ptr, Weak, class, or collection link",d->token);if(state[d]==2)return;state[d]=1;std::function<void(const Type&)> dependency=[&](const Type& t){if(owner(t)||t.name=="Weak")return;if(type_names.count(t.name))visit(type_names.at(t.name));};for(const auto& f:d->fields)dependency(f.second);for(const auto& v:d->variants)for(const auto& f:v.second)dependency(f.second);state[d]=2;type_order.push_back(d);};for(auto* d:types)visit(d);
        for(auto* d:functions){fields(d->parameters,d);d->result=resolve(d->result,d->unit,d->token,true);if(d->mode=="foreign"){
                if(!std::regex_match(d->foreign,std::regex("[A-Za-z_][A-Za-z_0-9]*(::[A-Za-z_][A-Za-z_0-9]*)*")))throw Error("invalid C++ function name",d->token);
                for(const auto& p:d->parameters)if(!numeric(p.second)&&p.second!=tbool)throw Error("C++ bindings currently accept only numeric and bool parameters",d->token);
                if(!numeric(d->result)&&d->result!=tbool&&d->result!=tvoid)throw Error("C++ binding result must be numeric, bool, or void",d->token);
            }}
        for(auto* d:constants)if(!d->result.name.empty())d->result=resolve(d->result,d->unit,d->token);
        for(auto* d:constants)global_type(d);
        for(auto* d:functions)check_function(d);
        for(auto& u:units){current_unit=u.get();current_function=nullptr;scopes.clear();in_test=true;for(auto test:u->tests)statement(test);}in_test=false;
        std::set<Unit*> done;size_t test_count=0;
        std::function<void(Unit*)> tests=[&](Unit* u){if(!done.insert(u).second)return;for(const auto& dep:u->dependencies)tests(dep.second);for(auto test:u->tests){++test_count;eval_variables.clear();execute(test);}};tests(entry);
        for(auto* d:constants)global_value(d);
        Decl* main=nullptr;if(entry->names.count("main")){main=entry->names.at("main");if(main->kind!="function"||main->mode=="foreign"||!main->parameters.empty()||main->result!=ti32)throw Error("entry must be fn main() -> i32",main->token);}
        for(const auto& u:units)if(u.get()!=entry&&u->names.count("main"))throw Error("an imported module cannot define main");if(require_main&&!main)throw Error("an executable needs fn main() -> i32");
        BuildOptions native=options;
        for(const auto& u:units)for(const auto& header:u->headers)
            if(std::find(native.headers.begin(),native.headers.end(),header)==native.headers.end())native.headers.push_back(header);
        Compiled result;result.cpp=generate(native,main);result.test_blocks=test_count;
        for(const auto& u:units)result.sources.push_back(u->file);
        for(const auto& header:native.headers)for(const auto& directory:native.include_dirs)
            if(fs::is_regular_file(directory/header)){result.sources.push_back(fs::canonical(directory/header));break;}
        return result;
    }
};
}
Compiled compile(const fs::path& source,const BuildOptions& options,bool require_main){return Compiler().run(source,options,require_main);}
}
