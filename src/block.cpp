#include "minilsm/block.h"

namespace minilsm {

bool Block::add_record(const Slice& key, const Slice& value) {
    if(is_full()) {
        return false;
    }
    std::size_t key_len = key.size();
    std::size_t value_len = value.size();

    size_t summary = key_len + value_len + sizeof(uint16_t) + 2 * sizeof(size_t);
    if(size_ + summary > capacity_) {
        return false;
    }
    uint16_t entry_offset = cursor_ - data_;
    // 1. 写入 key size + key
    memcpy(cursor_, &key_len, sizeof(key_len));
    cursor_ += sizeof(key_len);
    memcpy(cursor_, key.data(), key_len);
    cursor_ += key_len;

    // 2. 写入 value size + value
    memcpy(cursor_, &value_len, sizeof(value_len));
    cursor_ += sizeof(value_len);
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
    size_t start = 0;
    size_t end = offsets_.size() - 1;
    while(start <= end) {
        size_t mid = start + (end - start) / 2;
        uint16_t entry_offset = offsets_[mid];
        cursor_ = data_ + entry_offset;
        size_t key_size = *reinterpret_cast<const size_t*>(cursor_);
        cursor_ += sizeof(key_size);
        int cmp = Slice(cursor_, key_size).Compare(key);
        if(cmp == 0) {
            cursor_ += key_size;
            size_t value_size = *reinterpret_cast<const size_t*>(cursor_);
            cursor_ += sizeof(value_size);
            value = Slice(cursor_, value_size);
            return true;
        } else if(cmp < 0) {
            start = mid + 1;
        } else {
            end = mid - 1;
        }
    }
    return false;
}

// head: data | data
// tail: offsets | offset size (page tail)
bool Block::write_to_page() {
    size_t offsets_len = offsets_.size();
    size_t meta_total_size = sizeof(size_t) + offsets_len * sizeof(uint16_t);

    if(meta_total_size >= capacity_) {
        return false;
    }

    char* meta_start = data_ + capacity_ - meta_total_size;
    char* meta_cursor = meta_start;

    // 3. 先写 offsets_ 数组，再写 offsets_ 长度（页尾方向）
    memcpy(meta_cursor, offsets_.data(), offsets_len * sizeof(uint16_t));
    meta_cursor += sizeof(uint16_t) * offsets_len;
    memcpy(meta_cursor, &offsets_len, sizeof(offsets_len));

    return true;
}

bool Block::read_from_page(char* page_data, std::size_t page_capacity, Block& block) {
    // 校验 Page 合法性
    if(page_data == nullptr || page_capacity == 0) {
        return false;
    }

    // 从 Page 页尾反向读取元数据
    char* meta_cursor = page_data + page_capacity - sizeof(std::size_t);
    size_t offsets_len = *reinterpret_cast<const std::size_t*>(meta_cursor);

    const size_t meta_total_size = sizeof(std::size_t) + offsets_len * sizeof(uint16_t);
    if(meta_total_size > page_capacity) {
        return false;
    }

    meta_cursor = page_data + page_capacity - meta_total_size;
    block.offsets_.resize(offsets_len);
    memcpy(block.offsets_.data(), meta_cursor, offsets_len * sizeof(uint16_t));

    // 返回可用的 Block 对象
    return true;
}

} // namespace minilsm
