// MemTable模板实现
#include "minilsm/memtable.h"
#include "minilsm/comparator.h"
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
    InternalKey* internal_key = new InternalKey(key, type);
    if(!internal_key) {
        std::cerr << "new internal_key error" << std::endl;
        return false;
    }
    void* value_str = malloc(value.size());
    if(!value_str) {
        std::cerr << "malloc value_str error" << std::endl;
        return false;
    }
    memcpy(value_str, value.data(), value.size());
    cur_lock_.lock();
    bool ok = cur_skip_list_->insert(internal_key->UserKey(), Slice((const char*)value_str, value.size()));
    if(!ok) {
        cur_lock_.unlock();
        return true;
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

} // namespace minilsm
