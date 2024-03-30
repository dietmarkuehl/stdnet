// include/stdnet/socket.hpp                                          -*-C++-*-
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

#ifndef INCLUDED_STDNET_SOCKET
#define INCLUDED_STDNET_SOCKET
#pragma once

#include <stdnet/netfwd.hpp>
#include <stdnet/cpo.hpp>
#include <stdnet/socket_base.hpp>
#include <stdnet/basic_socket.hpp>
#include <stdnet/basic_stream_socket.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/internet.hpp>

#include <stdexec/functional.hpp>
#include <system_error>
#include <type_traits>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <atomic>
#include <string>

// ----------------------------------------------------------------------------

namespace stdnet
{
    enum class socket_errc: int;
    auto socket_category() noexcept -> ::std::error_category const&;

    namespace _Hidden
    {
        struct _Accept_desc;
        struct _Connect_desc;
        struct _Send_desc;
        struct _Send_to_desc;
        struct _Receive_desc;
        struct _Receive_from_desc;
    }

    using async_accept_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Accept_desc>;
    inline constexpr async_accept_t async_accept{};
    using async_connect_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Connect_desc>;
    inline constexpr async_connect_t async_connect{};
    using async_send_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Send_desc>;
    inline constexpr async_send_t async_send{};
    using async_send_to_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Send_to_desc>;
    inline constexpr async_send_to_t async_send_to{};
    using async_receive_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Receive_desc>;
    inline constexpr async_receive_t async_receive{};
    using async_receive_from_t = ::stdnet::_Hidden::_Cpo<::stdnet::_Hidden::_Receive_from_desc>;
    inline constexpr async_receive_from_t async_receive_from{};
}

struct stdnet::_Hidden::_Accept_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Accept_operation;
    template <typename _Acceptor>
    struct _Data
    {
        using _Acceptor_t = ::std::remove_cvref_t<_Acceptor>;
        using _Socket_t = _Acceptor_t::socket_type;
        using _Completion_signature
            = ::stdexec::set_value_t(_Socket_t, typename _Socket_t::endpoint_type);

        _Acceptor_t& _D_acceptor;
        _Data(_Acceptor_t& _A): _D_acceptor(_A) {}

        auto _Id() const { return this->_D_acceptor._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_acceptor.get_scheduler(); }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver),
                                 _Socket_t(this->_D_acceptor.get_scheduler()._Get_context(),
                                           ::std::move(*::std::get<2>(_O))),
                                 typename _Socket_t::endpoint_type(::std::get<0>(_O)));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<1>(*_Base) = sizeof(::std::get<0>(*_Base));
            return this->_D_acceptor.get_scheduler()._Accept(_Base);
        }
    };
};

struct stdnet::_Hidden::_Connect_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Connect_operation;
    template <typename _Socket>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t();

        _Socket& _D_socket;

        auto _Id() const { return this->_D_socket._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_socket.get_scheduler(); }
        auto _Set_value(_Operation&, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base) = this->_D_socket.get_endpoint();
            return this->_D_socket.get_scheduler()._Connect(_Base);
        }
    };
};

struct stdnet::_Hidden::_Send_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Send_operation;
    template <typename _Stream_t, typename _Buffers>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t(::std::size_t);

        _Stream_t& _D_stream;
        _Buffers   _D_buffers;

        auto _Id() const { return this->_D_stream._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_stream.get_scheduler(); }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver), ::std::get<2>(_O));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base).msg_iov    = this->_D_buffers.data();
            ::std::get<0>(*_Base).msg_iovlen = this->_D_buffers.size();
            return this->_D_stream.get_scheduler()._Send(_Base);
        }
    };
};

struct stdnet::_Hidden::_Send_to_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Send_operation;
    template <typename _Stream_t, typename _Buffers, typename _Endpoint>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t(::std::size_t);

        _Stream_t& _D_stream;
        _Buffers   _D_buffers;
        _Endpoint  _D_endpoint;

        auto _Id() const { return this->_D_stream._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_stream.get_scheduler(); }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver), ::std::get<2>(_O));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base).msg_iov     = this->_D_buffers.data();
            ::std::get<0>(*_Base).msg_iovlen  = this->_D_buffers.size();
            ::std::get<0>(*_Base).msg_name    = this->_D_endpoint._Data();
            ::std::get<0>(*_Base).msg_namelen = this->_D_endpoint._Size();
            return this->_D_stream.get_scheduler()._Send(_Base);
        }
    };
};

struct stdnet::_Hidden::_Receive_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Receive_operation;
    template <typename _Stream_t, typename _Buffers>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t(::std::size_t);

        _Stream_t& _D_stream;
        _Buffers   _D_buffers;

        auto _Id() const { return this->_D_stream._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_stream.get_scheduler(); }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver), ::std::get<2>(_O));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base).msg_iov    = this->_D_buffers.data();
            ::std::get<0>(*_Base).msg_iovlen = this->_D_buffers.size();
            return this->_D_stream.get_scheduler()._Receive(_Base);
        }
    };
};

