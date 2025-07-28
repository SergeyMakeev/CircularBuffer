#include "circular_buffer/circular_buffer.h"
#include "test_common.h"
#include <gtest/gtest.h>
#include <vector>
#include <numeric>
#include <algorithm>

using namespace dod;

/*

**Iterator debugging tests**

These tests verify that iterator debugging features work correctly
when CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL is enabled.

*/

class IteratorDebugTest : public ::testing::Test, public CircularBufferTestBase
{
protected:
    void SetUp() override
    {
        CircularBufferTestBase::SetUp();
    }

    void TearDown() override
    {
        CircularBufferTestBase::TearDown();
    }
};

// Test basic iterator validity
TEST_F(IteratorDebugTest, BasicIteratorValidity)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};
    
    auto it = buffer.begin();
    auto end_it = buffer.end();
    
    // These should not trigger debug assertions
    EXPECT_EQ(*it, 1);
    EXPECT_NE(it, end_it);
    EXPECT_LT(it, end_it);
    
    ++it;
    EXPECT_EQ(*it, 2);
    
    it += 2;
    EXPECT_EQ(*it, 4);
    
    --it;
    EXPECT_EQ(*it, 3);
}

// Test iterator range verification
TEST_F(IteratorDebugTest, IteratorRangeConsistency)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};
    
    auto begin_it = buffer.begin();
    auto end_it = buffer.end();
    
    // Verify range is consistent
    EXPECT_TRUE(verify_iterator_consistency(buffer));
    
    // Test iterator arithmetic consistency
    for (auto i = decltype(buffer)::size_type{0}; i < buffer.size(); ++i) {
        auto it = begin_it + i;
        EXPECT_EQ(*it, buffer[i]);
        EXPECT_EQ(it - begin_it, static_cast<ptrdiff_t>(i));
    }
}

// Test const iterator conversions
TEST_F(IteratorDebugTest, ConstIteratorConversion)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};
    
    auto it = buffer.begin();
    auto const_it = buffer.cbegin();
    
    // Non-const to const conversion should work
    circular_buffer<int, 10>::const_iterator converted_it = it;
    EXPECT_EQ(converted_it, const_it);
    EXPECT_EQ(*converted_it, *const_it);
}

// Test iterator operations after buffer modifications
TEST_F(IteratorDebugTest, IteratorAfterModification)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};
    
    auto original_begin = buffer.begin();
    auto original_end = buffer.end();
    
    // Add element - iterators should still be valid for existing elements
    buffer.push_back(6);
    
    // The iterators should still work for the original range
    // (In a production iterator debugging system, we might invalidate iterators,
    // but our simple implementation doesn't track this)
    size_t count = 0;
    for (auto it = buffer.begin(); it != buffer.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 6);
}

// Test iterator comparison edge cases
TEST_F(IteratorDebugTest, IteratorComparisonEdgeCases)
{
    circular_buffer<int, 5> buffer;
    
    // Empty buffer iterators
    auto empty_begin = buffer.begin();
    auto empty_end = buffer.end();
    
    EXPECT_EQ(empty_begin, empty_end);
    EXPECT_EQ(empty_end - empty_begin, 0);
    
    // Add some elements
    buffer.push_back(1);
    buffer.push_back(2);
    
    auto new_begin = buffer.begin();
    auto new_end = buffer.end();
    
    EXPECT_NE(new_begin, new_end);
    EXPECT_EQ(new_end - new_begin, 2);
}

// Test iterator dereferencing safety
TEST_F(IteratorDebugTest, IteratorDereferencing)
{
    circular_buffer<int, 10> buffer{10, 20, 30};
    
    auto it = buffer.begin();
    
    // Test dereferencing
    EXPECT_EQ(*it, 10);
    // Test pointer access (arrow operator would be used with struct/class types)
    
    // Test subscript operator
    EXPECT_EQ(it[0], 10);
    EXPECT_EQ(it[1], 20);
    EXPECT_EQ(it[2], 30);
    
    // Test pointer arithmetic and dereferencing
    EXPECT_EQ(*(it + 1), 20);
    EXPECT_EQ(*(it + 2), 30);
}

