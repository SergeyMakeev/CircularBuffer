# Circular Buffer Design Document

## 🎯 Project Overview

A high-performance, header-only circular buffer implementation that stores exactly N elements without the N-1 limitation found in many implementations. The design emphasizes performance, type safety, and flexible memory management while maintaining STL compatibility.

**Key Innovation**: Uses separate read/write indices with explicit size tracking to achieve full capacity utilization without the typical "one empty slot" limitation.

## 🏗️ Core Design Principles

### **1. Full Capacity Utilization**
- Uses separate read/write indices with explicit size tracking
- No "one empty slot" limitation - a 32-element buffer stores exactly 32 elements
- Distinguishes full vs empty states through size counter

### **2. Fixed Capacity (No Dynamic Reallocation)**
- Capacity determined at compile-time or construction-time
- No `resize()`, `reserve()`, or `shrink_to_fit()` operations
- Predictable memory usage patterns for real-time systems

### **3. Smart Storage Strategy**
- **Small buffers**: Storage embedded in container object for cache efficiency
- **Large buffers**: Storage allocated on heap to avoid large stack objects
- **Configurable threshold**: User control over embedded vs heap decision

### **4. Configurable Index Types**
- Template parameter for index type (default: `uint32_t`)
- Supports `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- Memory-optimized for different capacity ranges

### **5. Custom Alignment Support**
- Template parameter for memory alignment
- SIMD-friendly alignments (16, 32, 64 bytes)
- Automatic alignment optimization for performance-critical types

### **6. Platform Abstraction**
- Custom allocator support via preprocessor macros
- Platform-specific optimizations (Windows vs POSIX)
- Configurable assertions for debugging

## 📋 Template Signature

```cpp
template<typename T,                                           // Element type
         size_t Capacity,                                      // Fixed buffer size  
         overflow_policy Policy = overflow_policy::overwrite,  // Overflow behavior
         typename IndexType = uint32_t,                        // Index type
         size_t InlineThreshold = 64,                          // Embedded vs heap cutoff
         size_t Alignment = alignof(T)>                        // Memory alignment
class circular_buffer;
```

### **Template Parameters**

| Parameter | Purpose | Default | Constraints |
|-----------|---------|---------|-------------|
| `T` | Element type | - | Copyable/movable type |
| `Capacity` | Fixed buffer size | - | > 0, ≤ max(IndexType) |
| `Policy` | Overflow behavior | `overwrite` | `overwrite` or `discard` |
| `IndexType` | Index/size type | `uint32_t` | Unsigned integer type |
| `InlineThreshold` | Embedded/heap cutoff | 64 | Element count |
| `Alignment` | Memory alignment | `alignof(T)` | Power of 2, ≥ alignof(T) |

### **Template Parameter Validation**

The implementation includes compile-time checks:
```cpp
static_assert(std::is_unsigned_v<IndexType>, "IndexType must be unsigned integer");
static_assert(Capacity > 0, "Capacity must be greater than 0");
static_assert(Capacity <= std::numeric_limits<IndexType>::max(), "Capacity too large for IndexType");
static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be power of 2");
static_assert(Alignment >= alignof(T), "Alignment must be at least alignof(T)");
```

## 🚀 Storage Strategy

### **Embedded Storage (Small Buffers)**
- Used when `Capacity <= InlineThreshold`
- Storage array embedded directly in container object
- Excellent cache locality, zero allocation overhead
- Suitable for stack allocation

```cpp
// Capacity <= 64: embedded storage
circular_buffer<int, 32> small_buffer;  // ~140 bytes total object size
```

### **Heap Storage (Large Buffers)**
- Used when `Capacity > InlineThreshold`
- Storage allocated on heap with custom alignment
- Lightweight container object (~12-24 bytes)
- Single allocation at construction time

```cpp
// Capacity > 64: heap storage  
circular_buffer<int, 1024> large_buffer;  // ~16 bytes container + 4KB heap
```

### **Storage Selection Examples**

```cpp
// Embedded storage variants
circular_buffer<int, 64, overflow_policy::overwrite, uint8_t> tiny_embedded;           // ~259 bytes
circular_buffer<float, 32, overflow_policy::overwrite, uint16_t, 64, 32> simd_embedded;    // ~134 bytes, 32-byte aligned

