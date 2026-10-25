#include "runtime.hpp"
#include <cassert>
#include <functional>

using namespace ag_runtime;
struct Tracked {
    static int alive;
    int value;
    explicit Tracked(int n):value(n){++alive;}
    Tracked(const Tracked& other):value(other.value){++alive;}
    Tracked(Tracked&& other)noexcept:value(other.value){++alive;}
    ~Tracked(){--alive;}
};
int Tracked::alive=0;
static bool fails(const std::function<void()>& action){try{action();}catch(const std::runtime_error&){return true;}return false;}

int main(){
    // The allocation remains alive through aliases and container elements, and
    // is destroyed exactly when the final owning handle is dropped.
    Weak<Ptr<Tracked>> observer;
    {
        auto owner=Ptr<Tracked>::make(Tracked(42));
        observer=Weak<Ptr<Tracked>>(owner);
        auto alias=owner;
        Array<Ptr<Tracked>> retained;
        retained.push(owner);
        assert(owner.use_count()==3&&Tracked::alive==1);
        owner.reset();alias.reset();
        assert(Tracked::alive==1&&!observer.is_expired());
        retained.clear();
        assert(Tracked::alive==0&&observer.is_expired());
    }
    assert(observer.upgrade().empty_owner());
    assert(fails([&]{observer.upgrade().get();}));

    Array<bool> bits;bits.push(true);bits.at(0)=false;assert(!bits.get(0));
    auto alias=bits;alias.push(true);assert(bits.length()==2&&bits.use_count()==2);
    auto copy=bits.clone();copy.set(0,true);assert(!bits.get(0)&&copy.get(0));
    Weak<Array<bool>> watched(bits);bits.reset();alias.reset();
    assert(watched.is_expired()&&watched.upgrade().empty_owner());
    assert(fails([&]{watched.upgrade().push(true);}));

    HashMap<std::string,Ptr<Tracked>> map;
    map.set("second",Ptr<Tracked>::make(Tracked(2)));
    map.set("first",Ptr<Tracked>::make(Tracked(1)));
    assert(Tracked::alive==2&&map.keys().get(0)=="first");
    auto map_alias=map;assert(map_alias.remove("second")&&Tracked::alive==1);
    map.clear();assert(Tracked::alive==0);
    assert(fails([&]{map.get("missing");}));

    LinkedList<int> list;list.push(2);list.push_front(1);assert(list.pop_front()==1&&list.pop()==2);
    assert(fails([&]{list.pop();}));
    Queue<int> queue;queue.enqueue(1);queue.enqueue(2);assert(queue.dequeue()==1);
    Stack<int> stack;stack.push(1);stack.push(2);assert(stack.pop()==2);
    HashSet<int> set;assert(set.insert(2)&&set.insert(1)&&!set.insert(1));assert(set.items().get(0)==1);
    return 0;
}
