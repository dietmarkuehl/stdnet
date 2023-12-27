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
#include <cassert>
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>

#include <iostream> //-dk:TODO remove

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
    ::stdnet::_Intrusive_queue<::stdnet::_Io_operation> _D_queue{};
    ::stdnet::_Intrusive_list<::stdnet::_Io_operation>  _D_complete{};
    ::std::vector<::pollfd>                        _D_poll{};
    ::std::vector<::stdnet::_Io_operation*>             _D_outstanding{};

    auto _Poll() -> void;
    auto _Update_work() -> void;
    auto _Add(::stdnet::_Io_operation& _Op, short _Ev) -> void;
    auto _Remove(::std::size_t _Index) -> void;
    auto _Try_complete(::stdnet::_Io_operation& _Op) -> bool;
    auto _Complete(::stdnet::_Io_operation& _Op, ::std::int_least32_t _Rc) -> void;
    auto _Cancel(::stdnet::_Io_operation& _Op) -> void;

    auto _Accept(::stdnet::_Io_operation& _Op) -> bool;
    auto _Start_connect(::stdnet::_Io_operation& _Op) -> bool;
    auto _Connect(::stdnet::_Io_operation& _Op) -> bool;
    auto _Send(::stdnet::_Io_operation& _Op) -> bool;
    auto _Receive(::stdnet::_Io_operation& _Op) -> bool;

protected:
    auto _Do_submit(::stdnet::_Io_operation&) -> void override;
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

inline auto stdnet::_Poll_context::_Do_submit(::stdnet::_Io_operation& _Op) -> void
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
    std::cout << "entering poll: size=" << this->_D_poll.size() << "\n" << std::flush;
    for (auto const& p: this->_D_poll)
        std::cout << "   " << p.fd << " events=" << (p.events & POLLIN? "in ": "") << (p.events & POLLOUT? "out": "") << "\n" << std::flush;
    auto _Rc{::poll(this->_D_poll.data(), this->_D_poll.size(), -1)};
    std::cout << "    poll result=" << _Rc << "\n" << std::flush;
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
    for (::std::size_t _I{_D_poll.size()}; 0u < _I--; )
    {
        if (this->_D_poll[_I].revents)
        {
            bool _Done{this->_Try_complete(*this->_D_outstanding[_I])};
            this->_D_poll[_I].revents = {};
            this->_Remove(_I);
        }
    }
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Add(::stdnet::_Io_operation& _Op, short _Ev) -> void
{
    this->_D_poll.push_back(pollfd{ .fd = _Op._Handle(), .events = _Ev, .revents = {} });
    this->_D_outstanding.push_back(&_Op);
}

inline auto stdnet::_Poll_context::_Remove(::std::size_t _Index) -> void
{
    this->_D_poll[_Index] = this->_D_poll.back();
    this->_D_poll.pop_back();

    this->_D_outstanding[_Index] = this->_D_outstanding.back();
    this->_D_outstanding.pop_back();
}

inline auto stdnet::_Poll_context::_Try_complete(::stdnet::_Io_operation& _Op) -> bool
{
    return ::std::invoke([this, &_Op]{
        switch (_Op._Opcode())
        {
        default:
            assert("unknown opcode in poll context" == nullptr);
            break;
        case ::stdnet::_Opcode::_Nop:
            _Op._Set_result(::stdnet::_Result::_Success);
            this->_D_complete.push_back(_Op);
            break;
        case ::stdnet::_Opcode::_Cancel:
            this->_Cancel(_Op);
            break;
        case ::stdnet::_Opcode::_Accept:
            return this->_Accept(_Op);
            break;
        case ::stdnet::_Opcode::_Connect:
            return this->_Connect(_Op);
            break;
        case ::stdnet::_Opcode::_Receive:
            return this->_Receive(_Op);
            break;
        case ::stdnet::_Opcode::_Send:
            return this->_Send(_Op);
            break;
        }
        return false;
    });
}

inline auto stdnet::_Poll_context::_Complete(::stdnet::_Io_operation& _Op, ::std::int_least32_t _Rc) -> void
{
    _Op._Set_result(_Rc);
    this->_D_complete.push_back(_Op);
}

inline auto stdnet::_Poll_context::_Cancel(::stdnet::_Io_operation& _Op) -> void
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
        ::stdnet::_Io_operation* _Ptr{this->_D_outstanding[_Index]};

        this->_Remove(_Index);

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
            _Op._Set_result(::stdnet::_Result::_Not_supported);
            this->_D_complete.push_back(_Op);
            break;
        case ::stdnet::_Opcode::_Nop:
            _Op._Set_result(::stdnet::_Result::_Success);
            this->_D_complete.push_back(_Op);
            break;
        case ::stdnet::_Opcode::_Cancel:
            this->_Cancel(_Op);
            break;
        case ::stdnet::_Opcode::_Accept:
        case ::stdnet::_Opcode::_Receive:
            this->_Add(_Op, POLLIN);
            break;
        case ::stdnet::_Opcode::_Connect:
            if (this->_Start_connect(_Op))
            {
                this->_D_complete.push_back(_Op);
            }
            else
            {
                this->_Add(_Op, POLLOUT);
            }
            break;
        case ::stdnet::_Opcode::_Send:
            this->_Add(_Op, POLLOUT);
            break;
        }
    }
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Accept(::stdnet::_Io_operation& _Op) -> bool
{
    ::socklen_t _Len(_Op._Length());
    auto _Rc{::accept(_Op._Handle(),
                      static_cast<::sockaddr*>(_Op._Address()),
                      &_Len)};
    if (_Rc < 0)
    {
        if (errno == EWOULDBLOCK)
        {
            return false;
        }
        _Rc = -errno;
    }
    else
    {
        _Op._Set_length(_Len);
    }
    this->_Complete(_Op, _Rc);
    return true;
}

inline auto stdnet::_Poll_context::_Start_connect(::stdnet::_Io_operation& _Op) -> bool
{
    if (::connect(_Op._Handle(), static_cast<::sockaddr const*>(_Op._Address()), _Op._Length()) < 0)
    {
        switch (errno)
        {
        default:
            _Op._Set_result(-errno);
            return true;
        case EINTR:
        case EINPROGRESS:
            return false;
        }

    }
    _Op._Set_result(::stdnet::_Result::_Success);
    return true;
}

inline auto stdnet::_Poll_context::_Connect(::stdnet::_Io_operation& _Op) -> bool
{
    int         _Rc{};
    ::socklen_t _Len{sizeof(_Rc)};
    if (::getsockopt(_Op._Handle(), SOL_SOCKET, SO_ERROR, &_Rc, &_Len))
    {
        _Op._Set_error(errno);
    }
    else if (_Rc)
    {
        _Op._Set_error(_Rc);
    }
    else
    {
        _Op._Set_result(_Rc);
    }
    this->_D_complete.push_back(_Op);
    return true;
}

inline auto stdnet::_Poll_context::_Send(::stdnet::_Io_operation& _Op) -> bool
{
    return false;
}

inline auto stdnet::_Poll_context::_Receive(::stdnet::_Io_operation& _Op) -> bool
{
    return false;
}

// ----------------------------------------------------------------------------

#endif
