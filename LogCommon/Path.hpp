// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <memory>
#include <iterator>
#include <algorithm>
#include <type_traits>

namespace Instalog { namespace Path {

    /// @brief    Appends two paths together considering the middle backslash
    ///         
    /// @details    This isn't a very advanced method.  It only considers the last character of path and the first character of more. 
    ///
    /// @param    [in,out] path    Left of the path.  This variable will be modified to equal the final result.
    /// @param    [in] more    The right of the path to be appended
    std::wstring Append(std::wstring path, std::wstring const& more);

    /// @brief    "Prettifies" paths
    /// 
    /// @details    Lowercases everything in a path besides the drive letter and characters immediately following backslashes
    ///
    /// @param    first    An iterator to the beginning of the path to be pretified
    /// @param    last     An iterator one past the end of the path to be pretified
    void Prettify(std::wstring::iterator first, std::wstring::iterator last);

    /// @brief    Expands a short windows path to the corresponding long version
    ///
    /// @param    path    Full long path to the file
    bool ExpandShortPath(std::wstring &path);

    /// @brief    Resolve a path from the command line 
    /// 
    /// @details This is intended to work in the same way as Windows does it
    ///
    /// @param    [in,out]    path    Full pathname of the file.
    ///
    /// @return    true if the path exists and is not a directory, false otherwise
    bool ResolveFromCommandLine(std::wstring &path);

    /// @brief    Gets the Windows path.
    /// 
    /// @details    Does not include the trailing backslash (e.g. "C:\Windows") unless Windows is installed in the drive root
    ///
    /// @return    The Windows path.
    std::wstring GetWindowsPath();

    /**
     * Expands environment strings.
     *
     * @param input The input string to expand environment variables inside.
     *
     * @return The string with environment strings expanded.
     */
    std::wstring ExpandEnvStrings(std::wstring const& input);

    /**
     * @brief Represents a Windows path.
     * 
     * @details This class encapsulates the concept of a path string. It maintains both upper case and display case versions of the strings in question, and
     *          maintains the actual memory buffer in which the path data is stored. Comparisons are done in a case insensitive manner; but otherwise this
     *          is similar to the standard vector template.
     *
     * @todo Make this container allocator-aware.
     * @todo non-const operator[]/at() (possible "view" or something like that)
     */
    class path
    {
        // C++ standard references are current as of N3485.
    public:
        // 23.2.1 [container.requirements.general]/4
        // General container requirements
        typedef wchar_t value_type;
        typedef wchar_t* pointer;
        typedef wchar_t const* const_pointer;
        typedef wchar_t& reference_type;
        typedef wchar_t const& const_reference_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;
        path() throw();
        path(path const& other);
        path(path && other) throw();
        path& operator=(path other);
        iterator begin() throw();
        const_iterator begin() const throw();
        const_iterator cbegin() const throw();
        iterator end() throw();
        const_iterator end() const throw();
        const_iterator cend() const throw();
        void swap(path& other) throw();
        size_type size() const throw();
        size_type max_size() const throw();
        bool empty() const throw();
        ~path() throw();

        // 23.2.1 [container.requirements.general]/9
        // Reversible container requirements
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> reverse_const_iterator;
        reverse_iterator rbegin() throw();
        reverse_const_iterator rbegin() const throw();
        reverse_const_iterator crbegin() const throw();
        reverse_iterator rend() throw();
        reverse_const_iterator rend() const throw();
        reverse_const_iterator crend() const throw();

        // 23.2.3 [sequence.reqmts]/4
        // Sequence container requirements. path meets most of these. Exceptions are noted.
        // The constructor taking an initial size and element is not supported.
        template <typename InputIterator>
        path(InputIterator first, InputIterator last);
        // The constructor and copy assignment operator from initializer_list are left out
        // because MSVC++ doens't support initializer_list yet.
        // emplace is left out because MSVC++ doesn't support variadic templates yet.
        iterator insert(const_iterator insertionPoint, wchar_t character);
        iterator insert(const_iterator insertionPoint, size_type count, wchar_t character);
        template<typename InputIterator>
        iterator insert(const_iterator insertionPoint, InputIterator start, InputIterator finish);
        // Initializer list based insert not defined because MSVC++ doesn't support initializer list
        iterator erase(const_iterator removalPoint) throw();
        iterator erase(const_iterator removalBegin, const_iterator removalEnd) throw();
        void clear() throw();
        template<typename InputIterator>
        void assign(InputIterator start, InputIterator finish);
        // Initializer list based assign not defined because MSVC++ doesn't support initializer list
        void assign(size_type count, wchar_t character);

        // 23.2.3 [sequence.reqmts]/16
        // Sequence container optional requirements
        reference_type front() throw();
        const_reference_type front() const throw();
        reference_type back() throw();
        const_reference_type back() const throw();
        // Emplace front omitted because it is inefficient in this container
        // Emplace back omitted because MSVC++ doesn't support variadic templates
        void push_back(wchar_t character);
        // push front omitted because it is inefficient in this container.
        // pop front omitted because it is inefficient in this container.
        void pop_back() throw();
        const_reference_type operator[](size_type index) const throw();
        const_reference_type at(size_type index) const;

        // Additional members
        size_type capacity() const throw();
        void reserve(size_type count);
        pointer data() throw();
        const_pointer data() const throw();
        const_pointer c_str() const throw();
        path(wchar_t const* string);
        explicit path(std::wstring const& string);

