// stdnet/openssl_context.hpp                                         -*-C++-*-
// ----------------------------------------------------------------------------
//
//  Copyright (c) 2024 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Licensed under the Apache License Version 2.0 with LLVM Exceptions
//  (the "License"); you may not use this file except in compliance with
//  the License. You may obtain a copy of the License at
//
//    https://llvm.org/LICENSE.txt
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// ----------------------------------------------------------------------------

#ifndef INCLUDED_STDNET_OPENSSL_CONTEXT
#define INCLUDED_STDNET_OPENSSL_CONTEXT

#include <stdnet/netfwd.hpp>
#include <stdnet/container.hpp>
#include <stdnet/context_base.hpp>

#include <iostream> //-dk:TODO remove
#include <functional>
#include <system_error>
#include <string>

#include <openssl/ssl.h>
#include <openssl/err.h>

// ----------------------------------------------------------------------------

namespace stdnet::_Hidden
{
    class _Openssl_context;

    class _Openssl_error_category_t;
    auto _Openssl_error_category() -> _Openssl_error_category_t const&;
}

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Openssl_error_category_t
    : public ::std::error_category
{
    auto name() const noexcept -> char const* override;
    auto message(int _E) const -> ::std::string override;
};

inline auto stdnet::_Hidden::_Openssl_error_category_t::name() const noexcept
    -> char const*
{
    return "openssl-category";
}

inline auto stdnet::_Hidden::_Openssl_error_category_t::message(int _E) const
    -> ::std::string
{
    return ERR_lib_error_string(_E);
}

inline auto stdnet::_Hidden::_Openssl_error_category() -> stdnet::_Hidden::_Openssl_error_category_t const&
{
    static stdnet::_Hidden::_Openssl_error_category_t _Rc{};
    return _Rc;
}

// ----------------------------------------------------------------------------

class stdnet::_Hidden::_Openssl_context
    : public ::stdnet::_Hidden::_Context_base
{
private:
    using _Context_free_t = decltype([](auto* _Ctxt){ SSL_CTX_free(_Ctxt); });
    ::stdnet::_Hidden::_Context_base&           _D_base;
    ::std::unique_ptr<SSL_CTX, _Context_free_t> _D_context{::stdnet::_Hidden::_Openssl_context::_Make_context()};
    //::stdnet::_Hidden::_Container<::stdnet::_Hidden::_Libevent_record> _D_sockets;

    auto _Make_socket(int) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Make_socket(int, int, int, ::std::error_code&) -> ::stdnet::_Hidden::_Socket_id override;
    auto _Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&) -> void override;
    auto _Native_handle(::stdnet::_Hidden::_Socket_id) -> _Stdnet_native_handle_type override;
    auto _Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&) -> void override;
    auto _Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::_Hidden::_Endpoint const&, ::std::error_code&) -> void override;
    auto _Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&) -> void override;

    auto run_one() -> ::std::size_t override;

    auto _Cancel(::stdnet::_Hidden::_Io_base*, ::stdnet::_Hidden::_Io_base*) -> void override { /*-dk:TODO*/ }
    auto _Accept(::stdnet::_Hidden::_Context_base::_Accept_operation*) -> bool override { /*-dk:TODO*/ return {}; }
    auto _Connect(::stdnet::_Hidden::_Context_base::_Connect_operation*) -> bool override { /*-dk:TODO*/ return {}; }
    auto _Receive(::stdnet::_Hidden::_Context_base::_Receive_operation*) -> bool override { /*-dk:TODO*/ return {}; }
    auto _Send(::stdnet::_Hidden::_Context_base::_Send_operation*) -> bool override { /*-dk:TODO*/ return {}; }
    auto _Resume_after(::stdnet::_Hidden::_Context_base::_Resume_after_operation*) -> bool override;
    auto _Resume_at(::stdnet::_Hidden::_Context_base::_Resume_at_operation*) -> bool override;

    static SSL_CTX* _Make_context();
public:

    _Openssl_context(::stdnet::_Hidden::_Context_base&, ::std::string const& cert, ::std::string const& key);
    _Openssl_context(_Openssl_context&&) = delete;
};

// ----------------------------------------------------------------------------

inline SSL_CTX* stdnet::_Hidden::_Openssl_context::_Make_context()
{
    static bool const _Dummy(::std::invoke([]{
        SSL_library_init();
        SSL_load_error_strings();
        std::cout << "loaded context\n";
        return true;
        }));
    return SSL_CTX_new(TLS_method());
}

