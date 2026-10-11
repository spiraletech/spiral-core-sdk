# Spiral Core SDK

Dependency-free C++20 infrastructure shared across SpiralTech projects. Not an entity/world-state authority: SPIRAL-OS-HOME retains canonical world-state authority.

Build and test:
```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Target: Spiral::Core. Version 0.1.0: typed Result<T> and synchronous typed EventBus. Callback execution occurs outside the subscription lock; exceptions propagate; handler captures must outlive subscriptions. Avoid calling the event bus from real-time audio threads.
