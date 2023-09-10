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
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>

// ----------------------------------------------------------------------------

namespace stdnet
{
    using _Stdnet_native_handle_type = int;

    enum class socket_errc
    {
        already_open = 1,
        not_found
    };

    auto socket_category() noexcept -> ::std::error_category const&
    {
        struct _Category
            : ::std::error_category
        {
            auto name() const noexcept -> char const* override { return "socket"; }
            auto message(int _Error) const -> ::std::string override
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
}

// ----------------------------------------------------------------------------

class stdnet::socket_base
{
public:
    template <typename _Value_t, int _Level, int _Name>
    class _Socket_option
    {
    private:
        _Value_t _D_value;
    
    public:
        explicit _Socket_option(_Value_t _V): _D_value(_V) {}
        _Value_t _Value() const { return this->_D_value; }
        template <typename _Protocol> auto data(_Protocol&&) const -> _Value_t const* { return &this->_D_value; }
        template <typename _Protocol> auto data(_Protocol&&)       -> _Value_t const* { return &this->_D_value; }
        template <typename _Protocol> constexpr auto level(_Protocol&&) const -> int { return _Level; }
        template <typename _Protocol> constexpr auto name(_Protocol&&) const -> int { return _Name; }
        template <typename _Protocol> constexpr auto size(_Protocol&&) const -> ::socklen_t { return sizeof(_Value_t); }
    };
    class broadcast;
    class debug;
    class do_not_route;
    class keep_alive;
    class linger;
    class out_of_band_inline;
    class receive_buffer_size;
    class receive_low_watermark;
    class reuse_address
        : public _Socket_option<int, SOL_SOCKET, SO_REUSEADDR>
    {
    public:
        explicit reuse_address(bool _Value): _Socket_option(_Value) {}
        explicit operator bool() const { return this->_Value(); }
    };
    class send_buffer_size;
    class send_low_watermark;

    using shutdown_type = int; //-dk:TODO
    static constexpr shutdown_type shutdown_receive{1};
    static constexpr shutdown_type shutdown_send{2};
    static constexpr shutdown_type shutdown_both{3};

    using wait_type = int; //-dk:TODO
    static constexpr wait_type wait_read{1};
    static constexpr wait_type wait_write{2};
    static constexpr wait_type wait_error{3};

    using message_flags = int; //-dk:TODO
    static constexpr message_flags message_peek{1};
    static constexpr message_flags message_out_of_band{2};
    static constexpr message_flags message_do_not_route{3};

    static constexpr int max_listen_connections{SOMAXCONN};

protected:
    socket_base() = default;
    ~socket_base() = default;
};

// ----------------------------------------------------------------------------

template <typename Protocol>
class stdnet::basic_socket
{
};

// ----------------------------------------------------------------------------

template <typename _Protocol>
class stdnet::basic_stream_socket
    : public basic_socket<_Protocol>
{
public:
    using native_handle_type = _Stdnet_native_handle_type;
    using protocol_type = _Protocol;
    using endpoint_type = typename protocol_type::endpoint;

    basic_stream_socket(io_context&, endpoint_type const&)
        : stdnet::basic_socket<_Protocol>()
    {
    }
};

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
    protocol_type      _D_protocol; 
    native_handle_type _D_handle{};

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
    basic_socket_acceptor(::stdnet::io_context&, endpoint_type const& _Endpoint, bool _Reuse = true)
        : ::stdnet::socket_base()
        , _D_protocol(_Endpoint.protocol())
        , _D_handle(-1)
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
    basic_socket_acceptor(basic_socket_acceptor&&);
    template<typename _OtherProtocol>
    basic_socket_acceptor(::stdnet::basic_socket_acceptor<_OtherProtocol>&&);
    ~basic_socket_acceptor()
    {
        ::std::error_code _Error{};
        this->close(_Error);
    }
    basic_socket_acceptor& operator=(basic_socket_acceptor const&) = delete;
    basic_socket_acceptor& operator=(basic_socket_acceptor&&);
    template<typename _OtherProtocol>
    basic_socket_acceptor& operator=(::stdnet::basic_socket_acceptor<_OtherProtocol>&&);

    scheduler_type     get_scheduler() noexcept;
    executor_type      get_executor() noexcept;
    native_handle_type native_handle() { return this->_D_handle; }
    native_handle_type _Native_handle() const { return this->_D_handle; }
    void open(protocol_type const& _P = protocol_type())
    {
        _Dispatch([this, &_P](::std::error_code& _Error){ this->open(_P, _Error); });
    }
    void open(protocol_type const& _P, ::std::error_code& _Error)
    {
        if (this->is_open())
        {
            _Error = ::std::error_code(int(socket_errc::already_open), ::stdnet::socket_category());
        }
        this->_D_handle = ::socket(_P.family(), _P.type(), _P.protocol());
        if (this->_D_handle < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    void assign(protocol_type const&, native_handle_type const&);
    void assign(protocol_type const&, native_handle_type const&, ::std::error_code&);
    native_handle_type release();
    native_handle_type release(::std::error_code&);
    bool is_open() const noexcept { return this->_Native_handle() != -1; }
    void close()
    {
        _Dispatch([this](auto& _Error){ return this->close(_Error); });
    }
    void close(::std::error_code& _Error)
    {
        //-dk:TODO cancel outstanding work
        if (this->is_open() && ::close(this->native_handle()) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    void cancel();
    void cancel(::std::error_code&);
    template<typename _SettableSocketOption>
    void set_option(_SettableSocketOption const& _Option)
    {
        _Dispatch([this, _Option](::std::error_code& _Error){ this->set_option(_Option, _Error); });
    }
    template<typename _SettableSocketOption>
    void set_option(_SettableSocketOption const& _Option, ::std::error_code& _Error)
    {
        if (::setsockopt(this->native_handle(),
                     _Option.level(this->_D_protocol),
                     _Option.name(this->_D_protocol),
                     _Option.data(this->_D_protocol),
                     _Option.size(this->_D_protocol)
                     ) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
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
        if (::bind(this->native_handle(), _Endpoint._Data(), _Endpoint._Size()) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto listen(int _No = ::stdnet::socket_base::max_listen_connections) -> void
    {
        _Dispatch([this, _No](auto& _Error){ this->listen(_No, _Error); });
    }
    auto listen(int _No, ::std::error_code& _Error) -> void
    {
        if (::listen(this->native_handle(), _No) < 0)
        {
            _Error = ::std::error_code(errno, ::std::system_category());
        }
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
