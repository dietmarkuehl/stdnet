// stdnet/timer.hpp                                                   -*-C++-*-
// ----------------------------------------------------------------------------
//
//  Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Licensed under the Apache License Version 2.0 with LLVM Exceptions
//  (the "License"); you may not use this file except in compliance with
//  the License. You may obtain a copy of the License at
//
//    https://llvm.org/LICENSE.txt
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_TIMER
#define INCLUDED_STDNET_TIMER

#include <stdnet/netfwd.hpp>
#include <stdnet/cpo.hpp>

// ----------------------------------------------------------------------------

namespace stdnet
{
    namespace _Hidden
    {
        struct _Resume_after_desc;
        struct _Resume_at_desc;
    }

    using async_resume_after_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Resume_after_desc>;
    using async_resume_at_t    = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Resume_at_desc>;

    inline constexpr async_resume_after_t async_resume_after{};
    inline constexpr async_resume_at_t    async_resume_at{};
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Resume_after_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Resume_after_operation;
    template <typename _Scheduler, typename>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t();

        _Scheduler                   _D_scheduler;
        ::std::chrono::microseconds  _D_duration;

        auto _Id() const -> ::stdnet::_Hidden::_Socket_id { return {}; }
        auto _Events() const { return decltype(POLLIN)(); }
        auto _Get_scheduler() { return this->_D_scheduler; }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base) = this->_D_duration;
            return this->_D_scheduler._Resume_after(_Base);
        }
    };
};

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Resume_at_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Resume_at_operation;
    template <typename _Scheduler, typename>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t();

        _Scheduler                              _D_scheduler;
        ::std::chrono::system_clock::time_point _D_time;

        auto _Id() const -> ::stdnet::_Hidden::_Socket_id { return {}; }
        auto _Events() const { return decltype(POLLIN)(); }
        auto _Get_scheduler() { return this->_D_scheduler; }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base) = this->_D_time;
            return this->_D_scheduler._Resume_at(_Base);
        }
    };
};

// ----------------------------------------------------------------------------

#endif
