// stdnet/libevent_context.hpp                                        -*-C++-*-
// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
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

#ifndef INCLUDED_STDNET_LIBEVENT_CONTEXT
#define INCLUDED_STDNET_LIBEVENT_CONTEXT

#include <stdnet/netfwd.hpp>
#include <stdnet/container.hpp>
#include <stdnet/context_base.hpp>
#include <memory>
#include <new>
#include <system_error>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <event2/event.h>
#include <event2/util.h>
#include <unistd.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    extern "C"
    {
        auto _Libevent_callback(int, short, void*) -> void;
    }
    class _Libevent_error_category_t;
    class _Libevent_record;
    class _Libevent_context;

    auto _Libevent_error_category() -> _Libevent_error_category_t const&;
}

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Libevent_error_category_t
    : public ::std::error_category
{
    auto name() const noexcept -> char const* override { return "libevent-category"; }
    auto message(int _E) const -> ::std::string override { return evutil_socket_error_to_string(_E); }
};

inline auto stdnet::_Hidden::_Libevent_error_category() -> stdnet::_Hidden::_Libevent_error_category_t const&
{
    static stdnet::_Hidden::_Libevent_error_category_t _Rc{};
    return _Rc;
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Libevent_callback(int, short, void* _Arg) -> void
{
    auto _Op(static_cast<::stdnet::_Hidden::_Io_base*>(_Arg));
    _Op->_Work(*_Op->_Context, _Op);
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Libevent_record final
{
    _Libevent_record(::stdnet::_Stdnet_native_handle_type _H): _Handle(_H) {}
    ::stdnet::_Stdnet_native_handle_type                   _Handle;
    bool                                                   _Blocking{true};
};

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Libevent_context
    : public ::stdnet::_Hidden::_Context_base
{
private:
    ::stdnet::_Hidden::_Container<::stdnet::_Hidden::_Libevent_record> _D_sockets;
    ::std::unique_ptr<::event_base, auto(*)(event_base*)->void>    _Context;

    auto _Make_socket(int) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void override;
    auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type override;
    auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void override;
    auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::_Hidden::_Endpoint const&, ::std::error_code&) -> void override;
    auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void override;

    auto run_one() -> ::std::size_t override;

    auto _Cancel(::stdnet::_Hidden::_Io_base*, ::stdnet::_Hidden::_Io_base*) -> void override;
    auto _Accept(::stdnet::_Hidden::_Context_base::_Accept_operation*) -> bool override;
    auto _Connect(::stdnet::_Hidden::_Context_base::_Connect_operation*) -> bool override;
    auto _Receive(::stdnet::_Hidden::_Context_base::_Receive_operation*) -> bool override;
    auto _Send(::stdnet::_Hidden::_Context_base::_Send_operation*) -> bool override;
    auto _Resume_after(::stdnet::_Hidden::_Context_base::_Resume_after_operation*) -> bool override;
    auto _Resume_at(::stdnet::_Hidden::_Context_base::_Resume_at_operation*) -> bool override;

public:
    _Libevent_context();
    _Libevent_context(::event_base*);
};

// ----------------------------------------------------------------------------

inline stdnet::_Hidden::_Libevent_context::_Libevent_context()
    : _Context(::event_base_new(), +[](::event_base* _C){ ::event_base_free(_C); })
{
}

inline stdnet::_Hidden::_Libevent_context::_Libevent_context(::event_base* _C)
    : _Context(_C, +[](::event_base*){})
{
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Libevent_context::_Make_socket(int _Fd) -> ::stdnet::_Hidden::_Socket_id
{
    return this->_D_sockets._Insert(_Fd);
}

inline auto stdnet::_Hidden::_Libevent_context::_Make_socket(int _D, int _T, int _P, ::std::error_code& _Error)
    -> ::stdnet::_Hidden::_Socket_id
{
    int _Fd(::socket(_D, _T, _P));
    if (_Fd < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
        return ::stdnet::_Hidden::_Socket_id::_Invalid;
    }
    return this->_Make_socket(_Fd);
}

inline auto stdnet::_Hidden::_Libevent_context::_Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void
{
    _Stdnet_native_handle_type _Handle(this->_D_sockets[_Id]._Handle);
    this->_D_sockets._Erase(_Id);
    if (::close(_Handle) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Libevent_context::_Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type
{
    return this->_D_sockets[_Id]._Handle;
}

inline auto stdnet::_Hidden::_Libevent_context::_Set_option(::stdnet::_Hidden::_Socket_id _Id,
                                                         int                           _Level,
                                                         int                           _Name,
                                                         void const*                   _Data,
                                                         ::socklen_t                   _Size,
                                                         ::std::error_code&            _Error)
    -> void
{
    if (::setsockopt(this->_Native_handle(_Id), _Level, _Name, _Data, _Size) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Libevent_context::_Bind(::stdnet::_Hidden::_Socket_id _Id,
                                                   ::stdnet::_Hidden::_Endpoint const& _Endpoint,
                                                   ::std::error_code& _Error)
            -> void
{
    if (::bind(this->_Native_handle(_Id), _Endpoint._Data(), _Endpoint._Size()) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Libevent_context::_Listen(::stdnet::_Hidden::_Socket_id _Id,
                                                     int                           _No,
                                                     ::std::error_code&            _Error)
    -> void
{
    if (::listen(this->_Native_handle(_Id), _No) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Libevent_context::run_one() -> ::std::size_t
{
    // event_base_loop(..., EVLOOP_ONCE) may process multiple events but
    // it doesn't say how many. As a result, the return from run_one() may
    // be incorrect.
    return 0 == event_base_loop(this->_Context.get(), EVLOOP_ONCE)? 1u: 0;
}

inline auto stdnet::_Hidden::_Libevent_context::_Cancel(::stdnet::_Hidden::_Io_base* _Cancel_op,
                                                     ::stdnet::_Hidden::_Io_base* _Op) -> void
{
    if (_Op->_Extra && -1 == event_del(static_cast<::event *>(_Op->_Extra.get())))
    {
        assert("deleting a libevent event failed!" == nullptr);
    }
    _Cancel_op->_Cancel();
    _Op->_Cancel();
}

inline auto stdnet::_Hidden::_Libevent_context::_Accept(::stdnet::_Hidden::_Context_base::_Accept_operation* _Op) -> bool
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_READ, _Libevent_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Libevent_error_category()));
        return false;
    }
    _Op->_Context = this;
    _Op->_Extra = std::unique_ptr<void, auto(*)(void*)->void>(
        _Ev,
        +[](void* _Ev){ ::event_free(static_cast<::event*>(_Ev)); });

    _Op->_Work =
        [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Op)
        {
            auto _Id{_Op->_Id};
            auto& _Completion(*static_cast<_Accept_operation*>(_Op));

            while (true)
            {
                int _Rc = ::accept(_Ctxt._Native_handle(_Id), ::std::get<0>(_Completion)._Data(), &::std::get<1>(_Completion));
                if (0 <= _Rc)
                {
                    ::std::get<2>(_Completion) = _Ctxt._Make_socket(_Rc);
                    _Completion._Complete();
                    return true;
                }
                else
                {
                    switch (errno)
                    {
                    default:
                        _Completion._Error(::std::error_code(errno, ::std::system_category()));
                        return true;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return false;
                    }
                }
            }
        };

    ::event_add(_Ev, nullptr);
    return true;
}
// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Libevent_context::_Connect(::stdnet::_Hidden::_Context_base::_Connect_operation* _Op) -> bool
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    auto const& _Endpoint(::std::get<0>(*_Op));
    if (-1 == ::fcntl(_Handle, F_SETFL, O_NONBLOCK))
    {
        _Op->_Error(::std::error_code(errno, ::std::system_category()));
        return false;
    }
    if (0 == ::connect(_Handle, _Endpoint._Data(), _Endpoint._Size()))
    {
        _Op->_Complete();
        return false;
    }
    switch (errno)
    {
    default:
        _Op->_Error(::std::error_code(errno, ::std::system_category()));
        return false;
    case EINPROGRESS:
    case EINTR:
        break;
    }

    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_READ | EV_WRITE, _Libevent_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Libevent_error_category()));
        return false;
    }
    _Op->_Context = this;
    _Op->_Extra = std::unique_ptr<void, auto(*)(void*)->void>(
        _Ev,
        +[](void* _Ev){ ::event_free(static_cast<::event*>(_Ev)); });

    _Op->_Work =
        [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Op)
        {
            auto _Handle{_Ctxt._Native_handle(_Op->_Id)};
            auto& _Completion(*static_cast<_Connect_operation*>(_Op));

            int _Error{};
            ::socklen_t _Len{sizeof(_Error)};
            if (-1 == ::getsockopt(_Handle, SOL_SOCKET, SO_ERROR, &_Error, &_Len))
            {
                _Op->_Error(::std::error_code(errno, ::std::system_category()));
                return true;
            }
            if (0 == _Error)
            {
                _Op->_Complete();
            }
            else
            {
                _Op->_Error(::std::error_code(_Error, ::std::system_category()));
            }
            return true;
        };

    ::event_add(_Ev, nullptr);
    return true;
} 

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Libevent_context::_Receive(::stdnet::_Hidden::_Context_base::_Receive_operation* _Op) -> bool
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_READ, _Libevent_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Libevent_error_category()));
        return false;
    }
    _Op->_Context = this;
    _Op->_Extra = std::unique_ptr<void, auto(*)(void*)->void>(
        _Ev,
        +[](void* _Ev){ ::event_free(static_cast<::event*>(_Ev)); });

    _Op->_Work = [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Op)
        {
            auto _Id{_Op->_Id};
            auto& _Completion(*static_cast<_Receive_operation*>(_Op));

            while (true)
            {
                int _Rc = ::recvmsg(_Ctxt._Native_handle(_Id),
                                    &::std::get<0>(_Completion),
                                    ::std::get<1>(_Completion));
                if (0 <= _Rc)
                {
                    ::std::get<2>(_Completion) = _Rc;
                    _Completion._Complete();
                    return true;
                }
                else
                {
                    switch (errno)
                    {
                    default:
                        _Completion._Error(::std::error_code(errno, ::std::system_category()));
                        return true;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return false;
                    }
                }
            }
        };

    ::event_add(_Ev, nullptr);
    return true;
}

