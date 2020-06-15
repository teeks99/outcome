/* Storage for a very simple basic_result type
(C) 2017-2019 Niall Douglas <http://www.nedproductions.biz/> (6 commits)
File Created: Oct 2017


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OUTCOME_BASIC_RESULT_STORAGE_HPP
#define OUTCOME_BASIC_RESULT_STORAGE_HPP

#include "../success_failure.hpp"
#include "../trait.hpp"
#include "value_storage.hpp"

OUTCOME_V2_NAMESPACE_EXPORT_BEGIN

namespace detail
{
  template <class State, class E> constexpr inline void _set_error_is_errno(State & /*unused*/, const E & /*unused*/) {}
  template <class R, class EC, class NoValuePolicy> class basic_result_storage;
}  // namespace detail

namespace hooks
{
  template <class R, class S, class NoValuePolicy> constexpr inline uint16_t spare_storage(const detail::basic_result_storage<R, S, NoValuePolicy> *r) noexcept;
  template <class R, class S, class NoValuePolicy>
  constexpr inline void set_spare_storage(detail::basic_result_storage<R, S, NoValuePolicy> *r, uint16_t v) noexcept;
}  // namespace hooks

namespace policy
{
  struct base;
}  // namespace policy

namespace detail
{
  template <bool value_throws, bool error_throws> struct basic_result_storage_swap;
  template <class R, class EC, class NoValuePolicy>  //
  class basic_result_storage
  {
    static_assert(trait::type_can_be_used_in_basic_result<R>, "The type R cannot be used in a basic_result");
    static_assert(trait::type_can_be_used_in_basic_result<EC>, "The type S cannot be used in a basic_result");
    static_assert(std::is_void<EC>::value || std::is_default_constructible<EC>::value, "The type S must be void or default constructible");

    friend struct policy::base;
    template <class T, class U, class V>  //
    friend class basic_result_storage;
    template <class T, class U, class V> friend class basic_result_final;
    template <class T, class U, class V>
    friend constexpr inline uint16_t hooks::spare_storage(const detail::basic_result_storage<T, U, V> *r) noexcept;  // NOLINT
    template <class T, class U, class V>
    friend constexpr inline void hooks::set_spare_storage(detail::basic_result_storage<T, U, V> *r, uint16_t v) noexcept;  // NOLINT
    template <bool value_throws, bool error_throws> struct basic_result_storage_swap;

    struct disable_in_place_value_type
    {
    };
    struct disable_in_place_error_type
    {
    };

  protected:
    using _value_type = std::conditional_t<std::is_same<R, EC>::value, disable_in_place_value_type, R>;
    using _error_type = std::conditional_t<std::is_same<R, EC>::value, disable_in_place_error_type, EC>;

    using _state_type = value_storage_select_impl<_value_type>;

#ifdef STANDARDESE_IS_IN_THE_HOUSE
    detail::value_storage_trivial<_value_type> _state;
#else
    _state_type _state;
#endif
    devoid<_error_type> _error;

  public:
    // Used by iostream support to access state
    _state_type &_iostreams_state() { return _state; }
    const _state_type &_iostreams_state() const { return _state; }

    // Hack to work around MSVC bug in /permissive-
    _state_type &_msvc_nonpermissive_state() { return _state; }
    devoid<_error_type> &_msvc_nonpermissive_error() { return _error; }

  protected:
    basic_result_storage() = default;
    basic_result_storage(const basic_result_storage &) = default;             // NOLINT
    basic_result_storage(basic_result_storage &&) = default;                  // NOLINT
    basic_result_storage &operator=(const basic_result_storage &) = default;  // NOLINT
    basic_result_storage &operator=(basic_result_storage &&) = default;       // NOLINT
    ~basic_result_storage() = default;

