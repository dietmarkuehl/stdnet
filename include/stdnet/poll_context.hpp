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
#include <chrono>
#include <vector>
#include <cassert>
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>

// ----------------------------------------------------------------------------

namespace stdnet
{
    class _Poll_context_base;
    class _Poll_context;
}

// ----------------------------------------------------------------------------

class stdnet::_Poll_context_base
{
private:
    int _Fds[2];

public:
    _Poll_context_base();
    _Poll_context_base(_Poll_context_base&&) = delete;
    ~_Poll_context_base();
    auto _Produce() -> void;
    auto _Consume() -> void;
    auto _Consumer_fd() -> int;
};

// ----------------------------------------------------------------------------

class stdnet::_Poll_context
    : public ::stdnet::_Context_base
    , private ::stdnet::_Poll_context_base
    , private ::stdnet::_Io_operation_base
{
private:
    struct _Wakeup_fun
    {
        ::stdnet::_Poll_context* _D_context;
        auto operator()() -> void { this->_D_context->_Produce(); }
    };
    using _Timer = ::std::pair<::std::chrono::system_clock::time_point, ::stdnet::_Io_operation*>;
    using _Timer_compare = decltype([](auto const& _A, auto const& _B){ return _A.first < _B.first; });
    using _Queue_type = ::stdnet::_Intrusive_queue<::stdnet::_Io_operation, ::stdnet::_Poll_context::_Wakeup_fun>;

    ::stdnet::_Poll_context::_Queue_type                _D_queue;
    ::stdnet::_Intrusive_list<::stdnet::_Io_operation>  _D_complete{};
    ::std::vector<::pollfd>                             _D_poll{};
    ::std::vector<::stdnet::_Io_operation_base*>        _D_outstanding{};
    ::std::vector<::stdnet::_Poll_context::_Timer>      _D_timers{};

    auto _Poll(::std::chrono::system_clock::time_point const& _Now) -> void;
    auto _Update_work() -> void;
    auto _Add(::stdnet::_Io_operation_base& _Op, short _Ev) -> void;
    auto _Remove(::std::size_t _Index) -> void;
    auto _Add_timer(::stdnet::_Io_operation& _Op, ::std::chrono::system_clock::time_point const& time) -> void;
    auto _Try_complete(::stdnet::_Io_operation_base& _Op, short _Ev) -> bool;
    auto _Complete(::stdnet::_Io_operation& _Op, ::std::int_least32_t _Rc) -> void;
    auto _Cancel(::stdnet::_Io_operation& _Op) -> void;

    auto _Accept(::stdnet::_Io_operation& _Op) -> bool;
    auto _Start_connect(::stdnet::_Io_operation& _Op) -> bool;
    auto _Connect(::stdnet::_Io_operation& _Op) -> bool;
    auto _Send(::stdnet::_Io_operation& _Op) -> bool;
    auto _Receive(::stdnet::_Io_operation& _Op) -> bool;
    auto _Poll(::stdnet::_Io_operation& _Op, short _Ev) -> bool;

protected:
    auto _Do_submit(::stdnet::_Io_operation&) -> void override;
    auto _Do_run_one() -> ::std::size_t override;

public:
    _Poll_context();
    ~_Poll_context();
    auto empty() -> bool;
};

// ----------------------------------------------------------------------------

inline stdnet::_Poll_context_base::_Poll_context_base()
{
    if (::pipe(this->_Fds) < 0)
    {
        throw ::std::system_error(errno, ::std::system_category(), "creating notification descriptor");
    }
}

inline stdnet::_Poll_context_base::~_Poll_context_base()
{
    ::close(this->_Fds[0]);
    ::close(this->_Fds[1]);
}

inline auto stdnet::_Poll_context_base::_Produce() -> void
{
    char _C{'B'};
    ::write(this->_Fds[1], &_C, sizeof(_C));
}

inline auto stdnet::_Poll_context_base::_Consume() -> void
{
    char _B[64];
    ::read(this->_Fds[0], &_B, sizeof(_B));
}

inline auto stdnet::_Poll_context_base::_Consumer_fd() -> int
{
    return this->_Fds[0];
}

// ----------------------------------------------------------------------------

inline stdnet::_Poll_context::_Poll_context()
    : ::stdnet::_Poll_context_base{}
    , ::stdnet::_Io_operation_base(::stdnet::_Opcode::_Wakeup, this->_Consumer_fd())
    , _D_queue(this)
{
    this->_Add(*this, POLLIN);
}

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
    return this->_D_poll.size() == 1u && this->_D_timers.empty() && this->_D_complete.empty();
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
        auto _Now{::std::chrono::system_clock::now()};
        if (!this->_D_timers.empty() && this->_D_timers.front().first <= _Now)
        {
            auto& _Timer{this->_D_timers.front()};
            ::std::pop_heap(this->_D_timers.begin(), this->_D_timers.end(), _Timer_compare());
            this->_D_timers.pop_back();
            _Timer.second->_Complete();
            return 1u;
        }
        this->_Update_work();
        if (this->_D_complete.empty())
        {
            if (this->_D_outstanding.size() <= 1u && this->_D_timers.empty())
            {
                break;
            }
            else
            {
            }
            this->_Poll(_Now);
        }
    }
    
    return 0u;
}

