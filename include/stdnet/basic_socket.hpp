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
#include <stdnet/io_context_scheduler.hpp>
#include <stdnet/internet.hpp>
#include <iostream> //-dk:TODO

// ----------------------------------------------------------------------------

template <typename _Protocol>
class stdnet::basic_socket
    : public ::stdnet::socket_base
{
public:
    using scheduler_type     = ::stdnet::_Hidden::_Io_context_scheduler;
    using protocol_type      = _Protocol;

private:
    static constexpr ::stdnet::_Hidden::_Socket_id _S_unused{0xffff'ffff};
    ::stdnet::_Hidden::_Context_base* _D_context;
    protocol_type                     _D_protocol{::stdnet::ip::tcp::v6()}; 
    ::stdnet::_Hidden::_Socket_id     _D_id{_S_unused};

public:
    basic_socket()
        : _D_context(nullptr)
    {
    }
    basic_socket(::stdnet::_Hidden::_Context_base* _Context, ::stdnet::_Hidden::_Socket_id _Id)
        : _D_context(_Context)
        , _D_id(_Id)
    {
    }
    basic_socket(basic_socket&& _Other)
        : _D_context(_Other._D_context)
        , _D_protocol(_Other._D_protocol)
        , _D_id(::std::exchange(_Other._D_id, _S_unused))
    {
    }
    ~basic_socket()
    {
        if (this->_D_id != _S_unused)
        {
            ::std::error_code _Error{};
            this->_D_context->_Release(this->_D_id, _Error);
        }
    }
    auto get_scheduler() noexcept -> scheduler_type
    {
        return scheduler_type{this->_D_context};
    }
    auto _Id() const -> ::stdnet::_Hidden::_Socket_id { return this->_D_id; }
};


// ----------------------------------------------------------------------------

#endif
