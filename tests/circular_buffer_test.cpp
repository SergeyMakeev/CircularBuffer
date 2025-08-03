#include "circular_buffer/circular_buffer.h"
#include "test_common.h"
#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <numeric>
#include <string>
#include <vector>

using namespace dod;

// Static member definitions for test objects
int TestObject::constructor_calls = 0;
int TestObject::destructor_calls = 0;
int TestObject::copy_calls = 0;
int TestObject::move_calls = 0;

int MoveOnlyObject::constructor_calls = 0;
int MoveOnlyObject::destructor_calls = 0;
int MoveOnlyObject::move_calls = 0;

bool ExceptionObject::throw_on_construction = false;
bool ExceptionObject::throw_on_copy = false;
bool ExceptionObject::throw_on_move = false;
int ExceptionObject::construction_count = 0;

// ============================================================================
// Basic Construction and Destruction Tests
// ============================================================================

class CircularBufferTest
    : public ::testing::Test
    , public CircularBufferTestBase
{
  protected:
    void SetUp() override { CircularBufferTestBase::SetUp(); }

    void TearDown() override { CircularBufferTestBase::TearDown(); }
};

TEST_F(CircularBufferTest, DefaultConstruction)
{
    circular_buffer<int, 10> buffer;

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 10);
}

// Fill constructor test removed since we eliminated the fill constructor

TEST_F(CircularBufferTest, RangeConstruction)
{
    std::vector<int> data = {1, 2, 3, 4, 5};
    circular_buffer<int, 10> buffer(data.begin(), data.end());

    EXPECT_EQ(buffer.size(), 5);
    EXPECT_TRUE(verify_buffer_contents(buffer, data));
}

TEST_F(CircularBufferTest, InitializerListConstruction)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};

    EXPECT_EQ(buffer.size(), 5);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_TRUE(verify_buffer_contents(buffer, expected));
}

TEST_F(CircularBufferTest, CopyConstruction)
{
    circular_buffer<int, 10> original{1, 2, 3, 4, 5};
    circular_buffer<int, 10> copy(original);

    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());

    for (auto i = decltype(original)::size_type{0}; i < original.size(); ++i)
    {
        EXPECT_EQ(copy[i], original[i]);
    }
}

TEST_F(CircularBufferTest, MoveConstruction)
{
    circular_buffer<int, 10> original{1, 2, 3, 4, 5};
    size_t original_size = original.size();

    circular_buffer<int, 10> moved(std::move(original));

    EXPECT_EQ(moved.size(), original_size);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_TRUE(verify_buffer_contents(moved, expected));
}

// ============================================================================
// Assignment Tests
// ============================================================================

TEST_F(CircularBufferTest, CopyAssignment)
{
    circular_buffer<int, 10> original{1, 2, 3, 4, 5};
    circular_buffer<int, 10> copy;

    copy = original;

    EXPECT_EQ(copy.size(), original.size());
    for (auto i = decltype(original)::size_type{0}; i < original.size(); ++i)
    {
        EXPECT_EQ(copy[i], original[i]);
    }
}

TEST_F(CircularBufferTest, MoveAssignment)
{
    circular_buffer<int, 10> original{1, 2, 3, 4, 5};
    circular_buffer<int, 10> moved;
    size_t original_size = original.size();

    moved = std::move(original);

    EXPECT_EQ(moved.size(), original_size);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_TRUE(verify_buffer_contents(moved, expected));
}

TEST_F(CircularBufferTest, SelfAssignment)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    buffer = buffer; // Self-assignment
#ifdef __clang__
#pragma clang diagnostic pop
#endif

    EXPECT_EQ(buffer.size(), 5);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_TRUE(verify_buffer_contents(buffer, expected));
}

// ============================================================================
// Capacity and Size Tests
// ============================================================================

TEST_F(CircularBufferTest, CapacityQueries)
{
    circular_buffer<int, 42> buffer;

    EXPECT_EQ(buffer.capacity(), 42);
    EXPECT_EQ(decltype(buffer)::capacity(), 42);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0);
}

