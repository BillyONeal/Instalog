// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <algorithm>

namespace Instalog
{
    template <typename ForwardIterator, typename T>
    bool contains(
        ForwardIterator first,
        ForwardIterator last,
        T const& value)
    {
        return std::find(first, last, value) != last;
    }

} // Instalog
