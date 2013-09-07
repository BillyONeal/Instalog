// Copyright © 2013 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <type_traits>

namespace Instalog
{
template <typename T> struct enable_flags_operators : std::false_type
{
};

template <typename T>
inline typename std::enable_if<enable_flags_operators<T>::value, T>::type
operator|(T lhs, T rhs) throw()
{
    typedef typename std::underlying_type<T>::type type;
    return static_cast<T>(static_cast<type>(lhs) | static_cast<type>(rhs));
}

template <typename T>
inline typename std::enable_if<enable_flags_operators<T>::value, T>::type
operator&(T lhs, T rhs) throw()
{
    typedef typename std::underlying_type<T>::type type;
    return static_cast<T>(static_cast<type>(lhs) & static_cast<type>(rhs));
}

template <typename T>
inline typename std::enable_if<enable_flags_operators<T>::value, T>::type
operator^(T lhs, T rhs) throw()
{
    typedef typename std::underlying_type<T>::type type;
    return static_cast<T>(static_cast<type>(lhs) ^ static_cast<type>(rhs));
}

template <typename T>
inline typename std::enable_if<enable_flags_operators<T>::value, T>::type
operator~(T item) throw()
{
    typedef typename std::underlying_type<T>::type type;
    return static_cast<T>(~static_cast<type>(item));
}

template <typename T> inline bool has_flag(T target, T test)
{
    static_assert(enable_flags_operators<T>::value,
                  "has_flag may only be used on flag enumerations.");
    typedef typename std::underlying_type<T>::type type;
    return static_cast<type>(target & test) != 0;
}
}
