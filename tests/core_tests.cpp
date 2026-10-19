#include <spiral/core/event_bus.hpp>
#include <spiral/core/result.hpp>
#include <cassert>
#include <stdexcept>
struct Ping { int n; };
struct ThrowOnCopy {
 ThrowOnCopy() = default;
 ThrowOnCopy(const ThrowOnCopy&) { throw std::runtime_error("injected handler copy failure"); }
 void operator()(const Ping&) const {}
};
struct ReentrantCopy {
 spiral::core::EventBus* bus;
 int* observed;
 ReentrantCopy(spiral::core::EventBus& target, int& counter)
     : bus(&target), observed(&counter) {}
 ReentrantCopy(const ReentrantCopy& other)
     : bus(other.bus), observed(other.observed) { bus->publish(Ping{1}); }
 void operator()(const Ping&) const {}
};
int main(){
 spiral::core::Result<int> good(42); assert(good && good.value()==42);
 spiral::core::Result<int> bad(spiral::core::Error{7,"bad"}); assert(!bad && bad.error().code==7);
 spiral::core::EventBus bus; int count=0;
 const auto id=bus.subscribe<Ping>([&](const Ping& p){count+=p.n;});
 bus.publish(Ping{3}); assert(count==3);
 bus.unsubscribe<Ping>(id); bus.publish(Ping{2}); assert(count==3);
 int nested=0; bus.subscribe<Ping>([&](const Ping&){bus.subscribe<Ping>([&](const Ping&){++nested;});});
 bus.publish(Ping{1}); assert(nested==0); bus.publish(Ping{1}); assert(nested==1);
 // A failed subscription must not consume a deterministic subscription ID.
 spiral::core::EventBus failure_bus;
 const auto first=failure_bus.subscribe<Ping>([](const Ping&){});
 ThrowOnCopy throwing_handler;
 bool failed=false;
 try { (void)failure_bus.subscribe<Ping>(throwing_handler); }
 catch (const std::runtime_error&) { failed=true; }
 assert(failed);
 const auto next=failure_bus.subscribe<Ping>([](const Ping&){});
 assert(next==first+1);
 // Copying a handler may publish on the same bus; subscribe must not hold
 // its mutex while invoking that user-defined copy constructor.
 spiral::core::EventBus reentrant_bus;
 int observed=0;
 reentrant_bus.subscribe<Ping>([&](const Ping&){++observed;});
 ReentrantCopy reentrant_handler{reentrant_bus, observed};
 const auto reentrant_id=reentrant_bus.subscribe<Ping>(reentrant_handler);
 assert(reentrant_id==2 && observed>=1);
}
