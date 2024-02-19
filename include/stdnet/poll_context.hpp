// stdnet/poll_context.hpp                                            -*-C++-*-
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

#ifndef INCLUDED_STDNET_POLL_CONTEXT
#define INCLUDED_STDNET_POLL_CONTEXT

#include <stdnet/netfwd.hpp>
#include <stdnet/container.hpp>
#include <stdnet/context_base.hpp>
#include <stdnet/basic_stream_socket.hpp>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    struct _Poll_record;
    struct _Poll_context;
}

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Poll_record final
{
    _Poll_record(::stdnet::_Stdnet_native_handle_type _H): _Handle(_H) {}
    ::stdnet::_Stdnet_native_handle_type                   _Handle;
    bool                                                   _Blocking{true};
};

// ----------------------------------------------------------------------------

struct stdnet::_Hidden::_Poll_context final
    : ::stdnet::_Hidden::_Context_base
{
    ::stdnet::_Hidden::_Container<::stdnet::_Hidden::_Poll_record> _D_sockets;
    ::std::vector<::pollfd>     _D_poll;
    ::std::vector<::stdnet::_Hidden::_Io_base*> _D_outstanding;

    auto _Make_socket(int _Fd) -> ::stdnet::_Hidden::_Socket_id override final
    {
        return this->_D_sockets._Insert(_Fd);
    }
    auto _Make_socket(int _D, int _T, int _P, ::std::error_code& _Error)
        -> ::stdnet::_Hidden::_Socket_id override final
    {
        int _Fd(::socket(_D, _T, _P));
        if (_Fd < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
            return ::stdnet::_Hidden::_Socket_id::_Invalid;
        }
        return this->_Make_socket(_Fd);
    }
    auto _Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void override final
    {
        _Stdnet_native_handle_type _Handle(this->_D_sockets[_Id]._Handle);
        this->_D_sockets._Erase(_Id);
        if (::close(_Handle) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto _Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type override final
    {
        return this->_D_sockets[_Id]._Handle;
    }
    auto _Set_option(::stdnet::_Hidden::_Socket_id _Id,
                     int _Level,
                     int _Name,
                     void const* _Data,
                     ::socklen_t _Size,
                     ::std::error_code& _Error) -> void override final
    {
        if (::setsockopt(this->_Native_handle(_Id), _Level, _Name, _Data, _Size) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto _Bind(::stdnet::_Hidden::_Socket_id _Id,
               ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const& _Endpoint,
               ::std::error_code& _Error) -> void override final
    {
        if (::bind(this->_Native_handle(_Id), _Endpoint._Data(), _Endpoint._Size()) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto _Listen(::stdnet::_Hidden::_Socket_id _Id, int _No, ::std::error_code& _Error) -> void override final
    {
        if (::listen(this->_Native_handle(_Id), _No) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto run_one() -> ::std::size_t override final
    {
        if (this->_D_poll.empty())
        {
            return ::std::size_t{};
        }
        while (true)
        {
            int _Rc(::poll(this->_D_poll.data(), this->_D_poll.size(), -1));
            if (_Rc < 0)
            {
                switch (errno)
                {
                default:
                    return ::std::size_t();
                case EINTR:
                case EAGAIN:
                    break;
                }
            }
            else
            {
                for (::std::size_t _I(this->_D_poll.size()); 0 < _I--; )
                {
                    if (this->_D_poll[_I].revents & (this->_D_poll[_I].events | POLLERR))
                    {
                        ::stdnet::_Hidden::_Io_base* _Completion = this->_D_outstanding[_I];
                        auto _Id{_Completion->_Id};
                        if (_I + 1u != this->_D_poll.size())
                        {
                            this->_D_poll[_I] = this->_D_poll.back();
                            this->_D_outstanding[_I] = this->_D_outstanding.back();
                        }
                        this->_D_poll.pop_back();
                        this->_D_outstanding.pop_back();
                        _Completion->_Work(*this, _Completion);
                        return ::std::size_t(1);
                    }
                }
            }
        }
        return ::std::size_t{};
    }
    auto _Wakeup() -> void
    {
        //-dk:TODO wake-up polling thread
    }

    auto _Add_Outstanding(::stdnet::_Hidden::_Io_base* _Completion) -> bool
    {
        auto _Id{_Completion->_Id};
        if (this->_D_sockets[_Id]._Blocking || !_Completion->_Work(*this, _Completion))
        {
            this->_D_poll.emplace_back(this->_Native_handle(_Id), _Completion->_Event, short());
            this->_D_outstanding.emplace_back(_Completion);
            this->_Wakeup();
            return false;
        }
        return true;
    }

    auto _Cancel(::stdnet::_Hidden::_Io_base*) -> void override final
    {
        //-dk:TODO
    }
    auto _Accept(_Accept_operation* _Completion)
        -> bool override final
    {
        auto _Id(_Completion->_Id);
        _Completion->_Work =
            [](::stdnet::_Hidden::_Context_base& _Ctxt, ::stdnet::_Hidden::_Io_base* _Comp)
            {
                auto _Id{_Comp->_Id};
                auto& _Completion(*static_cast<_Accept_operation*>(_Comp));

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
        return this->_Add_Outstanding(_Completion);
    }
    auto _Receive(_Receive_operation*) -> bool override { return {}; /*-dk:TODO*/ }
    auto _Send(_Send_operation*) -> bool override { return {}; /*-dk:TODO*/ }
};

// ----------------------------------------------------------------------------

#endif