inline auto stdnet::_Poll_context::_Poll(::std::chrono::system_clock::time_point const& _Now) -> void
{
    int _Timeout{-1};
    if (not this->_D_timers.empty())
    {
        _Timeout = ::std::chrono::duration_cast<::std::chrono::milliseconds>(this->_D_timers.front().first - _Now).count();
    }
    auto _Rc{::poll(this->_D_poll.data(), this->_D_poll.size(), _Timeout)};
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
            this->_Try_complete(*this->_D_outstanding[_I], this->_D_poll[_I].revents);
            this->_D_poll[_I].revents = {};
            this->_Remove(_I);
        }
    }
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Poll_context::_Add(::stdnet::_Io_operation_base& _Op, short _Ev) -> void
{
    this->_D_poll.push_back(pollfd{ .fd = _Op._Handle(), .events = _Ev, .revents = {} });
    this->_D_outstanding.push_back(&_Op);
}

inline auto stdnet::_Poll_context::_Remove(::std::size_t _Index) -> void
{
    if (this->_D_outstanding[_Index] == this)
    {
        return;
    }
    this->_D_poll[_Index] = this->_D_poll.back();
    this->_D_poll.pop_back();

    this->_D_outstanding[_Index] = this->_D_outstanding.back();
    this->_D_outstanding.pop_back();
}

inline auto stdnet::_Poll_context::_Add_timer(::stdnet::_Io_operation& _Op,
                                              ::std::chrono::system_clock::time_point const& time) -> void
{
    this->_D_timers.emplace_back(time, &_Op);
    ::std::push_heap(this->_D_timers.begin(), this->_D_timers.end(), _Timer_compare());
}

inline auto stdnet::_Poll_context::_Try_complete(::stdnet::_Io_operation_base& _Op, short _Ev) -> bool
{
    return ::std::invoke([this, &_Op, _Ev]{
        switch (_Op._Opcode())
        {
        default:
            assert("unknown opcode in poll context" == nullptr);
            break;
        case ::stdnet::_Opcode::_Nop:
            static_cast<::stdnet::_Io_operation&>(_Op)._Set_result(::stdnet::_Result::_Success);
            this->_D_complete.push_back(static_cast<::stdnet::_Io_operation&>(_Op));
            break;
        case ::stdnet::_Opcode::_Wakeup:
            this->_Consume();
            break;
        case ::stdnet::_Opcode::_Cancel:
            this->_Cancel(static_cast<::stdnet::_Io_operation&>(_Op));
            break;
        case ::stdnet::_Opcode::_Accept:
            return this->_Accept(static_cast<::stdnet::_Io_operation&>(_Op));
        case ::stdnet::_Opcode::_Connect:
            return this->_Connect(static_cast<::stdnet::_Io_operation&>(_Op));
        case ::stdnet::_Opcode::_Receive:
            return this->_Receive(static_cast<::stdnet::_Io_operation&>(_Op));
        case ::stdnet::_Opcode::_Send:
            return this->_Send(static_cast<::stdnet::_Io_operation&>(_Op));
        case ::stdnet::_Opcode::_Poll:
            return this->_Poll(static_cast<::stdnet::_Io_operation&>(_Op), _Ev);
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
                         static_cast<::stdnet::_Io_operation*>(_Op._Address()))};
    if (_It == this->_D_outstanding.end())
    {
        _Op._Set_result(::stdnet::_Result::_Not_found);
        this->_D_complete.push_back(_Op);
    }
    else if (*_It != this)
    {
        auto _Index{_It - this->_D_outstanding.begin()};
        ::stdnet::_Io_operation* _Ptr{static_cast<::stdnet::_Io_operation*>(this->_D_outstanding[_Index])};

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
        case ::stdnet::_Opcode::_Receive:
            this->_Add(_Op, POLLIN);
            break;
        case ::stdnet::_Opcode::_Poll:
            this->_Add(_Op, _Op._Flags());
            break;
        case ::stdnet::_Opcode::_Wait_for:
            this->_Add_timer(_Op,  ::std::chrono::system_clock::now() + *static_cast<::std::chrono::system_clock::duration*>(_Op._Address()));
            break;
        case ::stdnet::_Opcode::_Wait_until:
            this->_Add_timer(_Op, *static_cast<::std::chrono::system_clock::time_point*>(_Op._Address()));
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
    auto _Rc{::sendmsg(_Op._Handle(), static_cast<::msghdr*>(_Op._Address()), _Op._Flags())};
    if (0 <= _Rc)
    {
        _Op._Set_result(_Rc);
    }
    else if (errno == EAGAIN)
    {
        return true;
    }
    else
    {
        _Op._Set_error(errno);
    }
    this->_D_complete.push_back(_Op);
    return false;
}

inline auto stdnet::_Poll_context::_Receive(::stdnet::_Io_operation& _Op) -> bool
{
    auto _Rc{::recvmsg(_Op._Handle(), static_cast<::msghdr*>(_Op._Address()), _Op._Flags())};
    if (0 <= _Rc)
    {
        _Op._Set_result(_Rc);
    }
    else if (errno == EAGAIN)
    {
        return true;
    }
    else
    {
        _Op._Set_error(errno);
    }
    this->_D_complete.push_back(_Op);
    return false;
}

inline auto stdnet::_Poll_context::_Poll(::stdnet::_Io_operation& _Op, short _Ev) -> bool
{
    _Op._Set_result(stdnet::_Result::_Success);
    _Op._Set_flags(_Ev);
    this->_D_complete.push_back(_Op);
    return false;
}

// ----------------------------------------------------------------------------

#endif
