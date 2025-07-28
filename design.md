# Circular Buffer Design Document

## 🎯 Project Overview

A high-performance, header-only circular buffer implementation that stores exactly N elements without the N-1 limitation found in many implementations. The design emphasizes performance, type safety, and flexible memory management while maintaining STL compatibility.

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

## 📋 Template Signature

```cpp
template<typename T,                                           // Element type
         size_t Capacity,                                      // Fixed buffer size  
         typename IndexType = uint32_t,                        // Index type
         size_t Alignment = alignof(T),                        // Memory alignment
         size_t EmbeddedThreshold = 64,                        // Embedded vs heap cutoff  
         overflow_policy Policy = overflow_policy::overwrite>  // Overflow behavior
class circular_buffer;
```

### **Template Parameters**

| Parameter | Purpose | Default | Constraints |
|-----------|---------|---------|-------------|
| `T` | Element type | - | Copyable/movable type |
| `Capacity` | Fixed buffer size | - | > 0, ≤ max(IndexType) |
| `IndexType` | Index/size type | `uint32_t` | Unsigned integer type |
| `Alignment` | Memory alignment | `alignof(T)` | Power of 2 |
| `EmbeddedThreshold` | Embedded/heap cutoff | 64 | Element count |
| `Policy` | Overflow behavior | `overwrite` | `overwrite` or `discard` |

## 🚀 Storage Strategy

### **Embedded Storage (Small Buffers)**
- Used when `Capacity <= EmbeddedThreshold`
- Storage array embedded directly in container object
- Excellent cache locality, zero allocation overhead
- Suitable for stack allocation

```cpp
// Capacity <= 64: embedded storage
circular_buffer<int, 32> small_buffer;  // ~140 bytes total object size
```

### **Heap Storage (Large Buffers)**
- Used when `Capacity > EmbeddedThreshold`
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
circular_buffer<int, 64, uint8_t> tiny_embedded;           // ~259 bytes
circular_buffer<float, 32, uint16_t, 32> simd_embedded;    // ~134 bytes, 32-byte aligned

// Heap storage variants  
circular_buffer<MyStruct, 256, uint32_t> standard_heap;    // ~16 bytes container
circular_buffer<double, 1000, uint32_t, 64> aligned_heap;  // ~16 bytes, 64-byte aligned
```

## 📊 Index Type Optimization

### **Memory Usage by Index Type**

| Index Type | Max Capacity | Container Overhead | Use Case |
|------------|--------------|-------------------|----------|
| `uint8_t`  | 255         | 3 bytes           | Tiny buffers |
| `uint16_t` | 65,535      | 6 bytes           | Small buffers |
| `uint32_t` | 4.2B        | 12 bytes          | Standard use |
| `uint64_t` | 18E         | 24 bytes          | Massive buffers |

### **Performance Optimizations by Index Type**

```cpp
// Power-of-2 capacity optimization
template<typename IndexType, size_t Capacity>
constexpr IndexType next_index(IndexType current) noexcept {
    if constexpr ((Capacity & (Capacity - 1)) == 0) {
        return (current + 1) & (Capacity - 1);  // Bit masking
    } else {
        return (current + 1) % Capacity;         // Modulo
    }
}
```

## 🎯 Overflow Handling

### **Overflow Policies**

```cpp
enum class overflow_policy {
    overwrite,  // Overwrite oldest elements when full (default)
    discard     // Discard new elements when full
};

enum class insert_result {
    inserted,     // Successfully inserted
    overwritten,  // Inserted by overwriting oldest element
    discarded     // Element was discarded (buffer full + discard policy)
};
```

### **Policy Behavior Examples**

```cpp
// Overwrite policy: always succeeds, may overwrite old data
circular_buffer<int, 3, uint32_t, alignof(int), 64, overflow_policy::overwrite> overwrite_buf;
auto result = overwrite_buf.push_back(42);  // Returns inserted/overwritten

