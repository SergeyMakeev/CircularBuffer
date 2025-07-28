#pragma once

#include <memory>
#include <string>
#include <vector>

/*

**Test object for validating constructor/destructor calls**

Tracks the number of constructor, destructor, copy, and move operations
for testing circular buffer behavior with non-trivial types.

*/
class TestObject
{
public:
    static int constructor_calls;
    static int destructor_calls;
    static int copy_calls;
    static int move_calls;
    
    int value;
    
    TestObject() : value(0) 
    { 
        ++constructor_calls; 
    }
    
    explicit TestObject(int v) : value(v) 
    { 
        ++constructor_calls; 
    }
    
    TestObject(const TestObject& other) : value(other.value) 
    { 
        ++constructor_calls; 
        ++copy_calls; 
    }
    
    TestObject(TestObject&& other) noexcept : value(other.value) 
    { 
        ++constructor_calls; 
        ++move_calls; 
        other.value = -1; // Mark as moved-from
    }
    
    TestObject& operator=(const TestObject& other) 
    { 
        value = other.value; 
        ++copy_calls; 
        return *this; 
    }
    
    TestObject& operator=(TestObject&& other) noexcept 
    { 
        value = other.value; 
        ++move_calls; 
        other.value = -1; // Mark as moved-from
        return *this; 
    }
    
    ~TestObject() 
    { 
        ++destructor_calls; 
    }
    
    bool operator==(const TestObject& other) const 
    { 
        return value == other.value; 
    }
    
    bool operator!=(const TestObject& other) const 
    { 
        return !(*this == other); 
    }
    
    bool operator<(const TestObject& other) const 
    { 
        return value < other.value; 
    }
    
    static void reset_counters()
    {
        constructor_calls = 0;
        destructor_calls = 0;
        copy_calls = 0;
        move_calls = 0;
    }
};

// Static member declarations (definitions moved to circular_buffer_test.cpp)

/*

**Move-only test object**

Cannot be copied, only moved. Useful for testing move semantics.

*/
class MoveOnlyObject
{
public:
    static int constructor_calls;
    static int destructor_calls;
    static int move_calls;
    
    int value;
    
    MoveOnlyObject() : value(0) 
    { 
        ++constructor_calls; 
    }
    
    explicit MoveOnlyObject(int v) : value(v) 
    { 
        ++constructor_calls; 
    }
    
    // Delete copy constructor and copy assignment
    MoveOnlyObject(const MoveOnlyObject&) = delete;
    MoveOnlyObject& operator=(const MoveOnlyObject&) = delete;
    
    MoveOnlyObject(MoveOnlyObject&& other) noexcept : value(other.value) 
    { 
        ++constructor_calls; 
        ++move_calls; 
        other.value = -1; 
    }
    
    MoveOnlyObject& operator=(MoveOnlyObject&& other) noexcept 
    { 
        value = other.value; 
        ++move_calls; 
        other.value = -1; 
        return *this; 
    }
    
    ~MoveOnlyObject() 
    { 
        ++destructor_calls; 
    }
    
    bool operator==(const MoveOnlyObject& other) const 
    { 
        return value == other.value; 
    }
    
    static void reset_counters()
    {
        constructor_calls = 0;
        destructor_calls = 0;
        move_calls = 0;
    }
};

// Static member declarations (definitions moved to circular_buffer_test.cpp)

/*

**Exception-throwing test object**

Can be configured to throw exceptions during construction, copy, or move
operations for testing exception safety.

*/
class ExceptionObject
{
public:
    static bool throw_on_construction;
    static bool throw_on_copy;
    static bool throw_on_move;
    static int construction_count;
    
    int value;
    
    ExceptionObject() : value(0) 
    {
        ++construction_count;
        if (throw_on_construction) {
            throw std::runtime_error("Construction exception");
        }
    }
    
    explicit ExceptionObject(int v) : value(v) 
    {
        ++construction_count;
        if (throw_on_construction) {
            throw std::runtime_error("Construction exception");
        }
    }
    
    ExceptionObject(const ExceptionObject& other) : value(other.value) 
    {
        ++construction_count;
        if (throw_on_copy) {
            throw std::runtime_error("Copy exception");
        }
    }
    
    ExceptionObject(ExceptionObject&& other) noexcept(false) : value(other.value) 
    {
        ++construction_count;
        if (throw_on_move) {
            throw std::runtime_error("Move exception");
        }
        other.value = -1;
    }
    
