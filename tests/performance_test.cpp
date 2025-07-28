#include "circular_buffer/circular_buffer.h"
#include "test_common.h"
#include <chrono>
#include <deque>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>

using namespace dod;

/*

**Performance benchmarking for circular buffer**

Compares circular buffer performance against std::deque and std::vector
for various operations to validate our performance goals.

*/

class PerformanceBenchmark
{
public:
    using clock_type = std::chrono::high_resolution_clock;
    using duration_type = std::chrono::nanoseconds;
    
    template<typename Func>
    duration_type measure_operation(const std::string& name, Func&& func, size_t iterations = 1)
    {
        // Warmup
        func();
        
        auto start = clock_type::now();
        for (size_t i = 0; i < iterations; ++i) {
            func();
        }
        auto end = clock_type::now();
        
        auto total_time = std::chrono::duration_cast<duration_type>(end - start);
        auto avg_time = total_time / iterations;
        
        std::cout << std::setw(40) << name << ": " 
                  << std::setw(10) << avg_time.count() << " ns"
                  << " (total: " << total_time.count() << " ns, iterations: " << iterations << ")"
                  << std::endl;
        
        return avg_time;
    }
    
    void print_header(const std::string& test_name)
    {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << test_name << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
    
    void print_comparison(const std::string& operation, duration_type cb_time, duration_type deque_time)
    {
        double ratio = static_cast<double>(cb_time.count()) / static_cast<double>(deque_time.count());
        std::cout << std::setw(40) << (operation + " speedup") << ": " 
                  << std::setw(10) << std::fixed << std::setprecision(2) << (1.0 / ratio) << "x";
        
        if (ratio < 1.0) {
            std::cout << " (FASTER)";
        } else if (ratio > 1.0) {
            std::cout << " (SLOWER)";
        } else {
            std::cout << " (SAME)";
        }
        std::cout << std::endl;
    }
};

// Push/Pop operations benchmark
void benchmark_push_pop_operations()
{
    PerformanceBenchmark bench;
    bench.print_header("Push/Pop Operations Benchmark");
    
    constexpr size_t capacity = 10000;
    constexpr size_t iterations = 1000;
    
    // Circular buffer push_back benchmark
    auto cb_push_back_time = bench.measure_operation("circular_buffer push_back", []() {
        circular_buffer<int, 10000> buffer;
        for (size_t i = 0; i < 10000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
        }
    }, iterations);
    
    // std::deque push_back benchmark
    auto deque_push_back_time = bench.measure_operation("std::deque push_back", []() {
        std::deque<int> deque;
        for (size_t i = 0; i < 10000; ++i) {
            deque.push_back(static_cast<int>(i));
        }
    }, iterations);
    
    bench.print_comparison("push_back", cb_push_back_time, deque_push_back_time);
    
    // Circular buffer push_front benchmark
    auto cb_push_front_time = bench.measure_operation("circular_buffer push_front", []() {
        circular_buffer<int, 10000> buffer;
        for (size_t i = 0; i < 10000; ++i) {
            buffer.push_front_unchecked(static_cast<int>(i));
        }
    }, iterations);
    
    // std::deque push_front benchmark
    auto deque_push_front_time = bench.measure_operation("std::deque push_front", []() {
        std::deque<int> deque;
        for (size_t i = 0; i < 10000; ++i) {
            deque.push_front(static_cast<int>(i));
        }
    }, iterations);
    
    bench.print_comparison("push_front", cb_push_front_time, deque_push_front_time);
    
    // Pop operations (pre-filled containers)
    circular_buffer<int, 10000> filled_buffer;
    std::deque<int> filled_deque;
    
    for (size_t i = 0; i < 10000; ++i) {
        filled_buffer.push_back_unchecked(static_cast<int>(i));
        filled_deque.push_back(static_cast<int>(i));
    }
    
    auto cb_pop_back_time = bench.measure_operation("circular_buffer pop_back", [&]() {
        auto buffer_copy = filled_buffer;
        while (!buffer_copy.empty()) {
            buffer_copy.pop_back();
        }
    }, iterations);
    
    auto deque_pop_back_time = bench.measure_operation("std::deque pop_back", [&]() {
        auto deque_copy = filled_deque;
        while (!deque_copy.empty()) {
            deque_copy.pop_back();
        }
    }, iterations);
    
    bench.print_comparison("pop_back", cb_pop_back_time, deque_pop_back_time);
}

// Random access benchmark
void benchmark_random_access()
{
    PerformanceBenchmark bench;
    bench.print_header("Random Access Benchmark");
    
    constexpr size_t capacity = 10000;
    constexpr size_t access_count = 100000;
    constexpr size_t iterations = 100;
    
    // Prepare containers
    circular_buffer<int, 10000> buffer;
    std::deque<int> deque;
    std::vector<int> vector;
    
    for (size_t i = 0; i < capacity; ++i) {
        int value = static_cast<int>(i);
        buffer.push_back_unchecked(value);
        deque.push_back(value);
        vector.push_back(value);
    }
    
    // Random indices for access
    std::vector<size_t> random_indices;
    random_indices.reserve(access_count);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, capacity - 1);
    