// Discard policy: rejects new elements when full
circular_buffer<int, 3, uint32_t, alignof(int), 64, overflow_policy::discard> discard_buf;
auto result = discard_buf.push_back(42);    // Returns inserted/discarded
```

## 📋 Public Interface

### **Type Definitions**

```cpp
using value_type = T;
using size_type = IndexType;
using index_type = IndexType;
using reference = T&;
using const_reference = const T&;
using pointer = T*;
using const_pointer = const T*;
```

### **Constructors**

```cpp
// Default construction (empty buffer)
circular_buffer();

// Fill constructor
explicit circular_buffer(const T& value);

// Range constructor
template<typename Iterator>
circular_buffer(Iterator first, Iterator last);

// Initializer list
circular_buffer(std::initializer_list<T> init);

// Copy/move constructors
circular_buffer(const circular_buffer& other);
circular_buffer(circular_buffer&& other) noexcept;

// Copy/move assignment
circular_buffer& operator=(const circular_buffer& other);
circular_buffer& operator=(circular_buffer&& other) noexcept;
```

### **Capacity and Size**

```cpp
// Capacity queries
static constexpr size_type capacity() noexcept { return Capacity; }
size_type size() const noexcept;
bool empty() const noexcept;
bool full() const noexcept;

// Clear all elements
void clear() noexcept;
```

### **Element Access**

```cpp
// Indexed access (unchecked)
reference operator[](size_type index) noexcept;
const_reference operator[](size_type index) const noexcept;

// Indexed access (bounds checked)
reference at(size_type index);
const_reference at(size_type index) const;

// Front/back access (undefined if empty)
reference front() noexcept;
const_reference front() const noexcept;
reference back() noexcept;
const_reference back() const noexcept;
```

### **Insert Operations with Result Feedback**

```cpp
// Modern interface: returns what happened
insert_result push_back(const T& value);
insert_result push_back(T&& value);
template<typename... Args> 
insert_result emplace_back(Args&&... args);

insert_result push_front(const T& value);
insert_result push_front(T&& value);
template<typename... Args> 
insert_result emplace_front(Args&&... args);

// Traditional interface: ignores overflow result
void push_back_unchecked(const T& value);
void push_back_unchecked(T&& value);
template<typename... Args> 
void emplace_back_unchecked(Args&&... args);

void push_front_unchecked(const T& value);
void push_front_unchecked(T&& value);
template<typename... Args> 
void emplace_front_unchecked(Args&&... args);
```

### **Remove Operations**

```cpp
// Traditional interface (undefined if empty)
void pop_back();
void pop_front();

// Safe interface with optional return
std::optional<T> pop_back_safe();
std::optional<T> pop_front_safe();

// Convenience aliases for optional interface
std::optional<T> pop() { return pop_back_safe(); }  // Default to back
```

### **Bulk Operations**

```cpp
// Bulk insert with statistics
struct bulk_insert_result {
    size_type inserted_count;
    size_type overwritten_count;
    size_type discarded_count;
};

template<typename Iterator>
bulk_insert_result push_back_range(Iterator first, Iterator last);

template<typename Iterator>
bulk_insert_result push_front_range(Iterator first, Iterator last);
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

## 💾 Memory Layout

### **Internal Data Members**

```cpp
private:
    storage_t m_storage;    // Embedded array or heap pointer
    IndexType m_head;       // Write position (next insert location)
    IndexType m_tail;       // Read position (next remove location)
    IndexType m_size;       // Current number of elements
```

### **Embedded vs Heap Storage Implementation**

```cpp
// Storage strategy selection
static constexpr bool uses_embedded_storage = (Capacity <= EmbeddedThreshold);

// Storage types
using embedded_storage_t = alignas(Alignment) std::array<std::byte, sizeof(T) * Capacity>;
using heap_storage_t = T*;
using storage_t = std::conditional_t<uses_embedded_storage, embedded_storage_t, heap_storage_t>;
```

## 🧪 Usage Examples

### **Basic Usage**

