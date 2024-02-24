// stdnet/buffer.hpp                                                  -*-C++-*-
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

#ifndef INCLUDED_STDNET_BUFFER
#define INCLUDED_STDNET_BUFFER

#include <sys/socket.h>
#include <cassert>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace stdnet
{
    struct buffer;
    struct buffer_sequence;

    template <::std::size_t _S>
    auto mutable_buffer(char (&)[_S]) -> ::stdnet::buffer;
    template <::std::size_t _S>
    auto mutable_buffer(char (&)[_S], ::std::size_t) -> ::stdnet::buffer;
}

// ----------------------------------------------------------------------------

struct stdnet::buffer
{
    ::iovec _Vec;
    buffer(void* _B, ::std::size_t _L): _Vec{ .iov_base = _B, .iov_len = _L } {}

    auto data() -> ::iovec*      { return &this->_Vec; }
    auto size() -> ::std::size_t { return 1u; }
};

template <::std::size_t _S>
inline auto stdnet::mutable_buffer(char (&_B)[_S]) -> ::stdnet::buffer
{
    return ::stdnet::buffer(_B, _S);
}

template <::std::size_t _S>
inline auto stdnet::mutable_buffer(char (& _B)[_S], ::std::size_t _Size) -> ::stdnet::buffer
{
    assert(_Size <= _S);
    return ::stdnet::buffer(_B, _Size);
}

// ----------------------------------------------------------------------------

#endif
