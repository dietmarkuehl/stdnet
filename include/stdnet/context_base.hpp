// stdnet/context_base.hpp                                            -*-C++-*-
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

#ifndef INCLUDED_STDNET_CONTEXT_BASE
#define INCLUDED_STDNET_CONTEXT_BASE

#include <stdnet/io_base.hpp>
#include <stdnet/endpoint.hpp>
#include <chrono>
#include <optional>
#include <system_error>
#include <sys/socket.h>
#include <sys/time.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    struct _Context_base;
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Context_base
{
    using _Accept_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::stdnet::_Hidden::_Endpoint,
                     ::socklen_t,
                     ::std::optional<::stdnet::_Hidden::_Socket_id>,
                     ::stdnet::_Hidden::_Context_base*
                     >
        >;
    using _Connect_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::stdnet::_Hidden::_Endpoint>
        >;
    using _Receive_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::msghdr, int, ::std::size_t>
        >;
    using _Send_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::msghdr, int, ::std::size_t>
        >;
    using _Resume_after_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::std::chrono::microseconds, ::timeval>
        >;
    using _Resume_at_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::std::chrono::system_clock::time_point, ::timeval>
        >;

    virtual ~_Context_base() = default;
    virtual auto _Make_socket(int, ::std::error_code& _Error) -> ::stdnet::_Hidden::_Socket_id = 0;
    virtual auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id = 0;
    virtual auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void = 0;
    virtual auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type = 0;
    virtual auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void = 0;
    virtual auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::_Hidden::_Endpoint const&, ::std::error_code&) -> void = 0;
    virtual auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void = 0;

    virtual auto run_one() -> ::std::size_t = 0;

    virtual auto _Cancel(::stdnet::_Hidden::_Io_base*, ::stdnet::_Hidden::_Io_base*) -> void = 0;
    virtual auto _Accept(::stdnet::_Hidden::_Context_base::_Accept_operation*) -> bool = 0;
    virtual auto _Connect(::stdnet::_Hidden::_Context_base::_Connect_operation*) -> bool = 0;
    virtual auto _Receive(::stdnet::_Hidden::_Context_base::_Receive_operation*) -> bool = 0;
    virtual auto _Send(::stdnet::_Hidden::_Context_base::_Send_operation*) -> bool = 0;
    virtual auto _Resume_after(::stdnet::_Hidden::_Context_base::_Resume_after_operation*) -> bool = 0;
    virtual auto _Resume_at(::stdnet::_Hidden::_Context_base::_Resume_at_operation*) -> bool = 0;
};

// ----------------------------------------------------------------------------

#endif