TEST_F(CircularBufferTest, SizeProgression)
{
    circular_buffer<int, 5> buffer;

    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(buffer.size(), i);
        EXPECT_EQ(buffer.empty(), i == 0);
        EXPECT_EQ(buffer.full(), i == 5);

        buffer.push_back(i);
    }

    EXPECT_EQ(buffer.size(), 5);
    EXPECT_FALSE(buffer.empty());
    EXPECT_TRUE(buffer.full());
}

// ============================================================================
// Element Access Tests
// ============================================================================

TEST_F(CircularBufferTest, IndexAccess)
{
    circular_buffer<int, 10> buffer{10, 20, 30, 40, 50};

    EXPECT_EQ(buffer[0], 10);
    EXPECT_EQ(buffer[1], 20);
    EXPECT_EQ(buffer[2], 30);
    EXPECT_EQ(buffer[3], 40);
    EXPECT_EQ(buffer[4], 50);

    // Test const access
    const auto& const_buffer = buffer;
    EXPECT_EQ(const_buffer[0], 10);
    EXPECT_EQ(const_buffer[4], 50);
}

TEST_F(CircularBufferTest, AtAccessWithBoundsChecking)
{
    circular_buffer<int, 5> buffer{1, 2, 3};

    EXPECT_EQ(buffer.at(0), 1);
    EXPECT_EQ(buffer.at(1), 2);
    EXPECT_EQ(buffer.at(2), 3);

    EXPECT_THROW(buffer.at(3), std::out_of_range);
    EXPECT_THROW(buffer.at(10), std::out_of_range);
}

TEST_F(CircularBufferTest, FrontBackAccess)
{
    circular_buffer<int, 10> buffer{100, 200, 300};

    EXPECT_EQ(buffer.front(), 100);
    EXPECT_EQ(buffer.back(), 300);

    // Test const access
    const auto& const_buffer = buffer;
    EXPECT_EQ(const_buffer.front(), 100);
    EXPECT_EQ(const_buffer.back(), 300);

    // Test modification
    buffer.front() = 999;
    buffer.back() = 888;

    EXPECT_EQ(buffer[0], 999);
    EXPECT_EQ(buffer[2], 888);
}

// ============================================================================
// Push Operations Tests
// ============================================================================

TEST_F(CircularBufferTest, PushBackBasic)
{
    circular_buffer<int, 5> buffer;

    auto result1 = buffer.push_back(1);
    EXPECT_EQ(result1, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.back(), 1);

    auto result2 = buffer.push_back(2);
    EXPECT_EQ(result2, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.back(), 2);
}

TEST_F(CircularBufferTest, PushBackMove)
{
    circular_buffer<TestObject, 5> buffer;

    TestObject obj(42);
    TestObject::reset_counters();

    auto result = buffer.push_back(std::move(obj));

    EXPECT_EQ(result, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.back().value, 42);
    EXPECT_GT(TestObject::move_calls, 0); // Should have used move
}

TEST_F(CircularBufferTest, EmplaceBack)
{
    circular_buffer<TestObject, 5> buffer;
    TestObject::reset_counters();

    auto result = buffer.emplace_back(123);

    EXPECT_EQ(result, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.back().value, 123);
    EXPECT_EQ(TestObject::constructor_calls, 1); // Only one construction
}

TEST_F(CircularBufferTest, PushFrontBasic)
{
    circular_buffer<int, 5> buffer;

    auto result1 = buffer.push_front(1);
    EXPECT_EQ(result1, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.front(), 1);

    auto result2 = buffer.push_front(2);
    EXPECT_EQ(result2, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 2);
    EXPECT_EQ(buffer.back(), 1);
}

TEST_F(CircularBufferTest, UncheckedPushOperations)
{
    circular_buffer<int, 3> buffer;

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);

    EXPECT_EQ(buffer.size(), 3);
    EXPECT_TRUE(buffer.full());

    // This should overwrite (no return value to check)
    buffer.push_back(4);

    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 2); // First element overwritten
    EXPECT_EQ(buffer.back(), 4);
}

