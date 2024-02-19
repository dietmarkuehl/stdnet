// stdnet/event_context.hpp                                           -*-C++-*-
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

#ifndef INCLUDED_STDNET_EVENT_CONTEXT
#define INCLUDED_STDNET_EVENT_CONTEXT

#include <stdnet/netfwd.hpp>
#include <stdnet/container.hpp>
#include <stdnet/context_base.hpp>
#include <stdnet/basic_stream_socket.hpp>
#include <memory>
#include <new>
#include <system_error>
#include <cerrno>
#include <cstdlib>
#include <event2/event.h>
#include <event2/util.h>
#include <unistd.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    extern "C"
    {
        auto _Event_callback(int, short, void*) -> void;
    }
    class _Event_error_category_t;
    class _Event_record;
    class _Event_context;

    auto _Event_error_category() -> _Event_error_category_t const&;
}

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Event_error_category_t
    : public ::std::error_category
{
    auto name() const noexcept -> char const* override { return "libevent-category"; }
    auto message(int _E) const -> ::std::string override { return evutil_socket_error_to_string(_E); }
};

inline auto stdnet::_Hidden::_Event_error_category() -> stdnet::_Hidden::_Event_error_category_t const&
{
    static stdnet::_Hidden::_Event_error_category_t _Rc{};
    return _Rc;
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Event_callback(int, short, void* _Arg) -> void
{
    auto _Op(static_cast<::stdnet::_Hidden::_Io_base*>(_Arg));
    _Op->_Work(*_Op->_Context, _Op);
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Event_record final
{
    _Event_record(::stdnet::_Stdnet_native_handle_type _H): _Handle(_H) {}
    ::stdnet::_Stdnet_native_handle_type                   _Handle;
    bool                                                   _Blocking{true};
};

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Event_context
    : public ::stdnet::_Hidden::_Context_base
{
private:
    ::stdnet::_Hidden::_Container<::stdnet::_Hidden::_Event_record> _D_sockets;
    ::std::unique_ptr<::event_base, auto(*)(event_base*)->void>    _Context;

    auto _Make_socket(int) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void override;
    auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type override;
    auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void override;
    auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const&, ::std::error_code&) -> void override;
    auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void override;

    auto run_one() -> ::std::size_t override;

    auto _Cancel(::stdnet::_Hidden::_Io_base*) -> void override;
    auto _Accept(_Accept_operation*) -> bool override;
    auto _Receive(_Receive_operation*) -> bool override;
    auto _Send(_Send_operation*) -> bool override;

public:
    _Event_context();
    _Event_context(::event_base*);
};

// ----------------------------------------------------------------------------

inline stdnet::_Hidden::_Event_context::_Event_context()
    : _Context(::event_base_new(), +[](::event_base* _C){ ::event_base_free(_C); })
{
}

inline stdnet::_Hidden::_Event_context::_Event_context(::event_base* _C)
    : _Context(_C, +[](::event_base*){})
{
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Event_context::_Make_socket(int _Fd) -> ::stdnet::_Hidden::_Socket_id
{
    return this->_D_sockets._Insert(_Fd);
}

inline auto stdnet::_Hidden::_Event_context::_Make_socket(int _D, int _T, int _P, ::std::error_code& _Error)
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

inline auto stdnet::_Hidden::_Event_context::_Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void
{
    _Stdnet_native_handle_type _Handle(this->_D_sockets[_Id]._Handle);
    this->_D_sockets._Erase(_Id);
    if (::close(_Handle) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Event_context::_Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type
{
    return this->_D_sockets[_Id]._Handle;
}

inline auto stdnet::_Hidden::_Event_context::_Set_option(::stdnet::_Hidden::_Socket_id _Id,
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

inline auto stdnet::_Hidden::_Event_context::_Bind(::stdnet::_Hidden::_Socket_id _Id,
                                                   ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const& _Endpoint,
                                                   ::std::error_code& _Error)
            -> void
{
    if (::bind(this->_Native_handle(_Id), _Endpoint._Data(), _Endpoint._Size()) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Event_context::_Listen(::stdnet::_Hidden::_Socket_id _Id,
                                                     int                           _No,
                                                     ::std::error_code&            _Error)
    -> void
{
    if (::listen(this->_Native_handle(_Id), _No) < 0)
    {
        _Error = ::std::error_code(errno, ::std::system_category());
    }
}

inline auto stdnet::_Hidden::_Event_context::run_one() -> ::std::size_t
{
    // event_base_loop(..., EVLOOP_ONCE) may process multiple events but
    // it doesn't say how many. As a result, the return from run_one() may
    // be incorrect.
    return 0 == event_base_loop(this->_Context.get(), EVLOOP_ONCE)? 1u: 0;
}

inline auto stdnet::_Hidden::_Event_context::_Cancel(::stdnet::_Hidden::_Io_base*) -> void
{
    //-dk:TODO
}

inline auto stdnet::_Hidden::_Event_context::_Accept(_Accept_operation* _Op) -> bool
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_READ, _Event_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Event_error_category()));
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
                    ::std::get<2>(_Completion) = ::stdnet::basic_stream_socket<::stdnet::ip::tcp>(&_Ctxt, _Ctxt._Make_socket(_Rc));
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

inline auto stdnet::_Hidden::_Event_context::_Receive(_Receive_operation* _Op) -> bool
{
    auto _Handle(this->_Native_handle(_Op->_Id));
    ::event* _Ev(::event_new(this->_Context.get(), _Handle, EV_READ, _Event_callback, _Op));
    if (_Ev == nullptr)
    {
        _Op->_Error(::std::error_code(evutil_socket_geterror(_Handle), stdnet::_Hidden::_Event_error_category()));
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

inline auto stdnet::_Hidden::_Event_context::_Send(_Send_operation*) -> bool 
{
    return {};
}

// ----------------------------------------------------------------------------

#endif