    for (size_t i = 0; i < access_count; ++i) {
        random_indices.push_back(dist(gen));
    }
    
    // Circular buffer random access
    auto cb_access_time = bench.measure_operation("circular_buffer operator[]", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (size_t idx : random_indices) {
            sum += buffer[idx];
        }
    }, iterations);
    
    // std::deque random access
    auto deque_access_time = bench.measure_operation("std::deque operator[]", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (size_t idx : random_indices) {
            sum += deque[idx];
        }
    }, iterations);
    
    // std::vector random access (baseline)
    auto vector_access_time = bench.measure_operation("std::vector operator[]", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (size_t idx : random_indices) {
            sum += vector[idx];
        }
    }, iterations);
    
    bench.print_comparison("random access vs deque", cb_access_time, deque_access_time);
    bench.print_comparison("random access vs vector", cb_access_time, vector_access_time);
}

// Iterator traversal benchmark
void benchmark_iterator_traversal()
{
    PerformanceBenchmark bench;
    bench.print_header("Iterator Traversal Benchmark");
    
    constexpr size_t capacity = 10000;
    constexpr size_t iterations = 1000;
    
    // Prepare containers
    circular_buffer<int, 10000> buffer;
    std::deque<int> deque;
    std::vector<int> vector;
    
    for (size_t i = 0; i < capacity; ++i) {
        int value = static_cast<int>(i);
        buffer.push_back_unchecked(value);
        deque.push_back(value);
        vector.push_back(value);
    }
    
    // Circular buffer iterator traversal
    auto cb_iter_time = bench.measure_operation("circular_buffer iteration", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            sum += *it;
        }
    }, iterations);
    
    // std::deque iterator traversal
    auto deque_iter_time = bench.measure_operation("std::deque iteration", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (auto it = deque.begin(); it != deque.end(); ++it) {
            sum += *it;
        }
    }, iterations);
    
    // std::vector iterator traversal
    auto vector_iter_time = bench.measure_operation("std::vector iteration", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (auto it = vector.begin(); it != vector.end(); ++it) {
            sum += *it;
        }
    }, iterations);
    
    bench.print_comparison("iteration vs deque", cb_iter_time, deque_iter_time);
    bench.print_comparison("iteration vs vector", cb_iter_time, vector_iter_time);
    
    // Range-based for loop
    auto cb_range_time = bench.measure_operation("circular_buffer range-for", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (const auto& value : buffer) {
            sum += value;
        }
    }, iterations);
    
    auto deque_range_time = bench.measure_operation("std::deque range-for", [&]() {
        volatile int sum = 0;  // volatile to prevent optimization
        for (const auto& value : deque) {
            sum += value;
        }
    }, iterations);
    
    bench.print_comparison("range-for vs deque", cb_range_time, deque_range_time);
}