// ============================================================================
// Overflow Policy Tests
// ============================================================================

TEST_F(CircularBufferTest, OverwritePolicyBehavior)
{
    circular_buffer<int, 3, overflow_policy::overwrite, uint32_t, 64, alignof(int)> buffer;

    // Fill the buffer
    EXPECT_EQ(buffer.push_back(1), insert_result::inserted);
    EXPECT_EQ(buffer.push_back(2), insert_result::inserted);
    EXPECT_EQ(buffer.push_back(3), insert_result::inserted);
    EXPECT_TRUE(buffer.full());

    // Next push should overwrite
    EXPECT_EQ(buffer.push_back(4), insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer[0], 2); // Original first element (1) was overwritten
    EXPECT_EQ(buffer[1], 3);
    EXPECT_EQ(buffer[2], 4);
}

TEST_F(CircularBufferTest, DiscardPolicyBehavior)
{
    circular_buffer<int, 3, overflow_policy::discard, uint32_t, 64, alignof(int)> buffer;

    // Fill the buffer
    EXPECT_EQ(buffer.push_back(1), insert_result::inserted);
    EXPECT_EQ(buffer.push_back(2), insert_result::inserted);
    EXPECT_EQ(buffer.push_back(3), insert_result::inserted);
    EXPECT_TRUE(buffer.full());

    // Next push should be discarded
    EXPECT_EQ(buffer.push_back(4), insert_result::discarded);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer[0], 1); // Original elements unchanged
    EXPECT_EQ(buffer[1], 2);
    EXPECT_EQ(buffer[2], 3);
}

// ============================================================================
// Pop Operations Tests
// ============================================================================

TEST_F(CircularBufferTest, DropBack)
{
    circular_buffer<int, 5> buffer{1, 2, 3, 4, 5};

    buffer.drop_back();
    EXPECT_EQ(buffer.size(), 4);
    EXPECT_EQ(buffer.back(), 4);

    buffer.drop_back();
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.back(), 3);
}

TEST_F(CircularBufferTest, DropFront)
{
    circular_buffer<int, 5> buffer{1, 2, 3, 4, 5};

    buffer.drop_front();
    EXPECT_EQ(buffer.size(), 4);
    EXPECT_EQ(buffer.front(), 2);

    buffer.drop_front();
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 3);
}

TEST_F(CircularBufferTest, TakeBack)
{
    circular_buffer<int, 5> buffer{1, 2, 3};

    auto result1 = buffer.take_back();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), 3);
    EXPECT_EQ(buffer.size(), 2);

    auto result2 = buffer.take_back();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), 2);
    EXPECT_EQ(buffer.size(), 1);

    auto result3 = buffer.take_back();
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), 1);
    EXPECT_EQ(buffer.size(), 0);

    // Should return nullopt when empty
    auto result4 = buffer.take_back();
    EXPECT_FALSE(result4.has_value());
}

TEST_F(CircularBufferTest, TakeFront)
{
    circular_buffer<int, 5> buffer{1, 2, 3};

    auto result1 = buffer.take_front();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), 1);
    EXPECT_EQ(buffer.size(), 2);

    buffer.clear();

    // Should return nullopt when empty
    auto result2 = buffer.take_front();
    EXPECT_FALSE(result2.has_value());
}

// ============================================================================
// Clear and RAII Tests
// ============================================================================

TEST_F(CircularBufferTest, Clear)
{
    circular_buffer<TestObject, 5> buffer;

    for (int i = 0; i < 3; ++i)
    {
        buffer.emplace_back(i);
    }

    EXPECT_EQ(buffer.size(), 3);

    TestObject::reset_counters();
    buffer.clear();

    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(TestObject::destructor_calls, 3); // All objects destroyed
}

