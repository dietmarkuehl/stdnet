// stdnet/io_context.hpp                                              -*-C++-*-
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

#ifndef INCLUDED_STDNET_IO_CONTEXT
#define INCLUDED_STDNET_IO_CONTEXT
#pragma once

#include <stdnet/netfwd.hpp>
#include <stdnet/basic_stream_socket.hpp>
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <limits>
#include <cerrno>

// ----------------------------------------------------------------------------

namespace stdnet
{
    namespace _Hidden_abstract
    {
        struct _Context;
    }
    namespace _Hidden
    {

        struct _Io_operation
        {
            _Io_operation*                     _Next{nullptr};
            ::stdnet::_Hidden::_Socket_id _Id;
            int                           _Event;
            bool                        (*_Work)(::stdnet::_Hidden_abstract::_Context&, _Io_operation*);

            _Io_operation(::stdnet::_Hidden::_Socket_id _Id, int _Event): _Id(_Id), _Event(_Event) {}

            virtual void _Complete() = 0;
            virtual void _Error(::std::error_code) = 0;
            virtual void _Cancel() = 0;
        };
        template <typename _Data>
        struct _Io_operation
            : _Io_operation
            , _Data
        {
            template <typename _D = _Data>
            _Io_operation(::stdnet::_Hidden::_Socket_id _Id, int _Event, _D&& _A = _Data())
                : _Io_operation(_Id, _Event)
                , _Data(::std::forward<_D>(_A))
            {
            }
        };

        template <typename _Record>
        struct _Container
        {
            ::std::vector<::std::variant<::std::size_t, _Record>> _Records;
            ::std::size_t                                         _Free{};

            ::stdnet::_Hidden::_Socket_id _Insert(_Record _R)
            {
                if (this->_Free == this->_Records.size())
                {
                    this->_Records.emplace_back(::std::move(_R));
                    return ::stdnet::_Hidden::_Socket_id(this->_Free++);
                }
                else
                {
                    ::std::size_t _Rc(std::exchange(this->_Free, ::std::get<0>(this->_Records[this->_Free])));
                    this->_Records[_Rc] = ::std::move(_R);
                    return ::stdnet::_Hidden::_Socket_id(_Rc);
                }
            }
            void _Erase(::stdnet::_Hidden::_Socket_id _Id)
            {
                this->_Records[::std::size_t(_Id)] = std::exchange(this->_Free, ::std::size_t(_Id));
            }
            auto operator[](::stdnet::_Hidden::_Socket_id _Id) -> _Record&
            {
                return ::std::get<1>(this->_Records[::std::size_t(_Id)]);
            }
        };
    }
    namespace _Hidden_abstract
    {
        struct _Context
        {
            using _Accept_operation = ::stdnet::_Hidden::_Io_operation<
                ::std::tuple<::stdnet::ip::tcp::endpoint,
                             ::socklen_t,
                             ::std::optional<::stdnet::basic_stream_socket<::stdnet::ip::tcp>>
                             >
                            >;
            virtual ~_Context() = default;
            virtual auto _Make_socket(int) -> ::stdnet::_Hidden::_Socket_id = 0;
            virtual auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id = 0;
            virtual auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void = 0;
            virtual auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type = 0;
            virtual auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void = 0;
            virtual auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const&, ::std::error_code&) -> void = 0;
            virtual auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void = 0;

            virtual auto run_one() -> ::std::size_t = 0;

