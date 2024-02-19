// stdnet/io_context.hpp                                              -*-C++-*-
// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
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

#ifndef INCLUDED_STDNET_IO_CONTEXT
#define INCLUDED_STDNET_IO_CONTEXT
#pragma once

#include <stdnet/netfwd.hpp>
#include <stdnet/context_base.hpp>
#include <stdnet/io_context_scheduler.hpp>
#include <stdnet/event_context.hpp>
#include <stdnet/poll_context.hpp>
#include <stdnet/basic_stream_socket.hpp>
#include <stdnet/container.hpp>
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <limits>
#include <cerrno>

// ----------------------------------------------------------------------------

namespace stdnet
{
    class io_context;
}

// ----------------------------------------------------------------------------

class stdnet::io_context
{
private:
    ::std::unique_ptr<::stdnet::_Hidden::_Context_base> _D_ownedx{new ::stdnet::_Hidden::_Poll_context()};
    ::std::unique_ptr<::stdnet::_Hidden::_Context_base> _D_owned{new ::stdnet::_Hidden::_Event_context()};
    ::stdnet::_Hidden::_Context_base&                   _D_context{*this->_D_owned};

public:
    using scheduler_type = ::stdnet::_Hidden::_Io_context_scheduler;
    class executor_type {};

    io_context() = default;
    io_context(::stdnet::_Hidden::_Context_base& _Context): _D_owned(), _D_context(_Context) {}
    io_context(io_context&&) = delete;

    auto _Make_socket(int _D, int _T, int _P, ::std::error_code& _Error) -> ::stdnet::_Hidden::_Socket_id
    {
        return this->_D_context._Make_socket(_D, _T, _P, _Error);
    }
    auto _Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void
    {
        return this->_D_context._Release(_Id, _Error);
    }
    auto _Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type
    {
        return this->_D_context._Native_handle(_Id);
    }
    auto _Set_option(::stdnet::_Hidden::_Socket_id _Id,
                     int _Level,
                     int _Name,
                     void const* _Data,
                     ::socklen_t _Size,
                     ::std::error_code& _Error) -> void
    {
        this->_D_context._Set_option(_Id, _Level, _Name, _Data, _Size, _Error);
    }
    auto _Bind(::stdnet::_Hidden::_Socket_id _Id, ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const& _Endpoint, ::std::error_code& _Error)
    {
        this->_D_context._Bind(_Id, _Endpoint, _Error);
    }
    auto _Listen(::stdnet::_Hidden::_Socket_id _Id, int _No, ::std::error_code& _Error)
    {
        this->_D_context._Listen(_Id, _No, _Error);
    }
    auto get_scheduler() -> scheduler_type { return scheduler_type(&this->_D_context); }

    ::std::size_t run_one() { return this->_D_context.run_one(); }
    ::std::size_t run()
    {
        ::std::size_t _Count{};
        while (::std::size_t _C = this->run_one())
        {
            _Count += _C;
        }
        return _Count;
    }
};

// ----------------------------------------------------------------------------

#endif
