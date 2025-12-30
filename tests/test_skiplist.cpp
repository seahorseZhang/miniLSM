#include "gtest/gtest.h"
#include "minilsm/skiplist.h"

using namespace minilsm;

TEST(SkipListCreate, Insert) {
	SkipList<int,int> sl;
	EXPECT_EQ(sl.size(), 0u);
	EXPECT_TRUE(sl.insert(1, 10));
	EXPECT_TRUE(sl.insert(2, 20));
	// duplicate insert should fail
	EXPECT_FALSE(sl.insert(1, 100));
	EXPECT_EQ(sl.size(), 2u);
}

TEST(SkipListRead, Find) {
	SkipList<int,int> sl;
	sl.insert(5, 50);
	sl.insert(10, 100);

	auto v1 = sl.find(5);
	ASSERT_TRUE(v1.has_value());
	EXPECT_EQ(v1.value(), 50);

	auto v2 = sl.find(10);
	ASSERT_TRUE(v2.has_value());
	EXPECT_EQ(v2.value(), 100);

	auto vn = sl.find(999);
	EXPECT_FALSE(vn.has_value());
}

TEST(SkipListUpdate, Update) {
	SkipList<int,int> sl;
	sl.insert(3, 30);
	EXPECT_TRUE(sl.update(3, 300));
	auto v = sl.find(3);
	ASSERT_TRUE(v.has_value());
	EXPECT_EQ(v.value(), 300);

	// updating non-existing key should return false
	EXPECT_FALSE(sl.update(99, 9900));
}

TEST(SkipListDelete, Erase) {
	SkipList<int,int> sl;
	for (int i = 0; i < 5; ++i) sl.insert(i, i*10);
	EXPECT_EQ(sl.size(), 5u);

	EXPECT_TRUE(sl.erase(2));
	EXPECT_EQ(sl.size(), 4u);
	EXPECT_FALSE(sl.find(2).has_value());

	// erasing again should fail
	EXPECT_FALSE(sl.erase(2));
}

TEST(SkipListCombined, FullWorkflow) {
	SkipList<int,int> sl;

	// Create
	for (int i = 0; i < 20; ++i) EXPECT_TRUE(sl.insert(i, i*11));
	EXPECT_EQ(sl.size(), 20u);

	// Read
	for (int i = 0; i < 20; ++i) {
		auto v = sl.find(i);
		ASSERT_TRUE(v.has_value());
		EXPECT_EQ(v.value(), i*11);
	}

	// Update some
	for (int i = 0; i < 20; i += 3) EXPECT_TRUE(sl.update(i, i*111));

	for (int i = 0; i < 20; ++i) {
		auto v = sl.find(i);
		ASSERT_TRUE(v.has_value());
		if (i % 3 == 0) EXPECT_EQ(v.value(), i*111);
		else EXPECT_EQ(v.value(), i*11);
	}

	// Delete half
	for (int i = 0; i < 10; ++i) EXPECT_TRUE(sl.erase(i));
	EXPECT_EQ(sl.size(), 10u);
}

TEST(SkipListBoundaryEmpty, BoundaryAndEmpty) {
	SkipList<int,int> sl;
	// empty find/erase
	EXPECT_FALSE(sl.find(0).has_value());
	EXPECT_FALSE(sl.erase(0));

	// boundary values
	int i_min = std::numeric_limits<int>::min();
	int i_max = std::numeric_limits<int>::max();
	EXPECT_TRUE(sl.insert(i_min, -1));
	EXPECT_TRUE(sl.insert(i_max, 1));
	auto vmin = sl.find(i_min);
	auto vmax = sl.find(i_max);
	ASSERT_TRUE(vmin.has_value()); EXPECT_EQ(vmin.value(), -1);
	ASSERT_TRUE(vmax.has_value()); EXPECT_EQ(vmax.value(), 1);
}

TEST(SkipListTypes, StringAndBool) {
	SkipList<std::string,std::string> sls;
	EXPECT_TRUE(sls.insert("a","alpha"));
	EXPECT_TRUE(sls.insert("b","beta"));
	auto sa = sls.find("a");
	ASSERT_TRUE(sa.has_value()); EXPECT_EQ(sa.value(), "alpha");

	SkipList<int,bool> slb;
	EXPECT_TRUE(slb.insert(1, true));
	EXPECT_TRUE(slb.insert(2, false));
	auto b1 = slb.find(1);
	ASSERT_TRUE(b1.has_value()); EXPECT_TRUE(b1.value());
}

TEST(SkipListRandomized, RandomizedOpsConsistencyWithMap) {
	SkipList<int,int> sl;
	std::map<int,int> ref;
	std::mt19937_64 rng(123456);
	std::uniform_int_distribution<int> keyd(0, 200);
	for (int step = 0; step < 2000; ++step) {
		int op = step % 4; // cycle through ops to ensure coverage
		int k = keyd(rng);
		if (op == 0) { // insert
			int v = k * 7;
			bool ok = sl.insert(k, v);
			if (ok) ref.emplace(k,v);
		} else if (op == 1) { // update
			if (!ref.empty()) {
				int v = k * 11;
				bool ok = sl.update(k, v);
				if (ok) ref[k] = v;
			}
		} else if (op == 2) { // erase
			bool ok = sl.erase(k);
			if (ok) ref.erase(k);
		} else { // find
			auto rv = sl.find(k);
			auto it = ref.find(k);
			if (it == ref.end()) EXPECT_FALSE(rv.has_value());
			else { ASSERT_TRUE(rv.has_value()); EXPECT_EQ(rv.value(), it->second); }
		}
	}
	// final consistency check
	// iterate reference map and verify skiplist
	for (auto &p : ref) {
		auto rv = sl.find(p.first);
		ASSERT_TRUE(rv.has_value());
		EXPECT_EQ(rv.value(), p.second);
	}
}
