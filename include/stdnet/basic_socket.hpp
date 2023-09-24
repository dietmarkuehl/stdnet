// stdnet/basic_socket.hpp                                            -*-C++-*-
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

#ifndef INCLUDED_STDNET_BASIC_SOCKET
#define INCLUDED_STDNET_BASIC_SOCKET

#include <stdnet/netfwd.hpp>
#include <stdnet/socket_base.hpp>

// ----------------------------------------------------------------------------

template <typename Protocol>
class stdnet::basic_socket
    : public ::stdnet::socket_base
{
};


// ----------------------------------------------------------------------------

#endif
