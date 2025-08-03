#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#define CIRCULAR_BUFFER_INLINE inline

// TODO: you could override memory allocator by defining CIRCULAR_BUFFER_ALLOC/CIRCULAR_BUFFER_FREE macros
#if !defined(CIRCULAR_BUFFER_ALLOC) || !defined(CIRCULAR_BUFFER_FREE)

#if defined(_WIN32)
// Windows
#include <xmmintrin.h>
#define CIRCULAR_BUFFER_ALLOC(sizeInBytes, alignment) _mm_malloc(sizeInBytes, alignment)
#define CIRCULAR_BUFFER_FREE(ptr) _mm_free(ptr)
#else
// Posix
#include <stdlib.h>
// Helper to round up size to be a multiple of alignment (required by aligned_alloc)
inline size_t round_up_to_alignment(size_t size, size_t alignment) noexcept { 
    return ((size + alignment - 1) / alignment) * alignment; 
}
#define CIRCULAR_BUFFER_ALLOC(sizeInBytes, alignment) aligned_alloc(alignment, round_up_to_alignment(sizeInBytes, alignment))
#define CIRCULAR_BUFFER_FREE(ptr) free(ptr)
#endif

#endif

// TODO: you could override asserts by defining CIRCULAR_BUFFER_ASSERT macro
#if !defined(CIRCULAR_BUFFER_ASSERT)
#include <assert.h>
#define CIRCULAR_BUFFER_ASSERT(expression) assert(expression)
#endif

// Helper macro to suppress unused parameter warnings
#if !defined(CIRCULAR_BUFFER_MAYBE_UNUSED)
#define CIRCULAR_BUFFER_MAYBE_UNUSED(x) (void)(x)
#endif

// Iterator debugging support similar to Microsoft STL
#if !defined(CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL)
#if defined(_ITERATOR_DEBUG_LEVEL)
#define CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL _ITERATOR_DEBUG_LEVEL
#else
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL 2
#else
#define CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL 0
#endif
#endif
#endif

#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
#define CIRCULAR_BUFFER_VERIFY_ITERATOR(iter) (iter)._verify_valid(__FILE__, __LINE__)
#define CIRCULAR_BUFFER_VERIFY_ITERATOR_RANGE(first, last)                                           \
    do                                                                                              \
    {                                                                                               \
        CIRCULAR_BUFFER_VERIFY_ITERATOR(first);                                                    \
        CIRCULAR_BUFFER_VERIFY_ITERATOR(last);                                                     \
        CIRCULAR_BUFFER_ASSERT((first).m_container == (last).m_container && "Iterators from different containers"); \
        CIRCULAR_BUFFER_ASSERT((first).m_index <= (last).m_index && "Invalid iterator range");    \
    } while (0)
#else
#define CIRCULAR_BUFFER_VERIFY_ITERATOR(iter) ((void)0)
#define CIRCULAR_BUFFER_VERIFY_ITERATOR_RANGE(first, last) ((void)0)
#endif

namespace dod
{

template <typename U, typename... Args> static U* construct(void* ptr, Args&&... args)
{
    U* p = new (ptr) U(std::forward<Args>(args)...);
    return std::launder(p);
}

template <typename U> static void destruct(U* ptr) { ptr->~U(); }

template <typename U, typename... Args> static U* reconstruct(U* ptr, Args&&... args)
{
    ptr->~U();
    U* p = new (ptr) U(std::forward<Args>(args)...);
    return std::launder(p);
}

// Helper to ensure alignment is at least alignof(void*) for compatibility with macOS.
template <typename T> inline constexpr size_t safe_alignment_of = 
    alignof(T) < alignof(void*) ? alignof(void*) : alignof(T);

} // namespace dod

