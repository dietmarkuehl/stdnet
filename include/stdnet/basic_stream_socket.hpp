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
#include <stdnet/basic_socket.hpp>

// ----------------------------------------------------------------------------

template <typename _Protocol>
class stdnet::basic_stream_socket
    : public basic_socket<_Protocol>
{
public:
    using native_handle_type = _Stdnet_native_handle_type;
    using protocol_type = _Protocol;
    using endpoint_type = typename protocol_type::endpoint;

    basic_stream_socket(::stdnet::_Hidden_abstract::_Context*, ::stdnet::_Hidden::_Socket_id)
    {
    }
    basic_stream_socket(::stdnet::io_context&, endpoint_type const&)
        : stdnet::basic_socket<_Protocol>()
    {
    }
};


// ----------------------------------------------------------------------------

#endif