struct stdnet::_Hidden::_Receive_from_desc
{
    using _Operation = ::stdnet::_Hidden::_Context_base::_Receive_operation;
    template <typename _Stream_t, typename _Buffers, typename _Endpoint>
    struct _Data
    {
        using _Completion_signature = ::stdexec::set_value_t(::std::size_t);

        _Stream_t& _D_stream;
        _Buffers   _D_buffers;
        _Endpoint  _D_endpoint;

        auto _Id() const { return this->_D_stream._Id(); }
        auto _Events() const { return POLLIN; }
        auto _Get_scheduler() { return this->_D_stream.get_scheduler(); }
        auto _Set_value(_Operation& _O, auto&& _Receiver)
        {
            ::stdexec::set_value(::std::move(_Receiver), ::std::get<2>(_O));
        }
        auto _Submit(auto* _Base) -> bool
        {
            ::std::get<0>(*_Base).msg_iov     = this->_D_buffers.data();
            ::std::get<0>(*_Base).msg_iovlen  = this->_D_buffers.size();
            ::std::get<0>(*_Base).msg_name    = this->_D_buffers._Data();
            ::std::get<0>(*_Base).msg_namelen = this->_D_buffers._Size();
            return this->_D_stream.get_scheduler()._Receive(_Base);
        }
    };
};

enum class stdnet::socket_errc: int
{
    already_open = 1,
    not_found
};

auto stdnet::socket_category() noexcept -> ::std::error_category const&
{
    struct _Category
        : ::std::error_category
    {
        auto name() const noexcept -> char const* override final { return "socket"; }
        auto message(int _Error) const -> ::std::string override final
        {
            switch (::stdnet::socket_errc(_Error))
            {
            default: return "none";
            case ::stdnet::socket_errc::already_open: return "already open";
            case ::stdnet::socket_errc::not_found: return "not found";
            }
        }
    };
    static const _Category _Rc{};
    return _Rc;
}

// ----------------------------------------------------------------------------