        // Uppercase range inteface.
        iterator ubegin() throw();
        const_iterator ubegin() const throw();
        const_iterator cubegin() const throw();
        iterator uend() throw();
        const_iterator uend() const throw();
        const_iterator cuend() const throw();
        reverse_iterator rubegin() throw();
        reverse_const_iterator rubegin() const throw();
        reverse_const_iterator crubegin() const throw();
        reverse_iterator ruend() throw();
        reverse_const_iterator ruend() const throw();
        reverse_const_iterator cruend() const throw();
        reference_type ufront() throw();
        const_reference_type ufront() const throw();
        reference_type uback() throw();
        const_reference_type uback() const throw();
    private:
        static void uppercase_range(size_type length, const_pointer start, pointer target);
        template <typename InputIterator>
        void range_construct(InputIterator first, InputIterator last, std::input_iterator_tag);
        template <typename ForwardIterator>
        void range_construct(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);
        template <typename InputIterator>
        iterator range_insert(const_iterator position, InputIterator first, InputIterator last, std::input_iterator_tag);
        template <typename ForwardIterator>
        iterator range_insert(const_iterator position, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);
        template <typename InputIterator>
        void range_assign(const_iterator position, InputIterator first, InputIterator last, std::input_iterator_tag);
        template <typename ForwardIterator>
        void range_assign(const_iterator position, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);
        void ensure_capacity(size_type desiredCapacity);
        pointer upperBase() throw();
        const_pointer upperBase() const throw();
        size_type size_;
        size_type capacity_;
        pointer base_;
    };

    template <typename InputIterator>
    inline path::path(InputIterator first, InputIterator last)
        : size_(0)
        , capacity_(0)
        , base_(nullptr)
    {
        this->range_construct(first, last, typename std::iterator_traits<InputIterator>::iterator_category());
    }

    template <typename InputIterator>
    void path::range_construct(InputIterator first, InputIterator last, std::input_iterator_tag)
    {
        this->reserve(260); //MAX_PATH
        for (; first != last; ++first)
        {
            this->push_back(*first);
        }
    }

    template <typename ForwardIterator>
    void path::range_construct(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
    {
        this->reserve(std::distance(first, last));
        this->insert(this->begin(), first, last);
    }

    template<typename InputIterator>
    typename path::iterator path::insert(path::const_iterator insertionPoint, InputIterator start, InputIterator finish)
    {
        return this->range_insert(insertionPoint, start, finish, typename std::iterator_traits<InputIterator>::iterator_category());
    }

    template <typename InputIterator>
    path::iterator path::range_insert(
        path::const_iterator position,
        InputIterator first,
        InputIterator last,
        std::input_iterator_tag)
    {
    }

    template <typename ForwardIterator>
    path::iterator path::range_insert(
        path::const_iterator position,
        ForwardIterator first,
        ForwardIterator last,
        std::forward_iterator_tag)
    {
        auto insertionIndex = std::distance(this->begin(), position);
        auto additionalLength = std::distance(first, last);
        auto requiredCapacity = this->size() + additionalLength;
        if (requiredCapacity <= this->capacity())
        {
            // Awesome, just copy the needed bits

            // First block, before the insertion; nothing required as there was no reallocation

            // Last block, move things after the insertion point after
            auto uBase = this->upperBase();
            auto base = this->base_;
            std::memmove(base + insertionIndex, base + insertionIndex + additionalLength, additionalLength);
            std::memmove(upperBase + insertionIndex, upperBase + insertionIndex + additionalLength, additionalLength);

            // Okay, now the inserted block
            std::copy(first, last, base + insertionIndex);
            uppercase_range(additionalLength, base + insertionIndex, uBase + insertionIndex);
        }
        else
        {
            // Boo! Reallocation required
        }

        return this->begin() + inseritonIndex;
    }

    template<typename InputIterator>
    void assign(InputIterator start, InputIterator finish)
    {
        this->range_assign(start, finish, typename std::iterator_traits<InputIterator>::iterator_category());
    }

    template <typename InputIterator>
    path::iterator path::range_assign(path::const_iterator position, InputIterator first, InputIterator last, std::input_iterator_tag)
    {
        this->clear();
        this->range_construct(position, first, last, std::input_iterator_tag());
    }

    template <typename ForwardIterator>
    path::iterator path::range_assign(path::const_iterator position, ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
    {
    }

    inline bool operator==(path const& lhs, path const& rhs) throw()
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        else
        {
            return std::equal(lhs.ubegin(), lhs.uend(), rhs.ubegin());
        }
    }

    inline bool operator!=(path const& lhs, path const& rhs) throw()
    {
        return !(lhs == rhs);
    }

    inline bool operator<(path const& lhs, path const& rhs) throw()
    {
        return std::lexicographical_compare(lhs.ubegin(), lhs.uend(), rhs.ubegin(), rhs.uend());
    }

    inline bool operator>(path const& lhs, path const& rhs) throw()
    {
        return rhs < lhs;
    }

    inline bool operator<=(path const& lhs, path const& rhs) throw()
    {
        return !(lhs > rhs);
    }

    inline bool operator>=(path const& lhs, path const& rhs) throw()
    {
        return !(lhs < rhs);
    }

    inline void swap(path& lhs, path& rhs) throw()
    {
        return lhs.swap(rhs);
    }
}}
