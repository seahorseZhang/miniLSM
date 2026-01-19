#include "minilsm/block.h"

namespace minilsm {

bool Block::add_record(const Slice& key, const Slice& value) {
    if(is_full()) {
        return false;
    }
    std::size_t key_len = key.size();
    std::size_t value_len = value.size();

    size_t summary = key_len + value_len + sizeof(uint16_t);
    if(size_ + summary > capacity_) {
        return false;
    }
    uint16_t entry_offset = cursor_ - data_;
    memcpy(cursor_ + entry_offset, key.data(), key_len);
    cursor_ += key_len;
    memcpy(cursor_, value.data(), value_len);
    cursor_ += value_len;
    // 4. 更新 size_
    size_ += summary;
    // 5. 写入 offset array
    offsets_.push_back(entry_offset);
    return true;
}

bool Block::get_record(const Slice& key, Slice& value) {
    if(is_full()) {
        return false;
    }
}

} // namespace minilsm