inline stdnet::_Hidden::_Openssl_context::_Openssl_context(
    ::stdnet::_Hidden::_Context_base& _Base,
    ::std::string const& cert,
    ::std::string const& key
    )
    : _D_base(_Base)
{
    if(1 != SSL_CTX_use_certificate_file(this->_D_context.get(), cert.c_str(),  SSL_FILETYPE_PEM))
    {
        throw ::std::system_error(ERR_get_error(),
                                  stdnet::_Hidden::_Openssl_error_category(),
                                  "failed to load certificate: '" + cert + "'"
                                  );
    }
    if (1 != SSL_CTX_use_PrivateKey_file(this->_D_context.get(), key.c_str(), SSL_FILETYPE_PEM))
    {
        throw ::std::system_error(ERR_get_error(),
                                  stdnet::_Hidden::_Openssl_error_category(),
                                  "failed to load private key: '" + key + "'"
                                  );
    }
    if (1 != SSL_CTX_check_private_key(this->_D_context.get()))
    {
        throw ::std::system_error(ERR_get_error(),
                                  stdnet::_Hidden::_Openssl_error_category(),
                                  "failed to check private key: '" + key + "'"
                                  );
    }
    SSL_CTX_set_options(this->_D_context.get(), SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Openssl_context::_Make_socket(int)
    -> ::stdnet::_Hidden::_Socket_id
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Make_socket not implemented");
    return {};
}

inline auto stdnet::_Hidden::_Openssl_context::_Make_socket(int, int, int, ::std::error_code&)
    -> ::stdnet::_Hidden::_Socket_id
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Make_socket(int, int, int) not implemented");
    return {};
}

inline auto stdnet::_Hidden::_Openssl_context::_Release(::stdnet::_Hidden::_Socket_id, ::std::error_code&)
    -> void
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Release not implemented");
}

inline auto stdnet::_Hidden::_Openssl_context::_Native_handle(::stdnet::_Hidden::_Socket_id)
    -> _Stdnet_native_handle_type
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Native_handle not implemented");
    return {};
}

inline auto stdnet::_Hidden::_Openssl_context::_Set_option(::stdnet::_Hidden::_Socket_id, int, int, void const*, ::socklen_t, ::std::error_code&)
    -> void
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Set_option not implemented");
}

inline auto stdnet::_Hidden::_Openssl_context::_Bind(::stdnet::_Hidden::_Socket_id, ::stdnet::_Hidden::_Endpoint const&, ::std::error_code&)
    -> void
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Bind not implemented");
}

inline auto stdnet::_Hidden::_Openssl_context::_Listen(::stdnet::_Hidden::_Socket_id, int, ::std::error_code&)
    -> void
{
    /*-dk:TODO*/
    throw ::std::runtime_error("openssl _Listen not implemented");
}

// ----------------------------------------------------------------------------

inline auto stdnet::_Hidden::_Openssl_context::run_one() -> ::std::size_t 
{
    return this->_D_base.run_one();
}

inline auto stdnet::_Hidden::_Openssl_context::_Resume_after(::stdnet::_Hidden::_Context_base::_Resume_after_operation* _Op) -> bool
{
    return this->_D_base._Resume_after(_Op);
}

inline auto stdnet::_Hidden::_Openssl_context::_Resume_at(::stdnet::_Hidden::_Context_base::_Resume_at_operation* _Op) -> bool
{
    return this->_D_base._Resume_at(_Op);
}

// ----------------------------------------------------------------------------

#endif
#if 0