            virtual auto _Cancel(::stdnet::_Hidden::_Io_operation*) -> void = 0;
            virtual auto _Accept(::stdnet::_Hidden::_Socket_id, _Accept_operation*) -> bool = 0;
        };
    }
    namespace _Hidden_poll
    {
        struct _Context;
        using _Operation = auto (*)(_Context*, ::stdnet::_Hidden::_Io_operation*) -> bool;
        struct _Record
        {
            _Record(::stdnet::_Stdnet_native_handle_type _H): _Handle(_H) {}
            ::stdnet::_Stdnet_native_handle_type _Handle;
            bool                                 _Blocking{true};
        };
        struct _Context final
            : ::stdnet::_Hidden_abstract::_Context
        {
            ::stdnet::_Hidden::_Container<::stdnet::_Hidden_poll::_Record> _D_sockets;
            ::std::vector<::pollfd>     _D_poll;
            ::std::vector<::stdnet::_Hidden::_Io_operation*> _D_outstanding;

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
                                ::stdnet::_Hidden::_Io_operation* _Completion = this->_D_outstanding[_I];
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

            auto _Add_Outstanding(::stdnet::_Hidden::_Io_operation* _Completion) -> bool
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

            auto _Cancel(::stdnet::_Hidden::_Io_operation*) -> void override final
            {
                //-dk:TODO
            }
            auto _Accept(::stdnet::_Hidden::_Socket_id _Id, _Accept_operation* _Completion)
                -> bool override final
            {
                _Completion->_Work =
                    [](::stdnet::_Hidden_abstract::_Context& _Ctxt, ::stdnet::_Hidden::_Io_operation* _Comp)
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
        };
    }
    namespace _Hidden_kqueue
    {

    }
    
    class io_context;
}

// ----------------------------------------------------------------------------

class stdnet::io_context
{
private:
    ::std::unique_ptr<::stdnet::_Hidden_abstract::_Context> _D_owned{new ::stdnet::_Hidden_poll::_Context()};
    ::stdnet::_Hidden_abstract::_Context&                   _D_context{*this->_D_owned};

public:
    class scheduler_type
    {
        friend class io_context;
    private:
        ::stdnet::_Hidden_abstract::_Context* _D_context;
        scheduler_type(::stdnet::_Hidden_abstract::_Context* _Context)
            : _D_context(_Context)
        {
        }

    public:
        auto _Cancel(_Hidden::_Io_operation* _Op) -> void
        {
            this->_D_context->_Cancel(_Op);
        }
        auto _Accept(::stdnet::_Hidden::_Socket_id _Id, _Hidden_abstract::_Context::_Accept_operation* _Op) -> bool
        {
            return this->_D_context->_Accept(_Id, _Op);
        }
       
    };
    class executor_type {};

    io_context() = default;
    io_context(::stdnet::_Hidden_abstract::_Context& _Context): _D_owned(), _D_context(_Context) {}
    io_context(io_context&&) = delete;

    auto _Make_socket(int _D, int _T, int _P, ::std::error_code& _Error) -> ::stdnet::_Hidden::_Socket_id
    {
        return this->_D_context._Make_socket(_D, _T, _P, _Error);
    }
    auto _Release(::stdnet::_Hidden::_Socket_id _Id, ::std::error_code& _Error) -> void
    {
        return this->_D_context._Release(_Id, _Error);
    }
    auto _Native_handle(::stdnet::_Hidden::_Socket_id _Id) -> _Stdnet_native_handle_type
    {
        return this->_D_context._Native_handle(_Id);
    }
    auto _Set_option(::stdnet::_Hidden::_Socket_id _Id,
                     int _Level,
                     int _Name,
                     void const* _Data,
                     ::socklen_t _Size,
                     ::std::error_code& _Error) -> void
    {
        this->_D_context._Set_option(_Id, _Level, _Name, _Data, _Size, _Error);
    }
    auto _Bind(::stdnet::_Hidden::_Socket_id _Id, ::stdnet::ip::basic_endpoint<::stdnet::ip::tcp> const& _Endpoint, ::std::error_code& _Error)
    {
        this->_D_context._Bind(_Id, _Endpoint, _Error);
    }
    auto _Listen(::stdnet::_Hidden::_Socket_id _Id, int _No, ::std::error_code& _Error)
    {
        this->_D_context._Listen(_Id, _No, _Error);
    }
    auto get_scheduler() -> scheduler_type { return scheduler_type(&this->_D_context); }

    ::std::size_t run_one() { return this->_D_context.run_one(); }
    ::std::size_t run()
    {
        ::std::size_t _Count{};
        while (::std::size_t _C = this->run_one())
        {
            _Count += _C;
        }
        return _Count;
    }
};

// ----------------------------------------------------------------------------

#endif
