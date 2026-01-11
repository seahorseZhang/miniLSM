// MemTable模板实现
#include "minilsm/memtable.h"
#include "minilsm/slice.h"
#include <fstream>
#include <iostream>
#include <type_traits>

namespace minilsm {
// 构造函数
MemTable::MemTable(const MemTableOptions& options)
    : cmp_(InternalKeyComparator(BytewiseComparator())), options_(options),
      cur_skip_list_(std::make_unique<MemTableSkipList>(&cmp_, options.skip_list_max_level, options.skip_list_p)) {
}

// 写入数据
bool MemTable::insert_table(const Slice& key, const Slice& value, EntryType type) {
    InternalKey internal_key(key, type);
    cur_lock_.lock();
    bool ok = cur_skip_list_->insert(internal_key.Encode(), value);
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
        cur_skip_list_ = std::make_unique<MemTableSkipList>(&cmp_, options_.skip_list_max_level, options_.skip_list_p);
    }
    cur_lock_.unlock();
    return true;
}

bool MemTable::put(const Slice& key, const Slice& value) {
    return insert_table(key, value, EntryType::kPut);
}

// 删除数据
bool MemTable::remove(const Slice& key) {
    return insert_table(key, Slice(), EntryType::kDelete);
}

// 查找数据
std::optional<Slice> MemTable::get(const Slice& key) {
    cur_lock_.lock();
    std::optional<Slice> ret = cur_skip_list_->find(key);
    cur_lock_.unlock();
    if(ret.has_value()) {
        return ret.value();
    }
    frozen_lock_.lock();
    for(auto& frozen_skip_list : frozen_skip_list_) {
        ret = frozen_skip_list->find(key);
        if(ret.has_value()) {
            frozen_lock_.unlock();
            return ret.value();
        }
    }
    frozen_lock_.unlock();
    return ret;
}

// 遍历所有数据
void MemTable::traverse(const std::function<void(const Slice&, const Slice&)>& callback) const {
    cur_skip_list_->traverse(callback);
}

// 清空
void MemTable::clear() {
    cur_skip_list_->clear();
}

// 获取大小
size_t MemTable::size() const {
    return cur_skip_list_->size();
}

} // namespace minilsm
