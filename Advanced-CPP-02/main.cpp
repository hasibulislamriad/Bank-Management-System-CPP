#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class KeyValueStore {
    std::unordered_map<std::string, std::string> data;
    mutable std::shared_mutex mutex;

public:
    void set(std::string key, std::string value) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        data[std::move(key)] = std::move(value);
    }

    std::optional<std::string> get(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto it = data.find(key);
        if (it == data.end()) return std::nullopt;
        return it->second;
    }

    bool erase(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        return data.erase(key) > 0;
    }

    std::size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return data.size();
    }
};

int main() {
    KeyValueStore store;
    store.set("app", "WHMCS");
    store.set("environment", "production");

    std::vector<std::thread> readers;
    for (int i = 0; i < 6; ++i) {
        readers.emplace_back([&store, i] {
            auto value = store.get("app");
            std::cout << "Reader " << i << ": "
                      << value.value_or("<missing>") << '\n';
        });
    }
    for (auto& t : readers) t.join();

    std::cout << "Entries: " << store.size() << '\n';
}
