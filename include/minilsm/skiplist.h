// skiplist.h - declarations for SkipList
#pragma once
#include <vector>
#include <random>
#include <memory>
#include <limits>
#include <optional>

namespace minilsm {

template<typename K, typename V>
class SkipList {
public:
    struct Node {
        K key;
        V value;
        std::vector<std::shared_ptr<Node>> forward;
        Node(int level, const K& k, const V& v);
    };

    explicit SkipList(int maxLevel = 16, double p = 0.5);
    ~SkipList();

    bool insert(const K& key, const V& value);
    std::optional<V> find(const K& key) const;
    bool update(const K& key, const V& value);
    bool erase(const K& key);
    void clear();
    size_t size() const;

private:
    int randomLevel();

    int maxLevel_;
    double p_;
    int level_;
    std::shared_ptr<Node> head_;
    size_t size_ = 0;

    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<double> dist_;
};

} // namespace minilsm