template <typename _AcceptableProtocol>
class stdnet::basic_socket_acceptor
    : public stdnet::socket_base
{
public:
    using scheduler_type     = ::stdnet::io_context::scheduler_type;
    using executor_type      = ::stdnet::io_context::executor_type;
    using native_handle_type = ::stdnet::_Stdnet_native_handle_type;
    using protocol_type      = _AcceptableProtocol;
    using endpoint_type      = typename protocol_type::endpoint;
    using socket_type        = typename protocol_type::socket;

private:
    ::stdnet::io_context&         _D_context;
    protocol_type                 _D_protocol; 
    ::stdnet::_Hidden::_Socket_id _D_id{};

private:
    template <typename _Fun_t>
    static void _Dispatch(_Fun_t&& _Fun)
    {
        ::std::error_code _error{};
        _Fun(_error);
        if (_error)
        {
            throw ::std::system_error(_error);
        }
    }

public:
    explicit basic_socket_acceptor(::stdnet::io_context&);
    basic_socket_acceptor(::stdnet::io_context&, protocol_type const& protocol);
    basic_socket_acceptor(::stdnet::io_context& _Context, endpoint_type const& _Endpoint, bool _Reuse = true)
        : ::stdnet::socket_base()
        , _D_context(_Context)
        , _D_protocol(_Endpoint.protocol())
        , _D_id(::stdnet::_Hidden::_Socket_id::_Invalid)
    {
        this->open(_Endpoint.protocol());
        if (_Reuse)
        {
            this->set_option(::stdnet::socket_base::reuse_address(true));
        }
        this->bind(_Endpoint);
        this->listen();
    }
    basic_socket_acceptor(::stdnet::io_context&, protocol_type const&, native_handle_type const&);
    basic_socket_acceptor(basic_socket_acceptor const&) = delete;
    basic_socket_acceptor(basic_socket_acceptor&& _Other)
        : ::stdnet::socket_base()
        , _D_protocol(_Other._D_protocol)
        , _D_id(::std::exchange(_Other._D_id, ::stdnet::_Hidden::_Socket_id::_Invalid))
    {
    }
    template<typename _OtherProtocol>
    basic_socket_acceptor(::stdnet::basic_socket_acceptor<_OtherProtocol>&&);
    ~basic_socket_acceptor()
    {
        if (this->_D_id != ::stdnet::_Hidden::_Socket_id::_Invalid)
        {
            ::std::error_code _Error{};
            this->close(_Error);
        }
    }
    basic_socket_acceptor& operator=(basic_socket_acceptor const&) = delete;
    basic_socket_acceptor& operator=(basic_socket_acceptor&&);
    template<typename _OtherProtocol>
    basic_socket_acceptor& operator=(::stdnet::basic_socket_acceptor<_OtherProtocol>&&);

    auto _Get_context() -> ::stdnet::io_context& { return this->_D_context; }
    auto get_scheduler() noexcept -> scheduler_type
    {
        return this->_D_context.get_scheduler();
    }
    executor_type      get_executor() noexcept;
    auto native_handle() -> native_handle_type { return this->_D_context._Native_handle(this->_D_id); }
    auto _Native_handle() const -> native_handle_type { return this->_D_context._Native_handle(this->_D_id); }
    auto _Id() const -> ::stdnet::_Hidden::_Socket_id { return this->_D_id; }
    auto open(protocol_type const& _P = protocol_type()) -> void
    {
        _Dispatch([this, &_P](::std::error_code& _Error){ this->open(_P, _Error); });
    }
    auto open(protocol_type const& _P, ::std::error_code& _Error) -> void
    {
        if (this->is_open())
        {
            _Error = ::std::error_code(int(socket_errc::already_open), ::stdnet::socket_category());
        }
        this->_D_id = this->_D_context._Make_socket(_P.family(), _P.type(), _P.protocol(), _Error);
    }
    void assign(protocol_type const&, native_handle_type const&);
    void assign(protocol_type const&, native_handle_type const&, ::std::error_code&);
    native_handle_type release();
    native_handle_type release(::std::error_code&);
    auto is_open() const noexcept -> bool { return this->_D_id != ::stdnet::_Hidden::_Socket_id::_Invalid; }
    auto close() -> void
    {
        _Dispatch([this](auto& _Error){ return this->close(_Error); });
    }
    auto close(::std::error_code& _Error) -> void
    {
        //-dk:TODO cancel outstanding work
        if (this->is_open())
        {
            this->_D_context._Release(this->_Id(), _Error);
            }
    }
    void cancel();
    void cancel(::std::error_code&);
    template<typename _SettableSocketOption>
    auto set_option(_SettableSocketOption const& _Option) -> void
    {
        _Dispatch([this, _Option](::std::error_code& _Error){ this->set_option(_Option, _Error); });
    }
    template<typename _SettableSocketOption>
    auto set_option(_SettableSocketOption const& _Option, ::std::error_code& _Error) -> void
    {
        this->_D_context._Set_option(
            this->_Id(),
            _Option.level(this->_D_protocol),
            _Option.name(this->_D_protocol),
            _Option.data(this->_D_protocol),
            _Option.size(this->_D_protocol),
            _Error);
    }

    template<typename _GettableSocketOption>
    void get_option(_GettableSocketOption&) const;
    template<typename _GettableSocketOption>
    void get_option(_GettableSocketOption&, ::std::error_code&) const;
    template<typename _IoControlCommand>
    void io_control(_IoControlCommand&);
    template<typename _IoControlCommand>
    void io_control(_IoControlCommand&, ::std::error_code&);
    void non_blocking(bool);
    void non_blocking(bool, ::std::error_code&);
    bool non_blocking() const;
    void native_non_blocking(bool);
    void native_non_blocking(bool, ::std::error_code&);
    bool native_non_blocking() const;
    auto bind(endpoint_type const& _Endpoint) -> void
    {
        _Dispatch([this, _Endpoint](::std::error_code& _Error){ this->bind(_Endpoint, _Error); });
    }
    auto bind(endpoint_type const& _Endpoint, ::std::error_code& _Error) -> void
    {
        this->_D_context._Bind(this->_D_id, _Endpoint, _Error);
    }
    auto listen(int _No = ::stdnet::socket_base::max_listen_connections) -> void
    {
        _Dispatch([this, _No](auto& _Error){ this->listen(_No, _Error); });
    }
    auto listen(int _No, ::std::error_code& _Error) -> void
    {
        this->_D_context._Listen(this->_D_id, _No, _Error);
    }
    endpoint_type local_endpoint() const;
    endpoint_type local_endpoint(::std::error_code&) const;
    void enable_connection_aborted(bool);
    bool enable_connection_aborted() const;
    socket_type accept();
    socket_type accept(::std::error_code&);
    socket_type accept(io_context&);
    socket_type accept(io_context&, ::std::error_code&);
    template<typename _CompletionToken>
    void /*DEDUCED*/ async_accept(_CompletionToken&&);
    template<typename _CompletionToken>
    void /*DEDUCED*/ async_accept(::stdnet::io_context&, _CompletionToken&&);
    socket_type accept(endpoint_type&);
    socket_type accept(endpoint_type&, ::std::error_code&);
    socket_type accept(::stdnet::io_context&, endpoint_type&);
    socket_type accept(::stdnet::io_context&, endpoint_type&, ::std::error_code&);
    template<typename _CompletionToken>
    void /*DEDUCED*/ async_accept(endpoint_type&, _CompletionToken&&);
    template<typename _CompletionToken>
    void /*DEDUCED*/ async_accept(::stdnet::io_context&, endpoint_type&, _CompletionToken&&);
    void wait(::stdnet::socket_base::wait_type);
    void wait(::stdnet::socket_base::wait_type, ::std::error_code& ec);
    template<typename _CompletionToken>
    void /*DEDUCED*/ async_wait(::stdnet::socket_base::wait_type, _CompletionToken&&);
};

// ----------------------------------------------------------------------------

#endif
