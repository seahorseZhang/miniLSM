// skiplist.h - SkipList implementation
#pragma once
#include "comparator.h"
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <vector>

namespace minilsm {
template <typename K, typename V>
class SkipList {
public:
    struct Node {
        K key;
        V value;
        std::vector<std::shared_ptr<Node>> forward;
        Node(int level, const K& k, const V& v) : key(k), value(v), forward(level) {
        }
    };

    SkipList& operator=(const SkipList&) = delete;
    explicit SkipList(Comparator<K>* cmp, int maxLevel = 12, double p = 0.5)
        : maxLevel_(maxLevel), p_(p), level_(1), gen_(rd_()), dist_(0.0, 1.0), cmp_(cmp) {
        head_ = std::make_shared<Node>(maxLevel_, K{}, V{});
    }

    ~SkipList() {
        clear();
    }

    bool insert(const K& key, const V& value) {
        std::vector<std::shared_ptr<Node>> update(maxLevel_, nullptr);
        std::shared_ptr<Node> x = head_;
        for(int i = level_ - 1; i >= 0; --i) {
            while(x->forward[i] && cmp_->Compare(x->forward[i]->key, key) < 0)
                x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if(x && cmp_->Compare(x->key, key) == 0)
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

        size_t key_size = sizeof(K);
        size_t value_size = sizeof(V);

        // 针对特殊类型的内存计算
        if constexpr(std::is_same_v<V, std::string> || std::is_same_v<V, std::vector<char>>) {
            value_size = n->value.size();
        }
        size_t node_overhead = sizeof(typename SkipList<K, V>::Node) +
                               sizeof(std::shared_ptr<typename SkipList<K, V>::Node>) * n->forward.size();
        current_mem_size_ += (key_size + value_size + node_overhead);
        return true;
    }

    std::optional<V> find(const K& key) const {
        std::shared_ptr<Node> x = head_;
        for(int i = level_ - 1; i >= 0; --i) {
            while(x->forward[i] && cmp_->Compare(x->forward[i]->key, key) < 0)
                x = x->forward[i];
        }
        x = x->forward[0];
        if(x && cmp_->Compare(x->key, key) == 0)
            return x->value;
        return std::nullopt;
    }

    bool update(const K& key, const V& value) {

        std::shared_ptr<Node> x = head_;
        for(int i = level_ - 1; i >= 0; --i) {
            while(x->forward[i] && cmp_->Compare(x->forward[i]->key, key) < 0)
                x = x->forward[i];
        }
        x = x->forward[0];
        if(x && cmp_->Compare(x->key, key) == 0) {
            x->value = value;
            return true;
        }
        // todo update mem size
        return false;
    }

    bool erase(const K& key) {
        {
            std::vector<std::shared_ptr<Node>> update(maxLevel_, nullptr);
            std::shared_ptr<Node> x = head_;
            for(int i = level_ - 1; i >= 0; --i) {
                while(x->forward[i] && cmp_->Compare(x->forward[i]->key, key) < 0)
                    x = x->forward[i];
                update[i] = x;
            }
            x = x->forward[0];
            if(!x || cmp_->Compare(x->key, key) != 0)
                return false;
            for(int i = 0; i < level_; ++i) {
                if(cmp_->Compare(update[i]->forward[i]->key, key) != 0)
                    break;
                update[i]->forward[i] = x->forward[i];
            }
            // shared_ptr will release ownership when no references remain
            while(level_ > 1 && head_->forward[level_ - 1] == nullptr)
                --level_;
            --size_;

            // decrease skiplist size
            size_t key_size = sizeof(K);
            size_t value_size = sizeof(V);
            if constexpr(std::is_same_v<V, std::string> || std::is_same_v<V, std::vector<char>>) {
                value_size = x->value.size();
            }
            size_t node_overhead = sizeof(typename SkipList<K, V>::Node) +
                                   sizeof(std::shared_ptr<typename SkipList<K, V>::Node>) * x->forward.size();
            current_mem_size_ -= (key_size + value_size + node_overhead);
            return true;
        }
    }

    void clear() {
        // Release all forward pointers; shared_ptr will clean up nodes
        for(int i = 0; i < maxLevel_; ++i)
            head_->forward[i].reset();
        level_ = 1;
        size_ = 0;
    }

    size_t size() const {
        return size_;
    }

    size_t mem_size() const {
        return current_mem_size_;
    }

    // 遍历所有元素
    template <typename F>
    void traverse(F&& callback) const {
        auto current = head_->forward[0];
        while(current) {
            callback(current->key, current->value);
            current = current->forward[0];
        }
    }

    // Export current skiplist to Graphviz DOT format
    std::string to_dot() const {
        {
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
    }

private:
    int randomLevel() {
        int lvl = 1;
        while(dist_(gen_) < p_ && lvl < maxLevel_)
            ++lvl;
        return lvl;
    }

    int maxLevel_;
    double p_;
    int level_;
    std::shared_ptr<Node> head_;
    size_t size_ = 0;
    std::size_t current_mem_size_ = 0;

    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<double> dist_;
    const Comparator<K>* cmp_;
};

} // namespace minilsm