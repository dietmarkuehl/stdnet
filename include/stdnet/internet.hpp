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
#include <stdexcept>
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

    constexpr tcp(int _F): _D_family(_F) {}

public:
    using endpoint = basic_endpoint<tcp>;
    using socket   = basic_stream_socket<tcp>;
    using acceptor = basic_socket_acceptor<tcp>;

    tcp() = delete;

    static constexpr auto v4() -> tcp { return tcp(PF_INET); }
    static constexpr auto v6() -> tcp { return tcp(PF_INET6); }

    constexpr auto family() const -> int { return this->_D_family; }
    constexpr auto type() const -> int { return SOCK_STREAM; }
    constexpr auto protocol() const -> int { return IPPROTO_TCP; }
};

// ----------------------------------------------------------------------------

class ::stdnet::ip::address_v4
{
public:
    using uint_type = uint_least32_t;
    struct bytes_type;

private:
    uint_type _D_address;

public:
    constexpr address_v4() noexcept: _D_address() {}
    constexpr address_v4(address_v4 const&) noexcept = default;
    constexpr address_v4(bytes_type const&);
    explicit constexpr address_v4(uint_type _A)
        : _D_address(_A)
    {
        if (!(_A <= 0xFF'FF'FF'FF))
        {
            throw ::std::out_of_range("IPv4 address is out of range");
        }
    }

    auto operator=(const address_v4& a) noexcept -> address_v4& = default;

    constexpr auto is_unspecified() const noexcept -> bool { return this->to_uint() == 0u; }
    constexpr auto is_loopback() const noexcept -> bool { return (this->to_uint() & 0xFF'00'00'00) == 0x7F'00'00'00; }
    constexpr auto is_multicast() const noexcept -> bool{ return (this->to_uint() & 0xF0'00'00'00) == 0xE0'00'00'00; }
    constexpr auto to_bytes() const noexcept -> bytes_type;
    constexpr auto to_uint() const noexcept -> uint_type  { return this->_D_address; }
    template<typename _Allocator = ::std::allocator<char>>
    auto to_string(const _Allocator& = _Allocator()) const
        -> ::std::basic_string<char, ::std::char_traits<char>, _Allocator>;

    static constexpr auto any() noexcept -> address_v4 { return address_v4(); }
    static constexpr auto loopback() noexcept -> address_v4 { return address_v4(0x7F'00'00'01u); }
    static constexpr auto broadcast() noexcept -> address_v4 { return address_v4(0xFF'FF'FF'FFu); }
};

#if 0
constexpr bool operator==(const address_v4& a, const address_v4& b) noexcept;
constexpr bool operator!=(const address_v4& a, const address_v4& b) noexcept;
constexpr bool operator< (const address_v4& a, const address_v4& b) noexcept;
constexpr bool operator> (const address_v4& a, const address_v4& b) noexcept;
constexpr bool operator<=(const address_v4& a, const address_v4& b) noexcept;
constexpr bool operator>=(const address_v4& a, const address_v4& b) noexcept;
// 21.5.6, address_v4 creation:
constexpr address_v4 make_address_v4(const address_v4::bytes_type& bytes);
constexpr address_v4 make_address_v4(address_v4::uint_type val);
constexpr address_v4 make_address_v4(v4_mapped_t, const address_v6& a);
address_v4 make_address_v4(const char* str);
address_v4 make_address_v4(const char* str, error_code& ec) noexcept;
address_v4 make_address_v4(const string& str);
address_v4 make_address_v4(const string& str, error_code& ec) noexcept;
address_v4 make_address_v4(string_view str);
address_v4 make_address_v4(string_view str, error_code& ec) noexcept;
// 21.5.7, address_v4 I/O:
template<class CharT, class Traits>
basic_ostream<CharT, Traits>& operator<<(
basic_ostream<CharT, Traits>& os, const address_v4& addr);
#endif

// ----------------------------------------------------------------------------

class ::stdnet::ip::address
{
private:
    union _Address_t
    {
        ::sockaddr_storage _Storage;
        ::sockaddr_in      _Inet;
        ::sockaddr_in6     _Inet6;
    };
    
    _Address_t _D_address;

public:
    constexpr address() noexcept
        : _D_address()
    {
        this->_D_address._Storage.ss_family = PF_INET;
    }
    constexpr address(address const&) noexcept = default;
    constexpr address(::stdnet::ip::address_v4 const& _Address) noexcept
    {
        this->_D_address._Inet.sin_family = AF_INET;
        this->_D_address._Inet.sin_addr.s_addr = htonl(_Address.to_uint());
        this->_D_address._Inet.sin_port = 0xFF'FF;
    }
    constexpr address(::stdnet::ip::address_v6 const&) noexcept;

    auto operator=(address const&) noexcept -> address& = default;
    auto operator=(::stdnet::ip::address_v4 const&) noexcept -> address&;
    auto operator=(::stdnet::ip::address_v6 const&) noexcept -> address&;

    auto _Data() const -> ::sockaddr_storage const& { return this->_D_address._Storage; }
    constexpr auto is_v4() const noexcept -> bool { return this->_D_address._Storage.ss_family == PF_INET; }
    constexpr auto is_v6() const noexcept -> bool { return this->_D_address._Storage.ss_family == PF_INET6; }
    constexpr auto to_v4() -> ::stdnet::ip::address_v4 const;
    constexpr auto to_v6() -> ::stdnet::ip::address_v6 const;
    constexpr auto is_unspecified() const noexcept -> bool;
    constexpr auto is_loopback() const noexcept -> bool;
    constexpr auto is_multicast() const noexcept -> bool;
    template<class _Allocator = ::std::allocator<char>>
    auto to_string(_Allocator const& = _Allocator()) const
        -> ::std::basic_string<char, ::std::char_traits<char>, _Allocator>;
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
        : _D_address(_Address._Data())
    {
        (_Address.is_v4()
         ? reinterpret_cast<::sockaddr_in&>(this->_D_address).sin_port
         : reinterpret_cast<::sockaddr_in6&>(this->_D_address).sin6_port) = htons(_Port);
    }

    constexpr auto protocol() const noexcept -> protocol_type
    {
        return this->_D_address.ss_family == PF_INET? ::stdnet::ip::tcp::v4(): ::stdnet::ip::tcp::v6();
    }
    constexpr auto address() const noexcept -> ::stdnet::ip::address;
    auto address(::stdnet::ip::address const&) noexcept -> void;
    constexpr auto port() const noexcept -> ::stdnet::ip::port_type;
    auto port(::stdnet::ip::port_type) noexcept -> void;

    auto _Data() const -> ::sockaddr const* { return reinterpret_cast<::sockaddr const*>(&this->_D_address); }
    auto _Size() const -> ::socklen_t
    {
        return this->_D_address.ss_family == PF_INET? sizeof(::sockaddr_in): sizeof(::sockaddr_in6);
    }
};

// ----------------------------------------------------------------------------

#endif
