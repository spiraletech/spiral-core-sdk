#include <spiral/core/event_bus.hpp>
#include <iostream>
#include <memory>
#include <type_traits>

struct Tick { int value; };

struct MoveOnlyHandler {
    std::unique_ptr<int> state;
    int* observed;
    void operator()(const Tick& tick) {
        *state += tick.value;
        *observed = *state;
    }
};

struct ReentrantDeleter {
    spiral::core::EventBus* bus;
    int* destroyed;
    int* reentrant_calls;
    void operator()(int* value) const {
        delete value;
        ++*destroyed;
        auto* calls = reentrant_calls;
        bus->subscribe<Tick>([calls](const Tick&) { ++*calls; });
        bus->publish(Tick{1});
    }
};

static_assert(!std::is_copy_constructible_v<MoveOnlyHandler>);
static_assert(std::is_move_constructible_v<MoveOnlyHandler>);

int main() {
    spiral::core::EventBus bus;
    int observed = 0;
    const auto id = bus.subscribe<Tick>(MoveOnlyHandler{std::make_unique<int>(0), &observed});
    bus.publish(Tick{2});
    if (observed != 2) { std::cerr << "first move-only invocation failed\n"; return 1; }
    bus.publish(Tick{3});
    if (observed != 5) { std::cerr << "move-only handler state was lost\n"; return 2; }
    bus.unsubscribe<Tick>(id);
    bus.publish(Tick{7});
    if (observed != 5) { std::cerr << "unsubscribed move-only handler invoked\n"; return 3; }
    // Releasing the move-only handler must also preserve B33's rule:
    // user-owned resources are destroyed outside the subscription lock.
    int destroyed = 0;
    int reentrant_calls = 0;
    auto resource = std::unique_ptr<int, ReentrantDeleter>(
        new int(7), ReentrantDeleter{&bus, &destroyed, &reentrant_calls});
    const auto reentrant_id = bus.subscribe<Tick>(
        [owned = std::move(resource)](const Tick&) { (void)owned; });
    bus.unsubscribe<Tick>(reentrant_id);
    if (destroyed != 1 || reentrant_calls != 1) {
        std::cerr << "move-only teardown did not safely reenter EventBus\n";
        return 4;
    }
    return 0;
}
