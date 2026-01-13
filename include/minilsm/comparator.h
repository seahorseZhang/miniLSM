#pragma once
#include "slice.h"
#include <string>

namespace minilsm {

template <typename T>
class Comparator {
public:
    virtual ~Comparator() = default;

    // ===== 核心接口 =====
    // 比较两个字节切片：返回值规则
    virtual int Compare(const T& a, const T& b) const = 0;

    // 返回比较器唯一名称
    virtual const char* Name() const = 0;
};

// 字节流比较器
class BytewiseComparatorImp : public Comparator<Slice> {
public:
    BytewiseComparatorImp(const BytewiseComparatorImp&) = delete;
    BytewiseComparatorImp& operator=(const BytewiseComparatorImp&) = delete;
    BytewiseComparatorImp(BytewiseComparatorImp&&) = delete;
    BytewiseComparatorImp& operator=(BytewiseComparatorImp&&) = delete;
    BytewiseComparatorImp() = default;

    // 核心比较逻辑
    int Compare(const Slice& a, const Slice& b) const override {
        return a.Compare(b);
    }

    const char* Name() const override {
        return "minilsm.BytewiseComparator";
    }
};

inline const BytewiseComparatorImp* BytewiseComparator() {
    static BytewiseComparatorImp instance;
    return &instance;
}

} // namespace minilsm