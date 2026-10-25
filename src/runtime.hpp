#pragma once
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <list>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ag_runtime {
inline void fail(const std::string& message){throw std::runtime_error(message);}
template<class T>T finite(T x){if constexpr(std::is_floating_point_v<T>)if(!std::isfinite(x))fail("non-finite floating-point result");return x;}
template<class T>T add(T a,T b){
    if constexpr(std::is_integral_v<T>){if((b>0&&a>std::numeric_limits<T>::max()-b)||(b<0&&a<std::numeric_limits<T>::min()-b))fail("integer overflow");}
    return finite<T>(a+b);
}
template<class T>T sub(T a,T b){
    if constexpr(std::is_integral_v<T>){if((b<0&&a>std::numeric_limits<T>::max()+b)||(b>0&&a<std::numeric_limits<T>::min()+b))fail("integer overflow");}
    return finite<T>(a-b);
}
template<class T>T mul(T a,T b){
    if constexpr(std::is_integral_v<T>){
        const auto lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();
        if(a>0? (b>0?a>hi/b:b<lo/a) : a<0? (b>0?a<lo/b:b<0&&a<hi/b) : false)fail("integer overflow");
    }return finite<T>(a*b);
}
template<class T>T divide(T a,T b){if(b==0)fail("division by zero");if constexpr(std::is_integral_v<T>)if(a==std::numeric_limits<T>::min()&&b==-1)fail("integer overflow");return finite<T>(a/b);}
template<class T>T rem(T a,T b){if(b==0)fail("division by zero");if(a==std::numeric_limits<T>::min()&&b==-1)return 0;return a%b;}
template<class T>T neg(T a){return sub<T>(0,a);}
template<class To,class From>To cast(From x){
    if constexpr(std::is_integral_v<To>){
        if constexpr(std::is_floating_point_v<From>){
            // Use an exclusive power-of-two upper bound: converting INT64_MAX
            // to a double can round it UP to 2^63, even for long double on ARM.
            long double n=static_cast<long double>(x);
            const long double upper=std::ldexp(1.0L,std::numeric_limits<To>::digits);
            const long double lower=std::is_signed_v<To>?-upper:0;
            if(!std::isfinite(n)||n<lower||n>=upper)fail("numeric cast out of range");
            if(std::trunc(n)!=n)fail("fractional integer cast");
        }else if constexpr(std::is_signed_v<From>){
            const auto n=static_cast<std::intmax_t>(x);
            if constexpr(std::is_signed_v<To>){if(n<static_cast<std::intmax_t>(std::numeric_limits<To>::min())||n>static_cast<std::intmax_t>(std::numeric_limits<To>::max()))fail("numeric cast out of range");}
            else if(n<0||static_cast<std::uintmax_t>(n)>static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))fail("numeric cast out of range");
        }else if(static_cast<std::uintmax_t>(x)>static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))fail("numeric cast out of range");
    }return finite<To>(static_cast<To>(x));
}
template<class T>std::string str(const T& value){std::ostringstream s;s.precision(17);s<<value;return s.str();}
inline std::string str(bool value){return value?"true":"false";}
inline std::string str(const std::string& value){return value;}
inline void print(const std::string& s){std::cout.write(s.data(),static_cast<std::streamsize>(s.size()));}
template<class T>void print(const T& value){print(str(value));}
inline void println(){std::cout<<'\n';}
template<class T>void println(const T& value){print(value);println();}
inline std::string read_line(){std::string line;std::getline(std::cin,line);return line;}
inline void assertion(bool condition){if(!condition)fail("assertion failed");}
// Every shared allocation is held by an owning handle. No source-level reference
// returned by get()/at() can escape: the frontend copies values on reads.
template<class T>class Ptr {
    std::shared_ptr<T> value;
public:
    using element_type=T;
    Ptr()=default;
    static Ptr make(T item){return from_shared(std::make_shared<T>(std::move(item)));}
    static Ptr from_shared(std::shared_ptr<T> p){Ptr out;out.value=std::move(p);return out;}
    const std::shared_ptr<T>& share()const{return value;}
    T& get()const{if(!value)fail("empty owning pointer");return *value;}
    bool empty_owner()const{return !value;}
    int64_t use_count()const{return cast<int64_t>(value.use_count());}
    void reset(){value.reset();}
    friend bool operator==(const Ptr& a,const Ptr& b){return a.value==b.value;}
    friend bool operator!=(const Ptr& a,const Ptr& b){return !(a==b);}
};
template<class Owner>class Weak {
    std::weak_ptr<typename Owner::element_type> value;
public:
    Weak()=default;
    explicit Weak(const Owner& owner):value(owner.share()){}
    Owner upgrade()const{return Owner::from_shared(value.lock());}
    bool is_expired()const{return value.expired();}
    int64_t use_count()const{return cast<int64_t>(value.use_count());}
    void reset(){value.reset();}
};
template<class T,class Storage>class Sequence {
    std::shared_ptr<Storage> values;
    Storage& data()const{if(!values)fail("empty owning pointer");return *values;}
public:
    using element_type=Storage;
    Sequence():values(std::make_shared<Storage>()){}
    static Sequence from_shared(std::shared_ptr<Storage> p){Sequence out(std::move(p));return out;}
    const std::shared_ptr<Storage>& share()const{return values;}
    bool empty_owner()const{return !values;}
    int64_t use_count()const{return cast<int64_t>(values.use_count());}
    decltype(auto) at(int64_t index)const{auto& s=data();if(index<0||static_cast<uint64_t>(index)>=s.size())fail("collection index out of bounds");auto it=s.begin();std::advance(it,index);return *it;}
    T get(int64_t index)const{return at(index);}
    void set(int64_t index,T item){at(index)=std::move(item);}
    void push(T value){data().push_back(std::move(value));}
    void push_front(T value){data().insert(data().begin(),std::move(value));}
    T pop(){auto& s=data();if(s.empty())fail("cannot pop an empty collection");T out=std::move(s.back());s.pop_back();return out;}
    T pop_front(){auto& s=data();if(s.empty())fail("cannot pop an empty collection");T out=std::move(s.front());s.erase(s.begin());return out;}
    void enqueue(T value){push(std::move(value));}
    T dequeue(){return pop_front();}
    T front()const{if(data().empty())fail("empty collection");return data().front();}
    T back()const{if(data().empty())fail("empty collection");return data().back();}
    T top()const{return back();}
    int64_t length()const{return cast<int64_t>(data().size());}
    bool is_empty()const{return data().empty();}
    void clear(){data().clear();}
    Sequence clone()const{return from_shared(std::make_shared<Storage>(data()));}
    void reset(){values.reset();}
    friend bool operator==(const Sequence& a,const Sequence& b){return a.values==b.values;}
    friend bool operator!=(const Sequence& a,const Sequence& b){return !(a==b);}
private:
    explicit Sequence(std::shared_ptr<Storage> p):values(std::move(p)){}
};
template<class T>using Array=Sequence<T,std::vector<T>>;
template<class T>using LinkedList=Sequence<T,std::list<T>>;
template<class T>using Deque=Sequence<T,std::deque<T>>;
template<class T>using Stack=Sequence<T,std::vector<T>>;
template<class T>using Queue=Sequence<T,std::deque<T>>;
template<class K,class V>class HashMap {
    using Storage=std::unordered_map<K,V>;
    std::shared_ptr<Storage> values;
    Storage& data()const{if(!values)fail("empty owning pointer");return *values;}
    explicit HashMap(std::shared_ptr<Storage> p):values(std::move(p)){}
public:
    using element_type=Storage;
    HashMap():values(std::make_shared<Storage>()){}
    static HashMap from_shared(std::shared_ptr<Storage> p){return HashMap(std::move(p));}
    const std::shared_ptr<Storage>& share()const{return values;}
    bool empty_owner()const{return !values;}
    int64_t use_count()const{return cast<int64_t>(values.use_count());}
    V& at(const K& key)const{auto& s=data();auto it=s.find(key);if(it==s.end())fail("hash map key not found");return it->second;}
    V get(const K& key)const{return at(key);}
    void set(K key,V value){data().insert_or_assign(std::move(key),std::move(value));}
    bool contains(const K& key)const{return data().find(key)!=data().end();}
    bool remove(const K& key){return data().erase(key)!=0;}
    int64_t length()const{return cast<int64_t>(data().size());}
    bool is_empty()const{return data().empty();}
    void clear(){data().clear();}
    HashMap clone()const{return from_shared(std::make_shared<Storage>(data()));}
    void reset(){values.reset();}
    friend bool operator==(const HashMap& a,const HashMap& b){return a.values==b.values;}
    friend bool operator!=(const HashMap& a,const HashMap& b){return !(a==b);}
    Array<K> keys()const{std::vector<K> ordered;for(const auto& pair:data())ordered.push_back(pair.first);std::sort(ordered.begin(),ordered.end());Array<K> out;for(const auto& key:ordered)out.push(key);return out;}
};
template<class K>class HashSet {
    using Storage=std::unordered_set<K>;
    std::shared_ptr<Storage> values;
    Storage& data()const{if(!values)fail("empty owning pointer");return *values;}
    explicit HashSet(std::shared_ptr<Storage> p):values(std::move(p)){}
public:
    using element_type=Storage;
    HashSet():values(std::make_shared<Storage>()){}
    static HashSet from_shared(std::shared_ptr<Storage> p){return HashSet(std::move(p));}
    const std::shared_ptr<Storage>& share()const{return values;}
    bool empty_owner()const{return !values;}
    int64_t use_count()const{return cast<int64_t>(values.use_count());}
    bool insert(K key){return data().insert(std::move(key)).second;}
    bool contains(const K& key)const{return data().find(key)!=data().end();}
    bool remove(const K& key){return data().erase(key)!=0;}
    int64_t length()const{return cast<int64_t>(data().size());}
    bool is_empty()const{return data().empty();}
    void clear(){data().clear();}
    HashSet clone()const{return from_shared(std::make_shared<Storage>(data()));}
    void reset(){values.reset();}
    friend bool operator==(const HashSet& a,const HashSet& b){return a.values==b.values;}
    friend bool operator!=(const HashSet& a,const HashSet& b){return !(a==b);}
    Array<K> items()const{std::vector<K> ordered(data().begin(),data().end());std::sort(ordered.begin(),ordered.end());Array<K> out;for(const auto& key:ordered)out.push(key);return out;}
};
template<class Owner>auto weak(const Owner& owner){return Weak<Owner>(owner);}
template<class Owner>Owner upgrade(const Weak<Owner>& observer){return observer.upgrade();}
template<class Owner>bool is_empty(const Owner& owner){return owner.empty_owner();}
template<class Owner>bool is_expired(const Weak<Owner>& observer){return observer.is_expired();}
template<class Owner>int64_t strong_count(const Owner& owner){return owner.use_count();}
template<class T>int64_t length(const T& value){return value.length();}
inline int64_t length(const std::string& s){return cast<int64_t>(s.size());}
}
