//          Copyright Billy O'Neal 2013
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Optimistic buffer is a memory allocation optimization. If the requested size
// is below some value, it will store that data internally rather than going to
// the heap.

#pragma once
#include <type_traits>
#include <algorithm>

namespace Instalog
{
namespace detail
{
// Selects either the value "left" or the value "right" depending on the value
// of useRight
template <typename T, T left, T right, bool useRight>
struct MaxImpl : public std::integral_constant<T, left> // use the left value
{
};

// when useRight is true.
template <typename T, T left, T right>
struct MaxImpl<T, left, right, true> : public std::integral_constant<
    T,
    right> // use the right value
{
};

// Pick the maximum of two values left and right.
template <typename T, T left, T right>
struct Max : public MaxImpl<T, left, right, (left < right)>
{
};
}

template <std::size_t SizeGuess = 32, std::size_t AlignmentValue = 1>
class OptimisticBuffer
{
    typedef unsigned char PointerBackingType;
    typedef PointerBackingType* PointerType;
    typedef PointerBackingType const* PointerConstType;
    // The required alignment of the stack allocated region.
    typedef typename detail::Max<std::size_t,
                                 std::alignment_of<unsigned char*>::value,
                                 AlignmentValue>::type AlignmentType;
    // The size of the stack allocated region.
    typedef typename detail::Max<std::size_t,
                                 sizeof(unsigned char*),
                                 SizeGuess>::type SizeType;
    // The type of stack allocation region.
    typedef typename std::aligned_storage<SizeType::value,
                                          AlignmentType::value>::type
    StorageType;
    // The size of the currently stored data.
    std::size_t currentSize;

    // The stack allocation itself.
    union
    {
        StorageType stackStorage; // In use when currentSize <= SizeType::value
        PointerType dynamicStorage; // In use when currentSize > SizeType::value
    };

    // True if this instance is using dynamic storage; otherwise, false.
    bool IsUsingDynamicMemory() const throw()
    {
        return currentSize > SizeType::value;
    }

    // Gets a pointer to the logical allocation.
    PointerConstType GetPointer() const throw()
    {
        if (IsUsingDynamicMemory())
        {
            return dynamicStorage;
        }
        else
        {
            return reinterpret_cast<PointerConstType>(&stackStorage);
        }
    }

    // Gets a pointer to the logical allocation.
    PointerType GetPointer() throw()
    {
        if (IsUsingDynamicMemory())
        {
            return dynamicStorage;
        }
        else
        {
            return reinterpret_cast<PointerType>(&stackStorage);
        }
    }

    // Deallocates memory.
    void Destroy() throw()
    {
        if (IsUsingDynamicMemory())
        {
            delete[] dynamicStorage;
        }

        currentSize = 0;
    }

    // Allocates memory.
    void Allocate(std::size_t allocationSize)
    {
        currentSize = allocationSize;
        if (IsUsingDynamicMemory())
        {
            // Dammit, allocation required.
            dynamicStorage = new unsigned char[allocationSize];
        }
    }

    public:

    /**
     * Initializes a new instance of the OptimisticBuffer class.
     * @param allocateSize (optional) Initial allocation.
     */
    OptimisticBuffer(std::size_t allocateSize = 0)
    {
        Allocate(allocateSize);
    }

    /**
     * Initializes a new instance of the OptimisticBuffer class.
     * @tparam std::size_t OtherSizeGuess The size guess for the
     * OptimisticBuffer to copy.
     * @tparam std::size_t OtherAlignmentValue The alignment of the
     * OptimisiticBuffer to copy.
     * @param toCopy The OptimisticBuffer to copy.
     */
    template <std::size_t OtherSizeGuess, std::size_t OtherAlignmentValue>
    OptimisticBuffer(
        OptimisticBuffer<OtherSizeGuess, OtherAlignmentValue> const& toCopy)
        : currentSize(toCopy.currentSize)
    {
        static_assert(
            OptimisticBuffer<OtherSizeGuess,
                             OtherAlignmentValue>::AlignmentType::value >
                AlignmentType::value,
            "Tried to copy from a more aligned memory block to a less aligned "
            "memory block.");
        Allocate(toCopy.Size());
        std::copy_n(toCopy.GetPointer(), toCopy.Size(), this->GetPointer());
    }

    /**
     * Assigns over this OptimisitcBuffer instance.
     * @tparam std::size_t OtherSizeGuess The size guess for the
     * OptimisticBuffer to copy.
     * @tparam std::size_t OtherAlignmentValue The alignment of the
     * OptimisiticBuffer to
     *  copy.
     * @param toCopy The OptimisticBuffer to copy.
     * @return *this
     */
    template <std::size_t OtherSizeGuess, std::size_t OtherAlignmentValue>
    OptimisticBuffer<SizeGuess, AlignmentValue>& operator=(
        OptimisticBuffer<OtherSizeGuess, OtherAlignmentValue> const& toCopy)
        : currentSize(toCopy.currentSize)
    {
        static_assert(
            OptimisticBuffer<OtherSizeGuess,
                             OtherAlignmentValue>::AlignmentType::value >
                AlignmentType::value,
            "Tried to copy assign from a more aligned memory block to a less "
            "aligned memory block.");
        if (GetPointer() != toCopy.GetPointer()) // Handle assignment to self.
        {
            if (Size() != toCopy.Size()) // Optimization -- if sizes match no
                                         // need to reallocate.
            {
                Destroy();
                Allocate(toCopy.Size());
            }

            std::copy_n(toCopy.GetPointer(), toCopy.Size(), this->GetPointer());
        }
        return *this;
    }

    /**
     * Gets the size of the allocated region.
     * @return The allocated size.
     */
    std::size_t Size() const
    {
        return currentSize;
    }

    /**
     * Resizes this instance to the given new size.
     * @param newSize The new size to allocate for.
     */
    void Resize(std::size_t newSize)
    {
        Destroy();
        Allocate(newSize);
    }

    /**
     * Array indexer operator.
     * @param index Zero-based index of this buffer to retrieve.
     * @return The indexed value.
     */
    unsigned char& operator[](std::size_t index) { return GetPointer()[index]; }

        /**
         * Array indexer operator.
         * @param index Zero-based index of this buffer to retrieve.
         * @return The indexed value.
         */
        unsigned char const&
    operator[](std::size_t index) const
    {
        return GetPointer()[index];
    }

    /**
     * Gets the pointer for this buffer.
     * @return The pointer for this buffer.
     */
    PointerBackingType* Get()
    {
        return GetPointer();
    }

    /**
     * Gets the pointer for this buffer.
     * @return The pointer for this buffer.
     */
    PointerBackingType const* Get() const
    {
        return GetPointer();
    }

    /**
     * Gets the pointer for this buffer as the given type.
     * @tparam typename T The type as which this buffer is retrieved.
     * @return The pointer for this buffer.
     */
    template <typename T> T* GetAs()
    {
        return reinterpret_cast<T*>(GetPointer());
    }

    /**
     * Gets the pointer for this buffer as the given type.
     * @tparam typename T The type as which this buffer is retrieved.
     * @return The pointer for this buffer.
     */
    template <typename T> T const* GetAs() const
    {
        return reinterpret_cast<T const*>(GetPointer());
    }

    /**
     * Destroys this instance of OptimisticBuffer.
     */
    ~OptimisticBuffer()
    {
        Destroy();
    }
};
}
