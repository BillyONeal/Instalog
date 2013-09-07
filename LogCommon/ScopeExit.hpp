#pragma once
#include <functional>
#include <boost/noncopyable.hpp>

namespace Instalog
{
struct ScopeExit : boost::noncopyable
{
    std::function<void()> func_;
    ScopeExit(std::function<void()> func) : func_(func)
    {
    }
    ~ScopeExit()
    {
        func_();
    }
};
}
