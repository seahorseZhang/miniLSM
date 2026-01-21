#pragma once

#include "slice.h" //
#include <vector>

namespace minilsm {
// format:
// data: | key | value |
// offset array
class Block {
public:
    Block(char* data, std::size_t capacity) noexcept : data_(data), capacity_(capacity), size_(0), offsets_() {
        cursor_ = data_;
        assert(data_ != nullptr);
    }

    bool add_record(const Slice& key, const Slice& value);

    bool get_record(const Slice& key, Slice& value);

    bool write_to_page();

    bool read_from_page(char* page_data, std::size_t page_capacity, Block& block);

    bool is_full() const {
        return size_ >= capacity_;
    }

    bool is_empty() const {
        return size_ == 0;
    }

private:
    char* data_; // block bytes
    char* cursor_;
    size_t capacity_;               // block capacity
    size_t size_;                   // block size
    std::vector<uint16_t> offsets_; //
};

} // namespace minilsm