// Test reverse iterator functionality
TEST_F(IteratorDebugTest, ReverseIterators)
{
    circular_buffer<int, 10> buffer{1, 2, 3, 4, 5};
    
    // Test reverse iteration
    std::vector<int> reverse_values;
    for (auto rit = buffer.rbegin(); rit != buffer.rend(); ++rit) {
        reverse_values.push_back(*rit);
    }
    
    std::vector<int> expected = {5, 4, 3, 2, 1};
    EXPECT_EQ(reverse_values, expected);
    
    // Test const reverse iterators
    const auto& const_buffer = buffer;
    std::vector<int> const_reverse_values;
    for (auto crit = const_buffer.crbegin(); crit != const_buffer.crend(); ++crit) {
        const_reverse_values.push_back(*crit);
    }
    
    EXPECT_EQ(const_reverse_values, expected);
}

// Test iterator stability during wraparound
TEST_F(IteratorDebugTest, IteratorStabilityDuringWraparound)
{
    circular_buffer<int, 4> buffer;
    
    // Fill buffer
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    buffer.push_back(4);
    
    // Get iterator to first element
    auto first_it = buffer.begin();
    EXPECT_EQ(*first_it, 1);
    
    // Cause wraparound by removing front and adding back
    buffer.pop_front();  // Remove 1
    buffer.push_back(5); // Add 5
    
    // Buffer should now contain {2, 3, 4, 5}
    // Verify new iterator state
    auto new_first_it = buffer.begin();
    EXPECT_EQ(*new_first_it, 2);
    
    // Verify full iteration works correctly
    std::vector<int> values;
    for (auto it = buffer.begin(); it != buffer.end(); ++it) {
        values.push_back(*it);
    }
    
    std::vector<int> expected = {2, 3, 4, 5};
    EXPECT_EQ(values, expected);
}

// Test iterator with different index types
TEST_F(IteratorDebugTest, IteratorWithDifferentIndexTypes)
{
    // Test with uint8_t indices
    circular_buffer<int, 10, uint8_t> buffer8{1, 2, 3, 4, 5};
    
    size_t count8 = 0;
    for (auto it = buffer8.begin(); it != buffer8.end(); ++it) {
        ++count8;
    }
    EXPECT_EQ(count8, 5);
    
    // Test with uint16_t indices
    circular_buffer<int, 10, uint16_t> buffer16{1, 2, 3};
    
    auto it16 = buffer16.begin();
    EXPECT_EQ(*it16, 1);
    ++it16;
    EXPECT_EQ(*it16, 2);
    ++it16;
    EXPECT_EQ(*it16, 3);
    ++it16;
    EXPECT_EQ(it16, buffer16.end());
}

// Test iterator performance characteristics
TEST_F(IteratorDebugTest, IteratorPerformanceCharacteristics)
{
    constexpr size_t large_size = 1000;
    circular_buffer<int, large_size> buffer;
    
    // Fill buffer
    for (size_t i = 0; i < large_size; ++i) {
        buffer.push_back(static_cast<int>(i));
    }
    
    PerformanceTimer timer;
    
    // Time iterator traversal
    auto traversal_time = timer.measure([&buffer]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            sum += *it;
        }
    });
    
    // Time random access via iterators
    auto random_access_time = timer.measure([&buffer]() {
        volatile int sum = 0;  // volatile to prevent optimization
        auto begin_it = buffer.begin();
        for (auto i = decltype(buffer)::size_type{0}; i < buffer.size(); i += 10) {
            sum += *(begin_it + i);
        }
    });
    
    // Just verify the operations completed (actual timing would be environment-dependent)
    EXPECT_GT(traversal_time.count(), 0);
    EXPECT_GT(random_access_time.count(), 0);
    
    // For a well-optimized implementation, iterator traversal should be reasonably fast
    // This is just a smoke test - real performance testing would need controlled environments
}

