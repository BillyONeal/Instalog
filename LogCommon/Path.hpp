// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <memory>
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

    class path_view;

    /**
     * @brief Represents a Windows path.
     * 
     * @details This class encapsulates the concept of a path string. It maintains both upper case and display case versions of the strings in question, and
     *          maintains the actual memory buffer in which the path data is stored. Comparisons are done in a case insensitive manner; but otherwise this
     *          is similar to the standard vector template.
     */
    template<typename AllocatorT = std::allocator<wchar_t>>
    class path
    {
        // C++ standard references are current as of N3485.
        static_assert(std::is_same<wchar_t, std::allocator_traits<AllocatorT>::value_type>::value, "The allocator for paths must store wchar_t instances.");
    public:
        // 23.2.1 [container.requirements.general]/4
        // General container requirements
        typedef wchar_t value_type;
        typedef wchar_t* pointer;
        typedef wchar_t const* const_pointer;
        typedef wchar_t& reference_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;
        // Default constructor below in allocator container requirements.
        path(path const& other);
        path(path && other);
        path& operator=(path other);
        iterator begin() throw();
        const_iterator begin() const throw();
        const_iterator cbegin() const throw();
        iterator end() throw();
        const_iterator end() const throw();
        const_iterator cend() const throw();
        void swap(path<AllocatorT>& other) throw();
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

        // 23.2.1 [container.requirements.general]/11
        // Allocator aware container requirements
        typedef AllocatorT allocator_type;
        allocator_type get_allocator() const;
        explicit path(allocator_type const& allocator = allocator_type());
        path(path const& other, allocator_type const& allocator);
        path(path && other);
        path(path && other, allocator_type const& allocator);

        // 23.2.3 [sequence.reqmts]/4
        // Sequence container requirements. path meets most of these. Exceptions are noted.
        // The constructor taking an initial size and element is not supported.
        template <typename iteratorType>
        path(iteratorType left, iteratorType right, Allocator const& allocator);
        // The constructor and copy assignment operator from initializer_list are left out
        // because MSVC++ doens't support initializer_list yet.
        // emplace is left out because MSVC++ doesn't support variadic templates yet.
        iterator insert(const_iterator insertionPoint, wchar_t character);
        iterator insert(const_iterator insertionPoint, size_type count, wchar_t character);
        template<typename iteratorType>
        iterator insert(const_iterator insertionPoint, iteratorType start, iteratorType finish);
        // Initializer list based insert not defined because MSVC++ doesn't support initializer list
        iterator erase(const_iterator removalPoint) throw();
        iterator erase(const_iterator removalBegin, const_iterator removalEnd) throw();
        void clear() throw();
        template<typename iteratorType>
        void assign(iteratorType start, iteratorType finish);
        // Initializer list based assign not defined because MSVC++ doesn't support initializer list
        void assign(size_type count, wchar_t character);

        // 23.2.3 [sequence.reqmts]/16
        // Sequence container optional requirements
        reference front() throw();
        const_reference front() const throw();
        reference back() throw();
        const_reference back() const throw();
        // Emplace front omitted because it is inefficient in this container
        // Emplace back omitted because MSVC++ doesn't support variadic templates
        void push_back(wchar_t character);
        // push front omitted because it is inefficient in this container.
        // pop front omitted because it is inefficient in this container.
        void pop_back() throw();
        reference operator[](size_type index) throw();
        const_reference operator[](size_type index) const throw();
        reference at(size_type index);
        const_reference at(size_type index) const;

        // Additional members
        size_type capacity() const throw();
        void reserve(size_type count);
        pointer data() throw();
        const_pointer data() const throw();
        const_pointer c_str() const throw();
        path(wchar_t const* string);
        explicit path(std::wstring const& string);
    private:
        allocator_type allocator_;
        size_type size_;
        size_type capacity_;
        pointer base_;
    };

    template<typename AllocatorT>
    bool operator==(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    bool operator!=(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    bool operator<(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    bool operator>(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    bool operator<=(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    bool operator>=(path<AllocatorT> const& lhs, path<AllocatorT> const& rhs) throw();

    template<typename AllocatorT>
    void swap(path<AllocatorT>& lhs, path<AllocatorT>& rhs) throw();
}}