namespace dod
{

/*

**Overflow handling policies for circular buffer**

*/
enum class overflow_policy
{
    overwrite,  // Overwrite oldest elements when full (default)
    discard     // Discard new elements when full
};

/*

**Result of insert operations**

Provides feedback about what happened during insertion operations.

*/
enum class insert_result
{
    inserted,     // Successfully inserted
    overwritten,  // Inserted by overwriting oldest element
    discarded     // Element was discarded (buffer full + discard policy)
};

/*

**Result statistics for bulk insert operations**

*/
template<typename SizeType>
struct bulk_insert_result
{
    SizeType inserted_count = 0;
    SizeType overwritten_count = 0;
    SizeType discarded_count = 0;
};

/*

**High-performance circular buffer implementation**

A fixed-capacity circular buffer that stores exactly N elements (no typical N-1 size limitation)
Uses separate head/tail/size tracking for full capacity utilization.

Key features:
- O(1) push/pop operations at both ends
- STL-compatible interface with iterators
- Configurable storage strategy (inline vs heap)
- Configurable index types for memory optimization
- Custom alignment support
- Overflow policies with result feedback

Template parameters:
- T: Element type
- Capacity: Fixed buffer size (> 0)
- IndexType: Type for indices and size (default: uint32_t)
- Alignment: Memory alignment (default: alignof(T))
- InlineThreshold: Cutoff for inline vs heap storage (default: 64 elements)
- Policy: Overflow behavior (default: overwrite)

*/
template<typename T,
         size_t Capacity,
         overflow_policy Policy = overflow_policy::overwrite,
         typename IndexType = uint32_t,
         size_t InlineThreshold = 64,
         size_t Alignment = alignof(T)>
class circular_buffer
{
    static_assert(std::is_unsigned_v<IndexType>, "IndexType must be unsigned integer");
    static_assert(Capacity > 0, "Capacity must be greater than 0");
    static_assert(Capacity <= std::numeric_limits<IndexType>::max(), "Capacity too large for IndexType");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be power of 2");
    static_assert(Alignment >= alignof(T), "Alignment must be at least alignof(T)");

public:
    using value_type = T;
    using size_type = IndexType;
    using index_type = IndexType;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using difference_type = std::ptrdiff_t;

    template<bool IsConst> class iterator_impl;
    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    
    // inline storage
    struct alignas(Alignment) inline_storage_t {
        std::array<std::byte, sizeof(T) * Capacity> data;
    };

    // heap storage
    using heap_storage_t = T*;

    // Storage strategy selection
    static constexpr bool uses_inline_storage = (Capacity <= InlineThreshold);
    using storage_t = std::conditional_t<uses_inline_storage, inline_storage_t, heap_storage_t>;

    storage_t m_storage;
    IndexType m_head = 0;      // Write position (next insert location)
    IndexType m_tail = 0;      // Read position (next remove location)
    IndexType m_size = 0;      // Current number of elements

#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
    mutable void* m_iterator_list = nullptr;  // Debug iterator tracking
#endif

    // Helper methods
    T* get_storage() noexcept
    {
        if constexpr (uses_inline_storage) {
            return reinterpret_cast<T*>(m_storage.data.data());
        } else {
            return m_storage;
        }
    }

    const T* get_storage() const noexcept
    {
        if constexpr (uses_inline_storage) {
            return reinterpret_cast<const T*>(m_storage.data.data());
        } else {
            return m_storage;
        }
    }

    // Index manipulation with power-of-2 optimization
    constexpr IndexType next_index(IndexType index) const noexcept
    {
        if constexpr ((Capacity & (Capacity - 1)) == 0) {
            // Power of 2: use bit masking (faster)
            return (index + 1) & (Capacity - 1);
        } else {
            // Generic: use modulo
            return (index + 1 < Capacity) ? index + 1 : 0;
        }
    }

    constexpr IndexType prev_index(IndexType index) const noexcept
    {
        if constexpr ((Capacity & (Capacity - 1)) == 0) {
            // Power of 2: use bit masking (faster)
            return (index - 1) & (Capacity - 1);
        } else {
            // Generic: use modulo
            return (index > 0) ? index - 1 : Capacity - 1;
        }
    }

