#include "minilsm/skiplist.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <sstream>
#include <string>

namespace minilsm {

template <typename K, typename V>
SkipList<K, V>::Node::Node(int level, const K& k, const V& v) : key(k), value(v), forward(level) {}

template <typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel, double p) : maxLevel_(maxLevel), p_(p), level_(1), gen_(rd_()), dist_(0.0, 1.0) {
    head_ = std::make_shared<Node>(maxLevel_, K{}, V{});
}

template <typename K, typename V> SkipList<K, V>::~SkipList() { clear(); }

template <typename K, typename V> bool SkipList<K, V>::insert(const K& key, const V& value) {
    std::vector<std::shared_ptr<Node>> update(maxLevel_, nullptr);
    std::shared_ptr<Node> x = head_;
    for(int i = level_ - 1; i >= 0; --i) {
        while(x->forward[i] && x->forward[i]->key < key)
            x = x->forward[i];
        update[i] = x;
    }
    x = x->forward[0];
    if(x && x->key == key)
        return false; // already exists
    int lvl = randomLevel();
    if(lvl > level_) {
        for(int i = level_; i < lvl; ++i)
            update[i] = head_;
        level_ = lvl;
    }
    auto n = std::make_shared<Node>(lvl, key, value);
    for(int i = 0; i < lvl; ++i) {
        n->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = n;
    }
    ++size_;
    return true;
}

template <typename K, typename V> std::optional<V> SkipList<K, V>::find(const K& key) const {
    std::shared_ptr<Node> x = head_;
    for(int i = level_ - 1; i >= 0; --i) {
        while(x->forward[i] && x->forward[i]->key < key)
            x = x->forward[i];
    }
    x = x->forward[0];
    if(x && x->key == key)
        return x->value;
    return std::nullopt;
}

template <typename K, typename V> bool SkipList<K, V>::update(const K& key, const V& value) {
    std::shared_ptr<Node> x = head_;
    for(int i = level_ - 1; i >= 0; --i) {
        while(x->forward[i] && x->forward[i]->key < key)
            x = x->forward[i];
    }
    x = x->forward[0];
    if(x && x->key == key) {
        x->value = value;
        return true;
    }
    return false;
}

template <typename K, typename V> bool SkipList<K, V>::erase(const K& key) {
    std::vector<std::shared_ptr<Node>> update(maxLevel_, nullptr);
    std::shared_ptr<Node> x = head_;
    for(int i = level_ - 1; i >= 0; --i) {
        while(x->forward[i] && x->forward[i]->key < key)
            x = x->forward[i];
        update[i] = x;
    }
    x = x->forward[0];
    if(!x || x->key != key)
        return false;
    for(int i = 0; i < level_; ++i) {
        if(update[i]->forward[i] != x)
            break;
        update[i]->forward[i] = x->forward[i];
    }
    // shared_ptr will release ownership when no references remain
    while(level_ > 1 && head_->forward[level_ - 1] == nullptr)
        --level_;
    --size_;
    return true;
}

template <typename K, typename V> void SkipList<K, V>::clear() {
    // Release all forward pointers; shared_ptr will clean up nodes
    for(int i = 0; i < maxLevel_; ++i)
        head_->forward[i].reset();
    level_ = 1;
    size_ = 0;
}

template <typename K, typename V> size_t SkipList<K, V>::size() const { return size_; }

template <typename K, typename V> int SkipList<K, V>::randomLevel() {
    int lvl = 1;
    while(dist_(gen_) < p_ && lvl < maxLevel_)
        ++lvl;
    return lvl;
}

template <typename K, typename V> std::string SkipList<K, V>::to_dot() const {
    std::ostringstream out;
    out << "digraph SkipList {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=record];\n";

    // collect all nodes in level 0 order
    std::vector<std::shared_ptr<Node>> nodes;
    auto cur = head_->forward[0];
    while(cur) {
        nodes.push_back(cur);
        cur = cur->forward[0];
    }

    // node definitions
    for(size_t i = 0; i < nodes.size(); ++i) {
        auto& n = nodes[i];
        out << "  node" << i << " [label=\"{" << n->key << "|" << n->value << "}\"]" << ";\n";
    }

    // head node
    out << "  head [label=\"HEAD\", shape=box];\n";

    // level edges
    for(int lvl = maxLevel_ - 1; lvl >= 0; --lvl) {
        // create invisible rank grouping per level (optional)
        for(size_t i = 0; i < nodes.size(); ++i) {
            auto tgt = nodes[i]->forward.size() > (size_t)lvl ? nodes[i]->forward[lvl] : nullptr;
            if(tgt) {
                // find index of tgt
                auto it = std::find_if(nodes.begin(), nodes.end(),
                                       [&](const std::shared_ptr<Node>& p) { return p.get() == tgt.get(); });
                if(it != nodes.end()) {
                    size_t j = std::distance(nodes.begin(), it);
                    out << "  node" << i << " -> node" << j << " [label=\"L" << lvl << "\"];\n";
                }
            }
        }
        // head forward at this level
        if(head_->forward.size() > 0 && head_->forward.size() > (size_t)lvl) {
            auto h = head_->forward[lvl];
            if(h) {
                auto it = std::find_if(nodes.begin(), nodes.end(),
                                       [&](const std::shared_ptr<Node>& p) { return p.get() == h.get(); });
                if(it != nodes.end()) {
                    size_t j = std::distance(nodes.begin(), it);
                    out << "  head -> node" << j << " [style=dashed,label=\"L" << lvl << "\"];\n";
                }
            }
        }
    }

    out << "}\n";
    return out.str();
}

// Explicit instantiations for common types so the shared library emits symbols.
template class SkipList<int, int>;
template class SkipList<long, long>;
template class SkipList<std::string, std::string>;
template class SkipList<int, bool>;

} // namespace minilsm
