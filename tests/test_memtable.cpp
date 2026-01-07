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
        options.max_mem_size = 1024; // 1KB
        mem_table_ = std::make_unique<MemTable<int, std::string>>(options);
    }

    void TearDown() override {
        // 清理WAL文件
        std::remove("./test_wal.log");
    }

    std::unique_ptr<MemTable<int, std::string>> mem_table_;
};

// 测试基本的CRUD功能
TEST_F(MemTableTest, BasicCRUDOperations) {
    // Test put operation
    EXPECT_TRUE(mem_table_->put(1, "value1"));
    EXPECT_TRUE(mem_table_->put(2, "value2"));
    EXPECT_TRUE(mem_table_->put(3, "value3"));
    EXPECT_EQ(mem_table_->size(), 3u);

    // Test get operation
    auto value1 = mem_table_->get(1);
    ASSERT_TRUE(value1.has_value());
    EXPECT_EQ(value1.value(), "value1");

    auto value2 = mem_table_->get(2);
    ASSERT_TRUE(value2.has_value());
    EXPECT_EQ(value2.value(), "value2");

    auto missing = mem_table_->get(999);
    EXPECT_FALSE(missing.has_value());

    // Test remove operation
    EXPECT_TRUE(mem_table_->remove(2));
    EXPECT_EQ(mem_table_->size(), 2u);
    EXPECT_FALSE(mem_table_->get(2).has_value());

    // Test remove non-existing key
    EXPECT_FALSE(mem_table_->remove(999));
}

// 测试内存限制功能
TEST_F(MemTableTest, MemoryLimit) {
    
}

// 测试Immutable状态
TEST_F(MemTableTest, ImmutableState) {
    mem_table_->put(1, "value1");
    EXPECT_FALSE(mem_table_->is_immutable());

    // 转换为不可变状态
    mem_table_->make_immutable();
    EXPECT_TRUE(mem_table_->is_immutable());

    // 在不可变状态下，修改操作应该失败
    EXPECT_FALSE(mem_table_->put(2, "value2"));
    EXPECT_FALSE(mem_table_->remove(1));

    // 读取操作仍然应该正常工作
    auto value = mem_table_->get(1);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "value1");
}

// 测试遍历功能
TEST_F(MemTableTest, TraverseFunctionality) {
    mem_table_->put(1, "value1");
    mem_table_->put(2, "value2");
    mem_table_->put(3, "value3");

    std::vector<std::pair<int, std::string>> results;
    mem_table_->traverse([&results](const int& key, const std::string& value) { results.emplace_back(key, value); });

    // 验证遍历结果
    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].first, 1);
    EXPECT_EQ(results[0].second, "value1");
    EXPECT_EQ(results[1].first, 2);
    EXPECT_EQ(results[1].second, "value2");
    EXPECT_EQ(results[2].first, 3);
    EXPECT_EQ(results[2].second, "value3");
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
                mem_table_->put(key, "value" + std::to_string(key));
            }
        }));
    }

    // 等待所有写入完成
    for(auto& future : futures) {
        future.wait();
    }

    // 验证写入结果
    EXPECT_EQ(mem_table_->size(), num_threads * operations_per_thread);

    // 并发读取
    futures.clear();
    std::atomic<int> read_count = 0;

    for(int i = 0; i < num_threads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [this, &read_count, num_threads, operations_per_thread]() {
            for(int j = 0; j < operations_per_thread; ++j) {
                int key = j;
                if(mem_table_->get(key).has_value()) {
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
    EXPECT_EQ(read_count, operations_per_thread);
}

// 测试清空功能
TEST_F(MemTableTest, ClearFunctionality) {
    mem_table_->put(1, "value1");
    mem_table_->put(2, "value2");
    mem_table_->put(3, "value3");
    EXPECT_EQ(mem_table_->size(), 3u);

    mem_table_->clear();
    EXPECT_EQ(mem_table_->size(), 0u);

    // 验证所有数据都已被清除
    EXPECT_FALSE(mem_table_->get(1).has_value());
    EXPECT_FALSE(mem_table_->get(2).has_value());
    EXPECT_FALSE(mem_table_->get(3).has_value());
}
