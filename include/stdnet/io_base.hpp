// stdnet/io_base.hpp                                                 -*-C++-*-
// ----------------------------------------------------------------------------
//  Copyright (C) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify,
//  merge, publish, distribute, sublicense, and/or sell copies of
//  the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
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
