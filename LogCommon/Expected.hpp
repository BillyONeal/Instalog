// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include <exception>
#include <memory>

namespace Instalog {

/**
 * Uninitalized expected<T> exception.
 * @sa std::logic_error
 */
struct uninitalized_expected : public std::logic_error
{
    /**
     * Initializes a new instance of the uninitalized_expected class.
     */
    uninitalized_expected()
        : std::logic_error("Attempted to read an expected<t> which was unintialized.")
    { }
};

template <typename Ty>
class expected;

template <typename Ty>
inline void swap(expected<Ty>& lhs, expected<Ty>& rhs);

/**
 * An expected of T. Contains either a T or an exception if an exception was encountered when producing T.
 * 
 * @remarks This class is loosely based on Andrei Alexandrescu's talk, "Systematic Error Handling in C++". See
 *          http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C
 * @tparam Ty Type of the contained object.
 */
template <typename Ty>
class expected
{
    static_assert(std::is_nothrow_destructible<Ty>::value, "expected<t> requires that Ty be nothrow destructable.");
public:
    typedef Ty value_type;
    typedef Ty& reference;
    typedef Ty const& const_reference;
    typedef Ty* pointer;
    typedef Ty const* const_pointer;
private:
    union
    {   // In C++11, one is allowed to have a union with a type with a constructor. But MSVC++ doens't support that
        // as of this writing.
        typename std::aligned_storage<sizeof(Ty), std::alignment_of<Ty>::value>::type tStorage;
        std::aligned_storage<sizeof(std::exception_ptr), std::alignment_of<std::exception_ptr>::value>::type eStorage;
    };

    /**
     * Whether or not this instance is valid. (That is, contains a T)
     */
    bool isValid;

    /**
     * Gets the exception storage reference.
     * 
     * @return The exception storage reference.
     */
    std::exception_ptr& get_exception_storage() throw() 
    {
        return *reinterpret_cast<std::exception_ptr*>(&eStorage);
    }

    /**
     * Gets the exception storage reference.
     * 
     * @return The exception storage reference.
     */
    std::exception_ptr const& get_exception_storage() const throw()
    {
        return *reinterpret_cast<std::exception_ptr const*>(&eStorage);
    }

    /**
     * Gets the value storage reference.
     * 
     * @return The value storage reference.
     */
    reference get_value_storage() throw() 
    {
        return *reinterpret_cast<pointer>(&tStorage);
    }

    /**
     * Gets the value storage reference.
     * 
     * @return The value storage reference.
     */
    const_reference get_value_storage() const throw()
    {
        return *reinterpret_cast<const_pointer>(&tStorage);
    }

    /**
     * Destorys the contents of this expected<T>.
     */
    void destroy() throw()
    {
        if (isValid)
        {
            get_value_storage().~Ty();
        }
        else
        {
            using std::exception_ptr;
            get_exception_storage().~exception_ptr();
        }
    }

    /**
     * Constructs an empty exception_ptr as this instance's default state.
     */
    void default_initialize() throw()
    {
        new (&get_exception_storage()) std::exception_ptr();
    }

public:

    /**
     * Initializes a new, invalid instance of the expected class.
     */
    expected()
        : isValid(false)
    {
        default_initialize();
    }

    /**
     * Initializes a new instance of the expected class. Requires that Ty be copy constructable.
     * @param source The actual expected item.
     */
    expected(const_reference source)
        : isValid(true)
    {
        new (&get_value_storage()) Ty(source);
    }

    /**
     * Initializes a new instance of the expected class. Requires that Ty be move constructable.
     * @param [in,out] source Source for the.
     */
    expected(Ty&& source)
        : isValid(true)
    {
        new (&get_value_storage()) Ty(std::move(source));
    }

    /**
     * Initializes a new instance of the expected class from an existing copy. Requires Ty to be copy constructable.
     * @param source Source expected<T> to copy.
     */
    expected(expected<Ty> const& source)
        : isValid(source.isValid)
    {
        if (source.isValid)
        {
            new (&get_value_storage()) Ty(source.get_value_storage());
        }
        else
        {
            new (&get_exception_storage()) std::exception_ptr(source.get_exception_storage());
        }
    }

    /**
     * Move initializes a new instance of the expected class from an existing copy. Requires Ty to be move constructable.
     * @param source Source expected<T> to move from.
     */
    expected(expected<Ty>&& source)
        : isValid(source.isValid)
    {
        if (source.isValid)
        {
            new (&get_value_storage()) Ty(std::move(source.get_value_storage()));
        }
        else
        {
            new (&get_exception_storage()) std::exception_ptr(std::move(source.get_exception_storage()));
        }
    }