namespace toy
{

// ----------------------------------------------------------------------------

struct socket
{
    static SSL_CTX* context() {
        static SSL_CTX* rc = []{
            SSL_library_init();
            SSL_load_error_strings();
            SSL_CTX* rc = SSL_CTX_new(TLS_method());

            if (SSL_CTX_use_certificate_file(rc, "tmp/cert.pem", SSL_FILETYPE_PEM) != 1) {
                throw std::runtime_error("failed to use certificated");
            }
            if (SSL_CTX_use_PrivateKey_file(rc, "tmp/cert.pem", SSL_FILETYPE_PEM) != 1) {
                throw std::runtime_error("failed to use private key");
            }
            return rc;
        }();
        return rc;
    }
    int  fd = -1;
    private:
    SSL* ssl{nullptr};
    public:
    socket() = default;
    socket(int fd)
        : fd(fd)
    {
        if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
            throw std::runtime_error("fcntl");
        }
    }
    void make_ssl() {
        auto ctx = context();
        ssl = SSL_new(ctx);
        if (!ssl) {
            throw std::runtime_error("failed to create SSL socket");
        }
        if (!SSL_set_fd(ssl, fd)) {
            throw std::runtime_error("failed to set file descriptor for SSL socket");
        }
    }
    SSL* get_ssl() {
        if (!ssl) {
            make_ssl();
        }
        return ssl;
    }
    socket(socket&& other)
        : fd(std::exchange(other.fd, -1))
        , ssl(std::exchange(other.ssl, nullptr))
    {
    }
    socket& operator= (socket&& other) {
        if (this != &other) {
            std::swap(fd, other.fd);
            std::swap(ssl, other.ssl);
        }
        return *this;
    }
    ~socket() {
        if (ssl != nullptr) {
            SSL_free(ssl);
        }
        if (fd != -1) ::close(fd);
    }
    friend std::ostream& operator<< (std::ostream& out, socket const& self) {
        return out << "fd=" << self.fd << ", ssl=" << self.ssl;
    }
};

class io_context;
struct io_scheduler {
    io_context* context;
    friend std::ostream& operator<< (std::ostream& out, io_scheduler const& s) {
        return out << s.context;
    }
};

struct io
    : immovable {
    io_context& c;
    int         fd;
    short int   events;
    virtual void complete() = 0;
    io(io_context& c, int fd, short int events): c(c), fd(fd), events(events)  {}
};

class io_context
    : public starter<toy::io_scheduler>
{
public:
    using scheduler = toy::io_scheduler;
    scheduler get_scheduler() { return { this }; }

private:
    using time_point_t = std::chrono::system_clock::time_point;
    std::vector<io*>      ios;
    std::vector<::pollfd> fds;
    toy::timer_queue<io*> times;

public:
    static constexpr bool has_timer = true; //-dk:TODO remove - used while adding timers to contexts

    io_context()
        : starter(get_scheduler()) {
    }
    void add(io* i) {
        ios.push_back(i);
        fds.push_back(  pollfd{ .fd = i->fd, .events = i->events });
    }
    void add(time_point_t time, io* op) {
        times.push(time, op);
    }
    void erase(io* i) {
        auto it = std::find(ios.begin(), ios.end(), i);
        if (it != ios.end()) {
            fds.erase(fds.begin() + (it - ios.begin()));
            ios.erase(it);
        }
    }
    void erase_timer(io* i) {
        times.erase(i);
    }
    void run() {
        while ( (!ios.empty() || not times.empty())) { 
            auto now{std::chrono::system_clock::now()};

            bool timed{false};
            while (!times.empty() && times.top().first <= now) {
                io* op{times.top().second};
                times.pop();
                op->complete();
                timed = true;
            }
            if (timed) {
                continue;
            }
            auto time{times.empty()
                     ? -1
                : std::chrono::duration_cast<std::chrono::milliseconds>(times.top().first - now).count()};
            if (0 < ::poll(fds.data(), fds.size(), time)) {
                for (size_t i = fds.size(); i--; )
                    // The check for i < fds.size() is added as complete() may
                    // cause elements to get canceled and be removed from the
                    // list.
                    if (i < fds.size() && fds[i].events & fds[i].revents) {
                        fds[i] = fds.back();
                        fds.pop_back();
                        auto c = std::exchange(ios[i], ios.back());
                        ios.pop_back();
                        c->complete();
                    }
            }
        }
    }
};

// ----------------------------------------------------------------------------

struct ssl_error_category
    : std::error_category {
    ssl_error_category(): std::error_category() {}
    char const* name() const noexcept override { return "ssl"; }
    std::string message(int err) const override {
        char error[256];
        ERR_error_string_n(err, error, sizeof(error));
        return error;
    }
};

ssl_error_category const& ssl_category() {
    static ssl_error_category rc{};
    return rc;
}

// ----------------------------------------------------------------------------

namespace hidden_async_accept {
    struct sender {
        using result_t = toy::socket;

        socket& d_socket;
        sender(socket& s): d_socket(s) {}

