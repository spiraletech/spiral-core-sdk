#include <spiral/core/contract.hpp>
#include <spiral/core/event_bus.hpp>
#include <spiral/core/result.hpp>
#include <cstdint>

struct Event { int value; };
int main() {
 using namespace spiral::core;
 Result<int> status{42};
 if (!status || status.value() != 42) return 1;
 const Message original{1, "spiral.contract.smoke", {0, 42, 255}};
 const auto wire = MessageCodec::encode(original);
 if (!wire) return 2;
 const auto decoded = MessageCodec::decode(*wire);
 if (!decoded || decoded.message->topic != original.topic ||
     decoded.message->payload != original.payload) return 3;
 EventBus bus;
 int sum = 0;
 const auto id = bus.subscribe<Event>([&sum](const Event& event) { sum += event.value; });
 bus.publish(Event{5});
 bus.unsubscribe<Event>(id);
 bus.publish(Event{10});
 return sum == 5 ? 0 : 4;
}