    template <class... Args>
    constexpr explicit basic_result_storage(in_place_type_t<_value_type> _,
                                            Args &&... args) noexcept(detail::is_nothrow_constructible<_value_type, Args...>)
        : _state{_, static_cast<Args &&>(args)...}
        , _error()
    {
    }
    template <class U, class... Args>
    constexpr basic_result_storage(in_place_type_t<_value_type> _, std::initializer_list<U> il,
                                   Args &&... args) noexcept(detail::is_nothrow_constructible<_value_type, std::initializer_list<U>, Args...>)
        : _state{_, il, static_cast<Args &&>(args)...}
        , _error()
    {
    }
    template <class... Args>
    constexpr explicit basic_result_storage(in_place_type_t<_error_type> /*unused*/, Args &&... args) noexcept(detail::is_nothrow_constructible<_error_type, Args...>)
        : _state{detail::status::have_error}
        , _error(static_cast<Args &&>(args)...)
    {
      _set_error_is_errno(_state, _error);
    }
    template <class U, class... Args>
    constexpr basic_result_storage(in_place_type_t<_error_type> /*unused*/, std::initializer_list<U> il, Args &&... args) noexcept(detail::is_nothrow_constructible<_error_type, std::initializer_list<U>, Args...>)
        : _state{detail::status::have_error}
        , _error{il, static_cast<Args &&>(args)...}
    {
      _set_error_is_errno(_state, _error);
    }
    struct compatible_conversion_tag
    {
    };
    template <class T, class U, class V>
    constexpr basic_result_storage(compatible_conversion_tag /*unused*/, const basic_result_storage<T, U, V> &o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&detail::is_nothrow_constructible<_error_type, U>)
        : _state(o._state)
        , _error(o._error)
    {
    }
    template <class T, class V>
    constexpr basic_result_storage(compatible_conversion_tag /*unused*/, const basic_result_storage<T, void, V> &o) noexcept(detail::is_nothrow_constructible<_value_type, T>)
        : _state(o._state)
        , _error(_error_type{})
    {
    }
    template <class T, class U, class V>
    constexpr basic_result_storage(compatible_conversion_tag /*unused*/, basic_result_storage<T, U, V> &&o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&detail::is_nothrow_constructible<_error_type, U>)
        : _state(static_cast<decltype(o._state) &&>(o._state))
        , _error(static_cast<U &&>(o._error))
    {
    }
    template <class T, class V>
    constexpr basic_result_storage(compatible_conversion_tag /*unused*/, basic_result_storage<T, void, V> &&o) noexcept(detail::is_nothrow_constructible<_value_type, T>)
        : _state(static_cast<decltype(o._state) &&>(o._state))
        , _error(_error_type{})
    {
    }

    struct make_error_code_compatible_conversion_tag
    {
    };
    template <class T, class U, class V>
    constexpr basic_result_storage(make_error_code_compatible_conversion_tag /*unused*/, const basic_result_storage<T, U, V> &o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&noexcept(make_error_code(std::declval<U>())))
        : _state(o._state)
        , _error(make_error_code(o._error))
    {
    }
    template <class T, class U, class V>
    constexpr basic_result_storage(make_error_code_compatible_conversion_tag /*unused*/, basic_result_storage<T, U, V> &&o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&noexcept(make_error_code(std::declval<U>())))
        : _state(static_cast<decltype(o._state) &&>(o._state))
        , _error(make_error_code(static_cast<U &&>(o._error)))
    {
    }

