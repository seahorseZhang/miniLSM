// MemTable模板实现
#include "minilsm/memtable.h"
#include "minilsm/comparator.h"
#include "minilsm/slice.h"
#include <fstream>
#include <iostream>
#include <type_traits>

namespace minilsm {

std::atomic<uint64_t> last_sequence_ = 0;

static const uint64_t maxSequenceNumber = ((0x1ULL << 56) - 1);

uint64_t AllocateSequenceNumbers(int n) {
    uint64_t start = last_sequence_.fetch_add(n, std::memory_order_relaxed);
    return start + 1;
}

// 构造函数
MemTable::MemTable(const MemTableOptions& options)
    : cmp_(InternalKeyComparator(BytewiseComparator())), options_(options),
      cur_skip_list_(std::make_unique<MemTableSkipList>(&cmp_, options.skip_list_max_level, options.skip_list_p)) {
}

// 写入数据
bool MemTable::insert_table(const Slice& key, const Slice& value, EntryType type) {
    char* internal_key_buf = new char[key.size() + 8];
    if(!internal_key_buf) {
        std::cerr << "new internal_key_buf error" << std::endl;
        return false;
    }
    InternalKey internal_key(key, AllocateSequenceNumbers(1), type, internal_key_buf);
    char* value_str = new char[value.size()];
    if(!value_str) {
        std::cerr << "new value_str error" << std::endl;
        return false;
    }
    memcpy(value_str, value.data(), value.size());
    cur_lock_.lock();
    bool ok = cur_skip_list_->insert(internal_key.Encode(), Slice((const char*)value_str, value.size()));
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
    char* internal_key_buf = new char[key.size() + 8];
    if(!internal_key_buf) {
        std::cerr << "new internal_key_buf error" << std::endl;
        return std::nullopt;
    }
    InternalKey internal_key(key, maxSequenceNumber, EntryType::kPut, internal_key_buf);
    cur_lock_.lock();
    MemTableIterator iter = cur_skip_list_->find_greater_or_equal(internal_key.Encode());
    cur_lock_.unlock();
    if(iter.has_value()) {
        InternalKey internal_key{};
        bool ok = internal_key.DecodeFrom(iter.key());
        if(!ok) {
            return std::nullopt;
        }
        if(internal_key.UserKey() != key || internal_key.Type() == EntryType::kDelete) {
            return std::nullopt;
        }
        return iter.value();
    }
    frozen_lock_.lock();
    for(auto& frozen_skip_list : frozen_skip_list_) {
        MemTableIterator iter = frozen_skip_list->find_greater_or_equal(internal_key.Encode());
        if(iter.has_value()) {
            InternalKey internal_key{};
            bool ok = internal_key.DecodeFrom(iter.key());
            if(!ok) {
                return std::nullopt;
            }
            if(internal_key.UserKey() != key || internal_key.Type() == EntryType::kDelete) {
                return std::nullopt;
            }
            frozen_lock_.unlock();
            return iter.value();
        }
    }
    frozen_lock_.unlock();
    return std::nullopt;
}

// 遍历所有数据
void MemTable::traverse(const std::function<void(const Slice&, const Slice&)>& callback) {
    cur_lock_.lock();
    for(MemTableIterator iter = cur_skip_list_->begin(); iter != cur_skip_list_->end(); ++iter) {
        InternalKey internal_key{};
        bool ok = internal_key.DecodeFrom(iter.key());
        if(!ok) {
            continue;
        }
        if(internal_key.Type() == EntryType::kDelete) {
            continue;
        }
        callback(internal_key.UserKey(), iter.value());
    }
    cur_lock_.unlock();
}

// 清空
void MemTable::clear() {
    cur_skip_list_->clear();
}

} // namespace minilsm
