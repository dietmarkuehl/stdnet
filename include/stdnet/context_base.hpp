// stdnet/context_base.hpp                                            -*-C++-*-
// ----------------------------------------------------------------------------
//  Copyright (C) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
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

#ifndef INCLUDED_STDNET_CONTEXT_BASE
#define INCLUDED_STDNET_CONTEXT_BASE

#include <stdnet/container.hpp>
#include <cstdint>

// ----------------------------------------------------------------------------

namespace stdnet {
    using _Native_handle = int;

    enum class _Opcode: int;
    enum class _Result: ::std::int_least64_t;
    class _Io_base;
    class _Context_base;
}

// ----------------------------------------------------------------------------

enum class stdnet::_Opcode
    : int
{
    _Nop,
    _Cancel,
    _Accept,
    _Connect,
    _Send,
    _Receive,
};

// ----------------------------------------------------------------------------

enum class stdnet::_Result
    : ::std::int_least64_t
{
    _Success = 0,
    _Cancelled = -1,
    _Not_found = -2,
};

// ----------------------------------------------------------------------------

class stdnet::_Io_base
    : public ::stdnet::_Intrusive_node<::stdnet::_Io_base>
{
private:
    ::stdnet::_Opcode        _D_opcode;
    ::stdnet::_Native_handle _D_handle;
    void*                    _D_address;
    ::std::uint32_t          _D_len;
    ::std::uint32_t          _D_flags;
    ::stdnet::_Result        _D_result{};

protected:
    virtual auto _Do_complete() -> void = 0;

public:
    _Io_base(::stdnet::_Opcode _O, void* _A = {}, ::stdnet::_Native_handle _H = {}, ::std::int32_t _L = {}, ::std::uint32_t _F = {})
        : _D_opcode(_O)
        , _D_handle(_H)
        , _D_address(_A)
        , _D_len(_L)
        , _D_flags(_F)
    {
    }
    auto _Complete() -> void { this->_Do_complete(); }

    auto _Opcode() -> ::stdnet::_Opcode { return this->_D_opcode; }
    auto _Handle() -> ::stdnet::_Native_handle { return this->_D_handle; }
    auto _Address() -> void* { return this->_D_address; }

    auto _Success() -> bool { return 0 <= this->_ResultValue(); }
    auto _Result() -> ::stdnet::_Result { return this->_D_result; }
    auto _ResultValue() -> ::std::int_least64_t { return ::std::int_least64_t(this->_D_result); }
    auto _Set_result(::stdnet::_Result _Result) { this->_D_result = _Result; }
    auto _Set_result(::std::int_least64_t _Result) { this->_D_result = ::stdnet::_Result(_Result); }
};

// ----------------------------------------------------------------------------

class stdnet::_Context_base
{
protected:
    virtual auto _Do_submit(_Io_base&) ->void = 0;
    virtual auto _Do_run_one() -> ::std::size_t = 0;

    _Context_base(_Context_base&&) = delete;

public:
    _Context_base() = default;
    auto _Submit(_Io_base& _Op) -> void { this->_Do_submit(_Op); }
    auto _Run_one() -> ::std::size_t { return this->_Do_run_one(); }
};

// ----------------------------------------------------------------------------
#endif
