#include "minilsm/memtable.h"
#include "gtest/gtest.h"
#include <fstream>
#include <future>
#include <thread>
#include <vector>

using namespace minilsm;

class MemTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建一个较小的MemTable用于测试内存限制
        MemTableOptions options;
        options.table_max_mem_size = 1024; // 1KB
        mem_table_ = std::make_unique<MemTable>(options);
    }

    void TearDown() override {
        // 清理WAL文件
        std::remove("./test_wal.log");
    }

    std::unique_ptr<MemTable> mem_table_;
};

// 测试基本的CRUD功能
TEST_F(MemTableTest, BasicCRUDOperations) {
    // Test put operation
    EXPECT_TRUE(mem_table_->put(Slice("1"), Slice("value1")));
    EXPECT_TRUE(mem_table_->put(Slice("2"), Slice("value2")));
    EXPECT_TRUE(mem_table_->put(Slice("3"), Slice("value3")));

    // Test get operation
    auto value1 = mem_table_->get(Slice("1"));
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ(value1.value(), Slice("value1"));

    auto value2 = mem_table_->get(Slice("2"));
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(value2.value(), Slice("value2"));

    auto missing = mem_table_->get(Slice("999"));
    EXPECT_FALSE(missing.has_value());

    // Test remove operation
    EXPECT_TRUE(mem_table_->remove(Slice("2")));
    EXPECT_FALSE(mem_table_->get(Slice("2")).has_value());
}

// 测试遍历功能
TEST_F(MemTableTest, TraverseFunctionality) {
    EXPECT_TRUE(mem_table_->put(Slice("1"), Slice("value1")));
    EXPECT_TRUE(mem_table_->put(Slice("2"), Slice("value2")));
    EXPECT_TRUE(mem_table_->put(Slice("3"), Slice("value3")));

    std::vector<std::pair<Slice, Slice>> results;
    mem_table_->traverse([&results](const Slice& key, const Slice& value) { results.emplace_back(key, value); });

    // 验证遍历结果
    EXPECT_EQ(results[0].first, Slice("1"));
    EXPECT_EQ(results[0].second, Slice("value1"));
    EXPECT_EQ(results[1].first, Slice("2"));
    EXPECT_EQ(results[1].second, Slice("value2"));
    EXPECT_EQ(results[2].first, Slice("3"));
    EXPECT_EQ(results[2].second, Slice("value3"));
}

// 测试并发操作
TEST_F(MemTableTest, ConcurrentOperations) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    std::vector<std::future<void>> futures;

    // 并发写入
    for(int i = 0; i < num_threads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [this, i, operations_per_thread]() {
            for(int j = 0; j < operations_per_thread; ++j) {
                int key = i * operations_per_thread + j;
                mem_table_->put(Slice(std::to_string(key)), Slice("value" + std::to_string(key)));
            }
        }));
    }

    // 等待所有写入完成
    for(auto& future : futures) {
        future.wait();
    }

    // 并发读取
    futures.clear();
    std::atomic<int> read_count = 0;

    for(int i = 0; i < num_threads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [this, &read_count, num_threads, operations_per_thread]() {
            for(int j = 0; j < operations_per_thread; ++j) {
                int key = j;
                if(mem_table_->get(Slice(std::to_string(key))).has_value()) {
                    read_count++;
                }
            }
        }));
    }

    // 等待所有读取完成
    for(auto& future : futures) {
        future.wait();
    }

    // 验证读取结果
    EXPECT_EQ(read_count, num_threads * operations_per_thread);
}

// 测试清空功能
TEST_F(MemTableTest, ClearFunctionality) {
    EXPECT_TRUE(mem_table_->put(Slice("1"), Slice("value1")));
    EXPECT_TRUE(mem_table_->put(Slice("2"), Slice("value2")));
    EXPECT_TRUE(mem_table_->put(Slice("3"), Slice("value3")));

    mem_table_->clear();

    // 验证所有数据都已被清除
    EXPECT_FALSE(mem_table_->get(Slice("1")).has_value());
    EXPECT_FALSE(mem_table_->get(Slice("2")).has_value());
    EXPECT_FALSE(mem_table_->get(Slice("3")).has_value());
}
