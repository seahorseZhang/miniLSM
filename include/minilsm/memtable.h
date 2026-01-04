#pragma once

#include "skiplist.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace minilsm {



// MemTable配置
struct MemTableOptions {
    size_t max_mem_size = 4 * 1024 * 1024; // 默认4MB
    int skip_list_max_level = 16;
    double skip_list_p = 0.5;
};

template <typename K, typename V>
class MemTable {
public:
    explicit MemTable(const MemTableOptions& options = MemTableOptions());
    ~MemTable() = default;

    // 禁用拷贝
    MemTable(const MemTable&) = delete;
    MemTable& operator=(const MemTable&) = delete;

    // 核心接口
    bool put(const K& key, const V& value, const std::string& wal_path = "./wal.log");
    bool remove(const K& key, const std::string& wal_path = "./wal.log");
    std::optional<V> get(const K& key) const;
    void make_immutable();
    bool is_full() const;
    bool is_immutable() const;
    void traverse(const std::function<void(const K&, const V&)>& callback) const;
    void clear();
    size_t size() const;
    size_t get_mem_size() const;

private:
    // 辅助函数声明
    void write_wal(const std::string& path, const K& key, const V& value);

    // 成员变量
    MemTableOptions options_;
    SkipList<K, V> skip_list_;
    std::atomic<bool> is_immutable_;
    std::atomic<size_t> current_mem_size_;
};

} // namespace minilsm