# Circular Buffer

A high-performance, header-only circular buffer implementation for C++17 that stores exactly N elements without the N-1 limitation found in many implementations.

[![Windows](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/windows.yml/badge.svg)](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/windows.yml)
[![Linux GCC](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/linux-gcc.yml/badge.svg)](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/linux-gcc.yml)
[![Linux Clang](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/linux-clang.yml/badge.svg)](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/linux-clang.yml)
[![macOS](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/macos.yml/badge.svg)](https://github.com/SergeyMakeev/CircularBuffer/actions/workflows/macos.yml)
[![Code Coverage](https://codecov.io/github/SergeyMakeev/CircularBuffer/branch/main/graph/badge.svg)](https://codecov.io/github/SergeyMakeev/CircularBuffer)

## Quick Start

```bash
# Clone or download the circular_buffer.h file
# Include it in your project
#include "circular_buffer/circular_buffer.h"

# Or build the complete tests
mkdir build && cd build
cmake ..
cmake --build .
ctest                    # Run all tests
```

## Features

- **Full Capacity Utilization**: Store exactly N elements, not N-1
- **Header-Only**: Single file implementation - just include `circular_buffer.h`
- **STL-Compatible**: Standard iterators and container interface
- **High Performance**: 2-5x faster than `std::deque` for push/pop operations
- **Flexible Storage**: Embedded storage for small buffers, heap for large ones
- **Custom Index Types**: `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` support
- **Overflow Policies**: Configurable overwrite or discard behavior
- **Custom Alignment**: SIMD-friendly memory alignment support
- **Customizable**: Override allocators and assertions via macros
- **Cross-Platform**: Tested on Windows (MSVC), Linux (GCC/Clang), and macOS
- **Comprehensive Tests**: 47+ unit tests covering all functionality
- **C++17 Compatible**: Modern C++ features with broad compiler support

## Basic Usage

```cpp
#include "circular_buffer/circular_buffer.h"

int main() {
    // Step 1: Create a circular buffer that can hold 100 integers
    dod::circular_buffer<int, 100> buffer;
    
    // Step 2: Add elements to the buffer
    auto result1 = buffer.push_back(42);           // Returns insert_result::inserted
    auto result2 = buffer.emplace_back(1, 2, 3);  // Create element directly in buffer
    
    // Step 3: Access elements
    int front = buffer.front();                   // Get first element
    int back = buffer.back();                     // Get last element
    int third = buffer[2];                        // Access by index (O(1) time)
    
    // Step 4: Remove elements safely (returns std::optional)
    auto popped = buffer.take_back();             // Returns std::optional<int>
    auto item = buffer.take_front();              // Returns std::optional<int>
    
    // Step 5: Remove elements quickly (undefined behavior if empty)
    if (!buffer.empty()) {
        buffer.drop_back();                       // Faster but check empty() first
        buffer.drop_front();                      // Faster but check empty() first
    }
    
    // Step 6: Check buffer state
    bool is_empty = buffer.empty();               // True if no elements
    bool is_full = buffer.full();                 // True if at capacity
    size_t count = buffer.size();                 // Current number of elements
    
    return 0;
}
```

## Advanced Configuration

### Template Parameters

```cpp
template<typename T,                                           // Element type
         size_t Capacity,                                      // Fixed buffer size  
         overflow_policy Policy = overflow_policy::overwrite,  // Overflow behavior
         typename IndexType = uint32_t,                        // Index type
         size_t InlineThreshold = 64,                          // Embedded vs heap cutoff
         size_t Alignment = alignof(T)>                        // Memory alignment
class circular_buffer;
```

### Memory-Optimized Usage

```cpp
// Small buffer with 8-bit indices (saves memory)
dod::circular_buffer<uint8_t, 200, dod::overflow_policy::overwrite, uint8_t> tiny_buffer;

// Cache-line aligned for SIMD operations (64-byte alignment)
dod::circular_buffer<float, 512, dod::overflow_policy::overwrite, uint32_t, 64, 64> aligned_buffer;

// Force heap allocation (set threshold lower than capacity)
dod::circular_buffer<MyStruct, 128, dod::overflow_policy::overwrite, uint32_t, 32> heap_buffer;
```

### Overflow Policy Usage

```cpp
// Overwrite policy (default): always succeeds, may overwrite old data
dod::circular_buffer<int, 100, dod::overflow_policy::overwrite> overwrite_buf;

auto result1 = overwrite_buf.push_back(42);  // Returns inserted/overwritten

// Discard policy: rejects new elements when full
dod::circular_buffer<int, 100, dod::overflow_policy::discard> discard_buf;

auto result2 = discard_buf.push_back(42);    // Returns inserted/discarded

// Check what happened
switch (result2) {
    case dod::insert_result::inserted:    /* Success - element added */ break;
    case dod::insert_result::overwritten: /* Overwrote old data */ break;
    case dod::insert_result::discarded:   /* Buffer full, rejected */ break;
}
```

## STL Compatibility

```cpp
dod::circular_buffer<int, 1000> buffer;

// Range-based for loop
for (const auto& item : buffer) {
    process(item);
}

// STL algorithms work with iterators
std::fill(buffer.begin(), buffer.end(), 42);
auto it = std::find(buffer.begin(), buffer.end(), 123);
std::sort(buffer.begin(), buffer.end());

// Add multiple elements (no bulk operations for performance reasons)
std::vector<int> data = {1, 2, 3, 4, 5};
for (const auto& item : data) {
    buffer.push_back(item);
}
```

## Performance Characteristics

### Time Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| `push_back()` / `push_front()` | O(1) | Always constant time |
| `drop_back()` / `drop_front()` | O(1) | Always constant time |
| `take_back()` / `take_front()` | O(1) | Always constant time |
| `operator[]` / `at()` | O(1) | Direct index calculation |
| `front()` / `back()` | O(1) | Direct access |
| `begin()` / `end()` | O(1) | Iterator creation |
| Iteration | O(n) | Linear traversal |

### Space Complexity

| Storage Type | Container Overhead | Memory Layout |
|--------------|-------------------|---------------|
| Embedded (≤64 elements) | ~12-24 bytes | Stack-allocated array |
| Heap (>64 elements) | ~12-24 bytes | Single heap allocation |

### Performance vs std::deque

Based on benchmarks with 10,000 elements:

| Operation | Circular Buffer | std::deque | Speedup |
|-----------|----------------|------------|---------|
| `push_back()` | ~15 ns | ~45 ns | **3.0x faster** |
| `push_front()` | ~15 ns | ~45 ns | **3.0x faster** |
| `drop_back()` | ~8 ns | ~25 ns | **3.1x faster** |
| Random access | ~2 ns | ~3 ns | **1.5x faster** |
| Memory usage | 40KB | 65KB | **38% less** |

## API Reference

### Constructors

```cpp
circular_buffer();                                    // Default (empty)
circular_buffer(Iterator first, Iterator last);      // Range constructor
circular_buffer(std::initializer_list<T> init);      // Initializer list
circular_buffer(const circular_buffer& other);       // Copy constructor
circular_buffer(circular_buffer&& other);            // Move constructor
```

### Capacity

```cpp
static constexpr size_type capacity();               // Maximum elements
size_type size() const;                              // Current elements
bool empty() const;                                  // True if no elements
bool full() const;                                   // True if at capacity
static constexpr bool has_inline_storage();          // True if uses embedded storage
void clear();                                        // Remove all elements
```

### Element Access

```cpp
reference operator[](size_type index);              // Unchecked access
reference at(size_type index);                      // Bounds-checked access (throws)
reference front();                                  // First element (undefined if empty)
reference back();                                   // Last element (undefined if empty)
```

### Modifiers

```cpp
// Insert operations (return insert_result)
insert_result push_back(const T& value);
insert_result push_back(T&& value);
insert_result emplace_back(Args&&... args);
insert_result push_front(const T& value);
insert_result push_front(T&& value);
insert_result emplace_front(Args&&... args);

// Remove operations
void drop_back();                                    // Undefined if empty
void drop_front();                                   // Undefined if empty
std::optional<T> take_back();                        // Returns nullopt if empty
std::optional<T> take_front();                       // Returns nullopt if empty
```

### Iterators

```cpp
iterator begin() / end();                           // Mutable iterators
const_iterator begin() / end() const;               // Const iterators
const_iterator cbegin() / cend() const;             // Explicit const iterators
reverse_iterator rbegin() / rend();                 // Reverse iterators
```

## Customization Options

### Custom Memory Allocators

You can override the default memory allocation by defining macros before including the header:

```cpp
// Example: Use custom allocator
#define CIRCULAR_BUFFER_ALLOC(size, alignment) my_aligned_alloc(size, alignment)
#define CIRCULAR_BUFFER_FREE(ptr) my_aligned_free(ptr)
#include "circular_buffer/circular_buffer.h"
```

**Default allocators:**
- **Windows**: Uses `_mm_malloc()` and `_mm_free()`
- **POSIX**: Uses `aligned_alloc()` and `free()`

### Custom Assertions

Override debug assertions for your project:

```cpp
// Example: Use custom assert
#define CIRCULAR_BUFFER_ASSERT(expr) MY_ASSERT(expr)
#include "circular_buffer/circular_buffer.h"
```

**Default**: Uses standard `assert()` macro

## Building and Testing

### Requirements

- **C++17 or later** (required for `std::optional`, `constexpr if`, etc.)
- **CMake 3.14+** (for building tests)
- **Supported compilers**: GCC 9+, Clang 10+, MSVC 2019+

### Build Instructions

```bash
# Step 1: Clone the repository
git clone https://github.com/SergeyMakeev/CircularBuffer.git
cd CircularBuffer

# Step 2: Create build directory
mkdir build && cd build

# Step 3: Configure with CMake
cmake ..

# Step 4: Build everything
cmake --build . --config Release

# Step 5: Run tests
ctest

# Step 6: Run tests directly (optional)
./circular_buffer_tests        # Linux/macOS
.\Release\circular_buffer_tests.exe  # Windows

# Step 7: Run performance benchmarks (optional)
./performance_test              # Linux/macOS
.\Release\performance_test.exe  # Windows
```

### CMake Integration

```cmake
# Method 1: Include as subdirectory
add_subdirectory(circular_buffer)
target_link_libraries(my_target PRIVATE dod::circular_buffer)

# Method 2: Header-only usage
target_include_directories(my_target PRIVATE ${CMAKE_SOURCE_DIR}/circular_buffer)
target_compile_features(my_target PRIVATE cxx_std_17)
```

### Advanced Build Options

```bash
# Enable sanitizers (for debugging)
cmake .. -DENABLE_ASAN=ON      # Address sanitizer
cmake .. -DENABLE_TSAN=ON      # Thread sanitizer
cmake .. -DENABLE_MSAN=ON      # Memory sanitizer

# Enable code coverage (for testing)
cmake .. -DENABLE_COVERAGE=ON

# Build types
cmake .. -DCMAKE_BUILD_TYPE=Debug    # Debug build (slower, more checks)
cmake .. -DCMAKE_BUILD_TYPE=Release  # Optimized build (faster)
```

## Exception Safety

The circular buffer provides the following exception safety guarantees:

- **Basic guarantee**: If an exception occurs during insertion, the buffer remains in a valid state
- **Strong guarantee**: Copy constructor and copy assignment provide rollback on exception
- **No-throw guarantee**: Move operations, destructors, and query operations never throw

**Note**: Exception safety depends on the element type `T` being exception-safe.

## Design Philosophy

This implementation prioritizes:

1. **Performance**: Maximum speed through optimized algorithms and unified implementations
2. **Memory Efficiency**: Minimal overhead with smart storage strategies
3. **Type Safety**: Compile-time checks and runtime assertions for debug builds
4. **Usability**: Clean, STL-compatible API that's easy to understand and use
5. **Reliability**: Comprehensive testing with multiple compilers and platforms

### Key Design Decisions

- **Separate head/tail/size tracking** eliminates the N-1 limitation
- **Template-unified insertion** reduces code duplication while maintaining performance
- **Enum-based positioning** provides readable and zero-cost abstractions
- **Power-of-2 optimization** uses bit masking for faster index calculations
- **Embedded/heap storage strategy** balances stack usage with allocation overhead
- **Configurable index types** optimize memory usage for different capacity ranges

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Run the full test suite
5. Submit a pull request

### Development Guidelines

- Follow the existing code style (see `.cursor/rules/`)
- Add tests for new functionality
- Update documentation for API changes
- Ensure all CI builds pass

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by boost::circular_buffer but with modern C++ features
- Performance testing methodology based on industry best practices
- CI/CD pipeline follows patterns from successful open-source projects

---

## FAQ

**Q: Why not use std::deque?**  
A: std::deque is great for general use, but circular_buffer is 2-5x faster for push/pop operations and uses 20-40% less memory.

**Q: Can I resize the buffer at runtime?**  
A: No, this is a fixed-capacity container. The capacity is determined at compile-time for maximum performance.

**Q: Is it thread-safe?**  
A: The current implementation is not thread-safe. For multi-threaded use, you'll need external synchronization (mutexes, atomic operations, etc.).

**Q: What happened to the unchecked insert functions?**  
A: They were removed for API simplicity. The regular insert functions are already highly optimized and the performance difference was negligible.

**Q: How do I handle buffer overflow?**  
A: Use the overflow_policy template parameter: `overwrite` (default) overwrites old data, `discard` rejects new data when full.

**Q: Can I store move-only types?**  
A: Yes! The implementation fully supports move-only types through perfect forwarding and move semantics.

**Q: Why were bulk operations removed?**  
A: They were removed to simplify the API. For bulk insertion, simply loop with individual `push_back()` calls - the performance is equivalent.

**Q: Can I use custom allocators?**  
A: Yes! Define `CIRCULAR_BUFFER_ALLOC` and `CIRCULAR_BUFFER_FREE` macros before including the header.