TEST_F(CircularBufferTest, RAIIDestruction)
{
    TestObject::reset_counters();

    {
        circular_buffer<TestObject, 5> buffer;
        buffer.emplace_back(1);
        buffer.emplace_back(2);
        buffer.emplace_back(3);

        EXPECT_EQ(TestObject::constructor_calls, 3);
    } // Buffer goes out of scope

    EXPECT_EQ(TestObject::destructor_calls, 3); // All objects destroyed
}

// ============================================================================
// Index Type Tests
// ============================================================================

TEST_F(CircularBufferTest, DifferentIndexTypes)
{
    // Test with uint8_t indices
    circular_buffer<int, 10, overflow_policy::overwrite, uint8_t> buffer8;
    EXPECT_EQ(buffer8.capacity(), 10);

    for (int i = 0; i < 10; ++i)
    {
        buffer8.push_back(i);
    }
    EXPECT_EQ(buffer8.size(), 10);

    // Test with uint16_t indices
    circular_buffer<int, 1000, overflow_policy::overwrite, uint16_t> buffer16;
    EXPECT_EQ(buffer16.capacity(), 1000);

    // Test with uint64_t indices
    circular_buffer<int, 100, overflow_policy::overwrite, uint64_t> buffer64;
    EXPECT_EQ(buffer64.capacity(), 100);
}

// ============================================================================
// Storage Strategy Tests
// ============================================================================

TEST_F(CircularBufferTest, EmbeddedStorageDetection)
{
    // Small buffer should use embedded storage
    circular_buffer<int, 32, overflow_policy::overwrite, uint32_t, 64, alignof(int)> small_buffer;
    EXPECT_TRUE(small_buffer.has_inline_storage());

    // Large buffer should use heap storage
    circular_buffer<int, 128, overflow_policy::overwrite, uint32_t, 64, alignof(int)> large_buffer;
    EXPECT_FALSE(large_buffer.has_inline_storage());
}

TEST_F(CircularBufferTest, CustomEmbeddedThreshold)
{
    // Force heap allocation with low threshold
    circular_buffer<int, 32, overflow_policy::overwrite, uint32_t, 16, alignof(int)> heap_buffer;
    EXPECT_FALSE(heap_buffer.has_inline_storage());

    // Force embedded storage with high threshold
    circular_buffer<int, 128, overflow_policy::overwrite, uint32_t, 256, alignof(int)> embedded_buffer;
    EXPECT_TRUE(embedded_buffer.has_inline_storage());
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_F(CircularBufferTest, BasicIteratorOperations)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};

    // Test begin/end
    EXPECT_EQ(std::distance(buffer.begin(), buffer.end()), 5);

    // Test iterator dereferencing
    auto it = buffer.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);

    // Test const iterators
    const auto& const_buffer = buffer;
    auto const_it = const_buffer.begin();
    EXPECT_EQ(*const_it, 1);

    // Test cbegin/cend
    auto cit = buffer.cbegin();
    EXPECT_EQ(*cit, 1);
}

TEST_F(CircularBufferTest, IteratorArithmetic)
{
    circular_buffer<int, 10> buffer{10, 20, 30, 40, 50};

    auto it = buffer.begin();

    // Test increment/decrement
    EXPECT_EQ(*(it++), 10);
    EXPECT_EQ(*it, 20);
    EXPECT_EQ(*(++it), 30);
    EXPECT_EQ(*(it--), 30);
    EXPECT_EQ(*it, 20);
    EXPECT_EQ(*(--it), 10);

    // Test addition/subtraction
    it = buffer.begin();
    EXPECT_EQ(*(it + 2), 30);
    EXPECT_EQ(*(it + 4), 50);

    it = buffer.end();
    EXPECT_EQ(*(it - 1), 50);
    EXPECT_EQ(*(it - 3), 30);

    // Test difference
    EXPECT_EQ(buffer.end() - buffer.begin(), 5);
}

