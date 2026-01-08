#pragma once

#include "skiplist.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace minilsm {

// MemTable配置
struct MemTableOptions {
    size_t table_max_mem_size = 4 * 1024 * 1024; // 默认4MB
    size_t max_table_num = 3;                    // cur + frozen skiplist最大个数
    int skip_list_max_level = 12;
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
    bool put(const K& key, const V& value);
    bool remove(const K& key);
    std::optional<V> get(const K& key) const;
    void make_immutable();
    bool is_immutable() const;
    void traverse(const std::function<void(const K&, const V&)>& callback) const;
    void clear();
    size_t size() const;

private:
    // 辅助函数声明
    void write_wal(const std::string& path, const K& key, const V& value);

    // 成员变量
    MemTableOptions options_;
    std::unique_ptr<SkipList<K, V>> cur_skip_list_;
    std::mutex cur_lock_;
    std::vector<std::unique_ptr<SkipList<K, V>>> frozen_skip_list_;
    std::mutex frozen_lock_;
};

} // namespace minilsm
