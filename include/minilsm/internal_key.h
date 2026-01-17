#pragma once
#include "comparator.h"
#include "slice.h"
#include <cstdint>
#include <string>

namespace minilsm {

// 操作类型（Put/Delete）
enum class EntryType : uint8_t {
    kPut = 0,   // 插入操作
    kDelete = 1 // 删除操作
};

struct ParsedInternalKey {
    Slice user_key;    //
    uint64_t sequence; // 序列号
    EntryType type;    //
};

// ===================== InternalKey 类 =====================
// 功能：封装 user_key + EntryType，提供序列化/反序列化
class InternalKey {
public:
    InternalKey() = default;

    InternalKey(const Slice& user_key, uint64_t seq, EntryType type, char* buf) {
        Encode(user_key, seq, type, buf);
    }

    bool DecodeFrom(const Slice& slice);

    Slice Encode() const {
        return Slice(buf_, buf_size_);
    }

    Slice UserKey() const {
        return Slice(buf_, buf_size_ - 8);
    }

    EntryType Type() const {
        return type_;
    }

    uint64_t Sequence() const {
        return seq_;
    }

private:
    // 格式：[user_key][type (56B)][type (8B)] —— 共 16 字节后缀，8字节后续可以扩展
    void Encode(const Slice& user_key, uint64_t seq, EntryType type, char* buf);

    const char* buf_ = nullptr; // serialized key + type
    size_t buf_size_ = 0;
    EntryType type_ = EntryType::kPut;
    uint64_t seq_ = 0;
};

// ===================== InternalKeyComparator 类 =====================
// 功能：先比较 user_key（用用户比较器）
class InternalKeyComparator : public Comparator<Slice> {
public:
    explicit InternalKeyComparator(const Comparator* user_comparator) : user_comparator_(user_comparator) {
    }

    // 核心：比较两个 InternalKey（Slice 形式）
    int Compare(const Slice& a, const Slice& b) const override;

    // 比较器名称（用于校验）
    const char* Name() const override {
        return "minilsm.InternalKeyComparator";
    }

    // 获取用户比较器
    const Comparator* user_comparator() const {
        return user_comparator_;
    }

private:
    const Comparator* user_comparator_; // 底层用户比较器（如字节序比较器）
};

bool ParseInternalKey(const Slice& internal_key, ParsedInternalKey* result);

} // namespace minilsm