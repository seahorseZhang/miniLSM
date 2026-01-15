#include "minilsm/internal_key.h"
#include <cstring>
#include <mutex>

namespace minilsm {

// 序列化 uint64_t 为 8 字节（little-endian）
void EncodeFixed64(std::string* dst, uint64_t value) {
    char buf[8];
    std::memcpy(buf, &value, sizeof(buf));
    dst->append(buf, 8);
}

// 反序列化 8 字节为 uint64_t（little-endian）
uint64_t DecodeFixed64(const char* ptr) {
    uint64_t result;
    std::memcpy(&result, ptr, sizeof(result));
    return result;
}

// ===================== InternalKey 实现 =====================
void InternalKey::Encode(const Slice& user_key, EntryType type) {
    rep_.assign(user_key.data(), user_key.size());

    uint64_t packed = static_cast<uint8_t>(type);
    EncodeFixed64(&rep_, packed);
    type_ = type;
}

bool InternalKey::DecodeFrom(const Slice& slice) {
    rep_.assign(slice.data(), slice.size());

    const char* suffix = rep_.data() + rep_.size();
    uint64_t packed = DecodeFixed64(suffix);
    type_ = static_cast<EntryType>(packed & 0xFF);

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

    return 0;
}

int InternalKeyComparator::Compare(const ParsedInternalKey& a, const ParsedInternalKey& b) const {
    int cmp = user_comparator_->Compare(a.user_key, b.user_key);
    return cmp;
}

} // namespace minilsm
