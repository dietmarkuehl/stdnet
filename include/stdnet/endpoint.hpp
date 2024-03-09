// stdnet/endpoint.hpp                                                -*-C++-*-
// ----------------------------------------------------------------------------
//
//  Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Licensed under the Apache License Version 2.0 with LLVM Exceptions
//  (the "License"); you may not use this file except in compliance with
//  the License. You may obtain a copy of the License at
//
//    https://llvm.org/LICENSE.txt
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_ENDPOINT
#define INCLUDED_STDNET_ENDPOINT

#include <cstring>
#include <sys/socket.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    class _Endpoint;
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Endpoint
{
private:
    ::sockaddr_storage _D_data{};
    ::socklen_t        _D_size{sizeof(::sockaddr_storage)};
public:
    _Endpoint() = default;
    _Endpoint(void const* _Data, ::socklen_t _Size)
        : _D_size(_Size)
    {
        ::std::memcpy(&this->_D_data, _Data, ::std::min(_Size, ::socklen_t(sizeof(::sockaddr_storage))));
    }
    template <typename _ET>
    _Endpoint(_ET& _E): _Endpoint(_E._Data(), _E._Size()) {}

    auto _Storage()       -> ::sockaddr_storage& { return this->_D_data; }
    auto _Storage() const -> ::sockaddr_storage const& { return this->_D_data; }
    auto _Data()       -> ::sockaddr*        { return reinterpret_cast<::sockaddr*>(&this->_D_data); }
    auto _Data() const -> ::sockaddr const*  { return reinterpret_cast<::sockaddr const*>(&this->_D_data); }
    auto _Size() const  -> ::socklen_t  { return this->_D_size; }
    auto _Size()        -> ::socklen_t& { return this->_D_size; }
};

// ----------------------------------------------------------------------------

#endif
