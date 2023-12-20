// stdnet/poll_context.hpp                                            -*-C++-*-
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

#ifndef INCLUDED_STDNET_POLL_CONTEXT
#define INCLUDED_STDNET_POLL_CONTEXT

#include <stdnet/context_base.hpp>
#include <stdnet/container.hpp>
#include <algorithm>
#include <vector>
#include <cerrno>
#include <poll.h>

// ----------------------------------------------------------------------------

namespace stdnet
{
    class _Poll_context;
}

// ----------------------------------------------------------------------------

class stdnet::_Poll_context
    : public ::stdnet::_Context_base
{
private:
    //-dk:TODO hook wake-up into the queue
    ::stdnet::_Intrusive_queue<::stdnet::_Io_base> _D_queue{};
    ::stdnet::_Intrusive_list<::stdnet::_Io_base>  _D_complete{};
    ::std::vector<::pollfd>                        _D_poll{};
    ::std::vector<::stdnet::_Io_base*>             _D_outstanding{};

    auto _Poll() -> void;
    auto _Update_work() -> void;
    auto _Add(::stdnet::_Io_base& _Op, short _Ev) -> void;
    auto _Cancel(::stdnet::_Io_base& _Op) -> void;

protected:
    auto _Do_submit(::stdnet::_Io_base&) -> void override;
    auto _Do_run_one() -> ::std::size_t override;

public:
    ~_Poll_context();
    auto empty() -> bool;
};

// ----------------------------------------------------------------------------

inline stdnet::_Poll_context::~_Poll_context()
{
    //-dk:TODO process completion list if non-empty
    //-dk:TODO cancel all outstanding work
    //-dk:TODO cancel all timers
    //-dk:TODO cancel all items in the queue
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::empty() -> bool
{
    return this->_D_poll.empty() && this->_D_complete.empty();
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Do_submit(::stdnet::_Io_base& _Op) -> void
{
    this->_D_queue._Push(_Op);
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Do_run_one() -> ::std::size_t
{
    while (true)
    {
        if (not this->_D_complete.empty())
        {
            auto& _Op{this->_D_complete.front()};
            this->_D_complete.pop_front();
            _Op._Complete();
            return 1u;
        }
        this->_Update_work();
        if (this->_D_complete.empty())
        {
            if (this->_D_outstanding.empty())
            {
                break;
            }
            this->_Poll();
        }
    }
    
    return 0u;
}

inline auto stdnet::_Poll_context::_Poll() -> void
{
    auto _Rc{::poll(this->_D_poll.data(), this->_D_poll.size(), -1)};
    if (_Rc < 0)
    {
        switch (errno)
        {
        default:
        case EAGAIN:
        case EINTR:
            break;
        case EFAULT:
            throw ::std::logic_error("invalid pointer used with poll");
        case EINVAL:
            throw ::std::runtime_error("too many file descriptors for poll");
        }
    }

}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Add(::stdnet::_Io_base& _Op, short _Ev) -> void
{
    this->_D_poll.push_back(pollfd{ .fd = _Op._Handle(), .events = _Ev, .revents = {} });
    this->_D_outstanding.push_back(&_Op);
}

inline auto stdnet::_Poll_context::_Cancel(::stdnet::_Io_base& _Op) -> void
{
    //-dk:TODO deal with cancelling timers
    auto _It{::std::find(this->_D_outstanding.begin(), this->_D_outstanding.end(),
                         _Op._Address())};
    if (_It == this->_D_outstanding.end())
    {
        _Op._Set_result(::stdnet::_Result::_Not_found);
        this->_D_complete.push_back(_Op);
    }
    else
    {
        auto _Index{_It - this->_D_outstanding.begin()};
        ::std::swap(this->_D_poll[_Index], this->_D_poll.back());
        this->_D_poll.pop_back();
        ::stdnet::_Io_base* _Ptr{this->_D_outstanding[_Index]};
        this->_D_outstanding[_Index] = this->_D_outstanding.back();
        this->_D_outstanding.pop_back();

        _Ptr->_Set_result(::stdnet::_Result::_Cancelled);
        this->_D_complete.push_back(*_Ptr);
        _Op._Set_result(::stdnet::_Result::_Success);
        this->_D_complete.push_back(_Op);
    }
}

inline auto stdnet::_Poll_context::_Update_work() -> void
{
    auto _Work{this->_D_queue._Extract()};
    while (not _Work.empty())
    {
        auto& _Op{_Work.front()};
        _Work.pop_front();
        switch (_Op._Opcode())
        {
        default:
            //-dk:TODO deal with unknown opcodes: produce an error
            break;
        case ::stdnet::_Opcode::_Nop:
            this->_D_complete.push_back(_Op);
            break;
        case ::stdnet::_Opcode::_Cancel:
            this->_Cancel(_Op);
            break;
        case ::stdnet::_Opcode::_Accept:
        case ::stdnet::_Opcode::_Connect:
        case ::stdnet::_Opcode::_Receive:
            this->_Add(_Op, POLLIN);
            break;
        case ::stdnet::_Opcode::_Send:
            this->_Add(_Op, POLLOUT);
            break;
        }
    }
}

// ----------------------------------------------------------------------------

#endif
