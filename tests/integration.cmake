cmake_minimum_required(VERSION 3.16)
string(RANDOM LENGTH 10 ALPHABET abcdef0123456789 nonce)
set(root "${WORK}/run ${nonce}")
file(MAKE_DIRECTORY "${root}")
if(WIN32)
  set(suffix .exe)
endif()
function(command cwd expected)
  execute_process(COMMAND "${COMPILER}" ${ARGN} WORKING_DIRECTORY "${cwd}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 60)
  if(NOT status EQUAL expected)
    message(FATAL_ERROR "${ARGN}: expected ${expected}, got ${status}\n${out}\n${err}")
  endif()
  string(REPLACE "\r\n" "\n" out "${out}")
  set(output "${out}" PARENT_SCOPE)
  set(diagnostic "${err}" PARENT_SCOPE)
endfunction()
function(program name source expected)
  file(WRITE "${root}/${name}.ar" "${source}")
  command("${root}" 0 run "${name}.ar")
  if(NOT output STREQUAL expected)
    message(FATAL_ERROR "${name}: expected [${expected}], got [${output}]")
  endif()
endfunction()
function(reject name source reason)
  file(WRITE "${root}/${name}.ar" "${source}")
  command("${root}" 1 check "${name}.ar")
  if(NOT diagnostic MATCHES "${reason}")
    message(FATAL_ERROR "${name}: expected diagnostic ${reason}, got ${diagnostic}")
  endif()
endfunction()
function(runtime_failure name source reason)
  file(WRITE "${root}/${name}.ar" "${source}")
  command("${root}" 1 run "${name}.ar")
  if(NOT diagnostic MATCHES "Argon runtime error:.*${reason}")
    message(FATAL_ERROR "${name}: wrong runtime diagnostic ${diagnostic}")
  endif()
endfunction()

command("${root}" 0 new "hello world")
set(project "${root}/hello world")
command("${project}/src" 0 check)
command("${project}/src" 0 run)
if(NOT output STREQUAL "Hello from Argon!\n42\n")
  message(FATAL_ERROR "project startup failed: ${output}")
endif()
command("${project}" 0 build --debug)
if(NOT EXISTS "${project}/argon-cxx/debug/hello-world${suffix}" OR NOT EXISTS "${project}/argon-cxx/release/hello-world${suffix}")
  message(FATAL_ERROR "separate build profiles missing")
endif()
command("${root}" 1 new "hello world")
command("${project}" 1 init)
file(SHA256 "${project}/argon-cxx/release/hello-world${suffix}" old_binary)
file(WRITE "${project}/src/main.ar" "fn main()->i32{return missing;}")
command("${project}" 1 run)
file(SHA256 "${project}/argon-cxx/release/hello-world${suffix}" new_binary)
if(NOT old_binary STREQUAL new_binary)
  message(FATAL_ERROR "failed check destroyed the previous binary")
endif()
file(MAKE_DIRECTORY "${root}/existing")
file(WRITE "${root}/existing/.gitignore" "keep-me\n")
command("${root}/existing" 0 init)
file(READ "${root}/existing/.gitignore" ignore)
if(NOT ignore STREQUAL "keep-me\n")
  message(FATAL_ERROR "init overwrote an existing ignore file")
endif()
file(MAKE_DIRECTORY "${root}/conflict/src")
file(WRITE "${root}/conflict/src/main.ar" "existing source")
command("${root}/conflict" 1 init)
if(EXISTS "${root}/conflict/Argon.toml")
  message(FATAL_ERROR "conflicting init partially created the project")
endif()

command("${root}" 0 new native --cpp)
command("${root}/native" 0 run)
if(NOT output STREQUAL "42\n")
  message(FATAL_ERROR "C++ library integration failed")
endif()
# A failed native compilation also preserves the last successful artifact.
file(SHA256 "${root}/native/argon-cxx/release/native${suffix}" old_binary)
file(WRITE "${root}/native/cpp/demo.cpp" "invalid C++")
command("${root}/native" 1 run)
file(SHA256 "${root}/native/argon-cxx/release/native${suffix}" new_binary)
if(NOT old_binary STREQUAL new_binary)
  message(FATAL_ERROR "failed native compilation replaced the old artifact")
endif()
file(WRITE "${root}/native/cpp/demo.cpp" "#include \"demo.hpp\"\nstd::int32_t demo::twice(std::int32_t x){return x*2;}")
file(WRITE "${root}/native/src/main.ar" "cpp \"demo.hpp\";extern cpp fn twice(x:f64)->f64=\"demo::twice\";fn main()->i32{unsafe{println(twice(21.0));}return 0;}")
command("${root}/native" 1 build)
# Existing overloaded C++ library functions use the exact declared signature.
command("${root}" 0 new math-library)
file(APPEND "${root}/math-library/Argon.toml" "\n[cpp]\nheaders = [\"cmath\"]\n")
file(WRITE "${root}/math-library/src/main.ar" "extern cpp fn root(x:f64)->f64=\"std::sqrt\";fn main()->i32{unsafe{println(root(81.0));}return 0;}")
command("${root}/math-library" 0 run)
if(NOT output STREQUAL "9\n")
  message(FATAL_ERROR "standard C++ library binding failed: ${output}")
endif()
file(WRITE "${root}/exit-code.ar" "fn main()->i32{return 7;}")
command("${root}" 7 run exit-code.ar)

command("${root}" 0 run "${SOURCE}/examples/tour.ar")
if(NOT output STREQUAL "720\nvalue: 42\nfour\n")
  message(FATAL_ERROR "language tour failed: ${output}")
endif()
program(order [=[
fn combine(a:i32,b:i32)->i32{return a*10+b;}
fn main()->i32{
 let x=1; println(combine(x,x=2)); println(x+(x=10));
 let a:Array<i32>=Array<i32>();a.push(3);let copy=a.clone();copy[0]=8;
 println(a[0]);println(copy[0]);let old=a[0]++;println(old);println(a[0]);
 return 0;
}
]=] "12\n12\n3\n8\n3\n4\n")
program(switch-flow [=[
fn select(n:i32)->i32{switch(n){case 1:return 10;default:return 20;}}
fn main()->i32{
 let total=0;
 for(let i=0;i<5;i++){
   switch(i){case 1:continue;case 3:break;default:total+=i;}
 }
 println(total);println(select(1));
 println(match false {true => "yes", false => "no"});
 println(match -1 {-1 => "negative", _ => "other"});
    println(match "x" {"_" => "underscore", _ => "fallback"});
 return 0;
}
]=] "6\n10\nno\nnegative\nfallback\n")
program(test-evaluation [=[
fn sum(n:i32)->i32{let result=0;for(let i=0;i<=n;i++){result+=i;}return result;}
test {let input=10;const computed:i32=sum(input);assert(computed==55);}
test {assert(computed==55);assert(match true {true=>true,false=>false});}
fn main()->i32{println(computed);return 0;}
]=] "55\n")
file(MAKE_DIRECTORY "${root}/modules")
file(WRITE "${root}/modules/math.ar" "test{const base:i32=40;}pub const amount:i32=base;pub fn add(a:i32,b:i32)->i32{return a+b;}priv fn secret()->i32{return 0;}")
program(imports "import modules.math;fn main()->i32{println(math.add(math.amount,2));return 0;}" "42\n")
reject(private-import "import modules.math;fn main()->i32{return math.secret();}" "unknown")
reject(immutable "fn main()->i32{const x=1;x=2;return 0;}" "immutable")
reject(missing-return "fn main()->i32{if(true){return 0;}}" "every path")
reject(switch-return "fn main()->i32{switch(1){case 1:break;return 0;default:return 0;}}" "every path")
reject(non-exhaustive "enum Result{Ok(value:i32),Err(message:string)}fn main()->i32{let r=Result::Ok(1);return match r{Result::Ok(n)=>n};}" "non-exhaustive")
reject(duplicate-pattern "fn main()->i32{return match true {true=>0,true=>1,false=>2};}" "duplicate")
reject(duplicate-number-pattern "fn main()->i32{return match 1i64 {1=>0,1i64=>1,_=>2};}" "duplicate")
reject(test-assertion "test{assert(false);}fn main()->i32{return 0;}" "compile-time assertion failed")
reject(test-io "test{println(1);}fn main()->i32{return 0;}" "I/O")
reject(test-indirect-io "fn effect()->i32{println(1);return 0;}test{const value:i32=effect();}fn main()->i32{return 0;}" "I/O")
reject(test-limit "test{while(true){}}fn main()->i32{return 0;}" "100000 steps")
reject(test-local "fn main()->i32{test{assert(true);}return 0;}" "expected")
reject(meta-outside-test "fn compute()->i32{return 42;}const answer:i32=compute();fn main()->i32{return 0;}" "global test block")
reject(test-return "test{return;}fn main()->i32{return 0;}" "return is not allowed")
reject(foreign-safe "extern cpp fn f(x:i32)->i32=\"demo::f\";fn main()->i32{return f(1);}" "unsafe")
reject(foreign-test "extern cpp fn f(x:i32)->i32=\"demo::f\";test{unsafe{f(1);}}fn main()->i32{return 0;}" "unsafe")
reject(bad-types "fn main()->i32{let x:i32=\"bad\";return 0;}" "expected i32")
reject(no-uninitialized "fn main()->i32{let x:i32;return 0;}" "expected")
runtime_failure(bounds "fn main()->i32{let a=Array<i32>();println(a[0]);return 0;}" "out of bounds")
runtime_failure(overflow "fn main()->i32{let a:i32=2147483647;println(a+1);return 0;}" "overflow")
runtime_failure(cast "fn main()->i32{println(i32(1.5));return 0;}" "fractional")
runtime_failure(cast-boundary "fn main()->i32{println(i64(9223372036854775808.0));return 0;}" "out of range")
reject(test-cast-boundary "test{const value:i64=i64(9223372036854775808.0);}fn main()->i32{return 0;}" "out of range")
runtime_failure(divzero "fn main()->i32{println(1/0);return 0;}" "division by zero")
runtime_failure(divoverflow "fn main()->i32{println(-9223372036854775808i64 / -1);return 0;}" "overflow")

# --check needs no native compiler and executes the same compile-time blocks.
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "ARGON_CXX=/nonexistent/compiler"
  "${COMPILER}" check "${SOURCE}/examples/tour.ar" RESULT_VARIABLE status ERROR_VARIABLE err TIMEOUT 20)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "checking depended on a native compiler: ${err}")
