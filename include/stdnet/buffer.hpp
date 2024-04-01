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
#include <string>
#include <system_error>
#include <cassert>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace stdnet
{
    enum class stream_errc: int;

    auto stream_category() noexcept -> ::std::error_category const&;

    auto make_error_code(::stdnet::stream_errc) noexcept -> ::std::error_code;
    auto make_error_condition(::stdnet::stream_errc) noexcept -> ::std::error_condition;

    class mutable_buffer;
    class const_buffer;

    template <typename> struct is_mutable_buffer_sequence;
    template <typename> struct is_const_buffer_sequence;
    template <typename> struct is_dynamic_buffer;

    struct buffer_sequence;

    template <::std::size_t _S>
    auto buffer(char (&)[_S]) -> ::stdnet::mutable_buffer;
    auto buffer(char*, ::std::size_t) -> ::stdnet::mutable_buffer;
    auto buffer(char const*, ::std::size_t) -> ::stdnet::const_buffer;
}

// ----------------------------------------------------------------------------

enum class stdnet::stream_errc: int
{
    eof,
    not_found
};

// ----------------------------------------------------------------------------

inline auto stdnet::stream_category() noexcept -> ::std::error_category const&
{
    struct _Category
        : ::std::error_category
    {
        auto name() const noexcept -> char const* override
        {
            return "stream_error";
        }
        auto message(int) const noexcept -> ::std::string override
        {
            return {};
        }
    };
    static _Category _Rc{};
    return _Rc; 
}

// ----------------------------------------------------------------------------

struct stdnet::mutable_buffer
{
    ::iovec _Vec;
    mutable_buffer(void* _B, ::std::size_t _L): _Vec{ .iov_base = _B, .iov_len = _L } {}

    auto data() -> ::iovec*      { return &this->_Vec; }
    auto size() -> ::std::size_t { return 1u; }
};

struct stdnet::const_buffer
{
    ::iovec _Vec;
    const_buffer(void const* _B, ::std::size_t _L): _Vec{ .iov_base = const_cast<void*>(_B), .iov_len = _L } {}

    auto data() -> ::iovec*      { return &this->_Vec; }
    auto size() -> ::std::size_t { return 1u; }
};

template <::std::size_t _S>
inline auto stdnet::buffer(char (&_B)[_S]) -> ::stdnet::mutable_buffer
{
    return ::stdnet::mutable_buffer(_B, _S);
}

inline auto stdnet::buffer(char* _B, ::std::size_t _Size) -> ::stdnet::mutable_buffer
{
    return ::stdnet::mutable_buffer(_B, _Size);
}

inline auto stdnet::buffer(char const* _B, ::std::size_t _Size) -> ::stdnet::const_buffer
{
    return ::stdnet::const_buffer(_B, _Size);
}

// ----------------------------------------------------------------------------

#endif
