// stdnet/container.hpp                                               -*-C++-*-
// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
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

#ifndef INCLUDED_STDNET_CONTAINER
#define INCLUDED_STDNET_CONTAINER

#include <stdnet/netfwd.hpp>
#include <cstddef>
#include <variant>
#include <vector>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    template <typename _Record>
    class _Container;
}

// ----------------------------------------------------------------------------

template <typename _Record>
class stdnet::_Hidden::_Container
{
private:
    ::std::vector<::std::variant<::std::size_t, _Record>> _Records;
    ::std::size_t                                         _Free{};

public:
    auto _Insert(_Record _R) -> ::stdnet::_Hidden::_Socket_id;
    auto _Erase(::stdnet::_Hidden::_Socket_id _Id) -> void;
    auto operator[](::stdnet::_Hidden::_Socket_id _Id) -> _Record&;
};

// ----------------------------------------------------------------------------

template <typename _Record>
inline auto stdnet::_Hidden::_Container<_Record>::_Insert(_Record _R) -> ::stdnet::_Hidden::_Socket_id
{
    if (this->_Free == this->_Records.size())
    {
        this->_Records.emplace_back(::std::move(_R));
        return ::stdnet::_Hidden::_Socket_id(this->_Free++);
    }
    else
    {
        ::std::size_t _Rc(std::exchange(this->_Free, ::std::get<0>(this->_Records[this->_Free])));
        this->_Records[_Rc] = ::std::move(_R);
        return ::stdnet::_Hidden::_Socket_id(_Rc);
    }
}

template <typename _Record>
inline auto stdnet::_Hidden::_Container<_Record>::_Erase(::stdnet::_Hidden::_Socket_id _Id) -> void
{
    this->_Records[::std::size_t(_Id)] = std::exchange(this->_Free, ::std::size_t(_Id));
}

template <typename _Record>
inline auto stdnet::_Hidden::_Container<_Record>::operator[](::stdnet::_Hidden::_Socket_id _Id) -> _Record&
{
    return ::std::get<1>(this->_Records[::std::size_t(_Id)]);
}

// ----------------------------------------------------------------------------

#endif
