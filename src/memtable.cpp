// MemTable模板实现
#include "minilsm/memtable.h"
#include <fstream>
#include <iostream>
#include <type_traits>

namespace minilsm {
// 构造函数
template <typename K, typename V>
MemTable<K, V>::MemTable(const MemTableOptions& options)
    : options_(options),
      cur_skip_list_(std::make_unique<SkipList<K, V>>(options.skip_list_max_level, options.skip_list_p)) {
}

// 写入数据
template <typename K, typename V>
bool MemTable<K, V>::put(const K& key, const V& value) {
    cur_lock_.lock();
    bool ok = cur_skip_list_->insert(key, value);
    if(!ok) {
        cur_lock_.unlock();
        return false;
    }
    if(cur_skip_list_->size() > options_.table_max_mem_size) {
        frozen_lock_.lock();
        frozen_skip_list_.push_back(std::move(cur_skip_list_));
        if(frozen_skip_list_.size() > options_.max_table_num) {
            frozen_lock_.unlock();
            cur_lock_.unlock();
            // todo wait bg
        }
        cur_skip_list_ = std::make_unique<SkipList<K, V>>(options_.skip_list_max_level, options_.skip_list_p);
    }
    cur_lock_.unlock();
    return true;
}

// 删除数据
template <typename K, typename V>
bool MemTable<K, V>::remove(const K& key) {
    return true;
}

// 查找数据
template <typename K, typename V>
std::optional<V> MemTable<K, V>::get(const K& key) const {
    return cur_skip_list_->find(key);
}

// 遍历所有数据
template <typename K, typename V>
void MemTable<K, V>::traverse(const std::function<void(const K&, const V&)>& callback) const {
    cur_skip_list_->traverse(callback);
}

// 清空
template <typename K, typename V>
void MemTable<K, V>::clear() {
    cur_skip_list_->clear();
}

// 获取大小
template <typename K, typename V>
size_t MemTable<K, V>::size() const {
    return cur_skip_list_->size();
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
