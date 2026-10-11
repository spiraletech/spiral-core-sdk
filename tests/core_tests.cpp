#include <spiral/core/event_bus.hpp>
#include <spiral/core/result.hpp>
#include <cassert>
struct Ping { int n; };
int main(){
 spiral::core::Result<int> good(42); assert(good && good.value()==42);
 spiral::core::Result<int> bad(spiral::core::Error{7,"bad"}); assert(!bad && bad.error().code==7);
 spiral::core::EventBus bus; int count=0;
 const auto id=bus.subscribe<Ping>([&](const Ping& p){count+=p.n;});
 bus.publish(Ping{3}); assert(count==3);
 bus.unsubscribe<Ping>(id); bus.publish(Ping{2}); assert(count==3);
 int nested=0; bus.subscribe<Ping>([&](const Ping&){bus.subscribe<Ping>([&](const Ping&){++nested;});});
 bus.publish(Ping{1}); assert(nested==0); bus.publish(Ping{1}); assert(nested==1);
}
