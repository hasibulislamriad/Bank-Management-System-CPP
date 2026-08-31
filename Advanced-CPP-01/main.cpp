#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopping = false;

public:
    explicit ThreadPool(std::size_t count = std::thread::hardware_concurrency()) {
        if (count == 0) count = 2;
        for (std::size_t i = 0; i < count; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this] { return stopping || !tasks.empty(); });
                        if (stopping && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using Result = std::invoke_result_t<F, Args...>;
        auto job = std::make_shared<std::packaged_task<Result()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto future = job->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopping) throw std::runtime_error("ThreadPool is stopping");
            tasks.emplace([job] { (*job)(); });
        }
        cv.notify_one();
        return future;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        cv.notify_all();
        for (auto& worker : workers) worker.join();
    }
};

int main() {
    ThreadPool pool(4);
    std::vector<std::future<long long>> results;

    for (int n = 1; n <= 8; ++n) {
        results.push_back(pool.submit([n] {
            long long value = 1;
            for (int i = 1; i <= n; ++i) value *= i;
            return value;
        }));
    }

    for (std::size_t i = 0; i < results.size(); ++i)
        std::cout << "Task " << i + 1 << " result: " << results[i].get() << '\n';
}
