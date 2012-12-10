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
        typedef pointer iterator;
        typedef const_pointer const_iterator;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;
        path();
        path(path const& other);
        path(path && other);
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
        // The enable_if use below is to comply with 23.2.3 [sequence.reqmts]/14:
        //     ... is called with a type InputIterator that does not qualify as an input iterator
        //     then the constructor shall not participate in overload resolution.
        template <typename InputIterator>
        path(typename std::enable_if<!std::is_integral<InputIterator>::value, InputIterator>::type first,
             InputIterator last);
        // The constructor and copy assignment operator from initializer_list are left out
        // because MSVC++ doens't support initializer_list yet.
        // emplace is left out because MSVC++ doesn't support variadic templates yet.
        iterator insert(const_iterator insertionPoint, wchar_t character);
        iterator insert(const_iterator insertionPoint, size_type count, wchar_t character);
        template<typename InputIterator>
        typename std::enable_if<!std::is_integral<InputIterator>::value, iterator>::type
            insert(const_iterator insertionPoint, InputIterator start, InputIterator finish);
        // Initializer list based insert not defined because MSVC++ doesn't support initializer list
        iterator erase(const_iterator removalPoint) throw();
        iterator erase(const_iterator removalBegin, const_iterator removalEnd) throw();
        void clear() throw();
        // The enable_if use below is to comply with 23.2.3 [sequence.reqmts]/14:
        template<typename InputIterator>
        typename std::enable_if<!std::is_integral<InputIterator>::value>::type
            assign(InputIterator start, InputIterator finish);
        // Initializer list based assign not defined because MSVC++ doesn't support initializer list
        void assign(size_type count, wchar_t character);

        // 23.2.3 [sequence.reqmts]/16
        // Sequence container optional requirements
        reference_type front() throw();
        reference_type const front() const throw();
        reference_type back() throw();
        reference_type const back() const throw();
        // Emplace front omitted because it is inefficient in this container
        // Emplace back omitted because MSVC++ doesn't support variadic templates
        void push_back(wchar_t character);
        // push front omitted because it is inefficient in this container.
        // pop front omitted because it is inefficient in this container.
        void pop_back() throw();
        reference_type const operator[](size_type index) const throw();
        reference_type const at(size_type index) const;

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
        reference_type const ufront() const throw();
        reference_type uback() throw();
        reference_type const uback() const throw();
    private:
        size_type size_;
        size_type capacity_;
        pointer base_;
    };

    template <typename InputIterator>
    path::path(typename std::enable_if<!std::is_integral<InputIterator>::value, InputIterator>::type first,
         InputIterator last)
    {
    }

    bool operator==(path const& lhs, path const& rhs) throw();

    bool operator!=(path const& lhs, path const& rhs) throw();

    bool operator<(path const& lhs, path const& rhs) throw();

    bool operator>(path const& lhs, path const& rhs) throw();

    bool operator<=(path const& lhs, path const& rhs) throw();

    bool operator>=(path const& lhs, path const& rhs) throw();

    void swap(path& lhs, path& rhs) throw();
}}