// Heap storage variants  
circular_buffer<MyStruct, 256> standard_heap;    // ~16 bytes container
circular_buffer<double, 1000, overflow_policy::overwrite, uint32_t, 64, 64> aligned_heap;  // ~16 bytes, 64-byte aligned
```

### **Platform-Specific Memory Allocation**

The implementation uses different allocation strategies per platform:

**Windows (MSVC)**:
```cpp
#define CIRCULAR_BUFFER_ALLOC(size, alignment) _mm_malloc(size, alignment)
#define CIRCULAR_BUFFER_FREE(ptr) _mm_free(ptr)
```

**POSIX (Linux/macOS)**:
```cpp
#define CIRCULAR_BUFFER_ALLOC(size, alignment) aligned_alloc(alignment, round_up_to_alignment(size, alignment))
#define CIRCULAR_BUFFER_FREE(ptr) free(ptr)
```

**Custom Allocation**: Users can override by defining macros before including the header.

## 📊 Index Type Optimization

### **Memory Usage by Index Type**

| Index Type | Max Capacity | Container Overhead | Use Case |
|------------|--------------|-------------------|----------|
| `uint8_t`  | 255         | 3 bytes           | Tiny buffers |
| `uint16_t` | 65,535      | 6 bytes           | Small buffers |
| `uint32_t` | 4.2 billion | 12 bytes          | Standard use |
| `uint64_t` | Massive     | 24 bytes          | Future-proofing |

### **Index Type Selection Guidelines**

```cpp
// Memory-optimized examples
circular_buffer<uint8_t, 200, overflow_policy::overwrite, uint8_t> byte_queue;     // 3-byte overhead
circular_buffer<Packet, 1000, overflow_policy::overwrite, uint16_t> packet_buffer; // 6-byte overhead  
circular_buffer<Event, 100000, overflow_policy::overwrite, uint32_t> event_log;    // 12-byte overhead
```

### **Index Calculation Optimization**

The implementation includes power-of-2 optimizations:

```cpp
constexpr IndexType next_index(IndexType index) const noexcept
{
    if constexpr ((Capacity & (Capacity - 1)) == 0)
    {
        // Power of 2: use bit masking (faster)
        return (index + 1) & (Capacity - 1);
    }
    else
    {
        // Generic: use conditional
        return (index + 1 < Capacity) ? index + 1 : 0;
    }
}
```

## 🌊 Overflow Policies

### **Overwrite Policy (Default)**
- New elements always succeed
- Oldest elements automatically overwritten when full
- Best for streaming data, sensor readings, logging

```cpp
circular_buffer<int, 100> buffer;  // Defaults to overwrite policy
auto result = buffer.push_back(42);  // Always succeeds (inserted or overwritten)
```

### **Discard Policy**
- New elements discarded when buffer is full
- Existing data preserved until explicitly removed
- Best for priority queues, finite resources

```cpp
circular_buffer<Task, 50, overflow_policy::discard> task_queue;
auto result = task_queue.push_back(task);  // May return discarded
```

### **Result Feedback**

```cpp
enum class insert_result {
    inserted,     // Successfully added to buffer
    overwritten,  // Added by overwriting oldest element  
    discarded     // Rejected due to full buffer + discard policy
};
```

### **Policy Implementation Details**

The overflow policy is enforced at compile-time through template specialization:

```cpp
template <insert_position Position, typename... Args> 
insert_result insert_impl(Args&&... args)
{
    bool was_full = full();
    if constexpr (Policy == overflow_policy::discard)
    {
        if (was_full)
        {
            return insert_result::discarded;
        }
    }
    // ... insertion logic
}
```

## 🎨 Complete API Reference

### **Capacity Management**

```cpp
// Compile-time information
static constexpr size_type capacity() noexcept;        // Maximum elements
static constexpr bool has_inline_storage() noexcept;   // True if embedded storage

// Runtime state  
size_type size() const noexcept;                       // Current element count
bool empty() const noexcept;                           // True if no elements
bool full() const noexcept;                            // True if at capacity
void clear() noexcept;                                 // Remove all elements
```

### **Element Access**

```cpp
// Indexed access (unchecked - assert in debug builds)
reference operator[](size_type index) noexcept;
const_reference operator[](size_type index) const noexcept;

// Indexed access (bounds checked - throws std::out_of_range)
reference at(size_type index);
const_reference at(size_type index) const;

// Front/back access (undefined if empty - assert in debug builds)
reference front() noexcept;
const_reference front() const noexcept;
reference back() noexcept;
const_reference back() const noexcept;
```

### **Insert Operations with Result Feedback**

```cpp
// Copy/move insertion
insert_result push_back(const T& value);
insert_result push_back(T&& value);
insert_result push_front(const T& value);
insert_result push_front(T&& value);

// In-place construction
template<typename... Args> 
insert_result emplace_back(Args&&... args);
template<typename... Args> 
insert_result emplace_front(Args&&... args);
```

### **Remove Operations**

```cpp
// Traditional interface (undefined if empty - assert in debug builds)
void drop_back();
void drop_front();

