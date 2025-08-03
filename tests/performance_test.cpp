#include "circular_buffer/circular_buffer.h"
#include "test_common.h"
#include <algorithm>
#include <benchmark/benchmark.h>
#include <deque>
#include <numeric>
#include <random>
#include <string>

using namespace dod;

// =============================================================================
// Test Configuration Constants
// =============================================================================

constexpr size_t SMALL_SIZE = 32;
constexpr size_t MEDIUM_SIZE = 2048;
constexpr size_t LARGE_SIZE = 1000000;

// Helper to prevent compiler optimizations
template <typename T> void DoNotOptimize(T&& value) { benchmark::DoNotOptimize(value); }

// =============================================================================
// Push/Pop Operations Benchmarks
// =============================================================================

template <typename Container> static void BM_PushBack(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        Container container;

        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
            {
                container.push_back(static_cast<int>(i));
            }
            else
            {
                container.push_back(static_cast<int>(i));
            }
        }

        DoNotOptimize(container);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Container> static void BM_PushFront(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        Container container;

        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
            {
                container.push_front(static_cast<int>(i));
            }
            else
            {
                container.push_front(static_cast<int>(i));
            }
        }

        DoNotOptimize(container);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Container> static void BM_PopBack(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        state.PauseTiming();
        Container container;

        // Pre-fill container
        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
            {
                container.push_back(static_cast<int>(i));
            }
            else
            {
                container.push_back(static_cast<int>(i));
            }
        }
        state.ResumeTiming();

        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
            {
                container.drop_back();
            }
            else
            {
                container.pop_back();
            }
        }

        DoNotOptimize(container);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// Random Access Benchmarks
// =============================================================================

template <typename Container> static void BM_RandomAccess(benchmark::State& state)
{
    const size_t size = state.range(0);

    // Pre-fill container
    Container container;
    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            container.push_back(static_cast<int>(i));
        }
        else
        {
            container.push_back(static_cast<int>(i));
        }
    }

    // Generate random indices
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, size - 1);

    for (auto _ : state)
    {
        volatile int sum = 0; // volatile to prevent optimization

        for (size_t i = 0; i < size; ++i)
        {
            size_t idx = dis(gen);
            sum += container[idx];
        }

        DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// Iterator Traversal Benchmarks
// =============================================================================

template <typename Container> static void BM_IteratorTraversal(benchmark::State& state)
{
    const size_t size = state.range(0);

    // Pre-fill container
    Container container;
    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            container.push_back(static_cast<int>(i));
        }
        else
        {
            container.push_back(static_cast<int>(i));
        }
    }

    for (auto _ : state)
    {
        volatile int sum = 0; // volatile to prevent optimization

        for (auto it = container.begin(); it != container.end(); ++it)
        {
            sum += *it;
        }

        DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Container> static void BM_RangeBasedLoop(benchmark::State& state)
{
    const size_t size = state.range(0);

    // Pre-fill container
    Container container;
    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            container.push_back(static_cast<int>(i));
        }
        else
        {
            container.push_back(static_cast<int>(i));
        }
    }

    for (auto _ : state)
    {
        volatile int sum = 0; // volatile to prevent optimization

        for (const auto& value : container)
        {
            sum += value;
        }

        DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// STL Algorithm Benchmarks
// =============================================================================

template <typename Container> static void BM_STLFind(benchmark::State& state)
{
    const size_t size = state.range(0);

    // Pre-fill container
    Container container;
    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            container.push_back(static_cast<int>(i));
        }
        else
        {
            container.push_back(static_cast<int>(i));
        }
    }

    for (auto _ : state)
    {
        auto it = std::find(container.begin(), container.end(), static_cast<int>(size / 2));
        DoNotOptimize(it);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Container> static void BM_STLAccumulate(benchmark::State& state)
{
    const size_t size = state.range(0);

    // Pre-fill container
    Container container;
    for (size_t i = 0; i < size; ++i)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            container.push_back(static_cast<int>(i));
        }
        else
        {
            container.push_back(static_cast<int>(i));
        }
    }

    for (auto _ : state)
    {
        auto sum = std::accumulate(container.begin(), container.end(), 0);
        DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// Wraparound Performance Benchmarks
// =============================================================================

template <typename CircularBuffer> static void BM_Wraparound(benchmark::State& state)
{
    const size_t capacity = 1000;
    const size_t operations = state.range(0);

    for (auto _ : state)
    {
        CircularBuffer buffer;

        // Fill buffer first
        for (size_t i = 0; i < capacity; ++i)
        {
            buffer.push_back(static_cast<int>(i));
        }

        // Now do push/pop operations that cause wraparound
        for (size_t i = 0; i < operations; ++i)
        {
            buffer.push_back(static_cast<int>(i));
            buffer.drop_front();
        }

        DoNotOptimize(buffer);
    }

    state.SetItemsProcessed(state.iterations() * operations);
}

// =============================================================================
// Construction Benchmarks
// =============================================================================

template <typename Container> static void BM_Construction(benchmark::State& state)
{
    for (auto _ : state)
    {
        Container container;
        DoNotOptimize(container);
    }
}

template <typename Container> static void BM_ConstructionWithSize(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        if constexpr (std::is_same_v<Container, circular_buffer<int, LARGE_SIZE>>)
        {
            Container container;
            for (size_t i = 0; i < size; ++i)
            {
                container.push_back(static_cast<int>(42));
            }
            DoNotOptimize(container);
        }
        else
        {
            Container container(size, 42);
            DoNotOptimize(container);
        }
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// Complex Type Benchmarks
// =============================================================================

template <typename Container> static void BM_StringOperations(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        Container container;

        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<std::string, LARGE_SIZE>>)
            {
                container.push_back("test_string_" + std::to_string(i));
            }
            else
            {
                container.push_back("test_string_" + std::to_string(i));
            }
        }

        DoNotOptimize(container);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Container> static void BM_VectorOperations(benchmark::State& state)
{
    const size_t size = state.range(0);

    for (auto _ : state)
    {
        Container container;

        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<Container, circular_buffer<std::vector<int>, LARGE_SIZE>>)
            {
                container.emplace_back(10, static_cast<int>(i));
            }
            else
            {
                container.emplace_back(10, static_cast<int>(i));
            }
        }

        DoNotOptimize(container);
    }

    state.SetItemsProcessed(state.iterations() * size);
}

