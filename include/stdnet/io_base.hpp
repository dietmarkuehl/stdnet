// stdnet/io_base.hpp                                                 -*-C++-*-
// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_IO_BASE
#define INCLUDED_STDNET_IO_BASE

#include <stdnet/netfwd.hpp>
#include <memory>
#include <system_error>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden {
    struct _Io_base;
    template <typename _Data> struct _Io_operation;
}

// ----------------------------------------------------------------------------
// The struct _Io_base is used as base class of operation states. Objects of
// this type are also used to kick off the actual work once a readiness
// indication was received.

struct stdnet::_Hidden::_Io_base
{
    using _Extra_t = ::std::unique_ptr<void, auto(*)(void*)->void>;

    _Io_base*                         _Next{nullptr}; // used for an intrusive list
    ::stdnet::_Hidden::_Context_base* _Context{nullptr};
    ::stdnet::_Hidden::_Socket_id     _Id;            // the entity affected
    int                               _Event;         // mask for expected events
    auto                            (*_Work)(::stdnet::_Hidden::_Context_base&, _Io_base*) -> bool = nullptr;
    _Extra_t                          _Extra{nullptr, +[](void*){}};

    _Io_base(::stdnet::_Hidden::_Socket_id _Id, int _Event): _Id(_Id), _Event(_Event) {}

    virtual auto _Complete() -> void = 0;
    virtual auto _Error(::std::error_code) -> void = 0;
    virtual auto _Cancel() -> void = 0;
};


// ----------------------------------------------------------------------------
// The struct _Io_operation is an _Io_base storing operation specific data.

template <typename _Data>
struct stdnet::_Hidden::_Io_operation
    : _Io_base
    , _Data
{
    template <typename _D = _Data>
    _Io_operation(::stdnet::_Hidden::_Socket_id _Id, int _Event, _D&& _A = _Data())
        : _Io_base(_Id, _Event)
        , _Data(::std::forward<_D>(_A))
    {
    }
};

// ----------------------------------------------------------------------------

#endif
