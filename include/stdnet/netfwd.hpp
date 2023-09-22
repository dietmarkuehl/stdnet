// stdnet/netfwd.hpp                                                  -*-C++-*-
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

#ifndef INCLUDED_STDNET_NETFWD
#define INCLUDED_STDNET_NETFWD
#pragma once

// ----------------------------------------------------------------------------

namespace stdnet
{
    using _Stdnet_native_handle_type = int;
    inline constexpr _Stdnet_native_handle_type _Stdnet_invalid_handle{-1};


    class io_context;
    class socket_base;
    template <typename> class basic_socket;
    template <typename> class basic_stream_socket;
    template <typename> class basic_socket_acceptor;
    namespace ip
    {
        template <typename> class basic_endpoint;
        class tcp;
        class address;
        class address_v4;
        class address_v6;
    }
}

// ----------------------------------------------------------------------------

#endif