// Safe interface with optional return
std::optional<T> take_back();
std::optional<T> take_front();
```

### **Iterator Support**

```cpp
// Iterator types
template<bool IsConst> class iterator_impl;
using iterator = iterator_impl<false>;
using const_iterator = iterator_impl<true>;
using reverse_iterator = std::reverse_iterator<iterator>;
using const_reverse_iterator = std::reverse_iterator<const_iterator>;

// Iterator accessors
iterator begin() noexcept;
const_iterator begin() const noexcept;
const_iterator cbegin() const noexcept;

iterator end() noexcept;
const_iterator end() const noexcept;
const_iterator cend() const noexcept;

reverse_iterator rbegin() noexcept;
const_reverse_iterator rbegin() const noexcept;
const_reverse_iterator crbegin() const noexcept;

reverse_iterator rend() noexcept;
const_reverse_iterator rend() const noexcept;
const_reverse_iterator crend() const noexcept;
```

## 🎪 Advanced Usage Examples

### **Real-Time Sensor Data**

```cpp
// High-frequency sensor readings with overwrite policy
circular_buffer<SensorReading, 1000> sensor_buffer;

void on_sensor_data(const SensorReading& reading) {
    auto result = sensor_buffer.push_back(reading);
    // Always succeeds - old data automatically discarded
}

// Process recent data
for (const auto& reading : sensor_buffer) {
    analyze(reading);
}
```

### **Fixed-Size Task Queue**

```cpp
// Limited task queue with discard policy
circular_buffer<Task, 100, overflow_policy::discard> task_queue;

bool schedule_task(const Task& task) {
    auto result = task_queue.push_back(task);
    return result != insert_result::discarded;
}

// Process tasks
while (auto task = task_queue.take_front()) {
    execute(*task);
}
```

### **STL Algorithm Integration**

```cpp
circular_buffer<int, 1000> buffer;

// Fill with test data
for (int i = 0; i < 500; ++i) {
    buffer.push_back(i);
}

// STL algorithms
std::fill(buffer.begin(), buffer.end(), 42);
auto it = std::find(buffer.begin(), buffer.end(), 123);
std::sort(buffer.begin(), buffer.end());

// Individual insertion (optimized performance)
std::vector<int> data = {1, 2, 3, 4, 5};
for (const auto& item : data) {
    buffer.push_back(item);
}
```

### **SIMD-Optimized Buffer**

```cpp
// 64-byte aligned buffer for AVX-512 operations
struct alignas(64) SimdData {
    float values[16];
};

circular_buffer<SimdData, 256, overflow_policy::overwrite, uint32_t, 32, 64> simd_buffer;