endif()
command("${root}" 0 emit-cpp "${SOURCE}/examples/tour.ar" -o first.cpp)
command("${root}" 0 emit-cpp "${SOURCE}/examples/tour.ar" -o second.cpp)
file(SHA256 "${root}/first.cpp" first)
file(SHA256 "${root}/second.cpp" second)
if(NOT first STREQUAL second)
  message(FATAL_ERROR "code generation was nondeterministic")
endif()
# Detect misspelled manifest keys instead of silently ignoring them.
file(APPEND "${root}/existing/Argon.toml" "entyr = \"main.ar\"\n")
command("${root}/existing" 1 check)
if(UNIX)
  command("${root}" 0 new symlinked)
  file(MAKE_DIRECTORY "${root}/outside")
  file(CREATE_LINK "${root}/outside" "${root}/symlinked/argon-cxx" SYMBOLIC)
  command("${root}/symlinked" 1 build)
endif()

# Owning and weak handles are checked by the frontend and exercised natively.
foreach(example IN ITEMS pointers family collections shared_collections linked_nodes debugging)
  command("${root}" 0 run "${SOURCE}/examples/${example}.ar")
endforeach()
program(ownership [=[
class Box { value:i32; }
struct Pair { first:i32; }
fn expired()->Weak<i32>{let p=Ptr<i32>(1);return weak(p);}
fn main()->i32{
 let box=Box(7);let alias=box;
 assert(strong_count(box)==2);alias.value=8;assert(box.value==8);
 {let typed:Ptr<Box>=Ptr<Box>(box);assert(strong_count(box)==3);}
 let observer=weak(box);alias.reset();box.reset();assert(is_expired(observer));
 let p=Ptr<Pair>(Pair(3));p->first=4;assert((*p).first==4);
 let n=Ptr<i32>(40);let nested=Ptr<Ptr<i32>>(n);**nested+=2;assert(*n==42);
 let nested_weak:Weak<Ptr<i32>>=weak(nested);let restored=upgrade(nested_weak);assert(**restored==42);
 let dead=expired();assert(is_empty(upgrade(dead)));
 let values=Array<bool>();values.push(true);values[0]=false;assert(!values[0]);
 let shared=values;assert(strong_count(values)==2);shared[0]=true;assert(values[0]);
 let weak_values=weak(values);shared.reset();values.reset();assert(is_expired(weak_values));
 let map=HashMap<i64,string>();map.set(1,"one");assert(map[1]=="one");
 println("ownership checks passed");return 0;
}
]=] "ownership checks passed\n")
program(recursive-collections [=[
struct Tree{value:i32;children:Array<Tree>;}
fn main()->i32{
 let parent=Tree(1,Array<Tree>());
 parent.children.push(Tree(2,Array<Tree>()));
 assert(parent.children[0].value==2);
 return 0;
}
]=] "")
reject(weak-type "fn main()->i32{let p=Ptr<i32>(1);let w=Weak<string>(p);return 0;}" "expected Ptr<string>")
reject(weak-dereference "fn main()->i32{let w=Weak<i32>();println(*w);return 0;}" "owning pointer")
reject(const-pointer "fn main()->i32{const p=Ptr<i32>(1);*p=2;return 0;}" "immutable")
reject(hash-key "fn main()->i32{let m=HashMap<Ptr<i32>,i32>();return 0;}" "hash keys")
reject(test-heap "test{let p=Ptr<i32>(1);}fn main()->i32{return 0;}" "unavailable")
runtime_failure(empty-owner "fn main()->i32{let p=Ptr<i32>();println(*p);return 0;}" "empty owning pointer")
runtime_failure(missing-key "fn main()->i32{let m=HashMap<string,i32>();println(m.get(\"missing\"));return 0;}" "key not found")
runtime_failure(empty-stack "fn main()->i32{let s=Stack<i32>();println(s.pop());return 0;}" "empty collection")
