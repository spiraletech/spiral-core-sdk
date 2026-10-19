#include <spiral/core/event_bus.hpp>
#include <iostream>
#include <memory>

struct Ping { int value; };

int main() {
    spiral::core::EventBus bus;
    int destroyed = 0;
    int reentrant_calls = 0;
    int removed_calls = 0;

    // The last callback owner must be released *after* the subscription
    // mutex is unlocked: its captured resource may reenter EventBus.
    auto resource = std::shared_ptr<int>(new int(7), [&](int* value) {
        delete value;
        ++destroyed;
        bus.subscribe<Ping>([&](const Ping&) { ++reentrant_calls; });
        bus.publish(Ping{1});
    });
    const auto removed_id = bus.subscribe<Ping>(
        [resource, &removed_calls](const Ping&) { ++removed_calls; });
    resource.reset();
    bus.unsubscribe<Ping>(removed_id);
    if (destroyed != 1 || reentrant_calls != 1) {
        std::cerr << "callback teardown failed to reenter EventBus\n";
        return 1;
    }
    bus.publish(Ping{1});
    if (removed_calls != 0 || reentrant_calls != 2) {
        std::cerr << "removed subscription invoked or reentrant subscription lost\n";
        return 1;
    }

    // Snapshot ownership must still permit a handler to remove itself
    // during publish without invalidating the active dispatch.
    spiral::core::EventBus self_bus;
    int self_calls = 0;
    std::size_t self_id = 0;
    self_id = self_bus.subscribe<Ping>([&](const Ping&) {
        ++self_calls;
        self_bus.unsubscribe<Ping>(self_id);
    });
    self_bus.publish(Ping{1});
    self_bus.publish(Ping{1});
    if (self_calls != 1) {
        std::cerr << "self-unsubscribe broke snapshot dispatch\n";
        return 1;
    }
    return 0;
}