        template <typename R>
        struct state: io {
            socket&            d_socket;
            ::sockaddr_storage addr{};
            ::socklen_t        len{sizeof(::sockaddr_storage)};
            bool               shaking_hands{false};
            socket             d_result;
            R                  receiver;
            hidden::io_operation::stop_callback<state, R> cb;

            state(socket& s, R r)
                : io(*get_scheduler(r).context, s.fd, POLLIN)
                , d_socket(s)
                , receiver(r) {
            }
            friend void start(state& self) { self.try_accept_or_submit(); }
            void submit() {
                cb.engage(*this);
                c.add(this);
            }
            void try_accept_or_submit() {
                int rc(::accept(fd, reinterpret_cast<::sockaddr*>(&addr), &len));
                if (0 <= rc) {
                    d_result = toy::socket(rc);
                    auto ssl = d_result.get_ssl();
                    SSL_set_accept_state(ssl);
                    shaking_hands = true;
                    this->io::fd = rc;
                    ssl_accept();
                }
                else if (errno == EINTR || errno == EWOULDBLOCK) {
                    submit();
                }
                else {
                    set_error(receiver, std::make_exception_ptr(std::runtime_error(std::string("accept: ") + ::strerror(errno))));
                }
            }
            void complete() override final {
                cb.disengage();

                if (shaking_hands) {
                    ssl_accept();
                }
                else {
                    try_accept_or_submit();
                }
            }
            void ssl_accept() {
                ERR_clear_error();
                auto ssl = d_result.get_ssl();
                switch (int ret = SSL_accept(ssl)) {
                default:
                    switch (int error = SSL_get_error(ssl, ret)) {
                        default:
                            set_error(receiver, std::make_exception_ptr(std::system_error(error, ssl_category())));
                            break;
                        case SSL_ERROR_WANT_READ:
                            ERR_get_error();
                            this->events = POLLIN;
                            submit();
                            break;
                        case SSL_ERROR_WANT_WRITE:
                            ERR_get_error();
                            this->events = POLLOUT;
                            submit();
                            break;
                    }
                    break;
                case 1: // success
                    set_value(receiver, std::move(d_result));
                    break;
                case 0:
                    set_error(receiver, std::make_exception_ptr(std::system_error(SSL_get_error(ssl, ret), ssl_category())));
                    break;
                }
            }
        };

        template <typename R>
        friend state<R> connect(sender const& self, R r) {
            return state<R>(self.d_socket, r);
        }
    };
}
using async_accept = hidden_async_accept::sender;

// ----------------------------------------------------------------------------

namespace hidden_async_connect {
    struct sender {
        using result_t = int;

        socket&            d_socket;
        ::sockaddr const*  addr;
        ::socklen_t        len;

        sender(socket& s, auto addr, auto len): d_socket(s), addr(addr), len(len) {}

        template <typename R>
        struct state: io {
            socket&            d_socket;
            ::sockaddr const*  addr;
            ::socklen_t        len;
            bool               shaking_hands{false};
            R                  receiver;
            hidden::io_operation::stop_callback<state, R> cb;

            state(socket& s, auto addr, auto len, R r)
                : io(*get_scheduler(r).context, s.fd, POLLOUT)
                , d_socket(s)
                , addr(addr)
                , len(len)
                , receiver(r) {
            }
            friend void start(state& self) {
                if (0 <= ::connect(self.fd, self.addr, self.len)) {
                    // immediate connect completed
                    self.connect();
                }
                else if (errno == EAGAIN || errno == EINPROGRESS) {
                    self.cb.engage(self);
                    self.c.add(&self);
                }
                else {
                    set_error(self.receiver, std::make_exception_ptr(std::runtime_error(std::string("connect: ") + ::strerror(errno))));
                }
            }
            void complete() override final {
                if (shaking_hands) {
                    connect();
                    return;
                }

                cb.disengage();
                int         rc{};
                ::socklen_t len{sizeof rc};
                if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &rc, &len)) {
                    set_error(receiver, std::make_exception_ptr(std::runtime_error(std::string("getsockopt: ") + ::strerror(errno))));
                }
                else if (rc) {
                    set_error(receiver, std::make_exception_ptr(std::runtime_error(std::string("connect: ") + ::strerror(rc))));
                }
                else {
                    this->connect();
                }
            }
            void connect() {
                auto ssl = d_socket.get_ssl();
                if (not shaking_hands) {
                    SSL_set_connect_state(ssl);
                    shaking_hands = true;
                }
                switch (int ret = SSL_connect(ssl)) {
                default:
                    switch (int error = SSL_get_error(ssl, ret)) {
                        default:
                            set_error(receiver, std::make_exception_ptr(std::system_error(error, ssl_category())));
                            break;
                        case SSL_ERROR_WANT_READ:
                            this->events = POLLIN;
                            this->cb.engage(*this);
                            this->c.add(this);
                            break;
                        case SSL_ERROR_WANT_WRITE:
                            this->events = POLLOUT;
                            this->cb.engage(*this);
                            this->c.add(this);
                            break;
                    }
                    break;
                case 1: // success
                    set_value(receiver, ret);
                    break;
                case 0:
                    set_error(receiver, std::make_exception_ptr(std::system_error(SSL_get_error(ssl, ret), ssl_category())));
                    break;
                }
            }
        };

        template <typename R>
        friend state<R> connect(sender const& self, R r) {
            return state<R>(self.d_socket, self.addr, self.len, r);
        }
    };
}
using async_connect = hidden_async_connect::sender;

