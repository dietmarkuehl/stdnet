// stdnet/basic_stream_socket.hpp                                     -*-C++-*-
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

#ifndef INCLUDED_STDNET_BASIC_STREAM_SOCKET
#define INCLUDED_STDNET_BASIC_STREAM_SOCKET

#include <stdnet/netfwd.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/basic_socket.hpp>
#include <functional>
#include <system_error>

// ----------------------------------------------------------------------------

template <typename _Protocol>
class stdnet::basic_stream_socket
    : public basic_socket<_Protocol>
{
public:
    using native_handle_type = _Stdnet_native_handle_type;
    using protocol_type = _Protocol;
    using endpoint_type = typename protocol_type::endpoint;

private:
    endpoint_type _D_endpoint;

public:
    basic_stream_socket(basic_stream_socket&&) = default;
    basic_stream_socket& operator= (basic_stream_socket&&) = default;
    basic_stream_socket(::stdnet::_Hidden::_Context_base* _Context, ::stdnet::_Hidden::_Socket_id _Id)
        : basic_socket<_Protocol>(_Context, _Id)
    {
    }
    basic_stream_socket(::stdnet::io_context& _Context, endpoint_type const& _Endpoint)
        : stdnet::basic_socket<_Protocol>(_Context.get_scheduler()._Get_context(),
            ::std::invoke([_P = _Endpoint.protocol(), &_Context]{
                ::std::error_code _Error{};
                auto _Rc(_Context._Make_socket(_P.family(), _P.type(), _P.protocol(), _Error));
                if (_Error)
                {
                    throw ::std::system_error(_Error);
                }
                return _Rc;
            }))
        , _D_endpoint(_Endpoint) 
    {
    }

    auto get_endpoint() const -> endpoint_type { return this->_D_endpoint; }
};


// ----------------------------------------------------------------------------

#endif
