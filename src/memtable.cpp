// MemTable模板实现
#include "minilsm/memtable.h"
#include <fstream>
#include <iostream>
#include <type_traits>

namespace minilsm {
// 构造函数
template <typename K, typename V>
MemTable<K, V>::MemTable(const MemTableOptions& options)
    : options_(options), skip_list_(options.skip_list_max_level, options.skip_list_p), is_immutable_(false) {
}

// 写入数据
template <typename K, typename V>
bool MemTable<K, V>::put(const K& key, const V& value) {
    bool ok = skip_list_.insert(key, value);
    if(ok) {
    }
    return ok;
}

// 删除数据
template <typename K, typename V>
bool MemTable<K, V>::remove(const K& key) {
    if(is_immutable_) {
        return false;
    }

    bool ok = skip_list_.erase(key);
    if(ok) {
        // 减去删除的键值对占用的内存
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
}

// 获取大小
template <typename K, typename V>
size_t MemTable<K, V>::size() const {
    return skip_list_.size();
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