// ----------------------------------------------------------------------------

namespace hidden_async_write_some {
    struct sender {
        using result_t = int;

        toy::socket&  socket;
        char const*   buffer;
        std::size_t   len;

        sender(toy::socket& socket, char const* buffer, std::size_t len)
            : socket(socket)
            , buffer(buffer)
            , len(len) {
        }
        sender(toy::socket& socket, std::array<::iovec, 1> const& buffer)
            : socket(socket)
            , buffer(static_cast<char*>(buffer[0].iov_base))
            , len(buffer[0].iov_len)
        {
        }

        template <typename R>
        struct state
            : toy::io
        {
            toy::socket&                                  d_socket;
            R                                             receiver;
            char const*                                   buffer;
            std::size_t                                   len;
            hidden::io_operation::stop_callback<state, R> cb;

            state(R            receiver,
                  toy::socket& socket,
                  char const*  buffer,
                  std::size_t  len)
                : io(*get_scheduler(receiver).context, socket.fd, POLLOUT)
                , d_socket(socket)
                , receiver(receiver)
                , buffer(buffer)
                , len(len)
                , cb() {
            }

            friend void start(state& self) {
                self.cb.engage(self);
                self.c.add(&self);
            }
            void complete() override final {
                cb.disengage();
                auto ssl = d_socket.get_ssl();
                auto res{SSL_write(ssl, buffer, len)};
                if (0 < res) {
                    set_value(std::move(receiver), result_t(res));
                }
                else {
                    switch (int error = SSL_get_error(ssl, res)) {
                        default:
                            set_error(std::move(receiver), std::make_exception_ptr(std::system_error(error, ssl_category())));
                            break;
                        case SSL_ERROR_WANT_WRITE:
                            this->events = POLLOUT;
                            start(*this);
                            break;
                        case SSL_ERROR_WANT_READ:
                            this->events = POLLIN;
                            start(*this);
                            break;
                    }
                }  
            }
        };
        template <typename R>
        friend state<R> connect(sender const& self, R receiver) {
            return state<R>(receiver, self.socket, self.buffer, self.len);
        }
    };
}

using async_write_some = hidden_async_write_some::sender;
using async_send = hidden_async_write_some::sender;

// ----------------------------------------------------------------------------

namespace hidden_async_read_some {
    struct sender {
        using result_t = int;

        toy::socket&  socket;
        char*         buffer;
        std::size_t   len;

        sender(toy::socket& socket, char* buffer, std::size_t len)
            : socket(socket)
            , buffer(buffer)
            , len(len) {
        }
        sender(toy::socket& socket, std::array<::iovec, 1> const& buffer)
            : socket(socket)
            , buffer(static_cast<char*>(buffer[0].iov_base))
            , len(buffer[0].iov_len)
        {
        }