TEST_F(CircularBufferTest, IteratorComparison)
{
    circular_buffer<int, 5> buffer{1, 2, 3, 4, 5};

    auto it1 = buffer.begin();
    auto it2 = buffer.begin() + 2;
    auto it3 = buffer.end();

    EXPECT_TRUE(it1 == buffer.begin());
    EXPECT_TRUE(it1 != it2);
    EXPECT_TRUE(it1 < it2);
    EXPECT_TRUE(it2 < it3);
    EXPECT_TRUE(it3 > it1);
    EXPECT_TRUE(it1 <= it2);
    EXPECT_TRUE(it2 >= it1);
}

TEST_F(CircularBufferTest, RangeBasedForLoop)
{
    circular_buffer<int, 5> buffer{1, 2, 3, 4, 5};

    std::vector<int> collected;
    for (const auto& value : buffer)
    {
        collected.push_back(value);
    }

    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(collected, expected);
}

TEST_F(CircularBufferTest, STLAlgorithms)
{
    circular_buffer<int, 10> buffer{5, 3, 8, 1, 9, 2, 7, 4, 6};

    // Test std::sort
    std::sort(buffer.begin(), buffer.end());

    for (auto i = decltype(buffer)::size_type{1}; i < buffer.size(); ++i)
    {
        EXPECT_LE(buffer[i - 1], buffer[i]);
    }

    // Test std::find
    auto it = std::find(buffer.begin(), buffer.end(), 5);
    EXPECT_NE(it, buffer.end());
    EXPECT_EQ(*it, 5);

    // Test std::accumulate
    int sum = std::accumulate(buffer.begin(), buffer.end(), 0);
    EXPECT_EQ(sum, 45); // Sum of 1-9
}

// Bulk operations tests removed since we eliminated bulk operations

// ============================================================================
// Wraparound Behavior Tests
// ============================================================================

TEST_F(CircularBufferTest, IndexWraparound)
{
    circular_buffer<int, 4> buffer;

    // Fill buffer
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    buffer.push_back(4);

    // Remove from front and add to back - should wrap around
    buffer.drop_front();
    buffer.push_back(5);

    std::vector<int> expected = {2, 3, 4, 5};
    EXPECT_TRUE(verify_buffer_contents(buffer, expected));

    // Continue wrapping
    buffer.drop_front();
    buffer.push_back(6);

    expected = {3, 4, 5, 6};
    EXPECT_TRUE(verify_buffer_contents(buffer, expected));
}

TEST_F(CircularBufferTest, FullCapacityUtilization)
{
    // Verify we can store exactly Capacity elements
    constexpr size_t test_capacity = 32;
    circular_buffer<int, test_capacity> buffer;

    // Fill to capacity
    for (size_t i = 0; i < test_capacity; ++i)
    {
        auto result = buffer.push_back(static_cast<int>(i));
        EXPECT_EQ(result, insert_result::inserted);
    }

    EXPECT_EQ(buffer.size(), test_capacity);
    EXPECT_TRUE(buffer.full());

    // Verify all elements are accessible
    for (auto i = decltype(buffer)::size_type{0}; i < test_capacity; ++i)
    {
        EXPECT_EQ(buffer[i], static_cast<int>(i));
    }
}

// ============================================================================
// Edge Cases and Error Conditions
// ============================================================================

TEST_F(CircularBufferTest, EmptyBufferBehavior)
{
    circular_buffer<int, 5> buffer;

    // Safe operations on empty buffer
    EXPECT_FALSE(buffer.take_back().has_value());
    EXPECT_FALSE(buffer.take_front().has_value());

    // Iterator behavior on empty buffer
    EXPECT_EQ(buffer.begin(), buffer.end());
    EXPECT_EQ(std::distance(buffer.begin(), buffer.end()), 0);
}

TEST_F(CircularBufferTest, SingleElementCapacity)
{
    circular_buffer<int, 1> buffer;

    auto result1 = buffer.push_back(42);
    EXPECT_EQ(result1, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.front(), 42);
    EXPECT_EQ(buffer.back(), 42);

    // Should overwrite the single element
    auto result2 = buffer.push_back(99);
    EXPECT_EQ(result2, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.front(), 99);
}

