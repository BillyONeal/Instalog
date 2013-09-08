// Copyright © 2012 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <memory>

namespace Instalog
{
template <typename T> std::unique_ptr<T> make_unique()
{
    std::unique_ptr<T> value(new T);
    return value;
}

// Variadic templates would be nice....
template <typename T, typename A> std::unique_ptr<T> make_unique(A&& arg)
{
    std::unique_ptr<T> value(new T(std::forward<A>(arg)));
    return value;
}

template <typename T, typename A, typename B>
std::unique_ptr<T> make_unique(A&& arg, B&& arg2)
{
    std::unique_ptr<T> value(
        new T(std::forward<A>(arg), std::forward<B>(arg2)));
    return value;
}

template <typename T, typename A, typename B, typename C>
std::unique_ptr<T> make_unique(A&& arg, B&& arg2, C&& arg3)
{
    std::unique_ptr<T> value(new T(
        std::forward<A>(arg), std::forward<B>(arg2), std::forward<C>(arg3)));
    return value;
}

template <typename T, typename A, typename B, typename C, typename D>
std::unique_ptr<T> make_unique(A&& arg, B&& arg2, C&& arg3, D&& arg4)
{
    std::unique_ptr<T> value(new T(std::forward<A>(arg),
                                   std::forward<B>(arg2),
                                   std::forward<C>(arg3),
                                   std::forward<D>(arg4)));
    return value;
}

// Only supporting up to 5 right now
template <typename T,
          typename A,
          typename B,
          typename C,
          typename D,
          typename E>
std::unique_ptr<T> make_unique(A&& arg, B&& arg2, C&& arg3, D&& arg4, E&& arg5)
{
    std::unique_ptr<T> value(new T(std::forward<A>(arg),
                                   std::forward<B>(arg2),
                                   std::forward<C>(arg3),
                                   std::forward<D>(arg4),
                                   std::forward<E>(arg5)));
    return value;
}
}
