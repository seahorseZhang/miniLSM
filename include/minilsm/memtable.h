#pragma once

#include "internal_key.h"
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

typedef SkipList<Slice, Slice> MemTableSkipList;

class MemTable {
public:
    explicit MemTable(const MemTableOptions& options = MemTableOptions());
    ~MemTable() = default;

    // 禁用拷贝
    MemTable(const MemTable&) = delete;
    MemTable& operator=(const MemTable&) = delete;

    // 核心接口
    bool put(const Slice& key, const Slice& value);
    bool remove(const Slice& key);
    std::optional<Slice> get(const Slice& key);
    void traverse(const std::function<void(const Slice&, const Slice&)>& callback) const;
    void clear();

private:
    // 辅助函数声明
    void write_wal(const std::string& path, const Slice& key, const Slice& value);
    bool insert_table(const Slice& key, const Slice& value, EntryType type);

    // 成员变量
    InternalKeyComparator cmp_;
    MemTableOptions options_;
    std::unique_ptr<MemTableSkipList> cur_skip_list_;
    std::mutex cur_lock_;
    std::vector<std::unique_ptr<MemTableSkipList>> frozen_skip_list_;
    std::mutex frozen_lock_;
};

} // namespace minilsm