// ============================================================================
// Template Parameter Edge Cases
// ============================================================================

TEST_F(CircularBufferTest, PowerOfTwoCapacity)
{
    // Test power-of-2 capacity optimization
    circular_buffer<int, 16> buffer; // 16 is power of 2

    for (int i = 0; i < 20; ++i)
    {
        buffer.push_back(i);
    }

    // Should have wrapped around due to overwrite policy
    EXPECT_EQ(buffer.size(), 16);
    EXPECT_TRUE(buffer.full());
}

TEST_F(CircularBufferTest, NonPowerOfTwoCapacity)
{
    // Test non-power-of-2 capacity
    circular_buffer<int, 15> buffer; // 15 is not power of 2

    for (int i = 0; i < 20; ++i)
    {
        buffer.push_back(i);
    }

    EXPECT_EQ(buffer.size(), 15);
    EXPECT_TRUE(buffer.full());
}

// ============================================================================
// Move Semantics and Performance Tests
// ============================================================================

TEST_F(CircularBufferTest, MoveOnlyTypes)
{
    circular_buffer<MoveOnlyObject, 5> buffer;

    MoveOnlyObject::reset_counters();

    buffer.emplace_back(1);
    buffer.emplace_back(2);

    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer[0].value, 1);
    EXPECT_EQ(buffer[1].value, 2);
    EXPECT_EQ(MoveOnlyObject::constructor_calls, 2);
}

TEST_F(CircularBufferTest, PerfectForwarding)
{
    circular_buffer<TestObject, 5> buffer;
    TestObject::reset_counters();

    // Test emplace with multiple arguments (if TestObject supported it)
    buffer.emplace_back(42);

    EXPECT_EQ(TestObject::constructor_calls, 1);
    EXPECT_EQ(TestObject::copy_calls, 0); // Should not copy
    EXPECT_EQ(TestObject::move_calls, 0); // Should construct in place
}

// ============================================================================
// Type Compatibility Tests
// ============================================================================

TEST_F(CircularBufferTest, StringType)
{
    circular_buffer<std::string, 5> buffer;

    buffer.push_back("hello");
    buffer.push_back("world");
    buffer.emplace_back("test");

    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer[0], "hello");
    EXPECT_EQ(buffer[1], "world");
    EXPECT_EQ(buffer[2], "test");
}

TEST_F(CircularBufferTest, VectorType)
{
    circular_buffer<std::vector<int>, 3> buffer;

    buffer.push_back({1, 2, 3});
    buffer.emplace_back(5, 42); // Vector with 5 elements of value 42

    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer[0], std::vector<int>({1, 2, 3}));
    EXPECT_EQ(buffer[1].size(), 5);
    EXPECT_EQ(buffer[1][0], 42);
}

// ============================================================================
// Coverage Tests - Additional tests to achieve 100% coverage
// ============================================================================

TEST_F(CircularBufferTest, PushFrontOverwriteBehavior)
{
    circular_buffer<int, 3> buffer;

    // Fill the buffer
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    EXPECT_TRUE(buffer.full());

    // Push front should overwrite - this tests the overwrite logic in push_front
    auto result = buffer.push_front(99);
    EXPECT_EQ(result, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 99);
    EXPECT_EQ(buffer[1], 1);
    EXPECT_EQ(buffer[2], 2);
}

TEST_F(CircularBufferTest, EmplaceFrontOperations)
{
    circular_buffer<TestObject, 5> buffer;
    TestObject::reset_counters();

    // Test emplace_front
    auto result = buffer.emplace_front(42);
    EXPECT_EQ(result, insert_result::inserted);
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.front().value, 42);
    EXPECT_EQ(TestObject::constructor_calls, 1); // Should construct in place

    // Test emplace_front when buffer becomes full
    buffer.emplace_front(1);
    buffer.emplace_front(2);
    buffer.emplace_front(3);
    buffer.emplace_front(4);
    EXPECT_TRUE(buffer.full());

    // Test emplace_front overwrite behavior
    auto overwrite_result = buffer.emplace_front(99);
    EXPECT_EQ(overwrite_result, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 5);
    EXPECT_EQ(buffer.front().value, 99);
}

