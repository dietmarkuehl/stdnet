
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
#include <sys/socket.h>

// ----------------------------------------------------------------------------

namespace stdnet {
    using _Native_handle = int;

    enum class _Opcode: int;
    enum class _Result: ::std::int_least64_t;
    class _Message;
    class _Io_operation_base;
    class _Io_operation;
    class _Context_base;
}

// ----------------------------------------------------------------------------

enum class stdnet::_Opcode
    : int
{
    _Nop,
    _Wakeup,
    _Cancel,
    _Accept,
    _Connect,
    _Send,
    _Receive,
    _Poll,
    _Wait_for,
    _Wait_until
};

// ----------------------------------------------------------------------------

enum class stdnet::_Result
    : ::std::int_least64_t
{
    _Success = 0,
    _Cancelled = -1,     // the operation was cancelled
    _Not_found = -2,     // cancallation didn't find the operation to cancel
    _Not_supported = -3, // the use opcode is not supported
};

// ----------------------------------------------------------------------------

class stdnet::_Message
{
private:
    ::msghdr _D_msg;
public:
    _Message(::iovec* _Vec, int _Vlen)
        : _D_msg{}
    {
        this->_D_msg.msg_iov = _Vec;
        this->_D_msg.msg_iovlen = _Vlen;
    }

    ::msghdr* _Address() { return &this->_D_msg; }
};

// ----------------------------------------------------------------------------

class stdnet::_Io_operation_base
    : public ::stdnet::_Intrusive_node<::stdnet::_Io_operation>
{
private:
    ::stdnet::_Opcode        _D_opcode;
    ::stdnet::_Native_handle _D_handle;

public:
    _Io_operation_base(::stdnet::_Opcode _Opcode, ::stdnet::_Native_handle _Handle): _D_opcode(_Opcode), _D_handle(_Handle) {}
    auto _Opcode() -> ::stdnet::_Opcode { return this->_D_opcode; }
    auto _Handle() -> ::stdnet::_Native_handle { return this->_D_handle; }
};

class stdnet::_Io_operation
    : public ::stdnet::_Io_operation_base
{
private:
    void*                    _D_address;
    ::std::int_least64_t     _D_len;
    ::std::uint32_t          _D_flags;
    ::stdnet::_Result        _D_result{};
    int                      _D_error;

protected:
    virtual auto _Do_complete() -> void = 0;

public:
    _Io_operation(::stdnet::_Opcode _O, void* _A = {}, ::std::int32_t _L = {}, ::stdnet::_Native_handle _H = {}, ::std::uint32_t _F = {})
        : ::stdnet::_Io_operation_base(_O, _H)
        , _D_address(_A)
        , _D_len(_L)
        , _D_flags(_F)
    {
    }
    auto _Complete() -> void { this->_Do_complete(); }

    auto _Address() -> void* { return this->_D_address; }
    auto _Length() -> ::std::int64_t { return this->_D_len; }
    auto _Set_length(::std::int_least64_t _Len) { this->_D_len = _Len; }
    auto _Flags() -> ::std::int64_t { return this->_D_flags; }
    auto _Set_flags(::std::int64_t _Flags) -> void { this->_D_flags = _Flags; }
    auto _Error() -> int { return this->_D_error; }
    auto _Set_error(int _E) { this->_D_error = _E; }

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
    virtual auto _Do_submit(::stdnet::_Io_operation&) -> void = 0;
        // Submit the operation to be queued: won't complete immediately.
    virtual auto _Do_try_or_submit(::stdnet::_Io_operation&) -> void;
        // Try the operation and, if not ready, submit to be queued: otherwise
        // the operation may complete immediately. The precondition is that
        // any attempt to to complete an operation is non-blocking. Thus, any handle
        // used has to be set up to be non-blocking.
        // The default implementation just delegates to to _Submit.
    virtual auto _Do_run_one() -> ::std::size_t = 0;
        // Run and try to complete one operation. This operation may block until
        // there is something to complete. If there is nothing to complete the
        // function returns 0.
    virtual auto _Do_run() -> ::std::size_t;
        // Run and try to complete multiple operations. The operation may block
        // and/or complete operations as long as there is something to complete.
        // The function returns the number of completed operations. The default
        // implementation calls _Run_one until this function returns 0.

public:
    _Context_base() = default;
    _Context_base(_Context_base&&) = delete;
    virtual ~_Context_base() = default;

    auto _Submit(::stdnet::_Io_operation& _Op) -> void { this->_Do_submit(_Op); }
    auto _Try_or_submit(::stdnet::_Io_operation& _Op) -> void { return _Do_try_or_submit(_Op); }
    auto _Run_one() -> ::std::size_t { return this->_Do_run_one(); }
    auto _Run() -> ::std::size_t { return this->_Do_run(); }
};

// ----------------------------------------------------------------------------

inline auto stdnet::_Context_base::_Do_try_or_submit(::stdnet::_Io_operation& _Op) -> void
{
    this->_Submit(_Op);
}

inline auto stdnet::_Context_base::_Do_run() -> ::std::size_t
{
    ::std::size_t _Rc{};
    while (0u != this->_Do_run_one())
    {
        ++_Rc;
    }
    return _Rc;
}

// ----------------------------------------------------------------------------

#endif
