#include "minilsm/internal_key.h"
#include <cstring>
#include <mutex>

namespace minilsm {

// 序列化 uint64_t 为 8 字节（little-endian）
void EncodeFixed64(char* dst, uint64_t value) {
    std::memcpy(dst, &value, sizeof(uint64_t));
}

// 反序列化 8 字节为 uint64_t（little-endian）
uint64_t DecodeFixed64(const char* ptr) {
    uint64_t result;
    std::memcpy(&result, ptr, sizeof(uint64_t));
    return result;
}

// ===================== InternalKey 实现 =====================
void InternalKey::Encode(const Slice& user_key, uint64_t seq, EntryType type, char* buf) {
    uint64_t packed = (seq << 8) | static_cast<uint8_t>(type);
    std::memcpy(buf, user_key.data(), user_key.size());
    EncodeFixed64(buf + user_key.size(), packed);
    buf_ = buf;
    buf_size_ = user_key.size() + 8;
    seq_ = seq;
    type_ = type;
}

bool InternalKey::DecodeFrom(const Slice& slice) {
    if(slice.size() < 8) {
        return false;
    }

    buf_ = slice.data();
    buf_size_ = slice.size();
    const char* suffix = slice.data() + slice.size() - 8;
    uint64_t packed = DecodeFixed64(suffix);
    type_ = static_cast<EntryType>(packed & 0xFF);
    seq_ = packed >> 8;

    if(type_ != EntryType::kPut && type_ != EntryType::kDelete) {
        return false;
    }
    return true;
}

// ===================== 辅助函数：ParseInternalKey =====================
bool ParseInternalKey(const Slice& internal_key, ParsedInternalKey* result) {
    if(internal_key.size() < 8) {
        return false;
    }

    // 1. 提取 user_key
    result->user_key = Slice(internal_key.data(), internal_key.size() - 8);

    // 2. 提取 sequence + type
    const char* suffix = internal_key.data() + internal_key.size() - 8;
    uint64_t packed = DecodeFixed64(suffix);
    result->type = static_cast<EntryType>(packed & 0xFF);
    result->sequence = packed >> 8;

    return (result->type == EntryType::kPut || result->type == EntryType::kDelete);
}

// ===================== InternalKeyComparator 实现 =====================
int InternalKeyComparator::Compare(const Slice& a, const Slice& b) const {
    ParsedInternalKey parsed_a, parsed_b;

    // 1. parse InternalKey
    if(!ParseInternalKey(a, &parsed_a) || !ParseInternalKey(b, &parsed_b)) {
        return a.Compare(b);
    }

    // 2. 第一步：比较 user_key
    int cmp = user_comparator_->Compare(parsed_a.user_key, parsed_b.user_key);
    if(cmp != 0) {
        return cmp;
    }
    // 3. 第二步：比较 sequence（逆序）
    if(parsed_a.sequence > parsed_b.sequence) {
        return -1;
    } else if(parsed_a.sequence < parsed_b.sequence) {
        return 1;
    }

    return 0;
}

} // namespace minilsm