    constexpr IndexType add_index(IndexType index, IndexType offset) const noexcept
    {
        if constexpr ((Capacity & (Capacity - 1)) == 0) {
            // Power of 2: use bit masking (faster)
            return (index + offset) & (Capacity - 1);
        } else {
            // Generic: use modulo
            return (index + offset) % Capacity;
        }
    }

    // Get physical index from logical index
    IndexType physical_index(IndexType logical_index) const noexcept
    {
        CIRCULAR_BUFFER_ASSERT(logical_index < m_size);
        return add_index(m_tail, logical_index);
    }

    // Core implementation for push operations
    template<typename U>
    insert_result push_back_impl(U&& value)
    {
        if constexpr (Policy == overflow_policy::discard) {
            if (full()) {
                return insert_result::discarded;
            }
        }

        bool was_full = full();
        T* storage = get_storage();

        if (was_full) {
            // Overwrite mode: reuse existing slot
            CIRCULAR_BUFFER_ASSERT(Policy == overflow_policy::overwrite);
            dod::reconstruct(&storage[m_head], std::forward<U>(value));
            m_head = next_index(m_head);
            m_tail = next_index(m_tail);  // Move tail to maintain size
            return insert_result::overwritten;
        } else {
            // Normal insertion
            dod::construct<T>(&storage[m_head], std::forward<U>(value));
            m_head = next_index(m_head);
            ++m_size;
            return insert_result::inserted;
        }
    }

    template<typename U>
    insert_result push_front_impl(U&& value)
    {
        if constexpr (Policy == overflow_policy::discard) {
            if (full()) {
                return insert_result::discarded;
            }
        }

        bool was_full = full();
        T* storage = get_storage();

        if (was_full) {
            // Overwrite mode: reuse existing slot
            CIRCULAR_BUFFER_ASSERT(Policy == overflow_policy::overwrite);
            m_head = prev_index(m_head);  // Move head back to maintain size
            m_tail = prev_index(m_tail);
            dod::reconstruct(&storage[m_tail], std::forward<U>(value));
            return insert_result::overwritten;
        } else {
            // Normal insertion
            m_tail = prev_index(m_tail);
            dod::construct<T>(&storage[m_tail], std::forward<U>(value));
            ++m_size;
            return insert_result::inserted;
        }
    }

    // Core implementation for emplace operations
    template<typename... Args>
    insert_result emplace_back_impl(Args&&... args)
    {
        if constexpr (Policy == overflow_policy::discard) {
            if (full()) {
                return insert_result::discarded;
            }
        }

        bool was_full = full();
        T* storage = get_storage();

        if (was_full) {
            // Overwrite mode: reuse existing slot
            CIRCULAR_BUFFER_ASSERT(Policy == overflow_policy::overwrite);
            dod::destruct(&storage[m_head]);
            dod::construct<T>(&storage[m_head], std::forward<Args>(args)...);
            m_head = next_index(m_head);
            m_tail = next_index(m_tail);  // Move tail to maintain size
            return insert_result::overwritten;
        } else {
            // Normal insertion
            dod::construct<T>(&storage[m_head], std::forward<Args>(args)...);
            m_head = next_index(m_head);
            ++m_size;
            return insert_result::inserted;
        }
    }

    template<typename... Args>
    insert_result emplace_front_impl(Args&&... args)
    {
        if constexpr (Policy == overflow_policy::discard) {
            if (full()) {
                return insert_result::discarded;
            }
        }

        bool was_full = full();
        T* storage = get_storage();

        if (was_full) {
            // Overwrite mode: reuse existing slot
            CIRCULAR_BUFFER_ASSERT(Policy == overflow_policy::overwrite);
            m_head = prev_index(m_head);  // Move head back to maintain size
            m_tail = prev_index(m_tail);
            dod::destruct(&storage[m_tail]);
            dod::construct<T>(&storage[m_tail], std::forward<Args>(args)...);
            return insert_result::overwritten;
        } else {
            // Normal insertion
            m_tail = prev_index(m_tail);
            dod::construct<T>(&storage[m_tail], std::forward<Args>(args)...);
            ++m_size;
            return insert_result::inserted;
        }
    }

