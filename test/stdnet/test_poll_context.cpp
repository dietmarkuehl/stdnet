// stdnet/test_poll_context.cpp                                       -*-C++-*-
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

#include <stdnet/poll_context.hpp>
#include <iostream> //-dk:TODO remove
#include <iomanip> //-dk:TODO remove
#include <thread>
#include <unordered_map>
#include <cerrno>
#include <cstring>

#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>

// ----------------------------------------------------------------------------

namespace
{
    namespace poll_context
    {
        struct socket
        {
            int fd{-1};
            socket()
                : fd(::socket(AF_INET6, SOCK_STREAM, 0))
            {
                CHECK(0 <= this->fd);
                int reuseaddr{1};
                CHECK(0 <= ::setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)));
            }
            socket(socket&& other): fd(std::exchange(other.fd, -1)) {}
            ~socket() { if (0 <= this->fd) ::close(fd); }
            operator int() const { return this->fd; }
        };

        struct server
            : poll_context::socket
        {
            server(unsigned short port)
                : poll_context::socket()
            {
                ::sockaddr_in6 any{};
                any.sin6_family = AF_INET6;
                any.sin6_addr = in6addr_any;
                any.sin6_len = sizeof(any);
                any.sin6_port = htons(port);
                CHECK(0 <= ::bind(this->fd, reinterpret_cast<sockaddr*>(&any), sizeof(any)));
                CHECK(0 <= ::listen(this->fd, 1));
            }
        };

        struct client
            : poll_context::socket
        {
            ::sockaddr_in6 address{};
            client(bool blocking, unsigned short port)
                : poll_context::socket()
            {
                if (!blocking)
                {
                    int flags = ::fcntl(this->fd, F_GETFL, 0);
                    CHECK(0 <= flags);
                    CHECK(0 <= ::fcntl(this->fd, F_SETFL, flags | O_NONBLOCK));
                }
                address.sin6_family = AF_INET6;
                address.sin6_addr = in6addr_loopback;
                address.sin6_len = sizeof(address);
                address.sin6_port = htons(port);
            }
        };

        using expected = std::unordered_map<void*, ::stdnet::_Result>;
        struct operation
            : ::stdnet::_Io_operation
        {
            expected& ex;
            operation(expected& ex, ::stdnet::_Opcode op, void* ptr = nullptr, std::int_least64_t len = {}, ::stdnet::_Native_handle h = {})
                : ::stdnet::_Io_operation(op, ptr, len, h)
                , ex(ex)
            {
            }
            auto _Do_complete() -> void override
            {
                auto it{ex.find(this)};
                if (it != ex.end() && it->second == this->_Result())
                    ex.erase(it);
            }
        };
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("poll_context creation", "[poll_context]")
{
    CHECK(not std::movable<::stdnet::_Poll_context>);
    CHECK(not std::copyable<::stdnet::_Poll_context>);

    ::stdnet::_Poll_context ctxt;
    CHECK(ctxt.empty());
}

TEST_CASE("poll_context cancel non-existing", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;
    ::poll_context::operation cancel{ex, ::stdnet::_Opcode::_Cancel};
    ex[&cancel] = ::stdnet::_Result::_Not_found;

    ctxt._Submit(cancel);
    auto rc = ctxt._Run_one();
    CHECK(rc == 1u);
    CHECK(ex.empty());
}

TEST_CASE("poll_context cancel existing", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    ::poll_context::operation receive{ex, ::stdnet::_Opcode::_Receive};
    ::poll_context::operation cancel{ex, ::stdnet::_Opcode::_Cancel, &receive};

    ex[&receive] = ::stdnet::_Result::_Cancelled;
    ex[&cancel] = ::stdnet::_Result::_Success;

    ctxt._Submit(receive);
    ctxt._Submit(cancel);
    auto rc = ctxt._Run_one() + ctxt._Run_one();
    CHECK(rc == 2u);
    CHECK(ex.empty());
}

TEST_CASE("poll_context accept", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    unsigned short         port{12345};
    ::poll_context::server server(port);
    CHECK(0 <= server);

    ::sockaddr_storage addr{};
    ::poll_context::operation accept{ex, ::stdnet::_Opcode::_Accept, &addr, sizeof(addr), server};

    ctxt._Submit(accept);

    poll_context::client client(true, port);
    std::thread cthread([&client]{
        CHECK(0 <= ::connect(client, reinterpret_cast<sockaddr*>(&client.address), sizeof(client.address)));
    });

    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);

    cthread.join();

    CHECK(addr.ss_family == client.address.sin6_family);
    CHECK(addr.ss_len == client.address.sin6_len);
    ::sockaddr_in6 const& a(*reinterpret_cast<::sockaddr_in6*>(&addr));
    CHECK(::memcmp(&a.sin6_addr, &client.address.sin6_addr, sizeof(a.sin6_addr)) == 0);
}

TEST_CASE("poll_context connect with ready server", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    unsigned short       port(12345);
    poll_context::client client(false, port);

    ::poll_context::operation connect{ex, ::stdnet::_Opcode::_Connect, &client.address, sizeof(client.address), client};

    poll_context::server server(port);

    ::sockaddr_storage addr;
    ::socklen_t len(sizeof(addr));
    std::thread cthread([server = std::move(server), &addr, &len]{
        CHECK(0 <= ::accept(server, reinterpret_cast<::sockaddr*>(&addr), &len));
    });

    ctxt._Submit(connect);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);

    cthread.join();
}