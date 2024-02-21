// stdnet/context_base.hpp                                            -*-C++-*-
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

#ifndef INCLUDED_STDNET_CONTEXT_BASE
#define INCLUDED_STDNET_CONTEXT_BASE

#include <stdnet/io_base.hpp>
#include <optional>
#include <system_error>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    struct _Context_base;
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Context_base
{
    using _Accept_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::stdnet::ip::tcp::endpoint,
                     ::socklen_t,
                     ::std::optional<::stdnet::basic_stream_socket<::stdnet::ip::tcp>>
                     >
        >;
    using _Receive_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::msghdr, int, ::std::size_t>
        >;
    using _Send_operation = ::stdnet::_Hidden::_Io_operation<
        ::std::tuple<::msghdr, int, ::std::size_t>
        >;

    virtual ~_Context_base() = default;
    virtual auto _Make_socket(int) -> ::stdnet::_Hidden::_Socket_id = 0;
    virtual auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id = 0;
    virtual auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void = 0;
    virtual auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type = 0;
    virtual auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void = 0;
    virtual auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const&, ::std::error_code&) -> void = 0;
    virtual auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void = 0;

    virtual auto run_one() -> ::std::size_t = 0;

    virtual auto _Cancel(::stdnet::_Hidden::_Io_base*, ::stdnet::_Hidden::_Io_base*) -> void = 0;
    virtual auto _Accept(_Accept_operation*) -> bool = 0;
    virtual auto _Receive(_Receive_operation*) -> bool = 0;
    virtual auto _Send(_Send_operation*) -> bool = 0;
};

// ----------------------------------------------------------------------------

#endif
