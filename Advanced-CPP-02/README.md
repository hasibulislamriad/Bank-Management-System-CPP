# Advanced C++ #02 — Concurrent Key-Value Store

A compact in-memory key-value store demonstrating templates, `std::shared_mutex`, concurrent reads, exclusive writes, optional values, and clean API design.

## Concepts
- C++17 templates
- `std::shared_mutex` and `shared_lock`
- `std::unordered_map`
- `std::optional`
- Thread-safe CRUD operations

## Build
```bash
g++ -std=c++17 -O2 -pthread main.cpp -o kv_store
./kv_store
```

Portfolio focus: modern C++17, data structures, concurrency and thread-safe design.