// =============================================================================
// Benchmark Registration - Push/Pop Operations
// =============================================================================

BENCHMARK_TEMPLATE(BM_PushBack, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_PushBack");
BENCHMARK_TEMPLATE(BM_PushBack, std::deque<int>)->Args({SMALL_SIZE})->Args({MEDIUM_SIZE})->Args({LARGE_SIZE})->Name("StdDeque_PushBack");

BENCHMARK_TEMPLATE(BM_PushFront, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_PushFront");
BENCHMARK_TEMPLATE(BM_PushFront, std::deque<int>)->Args({SMALL_SIZE})->Args({MEDIUM_SIZE})->Args({LARGE_SIZE})->Name("StdDeque_PushFront");

BENCHMARK_TEMPLATE(BM_PopBack, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_PopBack");
BENCHMARK_TEMPLATE(BM_PopBack, std::deque<int>)->Args({SMALL_SIZE})->Args({MEDIUM_SIZE})->Args({LARGE_SIZE})->Name("StdDeque_PopBack");

// =============================================================================
// Benchmark Registration - Random Access
// =============================================================================

BENCHMARK_TEMPLATE(BM_RandomAccess, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_RandomAccess");
BENCHMARK_TEMPLATE(BM_RandomAccess, std::deque<int>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_RandomAccess");

// =============================================================================
// Benchmark Registration - Iterator Traversal
// =============================================================================

BENCHMARK_TEMPLATE(BM_IteratorTraversal, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_IteratorTraversal");
BENCHMARK_TEMPLATE(BM_IteratorTraversal, std::deque<int>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_IteratorTraversal");

BENCHMARK_TEMPLATE(BM_RangeBasedLoop, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_RangeLoop");
BENCHMARK_TEMPLATE(BM_RangeBasedLoop, std::deque<int>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_RangeLoop");

// =============================================================================
// Benchmark Registration - STL Algorithms
// =============================================================================

BENCHMARK_TEMPLATE(BM_STLFind, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_STLFind");
BENCHMARK_TEMPLATE(BM_STLFind, std::deque<int>)->Args({SMALL_SIZE})->Args({MEDIUM_SIZE})->Args({LARGE_SIZE})->Name("StdDeque_STLFind");

BENCHMARK_TEMPLATE(BM_STLAccumulate, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_STLAccumulate");
BENCHMARK_TEMPLATE(BM_STLAccumulate, std::deque<int>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_STLAccumulate");

// =============================================================================
// Benchmark Registration - Wraparound Performance
// =============================================================================

BENCHMARK_TEMPLATE(BM_Wraparound, circular_buffer<int, 1000>)->Args({LARGE_SIZE})->Name("CircularBuffer_Wraparound");

// =============================================================================
// Benchmark Registration - Construction
// =============================================================================

BENCHMARK_TEMPLATE(BM_Construction, circular_buffer<int, LARGE_SIZE>)->Name("CircularBuffer_Construction");
BENCHMARK_TEMPLATE(BM_Construction, std::deque<int>)->Name("StdDeque_Construction");

BENCHMARK_TEMPLATE(BM_ConstructionWithSize, circular_buffer<int, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_ConstructionWithSize");
BENCHMARK_TEMPLATE(BM_ConstructionWithSize, std::deque<int>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_ConstructionWithSize");

// =============================================================================
// Benchmark Registration - Complex Types
// =============================================================================

BENCHMARK_TEMPLATE(BM_StringOperations, circular_buffer<std::string, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_StringOps");
BENCHMARK_TEMPLATE(BM_StringOperations, std::deque<std::string>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_StringOps");

BENCHMARK_TEMPLATE(BM_VectorOperations, circular_buffer<std::vector<int>, LARGE_SIZE>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("CircularBuffer_VectorOps");
BENCHMARK_TEMPLATE(BM_VectorOperations, std::deque<std::vector<int>>)
    ->Args({SMALL_SIZE})
    ->Args({MEDIUM_SIZE})
    ->Args({LARGE_SIZE})
    ->Name("StdDeque_VectorOps");

BENCHMARK_MAIN();