inline auto stdnet::_Hidden::_Libevent_context::_Send(::stdnet::_Hidden::_Context_base::_Send_operation* _Op) -> bool 
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_WRITE, _Libevent_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Libevent_error_category()));
        return false;
    }
    _Op->_Context = this;
    _Op->_Extra = std::unique_ptr<void, auto(*)(void*)->void>(
        _Ev,
        +[](void* _Ev){ ::event_free(static_cast<::event*>(_Ev)); });

    _Op->_Work = [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Op)
        {
            auto _Id{_Op->_Id};
            auto& _Completion(*static_cast<_Receive_operation*>(_Op));

            while (true)
            {
                int _Rc = ::sendmsg(_Ctxt._Native_handle(_Id),
                                    &::std::get<0>(_Completion),
                                    ::std::get<1>(_Completion));
                if (0 <= _Rc)
                {
                    ::std::get<2>(_Completion) = _Rc;
                    _Completion._Complete();
                    return true;
                }
                else
                {
                    switch (errno)
                    {
                    default:
                        _Completion._Error(::std::error_code(errno, ::std::system_category()));
                        return true;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return false;
                    }
                }
            }
        };

    ::event_add(_Ev, nullptr);
    return true;
}

auto ::stdnet::_Hidden::_Libevent_context::_Resume_after(::stdnet::_Hidden::_Context_base::_Resume_after_operation* _Op) -> bool
{
    ::event* _Ev(evtimer_new(this->_Context.get(), _Libevent_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Libevent_error_category()));
        return false;
    }
    _Op->_Context = this;
    _Op->_Extra = std::unique_ptr<void, auto(*)(void*)->void>(
        _Ev,
        +[](void* _Ev){ ::event_free(static_cast<::event*>(_Ev)); });

    _Op->_Work = [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Op)
        {
            auto& _Completion(*static_cast<_Resume_after_operation*>(_Op));
            _Completion._Complete();
            return true;
        };
    
    constexpr unsigned long long _F(1'000'000);
    ::std::chrono::microseconds _Duration(::std::get<0>(*_Op));
    ::timeval& _Tv(::std::get<1>(*_Op));
    _Tv.tv_sec = _Duration.count() / _F;
    _Tv.tv_usec = 1000 * _Duration.count() % _F;
    ::evtimer_add(_Ev, &_Tv);
    return true;
}

auto ::stdnet::_Hidden::_Libevent_context::_Resume_at(::stdnet::_Hidden::_Context_base::_Resume_at_operation*) -> bool
{
    //-dk:TODO
    return {};
}

// ----------------------------------------------------------------------------

#endif
