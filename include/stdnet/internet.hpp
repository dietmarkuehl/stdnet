// stdnet/internet.hpp                                                -*-C++-*-
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

#ifndef INCLUDED_STDNET_INTERNET
#define INCLUDED_STDNET_INTERNET
#pragma once

#include <stdnet/netfwd.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>

// ----------------------------------------------------------------------------

namespace stdnet::ip
{
    using port_type = ::std::uint_least16_t;
}

// ----------------------------------------------------------------------------

class ::stdnet::ip::tcp
{
private:
    int _D_family;
    int _D_protocol;

    constexpr tcp(int _F, int _P): _D_family(_F), _D_protocol(_P) {}

public:
    using endpoint = basic_endpoint<tcp>;
    using socket   = basic_stream_socket<tcp>;
    using acceptor = basic_socket_acceptor<tcp>;

    tcp() = delete;

    static constexpr tcp v4() { return tcp(PF_INET, AF_INET); }
    static constexpr tcp v6() { return tcp(PF_INET6, AF_INET6); }

    constexpr int family() const { return this->_D_family; }
    constexpr int type() const { return SOCK_STREAM; }
    constexpr int protocol() const { return this->_D_protocol; }
};

// ----------------------------------------------------------------------------

class ::stdnet::ip::address
{
private:
    ::sockaddr_storage _D_address;

public:
    constexpr address() noexcept
        : _D_address()
    {
        this->_D_address.ss_family = PF_INET;
    }
    constexpr address(address const&) noexcept;
    constexpr address(::stdnet::ip::address_v4 const&) noexcept;
    constexpr address(::stdnet::ip::address_v6 const&) noexcept;

    address& operator=(address const&) noexcept;
    address& operator=(::stdnet::ip::address_v4 const&) noexcept;
    address& operator=(::stdnet::ip::address_v6 const&) noexcept;

    constexpr bool is_v4() const noexcept { return this->_D_address.ss_family == PF_INET; }
    constexpr bool is_v6() const noexcept { return this->_D_address.ss_family == PF_INET6; }
    constexpr ::stdnet::ip::address_v4 to_v4() const;
    constexpr ::stdnet::ip::address_v6 to_v6() const;
    constexpr bool is_unspecified() const noexcept;
    constexpr bool is_loopback() const noexcept;
    constexpr bool is_multicast() const noexcept;
    template<class _Allocator = ::std::allocator<char>>
    ::std::basic_string<char, ::std::char_traits<char>, _Allocator>
    to_string(_Allocator const& = _Allocator()) const;
};

// ----------------------------------------------------------------------------

template <typename _Protocol>
class ::stdnet::ip::basic_endpoint
{
private:
    ::sockaddr_storage _D_address;

public:
    using protocol_type = _Protocol;

    constexpr basic_endpoint() noexcept
        : basic_endpoint(::stdnet::ip::address(), ::stdnet::ip::port_type())
    {
    }
    constexpr basic_endpoint(const protocol_type&, ::stdnet::ip::port_type) noexcept;
    constexpr basic_endpoint(const ip::address& _Address, ::stdnet::ip::port_type _Port) noexcept
    {
        if (_Address.is_v4())
        {
            ::sockaddr_in _Addr{};
            _Addr.sin_family = PF_INET;
            _Addr.sin_port   = htons(_Port);
            //-dk:TODO _Addr.sin_addr   = _Address._Data();
            ::std::memcpy(&this->_D_address, &_Addr, sizeof(::sockaddr_in));
        }
        else
        {
        }
    }

    constexpr protocol_type protocol() const noexcept
    {
        return this->_D_address.ss_family == PF_INET? ::stdnet::ip::tcp::v4(): ::stdnet::ip::tcp::v6();
    }
    constexpr ip::address address() const noexcept;
    void address(ip::address const&) noexcept;
    constexpr ::stdnet::ip::port_type port() const noexcept;
    void port(::stdnet::ip::port_type port_num) noexcept;

    ::sockaddr const* _Data() const { return reinterpret_cast<::sockaddr const*>(&this->_D_address); }
    ::socklen_t       _Size() const
    {
        return this->_D_address.ss_family == PF_INET? sizeof(::sockaddr_in): sizeof(::sockaddr_in6);
    }
};

// ----------------------------------------------------------------------------

#endif