    /**
     * Assignment operator.
     * @param source Source for the assignment.
     * @return A shallow copy of this instance.
     */
    expected<Ty>& operator=(expected<Ty> source)
    {
        source.swap(*this);
        return *this;
    }

    /**
     * Initializes this instance from the given exception pointer.
     * @param ptr The exception pointer from which this instance shall be initialized.
     * @return An expected<T> containing ptr.
     */
    static expected<Ty> from_exception(std::exception_ptr ptr) throw()
    {
        expected<Ty> result;
        new (&result.get_exception_storage()) std::exception_ptr(std::move(ptr));
        return std::move(result);
    }

    /**
     * Initializes this instance from the exception in flight.
     * @return An expected<T> containing the exception in flight.
     */
    static expected<Ty> from_exception() throw()
    {
        return from_exception(std::current_exception());
    }

    /**
     * Initializes this instance from the given exception.
     * @tparam typename E Type of the exception. This parameter should not be explictly specified.
     * @param [in,out] ex The exception from which the new instance shall be constructed.
     * @return A expected<T> containing ex.
     */
    template <typename E>
    static expected<Ty> from_exception(E&& ex) throw()
    {
        assert(typeid(E) == typeid(ex) && "Slicing detected.");
        return from_exception(std::make_exception_ptr(std::forward<E>(ex)));
    }

    /**
     * Destroys an expected<t>.
     */
    ~expected() throw()
    {
        destroy();
    }

    /**
     * Clears the contents of this expected<t>, and reverts it to a default-initalzied state.
     */
    void clear() throw()
    {
        destroy();
        isValid = false;
        default_initialize();
    }

    /**
     * Query if this instance is valid. Valid means that the instance contains a T, rather than an exception.
     * 
     * @return true if valid, false if not.
     */
    bool is_valid() const throw()
    {
        return this->isValid;
    }

    /**
     * Rethrows this instance if an exception is contained herein. If this instance is valid, does nothing.
     * @exception uninitalized_expected Thrown when this instance was default constructed.
     */
    void rethrow() const
    {
        if (is_valid())
        {
            return;
        }
        else if (get_exception_storage())
        {
            std::rethrow_exception(get_exception_storage());
        }
        else
        {
            throw uninitalized_expected();
        }
    }

    /**
     * Gets the item contained within this expected<T>.
     * @exception Unspecified The exception contained herein if this instance is not valid and was constructed with an exception.
     * @exception uninitialized_expected if this instance was default constructed.
     * @return The item contained in this expected<T>.
     */
    reference get()
    {
        rethrow();
        return get_value_storage();
    }

    /**
     * Gets the item contained within this expected<T>.
     * @exception Unspecified The exception contained herein if this instance is not valid and was constructed with an exception.
     * @exception uninitialized_expected if this instance was default constructed.
     * @return The item contained in this expected<T>.
     */
    const_reference get() const
    {
        rethrow();
        return get_value_storage();
    }

    /**
     * Swaps the given item with this instance.
     * @param [in,out] target Target for the swap.
     */
    void swap(expected<Ty>& target)
    {
        if (isValid)
        {
            if (target.isValid)
            {
                using std::swap;
                swap(get_value_storage(), target.get_value_storage());
            }
            else
            {
                std::swap(isValid, target.isValid);
                using std::exception_ptr;
                std::exception_ptr moved(std::move(target.get_exception_storage()));
                target.get_exception_storage().~exception_ptr();
                new (&target.get_value_storage()) Ty(std::move(get_value_storage()));
                get_value_storage().~Ty();
                new (&get_exception_storage()) exception_ptr(std::move(moved));
            }
        }
        else
        {
            if (target.isValid)
            {
                target.swap(*this);
            }
            else
            {
                using std::swap;
                swap(get_exception_storage(), target.get_exception_storage());
            }
        }
    }
};

/**
 * Swaps a pair of expected of Ts.
 * @tparam typename Ty Type of the expected<T>.
 * @param [in,out] lhs The left hand side.
 * @param [in,out] rhs The right hand side.
 */
template <typename Ty>
inline void swap(expected<Ty>& lhs, expected<Ty>& rhs)
{
    lhs.swap(rhs);
}

}