    struct make_exception_ptr_compatible_conversion_tag
    {
    };
    template <class T, class U, class V>
    constexpr basic_result_storage(make_exception_ptr_compatible_conversion_tag /*unused*/, const basic_result_storage<T, U, V> &o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&noexcept(make_exception_ptr(std::declval<U>())))
        : _state(o._state)
        , _error(make_exception_ptr(o._error))
    {
    }
    template <class T, class U, class V>
    constexpr basic_result_storage(make_exception_ptr_compatible_conversion_tag /*unused*/, basic_result_storage<T, U, V> &&o) noexcept(detail::is_nothrow_constructible<_value_type, T> &&noexcept(make_exception_ptr(std::declval<U>())))
        : _state(static_cast<decltype(o._state) &&>(o._state))
        , _error(make_exception_ptr(static_cast<U &&>(o._error)))
    {
    }
  };

// Neither value nor error type can throw during swap
#ifdef __cpp_exceptions
  template <> struct basic_result_storage_swap<false, false>
#else
  template <bool value_throws, bool error_throws> struct basic_result_storage_swap
#endif
  {
    template <class R, class EC, class NoValuePolicy> constexpr basic_result_storage_swap(basic_result_storage<R, EC, NoValuePolicy> &a, basic_result_storage<R, EC, NoValuePolicy> &b)
    {
      using std::swap;
      a._msvc_nonpermissive_state().swap(b._msvc_nonpermissive_state());
      swap(a._msvc_nonpermissive_error(), b._msvc_nonpermissive_error());
    }
  };
#ifdef __cpp_exceptions
  // Swap potentially throwing value first
  template <> struct basic_result_storage_swap<true, false>
  {
    template <class R, class EC, class NoValuePolicy> constexpr basic_result_storage_swap(basic_result_storage<R, EC, NoValuePolicy> &a, basic_result_storage<R, EC, NoValuePolicy> &b)
    {
      using std::swap;
      a._msvc_nonpermissive_state().swap(b._msvc_nonpermissive_state());
      swap(a._msvc_nonpermissive_error(), b._msvc_nonpermissive_error());
    }
  };
  // Swap potentially throwing error first
  template <> struct basic_result_storage_swap<false, true>
  {
    template <class R, class EC, class NoValuePolicy> constexpr basic_result_storage_swap(basic_result_storage<R, EC, NoValuePolicy> &a, basic_result_storage<R, EC, NoValuePolicy> &b)
    {
      struct _
      {
        status_bitfield_type &a, &b;
        bool all_good{false};
        ~_()
        {
          if(!all_good)
          {
            // We lost one of the values
            a.set_have_lost_consistency(true);
            b.set_have_lost_consistency(true);
          }
        }
      } _{a._msvc_nonpermissive_state()._status, b._msvc_nonpermissive_state()._status};
      strong_swap(_.all_good, a._msvc_nonpermissive_error(), b._msvc_nonpermissive_error());
      a._msvc_nonpermissive_state().swap(b._msvc_nonpermissive_state());
    }
  };
  // Both could throw
  template <> struct basic_result_storage_swap<true, true>
  {
    template <class R, class EC, class NoValuePolicy> basic_result_storage_swap(basic_result_storage<R, EC, NoValuePolicy> &a, basic_result_storage<R, EC, NoValuePolicy> &b)
    {
      using std::swap;
      // Swap value and status first, if it throws, status will remain unchanged
      a._msvc_nonpermissive_state().swap(b._msvc_nonpermissive_state());
      bool all_good = false;
      try
      {
        strong_swap(all_good, a._msvc_nonpermissive_error(), b._msvc_nonpermissive_error());
      }
      catch(...)
      {
        if(!all_good)
        {
          a._msvc_nonpermissive_state()._status.set_have_lost_consistency(true);
          b._msvc_nonpermissive_state()._status.set_have_lost_consistency(true);
        }
        else
        {
          // We may still be able to rescue tis
          // First try to put the value and status back
          try
          {
            a._msvc_nonpermissive_state().swap(b._msvc_nonpermissive_state());
            // If that succeeded, continue by rethrowing the exception
          }
          catch(...)
          {
            all_good = false;
          }
        }
        if(!all_good)
        {
          // We are now trapped. The value swapped, the error did not,
          // trying to restore the value failed. We now have
          // inconsistent result objects. Best we can do is fix up the
          // status bits to prevent has_value() == has_error().
          auto check = [](basic_result_storage<R, EC, NoValuePolicy> &x) {
            bool has_value = x._state._status.have_value();
            bool has_error = x._state._status.have_error();
            bool has_exception = x._state._status.have_exception();
            x._state._status.set_have_lost_consistency(true);
            if(has_value == (has_error || has_exception))
            {
              if(has_value)
              {
                // We know the value swapped and is now set, so clear error and exception
                x._state._status.set_have_error(false).set_have_exception(false);
              }
              else
              {
                // We know the value swapped and is now unset, so set error
                x._state._status.set_have_error(true);
                // TODO: Should I default construct reset _error? It's guaranteed default constructible.
              }
            }
          };
          check(a);
          check(b);
        }
        throw;
      }
    }
  };
#endif

}  // namespace detail
OUTCOME_V2_NAMESPACE_END

#endif
