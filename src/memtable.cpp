// MemTable模板实现
#include "minilsm/memtable.h"
#include <fstream>
#include <iostream>
#include <type_traits>

namespace minilsm {
// 构造函数
template <typename K, typename V>
MemTable<K, V>::MemTable(const MemTableOptions& options)
    : options_(options), skip_list_(options.skip_list_max_level, options.skip_list_p), is_immutable_(false),
      current_mem_size_(0) {
}

// 写入数据
template <typename K, typename V>
bool MemTable<K, V>::put(const K& key, const V& value, const std::string& wal_path) {
    if(is_immutable_) {
        return false;
    }

    write_wal(wal_path, key, V());
    bool ok = skip_list_.insert(key, value);
    if(ok) {
        // 计算内存占用：key的大小 + value的大小 + 节点开销
        // 使用sizeof计算基本类型大小，对字符串等容器类型使用size()方法
        size_t key_size = sizeof(K);
        size_t value_size = sizeof(V);

        // 针对特殊类型的内存计算
        if constexpr(std::is_same_v<V, std::string> || std::is_same_v<V, std::vector<char>>) {
            value_size = value.size();
        }

        // 增加节点基本开销估计
        size_t node_overhead = sizeof(typename SkipList<K, V>::Node) +
                               sizeof(std::shared_ptr<typename SkipList<K, V>::Node>) * options_.skip_list_max_level;

        current_mem_size_ += key_size + value_size + node_overhead;
    }
    return ok;
}

// 删除数据
template <typename K, typename V>
bool MemTable<K, V>::remove(const K& key, const std::string& wal_path) {
    if(is_immutable_) {
        return false;
    }

    // 先尝试查找值，用于计算内存占用
    auto old_val = skip_list_.find(key);
    if(!old_val) {
        return false;
    }

    write_wal(wal_path, key, V());
    bool ok = skip_list_.erase(key);
    if(ok) {
        // 减去删除的键值对占用的内存
        size_t key_size = sizeof(K);
        size_t value_size = sizeof(V);

        // 针对特殊类型的内存计算
        if constexpr(std::is_same_v<V, std::string> || std::is_same_v<V, std::vector<char>>) {
            value_size = old_val->size();
        }

        // 减去节点基本开销估计
        size_t node_overhead = sizeof(typename SkipList<K, V>::Node) +
                               sizeof(std::shared_ptr<typename SkipList<K, V>::Node>) * options_.skip_list_max_level;

        current_mem_size_ -= (key_size + value_size + node_overhead);
        if(current_mem_size_ < 0) {
            current_mem_size_ = 0;
        }
    }
    return ok;
}

// 查找数据
template <typename K, typename V>
std::optional<V> MemTable<K, V>::get(const K& key) const {
    return skip_list_.find(key);
}

// 转为只读
template <typename K, typename V>
void MemTable<K, V>::make_immutable() {
    is_immutable_ = true;
}

// 判断是否写满
template <typename K, typename V>
bool MemTable<K, V>::is_full() const {
    return current_mem_size_ >= options_.max_mem_size;
}

// 判断是否只读
template <typename K, typename V>
bool MemTable<K, V>::is_immutable() const {
    return is_immutable_;
}

// 遍历所有数据
template <typename K, typename V>
void MemTable<K, V>::traverse(const std::function<void(const K&, const V&)>& callback) const {
    skip_list_.traverse(callback);
}

// 清空
template <typename K, typename V>
void MemTable<K, V>::clear() {
    skip_list_.clear();
    current_mem_size_ = 0;
}

// 获取大小
template <typename K, typename V>
size_t MemTable<K, V>::size() const {
    return skip_list_.size();
}

// 获取内存占用
template <typename K, typename V>
size_t MemTable<K, V>::get_mem_size() const {
    return current_mem_size_;
}

// 写入WAL日志
template <typename K, typename V>
void MemTable<K, V>::write_wal(const std::string& path, const K& key, const V& value) {
    std::ofstream wal_file(path, std::ios::app);
    if(wal_file.is_open()) {
        wal_file << "K:" << key << " V:";

        // 对std::vector<char>类型进行特殊处理
        if constexpr(std::is_same_v<V, std::vector<char>>) {
            // 将vector<char>转换为string输出
            wal_file << std::string(value.begin(), value.end());
        } else {
            // 其他类型直接输出
            wal_file << value;
        }

        wal_file << std::endl;
        wal_file.close();
    } else {
        std::cerr << "Error opening WAL file: " << path << std::endl;
    }
}

// 显式实例化常见类型的MemTable
template class MemTable<int, int>;
template class MemTable<long, long>;
template class MemTable<std::string, std::string>;
template class MemTable<int, bool>;

template class MemTable<std::string, int>;
template class MemTable<std::string, long>;

template class MemTable<int, std::string>;
template class MemTable<long, std::string>;

template class MemTable<bool, std::string>;
template class MemTable<bool, int>;
template class MemTable<bool, long>;

} // namespace minilsm