        template <typename R>
        struct state
            : toy::io
        {
            toy::socket&                                  d_socket;
            R                                             receiver;
            char*                                         buffer;
            std::size_t                                   len;
            hidden::io_operation::stop_callback<state, R> cb;

            state(R            receiver,
                  toy::socket& socket,
                  char*        buffer,
                  std::size_t  len)
                : io(*get_scheduler(receiver).context, socket.fd, POLLIN)
                , d_socket(socket)
                , receiver(receiver)
                , buffer(buffer)
                , len(len)
                , cb() {
            }

            friend void start(state& self) {
                self.cb.engage(self);
                self.c.add(&self);
            }
            void complete() override final {
                cb.disengage();
                auto ssl = d_socket.get_ssl();
                auto res{SSL_read(ssl, buffer, len)};
                if (0 < res) {
                    set_value(std::move(receiver), result_t(res));
                }
                else {
                    switch (int error = SSL_get_error(ssl, res)) {
                        default:
                            set_error(std::move(receiver), std::make_exception_ptr(std::system_error(error, ssl_category())));
                            break;
                        case SSL_ERROR_WANT_WRITE:
                            this->events = POLLOUT;
                            start(*this);
                            break;
                        case SSL_ERROR_WANT_READ:
                            this->events = POLLIN;
                            start(*this);
                            break;
                    }
                }  
            }
        };
        template <typename R>
        friend state<R> connect(sender const& self, R receiver) {
            return state<R>(receiver, self.socket, self.buffer, self.len);
        }
    };
}

using async_read_some = hidden_async_read_some::sender;
using async_receive = hidden_async_read_some::sender;

// ----------------------------------------------------------------------------

namespace hidden::io_operation {
    template <typename Operation>
    struct sender {
        using result_t = typename Operation::result_t;

        toy::socket&  socket;
        Operation     op;

        template <typename R>
        struct state
            : toy::io
        {
            R                       receiver;
            Operation               op;
            stop_callback<state, R> cb;

            state(R         receiver,
                  int       fd,
                  Operation op)
                : io(*get_scheduler(receiver).context, fd, Operation::event == event_kind::read? POLLIN: POLLOUT)
                , receiver(receiver)
                , op(op)
                , cb() {
            }

            friend void start(state& self) {
                self.cb.engage(self);
                self.c.add(&self);
            }
            void complete() override final {
                cb.disengage();
                auto res{op(*this)};
                if (0 <= res)
                    set_value(std::move(receiver), typename Operation::result_t(res));
                else {
                    set_error(std::move(receiver), std::make_exception_ptr(std::system_error(errno, std::system_category())));
                }  
            }
        };
        template <typename R>
        friend state<R> connect(sender const& self, R receiver) {
            return state<R>(receiver, self.socket.fd, self.op);
        }
    };
}

template <typename MBS>
hidden::io_operation::sender<hidden::io_operation::receive_from_op<MBS>>
async_receive_from(toy::socket& s, const MBS& b, toy::address& addr, toy::message_flags f) {
    return {s, {b, addr, f}};
}
template <typename MBS>
hidden::io_operation::sender<hidden::io_operation::receive_from_op<MBS>>
async_receive_from(toy::socket& s, const MBS& b, toy::address& addr) {
    return {s, {b, addr, toy::message_flags{}}};
}

template <typename MBS>
hidden::io_operation::sender<hidden::io_operation::send_to_op<MBS>>
async_send_to(toy::socket& s, const MBS& b, toy::address addr, toy::message_flags f) {
    return {s, {b, addr, f}};
}
template <typename MBS>
hidden::io_operation::sender<hidden::io_operation::send_to_op<MBS>>
async_send_to(toy::socket& s, const MBS& b, toy::address addr) {
    return {s, {b, addr, toy::message_flags{}}};
}

// ----------------------------------------------------------------------------

namespace hidden_async_sleep_for {
    struct async_sleep_for {
        using result_t = toy::none;
        using duration_t = std::chrono::milliseconds;

        duration_t  duration;

        template <typename R>
        struct state
            : io {
            R                            receiver;
            duration_t                   duration;
            hidden::io_operation::stop_callback<state, R, true> cb;

            state(R receiver, duration_t duration)
                : io(*get_scheduler(receiver).context, 0, 0)
                , receiver(receiver)
                , duration(duration) {
            }
            friend void start(state& self) {
                self.cb.engage(self);
                self.c.add(std::chrono::system_clock::now() + self.duration, &self);;
            }
            void complete() override {
                cb.disengage();
                set_value(receiver, result_t{});
            }
        };
        template <typename R>
        friend state<R> connect(async_sleep_for self, R receiver) {
            return state<R>(receiver, self.duration);
        }
    };
}
using async_sleep_for = hidden_async_sleep_for::async_sleep_for;

// ----------------------------------------------------------------------------

}
#endif