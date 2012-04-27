#pragma once
#include <functional>
#include <boost/noncopyable.hpp>

namespace Instalog
{
    struct ScopeExit
    {
        std::function<void()> func_;
        ScopeExit(std::function<void()> func) : func_(func)
        { }
        ~ScopeExit()
        {
            func_();
        }
    };
}
