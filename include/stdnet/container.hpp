// stdnet/container.hpp                                               -*-C++-*-
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