TEST_F(CircularBufferTest, ConstAtMethod)
{
    circular_buffer<int, 5> buffer{10, 20, 30};
    const auto& const_buffer = buffer;

    // Test const at() method
    EXPECT_EQ(const_buffer.at(0), 10);
    EXPECT_EQ(const_buffer.at(1), 20);
    EXPECT_EQ(const_buffer.at(2), 30);

    // Test const at() with out of range
    EXPECT_THROW(const_buffer.at(3), std::out_of_range);
    EXPECT_THROW(const_buffer.at(10), std::out_of_range);
}

TEST_F(CircularBufferTest, DefaultOverwritePolicyBehavior)
{
    // Test default overwrite policy behavior
    circular_buffer<int, 3> buffer; // Uses default overwrite policy

    // Fill the buffer
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    EXPECT_TRUE(buffer.full());

    // Test overwrite behavior with default policy
    auto result = buffer.push_back(4);
    EXPECT_EQ(result, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 3);

    // Also test emplace_back to ensure coverage
    auto result2 = buffer.emplace_back(5);
    EXPECT_EQ(result2, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 3);
}

TEST_F(CircularBufferTest, HeapStorageOverwrite)
{
    // Test overwrite with heap storage to ensure coverage of all paths
    circular_buffer<int, 100> buffer; // Large enough to force heap storage
    EXPECT_FALSE(buffer.has_inline_storage());

    // Fill the buffer
    for (int i = 0; i < 100; ++i)
    {
        buffer.push_back(i);
    }
    EXPECT_TRUE(buffer.full());

    // Test overwrite with heap storage
    auto result = buffer.push_back(999);
    EXPECT_EQ(result, insert_result::overwritten);
    EXPECT_EQ(buffer.size(), 100);
    EXPECT_EQ(buffer.back(), 999);
}

TEST_F(CircularBufferTest, MoveConstructorHeapStorage)
{
    // Test move constructor with heap storage
    circular_buffer<int, 100> original; // Large enough to force heap storage
    for (int i = 0; i < 50; ++i)
    {
        original.push_back(i);
    }

    circular_buffer<int, 100> moved(std::move(original));
    EXPECT_EQ(moved.size(), 50);
    EXPECT_EQ(original.size(), 0); // Original should be empty after move

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_EQ(moved[i], i);
    }
}

TEST_F(CircularBufferTest, MoveAssignmentHeapStorage)
{
    // Test move assignment with heap storage
    circular_buffer<int, 100> original;
    for (int i = 0; i < 50; ++i)
    {
        original.push_back(i);
    }

    circular_buffer<int, 100> moved;
    moved = std::move(original);

    EXPECT_EQ(moved.size(), 50);
    EXPECT_EQ(original.size(), 0); // Original should be empty after move

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_EQ(moved[i], i);
    }
}

TEST_F(CircularBufferTest, IteratorEdgeCases)
{
    circular_buffer<int, 5> buffer{1, 2, 3};

    // Test iterator conversion from non-const to const
    auto it = buffer.begin();
    circular_buffer<int, 5>::const_iterator const_it = it;
    EXPECT_EQ(*const_it, 1);

    // Test iterator arithmetic edge cases
    auto it2 = buffer.cend();
    EXPECT_EQ(*(it2 - 1), 3);

    // Test operator->
    auto it3 = buffer.begin();
    EXPECT_EQ(it3.operator->(), &buffer[0]);
}

TEST_F(CircularBufferTest, DiscardPolicyWithEmplaceFront)
{
    circular_buffer<int, 3, overflow_policy::discard> buffer;

    // Fill buffer
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    EXPECT_TRUE(buffer.full());

    // Test emplace_front with discard policy
    auto result = buffer.emplace_front(99);
    EXPECT_EQ(result, insert_result::discarded);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 1); // Should be unchanged
}