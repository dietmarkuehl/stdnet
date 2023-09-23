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
#include <stdexec/functional.hpp>
#include <system_error>
#include <type_traits>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>

// ----------------------------------------------------------------------------

namespace stdnet
{
    enum class socket_errc
    {
        already_open = 1,
        not_found
    };

    auto socket_category() noexcept -> ::std::error_category const&;

    namespace _Stdnet_hidden
    {
        struct async_accept_t
        {
            template <typename _Receiver>
            struct _State_base
            {
                _Receiver _D_receiver;
                template <typename _RT>
                _State_base(_RT&& _R)
                    : _D_receiver(::std::forward<_RT>(_R))
                {
                }
                virtual auto _Start() -> void = 0;
            };
            template <typename _Receiver>
            struct _Upstream_receiver
            {
                using is_receiver = void;
                _State_base<_Receiver>* _D_state;
                friend auto tag_invoke(::stdexec::set_value_t, _Upstream_receiver&& _Self) noexcept -> void
                {
                    _Self._D_state->_Start();
                }
                template <typename _Error>
                friend auto tag_invoke(::stdexec::set_error_t, _Upstream_receiver&& _Self, _Error&& _E) -> void
                {
                    ::stdexec::set_error(::std::move(_Self._D_state->_D_receiver), ::std::forward<_Error>(_E));
                }
                friend auto tag_invoke(::stdexec::set_stopped_t, _Upstream_receiver&& _Self) -> void
                {
                    ::stdexec::set_stopped(::std::move(_Self._D_state->_D_receiver));
                }
                friend auto tag_invoke(::stdexec::get_env_t, _Upstream_receiver const& _Self) noexcept
                {
                    return ::stdexec::get_env(_Self._D_state->_D_receiver);
                }
            };
            template <typename _Protocol, typename _Receiver, typename _Upstream>
            struct _State
                : ::stdnet::_Hidden::_Io_operation<::stdnet::ip::tcp::endpoint>
                , _State_base<_Receiver>
            {
                using _Upstream_state_t = decltype(::stdexec::connect(::std::declval<_Upstream>(), ::std::declval<_Upstream_receiver<_Receiver>>()));

                ::stdnet::basic_socket_acceptor<_Protocol>& _D_acceptor;
                _Upstream_state_t                           _D_state;

                template <typename _RT, typename _Up_sender>
                _State(::stdnet::basic_socket_acceptor<_Protocol>& _Acceptor,
                       _RT&& _R, _Up_sender&& _Up)
                    : _State_base<_Receiver>(::std::forward<_RT>(_R))
                    , _D_acceptor(_Acceptor)
                    , _D_state(::stdexec::connect(::std::forward<_Up_sender>(_Up), _Upstream_receiver<_Receiver>(this)))
                {
                }
                friend auto tag_invoke(::stdexec::start_t, _State& _Self) noexcept -> void
                {
                    std::cout << "accept::start\n";
                    ::stdexec::start(_Self._D_state);
                }
                auto _Start() -> void override final
                {
                    std::cout << "actual accept::start\n";
                    this->_D_acceptor.get_scheduler()._Accept(this->_D_acceptor._Id(), this);
                }
                auto _Complete(void*) -> void override final
                {
                    std::cout << "accept::complete\n";
                }
            };
            template <typename _Protocol, ::stdexec::sender _Upstream>
            struct _Sender
            {
                using is_sender = void;
                using completion_signatures = ::stdexec::completion_signatures<
                    ::stdexec::set_value_t(::stdnet::basic_stream_socket<_Protocol>),
                    ::stdexec::set_error_t(::std::error_code), //-dk:TODO merge with _Upstream errors
                    ::stdexec::set_stopped_t()
                    >;
                ::stdnet::basic_socket_acceptor<_Protocol>& _D_acceptor;
                _Upstream                                   _D_upstream;
                template <typename _Receiver>
                friend auto tag_invoke(::stdexec::connect_t, _Sender const& _Self, _Receiver&& _R)
                {
                    return _State<_Protocol, ::std::remove_cvref_t<_Receiver>, _Upstream>(
                        _Self._D_acceptor,
                        ::std::forward<_Receiver>(_R),
                        ::std::move(_Self._D_upstream)
                        );
                }
            };

            template <typename _Protocol>
            friend auto tag_invoke(async_accept_t, ::stdnet::basic_socket_acceptor<_Protocol>& _Acceptor)
            {
                return _Sender<_Protocol, decltype(::stdexec::just())>{_Acceptor, ::stdexec::just()};
            }
            template <::stdexec::sender _Upstream, typename _Protocol>
            friend auto tag_invoke(async_accept_t, _Upstream&& _U, ::stdnet::basic_socket_acceptor<_Protocol>& _Acceptor)
            {
                return _Sender<_Protocol, ::std::remove_cvref_t<_Upstream>>{_Acceptor, ::std::forward<_Upstream>(_U)};
            }

            template <typename _Acceptor_t>
                requires ::stdexec::tag_invocable<async_accept_t, _Acceptor_t&>
            auto operator()(_Acceptor_t& _Acceptor) const
            {
                return tag_invoke(*this, _Acceptor);
            }
            template <::stdexec::sender _Upstream, typename _Acceptor_t>
                requires ::stdexec::tag_invocable<async_accept_t, _Upstream, _Acceptor_t&>
            auto operator()(_Upstream&& _U, _Acceptor_t& _Acceptor) const
            {
                return tag_invoke(*this, ::std::forward<_Upstream>(_U), _Acceptor);
            }
        };
    }
    using _Stdnet_hidden::async_accept_t;
    inline constexpr async_accept_t async_accept{};
}

auto ::stdnet::socket_category() noexcept -> ::std::error_category const&
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
        ::std::error_code _Error{};
        this->close(_Error);
    }
    basic_socket_acceptor& operator=(basic_socket_acceptor const&) = delete;
    basic_socket_acceptor& operator=(basic_socket_acceptor&&);
    template<typename _OtherProtocol>
    basic_socket_acceptor& operator=(::stdnet::basic_socket_acceptor<_OtherProtocol>&&);

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
