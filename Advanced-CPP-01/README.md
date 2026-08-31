# Advanced C++ #01 — Thread Pool & Task Scheduler

A practical C++17 concurrency project demonstrating a reusable thread pool, task queue, futures, synchronization, and graceful shutdown.

## Concepts
- `std::thread`
- `std::mutex` / `std::condition_variable`
- `std::future` / `std::packaged_task`
- RAII and exception propagation
- Producer/consumer pattern

## Build
```bash
g++ -std=c++17 -O2 -pthread main.cpp -o thread_pool
./thread_pool
```

Portfolio focus: modern C++, concurrency, performance and clean API design.
