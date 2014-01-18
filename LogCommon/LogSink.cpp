// Copyright © 2012-2013 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "LogSink.hpp"
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_numeric.hpp>

namespace Instalog
{
    // Member functions for class log_sink
    log_sink::~log_sink() BOOST_NOEXCEPT_OR_NOTHROW
    {
    }

    // Member functions for class string_sink
    void string_sink::append(char const* data, std::size_t dataLength)
    {
        this->target.append(data, dataLength);
    }
    std::string const& string_sink::get() const BOOST_NOEXCEPT_OR_NOTHROW
    {
        return this->target;
    }

    // boost::spirit::karma numeric generators. These perform default
    // formatting of numbers.
#define GENERATE_KARMA_GENERATOR(t, parser) \
    stack_result_for_digits<t>::type format_value(t value) BOOST_NOEXCEPT_OR_NOTHROW \
    { \
    using namespace boost::spirit::karma; \
    stack_result_for_digits<t>::type result; \
    char *begin = result.data(); \
    char *end = begin; \
    generate(end, parser, value); \
    result.set_size(end - begin); \
    return result; \
    }

    // Stamp out for each of the numeric types with macros.
    GENERATE_KARMA_GENERATOR(short, short_);
    GENERATE_KARMA_GENERATOR(int, int_);
    GENERATE_KARMA_GENERATOR(long, long_);
    GENERATE_KARMA_GENERATOR(long long, long_long);
    GENERATE_KARMA_GENERATOR(unsigned short, ushort_);
    GENERATE_KARMA_GENERATOR(unsigned long, ulong_);
    GENERATE_KARMA_GENERATOR(unsigned int, uint_);
    GENERATE_KARMA_GENERATOR(unsigned long long, ulong_long);
    /* floats will call the double overload. */
    GENERATE_KARMA_GENERATOR(double, double_);

#undef GENERATE_KARMA_GENERATOR

    format_intrusive_result format_value(std::string const& value) BOOST_NOEXCEPT_OR_NOTHROW
    {
        return format_intrusive_result(value.c_str(), value.size());
    }

    // Format character pointers. (Assume null terminated)
    format_intrusive_result format_value(char const* ptr) BOOST_NOEXCEPT_OR_NOTHROW
    {
        return format_intrusive_result(ptr, std::strlen(ptr));
    }

    // Format single characters
    format_character_result format_value(char value) BOOST_NOEXCEPT_OR_NOTHROW
    {
        return format_character_result(value);
    }

    // Format wide character versions of the above.
    std::string format_value(std::wstring const& value)
    {
        static_assert(sizeof(wchar_t) == 2, "This method needs to be inspected / updated if wchar_t is not UTF-16.");
        std::string result;
        result.reserve(value.size());
        utf8::utf16to8(value.cbegin(), value.cend(), std::back_inserter(result));
        return result;
    }

    std::string format_value(wchar_t const* value)
    {
        static_assert(sizeof(wchar_t) == 2, "This method needs to be inspected / updated if wchar_t is not UTF-16.");
        auto const valueLength = std::wcslen(value);
        std::string result;
        result.reserve(valueLength);
        utf8::utf16to8(value, value + valueLength, std::back_inserter(result));
        return result;
    }

    format_stack_result<4> format_value(wchar_t value)
    {
        static_assert(sizeof(wchar_t) == 2, "This method needs to be inspected / updated if wchar_t is not UTF-16.");
        if (utf8::internal::is_lead_surrogate(value) || utf8::internal::is_trail_surrogate(value))
        {
            throw utf8::invalid_utf16(static_cast<std::uint16_t>(value));
        }

        format_stack_result<4> result;
        auto const endIterator = utf8::append(value, result.data());
        result.set_size(endIterator - result.data());
        return result;
    }

    format_intrusive_result format_value(boost::string_ref value)
    {
        return format_intrusive_result(value.data(), value.length());
    }

}