// Buffer automatically maintains 64-byte alignment for all elements
```

## 🔧 Customization and Extension

### **Custom Allocators**

```cpp
// Example: Pool allocator integration
#define CIRCULAR_BUFFER_ALLOC(size, alignment) my_pool_alloc(size, alignment)
#define CIRCULAR_BUFFER_FREE(ptr) my_pool_free(ptr)
#include "circular_buffer/circular_buffer.h"
```

### **Custom Assertions**

```cpp
// Example: Logging assertions
#define CIRCULAR_BUFFER_ASSERT(expr) do { \
    if (!(expr)) { \
        log_error("Assertion failed: " #expr); \
        std::abort(); \
    } \
} while(0)
#include "circular_buffer/circular_buffer.h"
```

### **Alignment Helpers**

The implementation includes a `safe_alignment_of` helper for macOS compatibility:

```cpp
template <typename T> 
inline constexpr size_t safe_alignment_of = alignof(T) < alignof(void*) ? alignof(void*) : alignof(T);
```

This ensures alignment is at least `alignof(void*)` to meet macOS allocation requirements.

## 🔬 Exception Safety

### **Exception Safety Guarantees**

| Operation | Guarantee | Notes |
|-----------|-----------|-------|
| Constructors | Strong | Rollback on exception |
| Copy assignment | Strong | Rollback on exception |
| Move operations | No-throw | Marked `noexcept` where possible |
| Insert operations | Basic | Buffer remains valid |
| Access operations | No-throw | May assert in debug builds |
| Destructors | No-throw | Always `noexcept` |

### **Exception-Safe Design Patterns**

```cpp
// Safe insertion with rollback
template <insert_position Position, typename... Args> 
insert_result insert_impl(Args&&... args)
{
    // Check state before modification
    bool was_full = full();
    
    // Use placement new with exception safety
    T* storage = get_storage();
    dod::construct<T>(&storage[m_head], std::forward<Args>(args)...);
    
    // Update indices only after successful construction
    // ...
}
```

## 🔧 Build Integration

### **CMake Integration**

```cmake
# Header-only library
target_include_directories(my_target PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(my_target PRIVATE cxx_std_17)

# Or as a CMake target
add_subdirectory(circular_buffer)
target_link_libraries(my_target PRIVATE circular_buffer)
```

### **Compiler Requirements**

- **C++17 or later** (for `std::optional`, `constexpr if`, `std::launder`)
- **Supported compilers**: GCC 9+, Clang 10+, MSVC 2019+
- **Platforms**: Windows, Linux, macOS

### **Required C++17 Features**

| Feature | Usage |
|---------|-------|
| `constexpr if` | Template specialization |
| `std::optional` | Safe element removal |
| `std::launder` | Placement new safety |
| Structured bindings | Iterator implementation |
| Template argument deduction | Constructor templates |

## 📈 Performance Characteristics

### **Time Complexity**

| Operation | Complexity | Notes |
|-----------|------------|-------|
| `push_back()` | O(1) | Always constant time |
| `push_front()` | O(1) | Always constant time |
| `drop_back()` | O(1) | Always constant time |
| `drop_front()` | O(1) | Always constant time |
| `take_back()` | O(1) | Always constant time |
| `take_front()` | O(1) | Always constant time |
| `operator[]` | O(1) | Index calculation only |
| `front()` / `back()` | O(1) | Direct access |
| `size()` / `empty()` | O(1) | Cached values |

### **Space Complexity**

| Storage Type | Space Overhead | Notes |
|--------------|----------------|-------|
| Embedded | 3 * sizeof(IndexType) | Plus alignment padding |
| Heap | 3 * sizeof(IndexType) + sizeof(T*) | Plus heap allocation |

### **Performance Optimizations**

1. **Power-of-2 capacity**: Uses bit masking instead of modulo
2. **Template unification**: Single implementation for all insert operations
3. **Placement new**: Direct construction without temporary objects
4. **Index caching**: Avoids repeated calculations
5. **Branch prediction**: Optimized conditional logic

### **Performance Goals**

- **2-5x faster** than `std::deque` for push/pop operations
- **10-20% faster** than `std::deque` for random access
- **20-40% less** memory overhead than `std::deque`
- **30-50% fewer** cache misses for sequential access patterns

## 🎯 Design Rationale

### **Why Separate Head/Tail/Size?**
- Eliminates the N-1 element limitation of single-index designs
- Enables distinction between full and empty states
- Simplifies wraparound logic and boundary conditions

### **Why Configurable Index Types?**
- Memory optimization for small buffers (uint8_t saves 9 bytes vs uint32_t)
- Performance optimization for specific capacity ranges
- Future-proofing for massive buffers (uint64_t)

### **Why Embedded vs Heap Storage?**
- Stack allocation preferred for small buffers (better cache locality)
- Heap allocation necessary for large buffers (avoid stack overflow)
- User-configurable threshold for different use cases

### **Why Template Overflow Policy?**
- Compile-time optimization (no runtime branching)
- Different applications have different overflow requirements
- Template specialization enables policy-specific optimizations

### **Why Drop/Take Instead of Pop?**
- Clear distinction between unsafe (`drop_*`) and safe (`take_*`) operations
- Modern C++ best practice (explicit error handling via optional)
- Avoids confusion with std::stack's `pop()` that returns void

### **Why Remove Unchecked Operations?**
- API simplification - fewer functions to learn and maintain
- Performance difference was negligible in modern compilers
- Regular operations are already highly optimized

### **Why Remove Bulk Operations?**
- API simplification - reduced complexity
- Individual operations are equivalent in performance
- Range-based loops provide equivalent functionality

### **Why Platform-Specific Allocation?**
- Windows `_mm_malloc` provides better alignment guarantees
- POSIX `aligned_alloc` requires size to be multiple of alignment
- Custom macros allow user override for specialized allocators

---

## 📁 File Structure

```
circular_buffer/
├── circular_buffer.h              # Main implementation (826 lines)
└── CMakeLists.txt                 # Library configuration

tests/
├── circular_buffer_test.cpp       # Comprehensive unit tests (47+ tests)
├── performance_test.cpp           # Performance benchmarks  
├── test_common.h                  # Test utilities and helper objects
└── iterator_debug_test.cpp        # Iterator debugging tests

.github/workflows/
├── windows.yml                    # Windows MSVC builds
├── linux-gcc.yml                 # Linux GCC builds
├── linux-clang.yml               # Linux Clang builds
├── macos.yml                      # macOS builds
├── coverage.yml                   # Code coverage
├── sanitizers.yml                 # Memory/address sanitizers
└── performance.yml                # Performance regression tests

.cursor/rules/
├── coding_style.mdc              # Development guidelines
├── comment_style.mdc             # Documentation standards
├── performance.mdc               # Performance optimization rules
├── ascii.mdc                     # Character encoding rules
└── power-shell.mdc               # Windows PowerShell compatibility
```

This design provides a comprehensive, high-performance circular buffer that maximizes capacity utilization while offering flexible memory management and overflow handling strategies. 