    void destroy_all() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            T* storage = get_storage();
            IndexType current = m_tail;
            for (IndexType i = 0; i < m_size; ++i) {
                dod::destruct(&storage[current]);
                current = next_index(current);
            }
        }
        m_size = 0;
        m_head = 0;
        m_tail = 0;
    }

    void allocate_storage()
    {
        if constexpr (!uses_inline_storage) {
            m_storage = static_cast<T*>(CIRCULAR_BUFFER_ALLOC(sizeof(T) * Capacity, Alignment));
            CIRCULAR_BUFFER_ASSERT(m_storage != nullptr);
        }
    }

    void deallocate_storage() noexcept
    {
        if constexpr (!uses_inline_storage) {
            if (m_storage) {
                CIRCULAR_BUFFER_FREE(m_storage);
                m_storage = nullptr;
            }
        }
    }

public:
    // Returns the fixed capacity
    static constexpr size_type capacity() noexcept { return Capacity; }

    // Returns whether inline storage is used
    static constexpr bool uses_inline_storage() noexcept { return uses_inline_storage; }

    /*

    **Default constructor**

    Creates an empty circular buffer.

    */
    circular_buffer()
#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
        : m_iterator_list(nullptr)
#endif
    {
        allocate_storage();
    }

    /*

    **Fill constructor**

    Creates a circular buffer filled with copies of the given value.
    If the buffer capacity is exceeded, behavior depends on overflow policy.

    */
    explicit circular_buffer(const T& value)
        : circular_buffer()
    {
        for (size_type i = 0; i < Capacity; ++i) {
            push_back_unchecked(value);
        }
    }

    /*

    **Range constructor**

    Creates a circular buffer from a range of elements.
    If the range contains more elements than capacity, behavior depends on overflow policy.

    */
    template<typename Iterator>
    circular_buffer(Iterator first, Iterator last)
        : circular_buffer()
    {
        for (auto it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    /*

    **Initializer list constructor**

    Creates a circular buffer from an initializer list.
    If the list contains more elements than capacity, behavior depends on overflow policy.

    */
    circular_buffer(std::initializer_list<T> init)
        : circular_buffer(init.begin(), init.end())
    {
    }

    /*

    **Copy constructor**

    Creates a copy of another circular buffer.

    */
    circular_buffer(const circular_buffer& other)
        : circular_buffer()
    {
        // Copy elements maintaining order
        const T* other_storage = other.get_storage();
        T* storage = get_storage();
        
        IndexType other_current = other.m_tail;
        for (IndexType i = 0; i < other.m_size; ++i) {
            dod::construct<T>(&storage[m_head], other_storage[other_current]);
            m_head = next_index(m_head);
            other_current = other.next_index(other_current);
        }
        m_size = other.m_size;
    }

    /*

    **Move constructor**

    Takes ownership of another circular buffer's contents.

    */
    circular_buffer(circular_buffer&& other) noexcept
#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
        : m_iterator_list(nullptr)
#endif
    {
        if constexpr (uses_inline_storage) {
            // Inline storage: must move elements individually
            allocate_storage();
            T* storage = get_storage();
            T* other_storage = other.get_storage();
            
            IndexType other_current = other.m_tail;
            for (IndexType i = 0; i < other.m_size; ++i) {
                dod::construct<T>(&storage[m_head], std::move(other_storage[other_current]));
                m_head = next_index(m_head);
                other_current = other.next_index(other_current);
            }
            m_size = other.m_size;
            other.destroy_all();
        } else {
            // Heap storage: steal the pointer
            m_storage = other.m_storage;
            m_head = other.m_head;
            m_tail = other.m_tail;
            m_size = other.m_size;
            
            other.m_storage = nullptr;
            other.m_head = 0;
            other.m_tail = 0;
            other.m_size = 0;
        }
    }

    /*

    **Destructor**

    Destroys all elements and deallocates storage.

    */
    ~circular_buffer()
    {
        destroy_all();
        deallocate_storage();
    }

    /*

    **Copy assignment operator**

    */
    circular_buffer& operator=(const circular_buffer& other)
    {
        if (this != &other) {
            destroy_all();
            
            const T* other_storage = other.get_storage();
            T* storage = get_storage();
            
            IndexType other_current = other.m_tail;
            for (IndexType i = 0; i < other.m_size; ++i) {
                dod::construct<T>(&storage[m_head], other_storage[other_current]);
                m_head = next_index(m_head);
                other_current = other.next_index(other_current);
            }
            m_size = other.m_size;
        }
        return *this;
    }

    /*

    **Move assignment operator**

    */
    circular_buffer& operator=(circular_buffer&& other) noexcept
    {
        if (this != &other) {
            destroy_all();
            
            if constexpr (uses_inline_storage) {
                // Inline storage: must move elements individually
                T* storage = get_storage();
                T* other_storage = other.get_storage();
                
                IndexType other_current = other.m_tail;
                for (IndexType i = 0; i < other.m_size; ++i) {
                    dod::construct<T>(&storage[m_head], std::move(other_storage[other_current]));
                    m_head = next_index(m_head);
                    other_current = other.next_index(other_current);
                }
                m_size = other.m_size;
                other.destroy_all();
            } else {
                // Heap storage: steal the pointer
                deallocate_storage();
                
                m_storage = other.m_storage;
                m_head = other.m_head;
                m_tail = other.m_tail;
                m_size = other.m_size;
                
                other.m_storage = nullptr;
                other.m_head = 0;
                other.m_tail = 0;
                other.m_size = 0;
            }
        }
        return *this;
    }

    // Capacity and size queries
    size_type size() const noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0; }
    bool full() const noexcept { return m_size == Capacity; }

    /*

    **Clear all elements**

    Destroys all elements and resets the buffer to empty state.

    */
    void clear() noexcept
    {
        destroy_all();
    }

    // Element access
    reference operator[](size_type index) noexcept
    {
        CIRCULAR_BUFFER_ASSERT(index < m_size);
        return get_storage()[physical_index(index)];
    }

    const_reference operator[](size_type index) const noexcept
    {
        CIRCULAR_BUFFER_ASSERT(index < m_size);
        return get_storage()[physical_index(index)];
    }

    reference at(size_type index)
    {
        if (index >= m_size) {
            throw std::out_of_range("circular_buffer index out of range");
        }
        return get_storage()[physical_index(index)];
    }

    const_reference at(size_type index) const
    {
        if (index >= m_size) {
            throw std::out_of_range("circular_buffer index out of range");
        }
        return get_storage()[physical_index(index)];
    }

    reference front() noexcept
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        return get_storage()[m_tail];
    }

    const_reference front() const noexcept
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        return get_storage()[m_tail];
    }

    reference back() noexcept
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        return get_storage()[prev_index(m_head)];
    }

    const_reference back() const noexcept
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        return get_storage()[prev_index(m_head)];
    }

    // Insert operations with result feedback
    insert_result push_back(const T& value)
    {
        return push_back_impl(value);
    }

    insert_result push_back(T&& value)
    {
        return push_back_impl(std::move(value));
    }

    template<typename... Args>
    insert_result emplace_back(Args&&... args)
    {
        return emplace_back_impl(std::forward<Args>(args)...);
    }

    insert_result push_front(const T& value)
    {
        return push_front_impl(value);
    }

    insert_result push_front(T&& value)
    {
        return push_front_impl(std::move(value));
    }

    template<typename... Args>
    insert_result emplace_front(Args&&... args)
    {
        return emplace_front_impl(std::forward<Args>(args)...);
    }

    // Traditional interface (ignores overflow result)
    void push_back_unchecked(const T& value)
    {
        push_back_impl(value);
    }

    void push_back_unchecked(T&& value)
    {
        push_back_impl(std::move(value));
    }

    template<typename... Args>
    void emplace_back_unchecked(Args&&... args)
    {
        emplace_back_impl(std::forward<Args>(args)...);
    }

    void push_front_unchecked(const T& value)
    {
        push_front_impl(value);
    }

    void push_front_unchecked(T&& value)
    {
        push_front_impl(std::move(value));
    }

    template<typename... Args>
    void emplace_front_unchecked(Args&&... args)
    {
        emplace_front_impl(std::forward<Args>(args)...);
    }

    // Remove operations
    void pop_back()
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        m_head = prev_index(m_head);
        dod::destruct(&get_storage()[m_head]);
        --m_size;
    }

    void pop_front()
    {
        CIRCULAR_BUFFER_ASSERT(!empty());
        dod::destruct(&get_storage()[m_tail]);
        m_tail = next_index(m_tail);
        --m_size;
    }

    // Safe interface with optional return
    std::optional<T> pop_back_safe()
    {
        if (empty()) {
            return std::nullopt;
        }
        
        T result = std::move(back());
        pop_back();
        return result;
    }

    std::optional<T> pop_front_safe()
    {
        if (empty()) {
            return std::nullopt;
        }
        
        T result = std::move(front());
        pop_front();
        return result;
    }

    // Convenience alias for optional interface
    std::optional<T> pop() { return pop_back_safe(); }

    // Bulk operations
    template<typename Iterator>
    bulk_insert_result<size_type> push_back_range(Iterator first, Iterator last)
    {
        bulk_insert_result<size_type> result;
        
        for (auto it = first; it != last; ++it) {
            auto insert_res = push_back(*it);
            switch (insert_res) {
                case insert_result::inserted:
                    ++result.inserted_count;
                    break;
                case insert_result::overwritten:
                    ++result.overwritten_count;
                    break;
                case insert_result::discarded:
                    ++result.discarded_count;
                    break;
            }
        }
        
        return result;
    }

    template<typename Iterator>
    bulk_insert_result<size_type> push_front_range(Iterator first, Iterator last)
    {
        bulk_insert_result<size_type> result;
        
        for (auto it = first; it != last; ++it) {
            auto insert_res = push_front(*it);
            switch (insert_res) {
                case insert_result::inserted:
                    ++result.inserted_count;
                    break;
                case insert_result::overwritten:
                    ++result.overwritten_count;
                    break;
                case insert_result::discarded:
                    ++result.discarded_count;
                    break;
            }
        }
        
        return result;
    }

    // Iterator accessors
    iterator begin() noexcept { return iterator(this, 0); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator cbegin() const noexcept { return const_iterator(this, 0); }

    iterator end() noexcept { return iterator(this, m_size); }
    const_iterator end() const noexcept { return const_iterator(this, m_size); }
    const_iterator cend() const noexcept { return const_iterator(this, m_size); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
    void _invalidate_iterators() const noexcept
    {
        // TODO: Implement iterator invalidation for debugging
    }
#endif

public:
    /*

    **Bidirectional iterator implementation**

    Template-unified implementation for both const and non-const iterators.
    Provides efficient random access to circular buffer elements.

    */
    template<bool IsConst>
    class iterator_impl
    {
        template<bool IC> friend class iterator_impl;
        friend class circular_buffer;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;

    private:
        using container_pointer = std::conditional_t<IsConst, const circular_buffer*, circular_buffer*>;
        
        container_pointer m_container;
        IndexType m_index;  // Logical index

#if CIRCULAR_BUFFER_ITERATOR_DEBUG_LEVEL > 0
        void _verify_valid(const char* file, int line) const
        {
            CIRCULAR_BUFFER_MAYBE_UNUSED(file);
            CIRCULAR_BUFFER_MAYBE_UNUSED(line);
            CIRCULAR_BUFFER_ASSERT(m_container != nullptr && "Iterator not associated with container");
            CIRCULAR_BUFFER_ASSERT(m_index <= m_container->size() && "Iterator out of range");
        }
#endif

    public:
        iterator_impl() noexcept : m_container(nullptr), m_index(0) {}

        iterator_impl(container_pointer container, IndexType index) noexcept
            : m_container(container), m_index(index) {}

        // Allow non-const to const conversion
        template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
        iterator_impl(const iterator_impl<OtherIsConst>& other) noexcept
            : m_container(other.m_container), m_index(other.m_index) {}

        reference operator*() const noexcept
        {
            CIRCULAR_BUFFER_VERIFY_ITERATOR(*this);
            CIRCULAR_BUFFER_ASSERT(m_index < m_container->size());
            return (*m_container)[m_index];
        }

        pointer operator->() const noexcept
        {
            return &(operator*());
        }

        reference operator[](difference_type n) const noexcept
        {
            return *(*this + n);
        }

        iterator_impl& operator++() noexcept
        {
            CIRCULAR_BUFFER_VERIFY_ITERATOR(*this);
            ++m_index;
            return *this;
        }

        iterator_impl operator++(int) noexcept
        {
            iterator_impl temp = *this;
            ++(*this);
            return temp;
        }

        iterator_impl& operator--() noexcept
        {
            CIRCULAR_BUFFER_VERIFY_ITERATOR(*this);
            --m_index;
            return *this;
        }

        iterator_impl operator--(int) noexcept
        {
            iterator_impl temp = *this;
            --(*this);
            return temp;
        }

        iterator_impl& operator+=(difference_type n) noexcept
        {
            CIRCULAR_BUFFER_VERIFY_ITERATOR(*this);
            m_index += static_cast<IndexType>(n);
            return *this;
        }

        iterator_impl& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        iterator_impl operator+(difference_type n) const noexcept
        {
            iterator_impl temp = *this;
            return temp += n;
        }

        iterator_impl operator-(difference_type n) const noexcept
        {
            iterator_impl temp = *this;
            return temp -= n;
        }

        difference_type operator-(const iterator_impl& other) const noexcept
        {
            CIRCULAR_BUFFER_VERIFY_ITERATOR(*this);
            CIRCULAR_BUFFER_VERIFY_ITERATOR(other);
            CIRCULAR_BUFFER_ASSERT(m_container == other.m_container);
            return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
        }

        bool operator==(const iterator_impl& other) const noexcept
        {
            return m_container == other.m_container && m_index == other.m_index;
        }

        bool operator!=(const iterator_impl& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator<(const iterator_impl& other) const noexcept
        {
            CIRCULAR_BUFFER_ASSERT(m_container == other.m_container);
            return m_index < other.m_index;
        }

        bool operator<=(const iterator_impl& other) const noexcept
        {
            return !(other < *this);
        }

        bool operator>(const iterator_impl& other) const noexcept
        {
            return other < *this;
        }

        bool operator>=(const iterator_impl& other) const noexcept
        {
            return !(*this < other);
        }
    };
};

// Global operator+ for iterator + difference_type
template<typename T, size_t Capacity, overflow_policy Policy, typename IndexType, size_t InlineThreshold, size_t Alignment, bool IsConst>
typename circular_buffer<T, Capacity, Policy, IndexType, InlineThreshold, Alignment>::template iterator_impl<IsConst>
operator+(typename circular_buffer<T, Capacity, Policy, IndexType, InlineThreshold, Alignment>::template iterator_impl<IsConst>::difference_type n,
          const typename circular_buffer<T, Capacity, Policy, IndexType, InlineThreshold, Alignment>::template iterator_impl<IsConst>& it) noexcept
{
    return it + n;
}

} // namespace dod 