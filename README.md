# Spiral Core SDK

Dependency-free C++20 infrastructure shared across SpiralTech projects. This SDK is **not** an entity/world-state authority: SPIRAL-OS-HOME retains canonical world-state authority. Do not call the synchronous EventBus from real-time audio threads.

Build and test:
```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Install to a separate prefix and consume the exported `Spiral::Core` interface target:
```sh
cmake --install build --prefix ./sdk-install
cmake -S tests/downstream_consumer -B consumer-build -DCMAKE_PREFIX_PATH="$(pwd)/sdk-install"
cmake --build consumer-build
ctest --test-dir consumer-build --output-on-failure
```

Consumers use `find_package(SpiralCore 0.2 CONFIG REQUIRED)` followed by `target_link_libraries(app PRIVATE Spiral::Core)`. The installed target supplies the public include path and C++20 requirement; no source-tree include paths are required. Windows multi-configuration generators must pass the desired `--config` to build/install and `-C` to CTest.

The SDK exposes `Result<T>`, synchronous typed `EventBus`, and the version-1 `MessageCodec` wire contract. EventBus callbacks execute outside the subscription lock; exceptions propagate and captured references must outlive subscriptions. The installed-consumer smoke test validates public headers, wire round-trip, and EventBus subscribe/publish/unsubscribe behavior.
