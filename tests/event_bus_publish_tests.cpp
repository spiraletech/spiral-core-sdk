#include <spiral/core/event_bus.hpp>
#include <stdexcept>
#include <iostream>

struct Ping { int value; };
struct StatefulHandler {
 mutable int local_state{0};
 int* observed;
 void operator()(const Ping&) const { *observed = ++local_state; }
};
struct CopyTrap {
 bool* reject_copy;
 int* calls;
 CopyTrap(bool& reject, int& count) : reject_copy(&reject), calls(&count) {}
 CopyTrap(const CopyTrap& other) : reject_copy(other.reject_copy), calls(other.calls) {
  if (*reject_copy) throw std::runtime_error("handler copied during publish");
 }
 void operator()(const Ping&) const { ++*calls; }
};

int main() {
 int failures = 0;
 spiral::core::EventBus state_bus;
 int observed_state = 0;
 state_bus.subscribe<Ping>(StatefulHandler{0, &observed_state});
 state_bus.publish(Ping{1});
 if (observed_state != 1) { std::cerr << "first publish state incorrect\n"; ++failures; }
 state_bus.publish(Ping{1});
 if (observed_state != 2) { std::cerr << "handler state lost between publishes\n"; ++failures; }

 spiral::core::EventBus copy_bus;
 bool reject_copy = false;
 int calls = 0;
 CopyTrap trap{reject_copy, calls};
 copy_bus.subscribe<Ping>(trap);
 reject_copy = true;
 try { copy_bus.publish(Ping{1}); }
 catch (const std::runtime_error&) { std::cerr << "handler copied during publish\n"; ++failures; }
 if (calls != 1) { std::cerr << "publish handler not invoked exactly once\n"; ++failures; }
 return failures == 0 ? 0 : 1;
}