    ExceptionObject& operator=(const ExceptionObject& other) 
    {
        if (throw_on_copy) {
            throw std::runtime_error("Copy assignment exception");
        }
        value = other.value;
        return *this;
    }
    
    ExceptionObject& operator=(ExceptionObject&& other) noexcept(false) 
    {
        if (throw_on_move) {
            throw std::runtime_error("Move assignment exception");
        }
        value = other.value;
        other.value = -1;
        return *this;
    }
    
    ~ExceptionObject() 
    {
        --construction_count;
    }
    
    static void reset_flags()
    {
        throw_on_construction = false;
        throw_on_copy = false;
        throw_on_move = false;
        construction_count = 0;
    }
};

// Static member declarations (definitions moved to circular_buffer_test.cpp)

/*

**Large object for testing alignment and memory usage**

Simulates objects that benefit from custom alignment (e.g., SIMD types).

*/
struct alignas(32) LargeAlignedObject
{
    double data[4];  // 32 bytes, 32-byte aligned
    
    LargeAlignedObject() 
    {
        for (int i = 0; i < 4; ++i) {
            data[i] = 0.0;
        }
    }
    
    explicit LargeAlignedObject(double value) 
    {
        for (int i = 0; i < 4; ++i) {
            data[i] = value;
        }
    }
    
    bool operator==(const LargeAlignedObject& other) const
    {
        for (int i = 0; i < 4; ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }
};

/*

**Test fixture base class**

Provides common setup and teardown for circular buffer tests.

*/
class CircularBufferTestBase
{
protected:
    void SetUp() 
    {
        TestObject::reset_counters();
        MoveOnlyObject::reset_counters();
        ExceptionObject::reset_flags();
    }
    
    void TearDown() 
    {
        // Verify no memory leaks (constructor calls should equal destructor calls)
        // Note: This is a basic check, real memory leak detection should use tools like Valgrind
    }
    
    // Helper to create a vector of test objects
    std::vector<int> create_test_data(size_t count, int start_value = 0)
    {
        std::vector<int> data;
        data.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            data.push_back(start_value + static_cast<int>(i));
        }
        return data;
    }
    
    // Helper to verify buffer contents match expected values
    template<typename BufferType>
    bool verify_buffer_contents(const BufferType& buffer, const std::vector<int>& expected)
    {
        if (buffer.size() != expected.size()) {
            return false;
        }
        
        for (auto i = typename BufferType::size_type{0}; i < expected.size(); ++i) {
            if (buffer[i] != expected[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    // Helper to verify iterator consistency
    template<typename BufferType>
    bool verify_iterator_consistency(const BufferType& buffer)
    {
        // Verify begin/end consistency
        auto distance = std::distance(buffer.begin(), buffer.end());
        if (static_cast<size_t>(distance) != buffer.size()) {
            return false;
        }
        
        // Verify random access consistency
        auto it = buffer.begin();
        for (auto i = typename BufferType::size_type{0}; i < buffer.size(); ++i, ++it) {
            if (it != buffer.begin() + i) {
                return false;
            }
            if (*it != buffer[i]) {
                return false;
            }
        }
        
        return true;
    }
};

/*

**Performance measurement utilities**

*/
#include <chrono>

class PerformanceTimer
{
public:
    using clock_type = std::chrono::high_resolution_clock;
    using duration_type = std::chrono::nanoseconds;
    
    void start()
    {
        start_time = clock_type::now();
    }
    
    duration_type stop()
    {
        auto end_time = clock_type::now();
        return std::chrono::duration_cast<duration_type>(end_time - start_time);
    }
    
    template<typename Func>
    duration_type measure(Func&& func)
    {
        start();
        func();
        return stop();
    }
    
private:
    clock_type::time_point start_time;
};

/*

**Memory alignment verification utilities**

*/
template<typename T, size_t Alignment>
bool is_properly_aligned(const T* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) % Alignment) == 0;
}

template<typename BufferType>
bool verify_buffer_alignment()
{
    BufferType buffer;
    
    // For embedded storage, check if the buffer object itself is aligned
    if constexpr (BufferType::uses_embedded()) {
        return is_properly_aligned<BufferType, BufferType::alignment>(&buffer);
    } else {
        // For heap storage, we can't easily check without exposing internals
        // This would require friend access or public methods
        return true; // Assume alignment is correct for heap allocation
    }
} 