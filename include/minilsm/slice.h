#pragma once // 防止头文件重复包含（比#ifndef 更简洁）
#include <assert.h>
#include <cstddef> // size_t
#include <cstdint> // uint8_t（无符号字节）
#include <cstring> // memcmp
#include <string>  // std::string

namespace minilsm {

// lite bytes view：including bytes and its length
// same with LevelDB/RocksDB
class Slice {
public:
    Slice() noexcept : data_(nullptr), size_(0) {
    }

    Slice(const char* data, size_t size) noexcept : data_(data), size_(size) {
    }

    Slice(const std::string& s) noexcept : data_(s.data()), size_(s.size()) {
    }

    Slice(const char* data) noexcept : data_(data), size_(strlen(data)) {
    }

    Slice& operator=(const Slice&) = default;
    Slice& operator=(Slice&&) = default;
    Slice(const Slice&) = default;

    const char* data() const noexcept {
        return data_;
    }
    size_t size() const noexcept {
        return size_;
    }
    bool empty() const noexcept {
        return size_ == 0;
    }

    uint8_t operator[](size_t n) const noexcept {
        assert(n < size_);
        return static_cast<uint8_t>(data_[n]);
    }

    // 转为 std::string
    std::string ToString() const {
        return std::string(data_, size_);
    }

    // ========== 比较接口：无符号字节逐字节比较） ==========
    int Compare(const Slice& other) const noexcept;

    // 判断是否以指定前缀开头
    bool StartsWith(const Slice& prefix) const noexcept {
        return (size_ >= prefix.size_) && (memcmp(data_, prefix.data_, prefix.size_) == 0);
    }

private:
    // 只读成员：禁止修改
    const char* data_; // 字节流指针（无所有权）
    size_t size_;      // 有效字节长度
};

inline int Compare(const Slice& a, const Slice& b) noexcept {
    return a.Compare(b);
}

inline bool operator==(const Slice& a, const Slice& b) noexcept {
    return (a.size() == b.size()) && (memcmp(a.data(), b.data(), a.size()) == 0);
}

inline bool operator!=(const Slice& a, const Slice& b) noexcept {
    return !(a == b);
}

} // namespace minilsm

inline int minilsm::Slice::Compare(const Slice& other) const noexcept {
    const size_t min_len = std::min(size_, other.size_);
    int cmp = memcmp(data_, other.data_, min_len);
    if(cmp != 0) {
        return cmp;
    }
    // 长度不同：短的更小
    return (size_ < other.size_) ? -1 : (size_ > other.size_) ? 1 : 0;
}