// Memory usage benchmark
void benchmark_memory_usage()
{
    PerformanceBenchmark bench;
    bench.print_header("Memory Usage Analysis");
    
    constexpr size_t capacity = 1000;
    constexpr size_t embedded_threshold = capacity + 1;
    
    // Object sizes
    size_t cb_embedded_size = sizeof(circular_buffer<int, capacity, uint32_t, alignof(int), embedded_threshold>);
    size_t cb_heap_size = sizeof(circular_buffer<int, capacity, uint32_t, alignof(int), 1>);
    size_t deque_size = sizeof(std::deque<int>);
    size_t vector_size = sizeof(std::vector<int>);
    
    std::cout << "Container object sizes:" << std::endl;
    std::cout << std::setw(40) << "circular_buffer (embedded)" << ": " << cb_embedded_size << " bytes" << std::endl;
    std::cout << std::setw(40) << "circular_buffer (heap)" << ": " << cb_heap_size << " bytes" << std::endl;
    std::cout << std::setw(40) << "std::deque" << ": " << deque_size << " bytes" << std::endl;
    std::cout << std::setw(40) << "std::vector" << ": " << vector_size << " bytes" << std::endl;
    
    std::cout << "\nMemory efficiency (smaller is better):" << std::endl;
    std::cout << std::setw(40) << "CB embedded vs deque ratio" << ": " 
              << std::fixed << std::setprecision(2) 
              << static_cast<double>(cb_embedded_size) / static_cast<double>(deque_size) << "x" << std::endl;
    std::cout << std::setw(40) << "CB heap vs deque ratio" << ": " 
              << std::fixed << std::setprecision(2) 
              << static_cast<double>(cb_heap_size) / static_cast<double>(deque_size) << "x" << std::endl;
}

// Wraparound performance test
void benchmark_wraparound_performance()
{
    PerformanceBenchmark bench;
    bench.print_header("Wraparound Performance Test");
    
    constexpr size_t capacity = 1000;
    constexpr size_t operations = 100000;
    constexpr size_t iterations = 100;
    
    // Circular buffer with heavy wraparound
    auto cb_wraparound_time = bench.measure_operation("circular_buffer wraparound", []() {
        circular_buffer<int, 1000> buffer;
        
        // Fill buffer first
        for (size_t i = 0; i < 1000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
        }
        
        // Now do push/pop operations that cause wraparound
        for (size_t i = 0; i < 100000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            buffer.pop_front();
        }
    }, iterations);
    
    // std::deque equivalent operations
    auto deque_equivalent_time = bench.measure_operation("std::deque equivalent", []() {
        std::deque<int> deque;
        
        // Fill deque first
        for (size_t i = 0; i < 1000; ++i) {
            deque.push_back(static_cast<int>(i));
        }
        
        // Do push/pop operations
        for (size_t i = 0; i < 100000; ++i) {
            deque.push_back(static_cast<int>(i));
            deque.pop_front();
        }
    }, iterations);
    
    bench.print_comparison("wraparound operations", cb_wraparound_time, deque_equivalent_time);
}

// Construction/destruction benchmark
void benchmark_construction_destruction()
{
    PerformanceBenchmark bench;
    bench.print_header("Construction/Destruction Benchmark");
    
    constexpr size_t capacity = 1000;
    constexpr size_t embedded_threshold = capacity + 1;
    constexpr size_t iterations = 10000;
    
    // Circular buffer construction (embedded)
    auto cb_embedded_ctor_time = bench.measure_operation("circular_buffer ctor (embedded)", []() {
        circular_buffer<int, 1000, uint32_t, alignof(int), 1001> buffer;
        // Add some elements to avoid optimization
        buffer.push_back_unchecked(42);
    }, iterations);
    
    // Circular buffer construction (heap)
    auto cb_heap_ctor_time = bench.measure_operation("circular_buffer ctor (heap)", []() {
        circular_buffer<int, 1000, uint32_t, alignof(int), 1> buffer;
        // Add some elements to avoid optimization
        buffer.push_back_unchecked(42);
    }, iterations);
    
    // std::deque construction
    auto deque_ctor_time = bench.measure_operation("std::deque ctor", []() {
        std::deque<int> deque;
        // Add some elements to avoid optimization
        deque.push_back(42);
    }, iterations);
    
    // std::vector construction
    auto vector_ctor_time = bench.measure_operation("std::vector ctor", []() {
        std::vector<int> vector;
        // Add some elements to avoid optimization
        vector.push_back(42);
    }, iterations);
    
    bench.print_comparison("embedded ctor vs deque", cb_embedded_ctor_time, deque_ctor_time);
    bench.print_comparison("heap ctor vs deque", cb_heap_ctor_time, deque_ctor_time);
}