```cpp
#include "circular_buffer/circular_buffer.h"

// Default configuration: 32-bit indices, natural alignment, overwrite policy
dod::circular_buffer<int, 100> buffer;

// Add elements
auto result1 = buffer.push_back(42);           // Returns insert_result::inserted
auto result2 = buffer.emplace_back(1, 2, 3);  // Construct in-place

// Access elements
int front = buffer.front();                   // Get first element
int back = buffer.back();                     // Get last element
int third = buffer[2];                        // Index access

// Remove elements  
buffer.pop_back();                            // Remove last (undefined if empty)
auto popped = buffer.pop_back_safe();         // Returns std::optional<int>
auto item = buffer.pop();                     // Alias for pop_back_safe()

// Query state
bool is_empty = buffer.empty();
bool is_full = buffer.full();
size_t count = buffer.size();
```

### **Memory-Optimized Usage**

```cpp
// Tiny buffer with 8-bit indices
dod::circular_buffer<uint8_t, 200, uint8_t> tiny_buffer;

// Cache-line aligned for SIMD operations
dod::circular_buffer<float, 512, uint32_t, 64> aligned_buffer;

// Force heap allocation for large embedded threshold
dod::circular_buffer<MyStruct, 128, uint32_t, alignof(MyStruct), 32> heap_buffer;
```

### **Overflow Policy Usage**

```cpp
// Overwrite policy (default)
dod::circular_buffer<int, 3, uint32_t, alignof(int), 64, 
                     dod::overflow_policy::overwrite> overwrite_buf;

overwrite_buf.push_back(1);  // inserted
overwrite_buf.push_back(2);  // inserted  
overwrite_buf.push_back(3);  // inserted
auto result = overwrite_buf.push_back(4);  // overwritten (removes element 1)

// Discard policy
dod::circular_buffer<int, 3, uint32_t, alignof(int), 64,
                     dod::overflow_policy::discard> discard_buf;

discard_buf.push_back(1);    // inserted
discard_buf.push_back(2);    // inserted
discard_buf.push_back(3);    // inserted
auto result = discard_buf.push_back(4);  // discarded (element 4 rejected)
```

### **Range-Based Operations**

```cpp
dod::circular_buffer<int, 1000> buffer;

// Range-based for loop
for (const auto& item : buffer) {
    process(item);
}

// STL algorithms
std::fill(buffer.begin(), buffer.end(), 42);
auto it = std::find(buffer.begin(), buffer.end(), 123);
std::sort(buffer.begin(), buffer.end());

// Bulk insertion
std::vector<int> data = {1, 2, 3, 4, 5};
auto bulk_result = buffer.push_back_range(data.begin(), data.end());
std::cout << "Inserted: " << bulk_result.inserted_count << std::endl;
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

- **C++17 or later**
- **Supported compilers**: GCC 9+, Clang 10+, MSVC 2019+
- **Platforms**: Windows, Linux, macOS

## 📈 Performance Characteristics

### **Time Complexity**

| Operation | Complexity | Notes |
|-----------|------------|-------|
| `push_back()` | O(1) | Always constant time |
| `push_front()` | O(1) | Always constant time |
| `pop_back()` | O(1) | Always constant time |
| `pop_front()` | O(1) | Always constant time |
| `operator[]` | O(1) | Index calculation only |
| `front()` / `back()` | O(1) | Direct access |
| `size()` / `empty()` | O(1) | Cached values |

### **Space Complexity**

| Storage Type | Space Overhead | Notes |
|--------------|----------------|-------|
| Embedded | 3 * sizeof(IndexType) | Plus alignment padding |
| Heap | 3 * sizeof(IndexType) + sizeof(T*) | Plus heap allocation |

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

### **Why Optional Pop Methods?**
- Safe alternative to undefined behavior on empty buffer
- Modern C++ best practice (explicit error handling)
- Maintains backward compatibility with traditional void pop methods

---

## 📁 File Structure

```
circular_buffer/
├── circular_buffer.h              # Main implementation
└── CMakeLists.txt                 # Library configuration

tests/
├── circular_buffer_test.cpp       # Comprehensive unit tests
├── performance_test.cpp           # Performance benchmarks  
├── iterator_debug_test.cpp        # Iterator debugging tests
└── test_common.h                  # Test utilities

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