// Test iterator with complex types
TEST_F(IteratorDebugTest, IteratorWithComplexTypes)
{
    circular_buffer<std::string, 5> buffer;
    
    buffer.push_back("first");
    buffer.push_back("second");
    buffer.push_back("third");
    
    // Test iterator with complex types
    std::vector<std::string> collected;
    for (auto it = buffer.begin(); it != buffer.end(); ++it) {
        collected.push_back(*it);
    }
    
    std::vector<std::string> expected = {"first", "second", "third"};
    EXPECT_EQ(collected, expected);
    
    // Test modification through iterator
    auto it = buffer.begin();
    *it = "modified";
    EXPECT_EQ(buffer[0], "modified");
}

// Test iterator edge cases with empty and single-element buffers
TEST_F(IteratorDebugTest, IteratorEdgeCases)
{
    // Empty buffer
    circular_buffer<int, 5> empty_buffer;
    EXPECT_EQ(empty_buffer.begin(), empty_buffer.end());
    EXPECT_EQ(std::distance(empty_buffer.begin(), empty_buffer.end()), 0);
    
    // Single element buffer
    circular_buffer<int, 5> single_buffer;
    single_buffer.push_back(42);
    
    EXPECT_NE(single_buffer.begin(), single_buffer.end());
    EXPECT_EQ(std::distance(single_buffer.begin(), single_buffer.end()), 1);
    EXPECT_EQ(*single_buffer.begin(), 42);
    
    auto it = single_buffer.begin();
    ++it;
    EXPECT_EQ(it, single_buffer.end());
}

#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0

// Tests that only run when iterator debugging is enabled
TEST_F(IteratorDebugTest, DebugModeSpecificTests)
{
    circular_buffer<int, 5> buffer{1, 2, 3};
    
    // These tests would ideally verify that debug assertions fire correctly,
    // but since we can't easily test assertion failures in unit tests,
    // we just verify that normal operations work correctly
    
    auto it = buffer.begin();
    auto end_it = buffer.end();
    
    // Normal operations should work fine
    EXPECT_NO_THROW({
        *it;
        ++it;
        it != end_it;
        it < end_it;
    });
    
    // Test iterator comparison between different containers would ideally
    // trigger debug assertions, but we can't easily test that here
}

#endif

// Test STL algorithm compatibility with debug iterators
TEST_F(IteratorDebugTest, STLAlgorithmCompatibility)
{
    circular_buffer<int, 10> buffer{9, 3, 7, 1, 8, 2, 6, 4, 5};
    
    // Test std::sort with debug iterators
    std::sort(buffer.begin(), buffer.end());
    
    for (auto i = decltype(buffer)::size_type{1}; i < buffer.size(); ++i) {
        EXPECT_LE(buffer[i-1], buffer[i]);
    }
    
    // Test std::find
    auto found_it = std::find(buffer.begin(), buffer.end(), 5);
    EXPECT_NE(found_it, buffer.end());
    EXPECT_EQ(*found_it, 5);
    
    // Test std::copy
    std::vector<int> copied_values;
    copied_values.resize(buffer.size());
    std::copy(buffer.begin(), buffer.end(), copied_values.begin());
    
    for (auto i = decltype(buffer)::size_type{0}; i < buffer.size(); ++i) {
        EXPECT_EQ(copied_values[i], buffer[i]);
    }
    
    // Test std::accumulate
    int sum = std::accumulate(buffer.begin(), buffer.end(), 0);
    EXPECT_EQ(sum, 45);  // Sum of 1-9
}

// Test iterator with move-only types
TEST_F(IteratorDebugTest, IteratorWithMoveOnlyTypes)
{
    circular_buffer<MoveOnlyObject, 5> buffer;
    
    buffer.emplace_back(1);
    buffer.emplace_back(2);
    buffer.emplace_back(3);
    
    // Test iteration with move-only types
    std::vector<int> values;
    for (const auto& obj : buffer) {
        values.push_back(obj.value);
    }
    
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(values, expected);
    
    // Test iterator arithmetic
    auto it = buffer.begin();
    EXPECT_EQ(it->value, 1);
    ++it;
    EXPECT_EQ(it->value, 2);
    it += 1;
    EXPECT_EQ(it->value, 3);
} 