// Index type performance comparison
void benchmark_index_types()
{
    PerformanceBenchmark bench;
    bench.print_header("Index Type Performance Comparison");
    
    constexpr size_t capacity = 1000;
    constexpr size_t operations = 100000;
    constexpr size_t iterations = 100;
    
    // uint8_t indices
    auto uint8_time = bench.measure_operation("uint8_t indices", []() {
        circular_buffer<int, 1000, uint8_t> buffer;
        for (size_t i = 0; i < std::min(static_cast<size_t>(100000), static_cast<size_t>(255)); ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    // uint16_t indices
    auto uint16_time = bench.measure_operation("uint16_t indices", []() {
        circular_buffer<int, 1000, uint16_t> buffer;
        for (size_t i = 0; i < 100000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    // uint32_t indices
    auto uint32_time = bench.measure_operation("uint32_t indices", []() {
        circular_buffer<int, 1000, uint32_t> buffer;
        for (size_t i = 0; i < 100000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    // uint64_t indices
    auto uint64_time = bench.measure_operation("uint64_t indices", []() {
        circular_buffer<int, 1000, uint64_t> buffer;
        for (size_t i = 0; i < 100000; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    std::cout << "\nIndex type performance comparison (relative to uint32_t):" << std::endl;
    bench.print_comparison("uint8_t vs uint32_t", uint8_time, uint32_time);
    bench.print_comparison("uint16_t vs uint32_t", uint16_time, uint32_time);
    bench.print_comparison("uint64_t vs uint32_t", uint64_time, uint32_time);
}

// Power-of-2 vs non-power-of-2 capacity benchmark
void benchmark_capacity_optimization()
{
    PerformanceBenchmark bench;
    bench.print_header("Capacity Optimization Benchmark");
    
    constexpr size_t operations = 100000;
    constexpr size_t iterations = 100;
    
    // Power of 2 capacity (should use bit masking)
    auto pow2_time = bench.measure_operation("Power-of-2 capacity (1024)", [&]() {
        circular_buffer<int, 1024> buffer;
        for (size_t i = 0; i < operations; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    // Non-power of 2 capacity (uses modulo)
    auto non_pow2_time = bench.measure_operation("Non-power-of-2 capacity (1000)", [&]() {
        circular_buffer<int, 1000> buffer;
        for (size_t i = 0; i < operations; ++i) {
            buffer.push_back_unchecked(static_cast<int>(i));
            if (buffer.full()) buffer.pop_front();
        }
    }, iterations);
    
    bench.print_comparison("power-of-2 optimization", pow2_time, non_pow2_time);
}

// Complex type performance test
void benchmark_complex_types()
{
    PerformanceBenchmark bench;
    bench.print_header("Complex Type Performance Test");
    
    constexpr size_t capacity = 1000;
    constexpr size_t iterations = 1000;
    
    // String operations
    auto cb_string_time = bench.measure_operation("circular_buffer<string> ops", []() {
        circular_buffer<std::string, 1000> buffer;
        for (size_t i = 0; i < 1000; ++i) {
            buffer.push_back_unchecked("test_string_" + std::to_string(i));
        }
        buffer.clear();
    }, iterations);
    
    auto deque_string_time = bench.measure_operation("std::deque<string> ops", []() {
        std::deque<std::string> deque;
        for (size_t i = 0; i < 1000; ++i) {
            deque.push_back("test_string_" + std::to_string(i));
        }
        deque.clear();
    }, iterations);
    
    bench.print_comparison("string operations", cb_string_time, deque_string_time);
    
    // Vector operations
    auto cb_vector_time = bench.measure_operation("circular_buffer<vector> ops", []() {
        circular_buffer<std::vector<int>, 1000> buffer;
        for (size_t i = 0; i < 1000; ++i) {
            buffer.emplace_back_unchecked(10, static_cast<int>(i));
        }
        buffer.clear();
    }, iterations);
    
    auto deque_vector_time = bench.measure_operation("std::deque<vector> ops", []() {
        std::deque<std::vector<int>> deque;
        for (size_t i = 0; i < 1000; ++i) {
            deque.emplace_back(10, static_cast<int>(i));
        }
        deque.clear();
    }, iterations);
    
    bench.print_comparison("vector operations", cb_vector_time, deque_vector_time);
}

int main()
{
    std::cout << "Circular Buffer Performance Benchmarks" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Note: Timings may vary based on system load and compiler optimizations." << std::endl;
    std::cout << "Performance ratios > 1.0x indicate circular_buffer is faster." << std::endl;
    
    benchmark_push_pop_operations();
    benchmark_random_access();
    benchmark_iterator_traversal();
    benchmark_memory_usage();
    benchmark_wraparound_performance();
    benchmark_construction_destruction();
    benchmark_index_types();
    benchmark_capacity_optimization();
    benchmark_complex_types();
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "Performance benchmarking completed!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    return 0;
} 