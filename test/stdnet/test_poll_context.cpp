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
            socket(int fd): fd(fd) {}
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
    auto rc{ctxt._Run_one() + ctxt._Run_one() + ctxt._Run_one()};
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
    poll_context::server server(port);

    ::sockaddr_storage addr;
    ::socklen_t        len(sizeof(addr));
    std::thread cthread([server = std::move(server), &addr, &len]{
        CHECK(0 <= ::accept(server, reinterpret_cast<::sockaddr*>(&addr), &len));
    });

    poll_context::client      client(false, port);
    ::poll_context::operation connect{ex, ::stdnet::_Opcode::_Connect, &client.address, sizeof(client.address), client};
    ctxt._Submit(connect);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);

    cthread.join();
}

TEST_CASE("poll_context send", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    unsigned short port(12345);
    poll_context::server server(port);
    poll_context::client client(true, port);
    CHECK(0 <= ::connect(client.fd, reinterpret_cast<::sockaddr*>(&client.address), sizeof(client.address)));

    ::sockaddr_storage addr;
    ::socklen_t        len(sizeof(addr));
    CHECK(0 <= ::accept(server, reinterpret_cast<::sockaddr*>(&addr), &len));

    ::iovec vec[] = {
        { .iov_base = const_cast<char*>("hello, "), .iov_len = 7 },
        { .iov_base = const_cast<char*>("world\n"), .iov_len = 6 }
    };
    ::stdnet::_Message message(vec, 2);
    ::poll_context::operation send{ex, ::stdnet::_Opcode::_Send, message._Address(), 0, client};
    ctxt._Submit(send);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);
    CHECK(send._ResultValue() == 13);
}

TEST_CASE("poll_context receive", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    unsigned short port(12345);
    poll_context::server server(port);
    poll_context::client client(true, port);
    CHECK(0 <= ::connect(client.fd, reinterpret_cast<::sockaddr*>(&client.address), sizeof(client.address)));

    ::sockaddr_storage addr;
    ::socklen_t        len(sizeof(addr));
    int                fd{::accept(server, reinterpret_cast<::sockaddr*>(&addr), &len)};
    CHECK(0 <= fd);
    poll_context::socket socket(fd);

    char               rbuffer[64];
    ::iovec            rvec[] = { { .iov_base = rbuffer, .iov_len = sizeof(rbuffer) } };
    ::stdnet::_Message rmessage(rvec, 1);
    ::poll_context::operation receive{ex, ::stdnet::_Opcode::_Receive, rmessage._Address(), 0, socket};
    ctxt._Submit(receive);

    ::iovec svec[] = {
        { .iov_base = const_cast<char*>("hello, "), .iov_len = 7 },
        { .iov_base = const_cast<char*>("world\n"), .iov_len = 6 }
    };
    ::stdnet::_Message smessage(svec, 2);
    ::poll_context::operation send{ex, ::stdnet::_Opcode::_Send, smessage._Address(), 0, client};
    ctxt._Submit(send);

    auto rc{ctxt._Run_one() + ctxt._Run_one()};
    CHECK(rc == 2u);
    CHECK(send._ResultValue() == 13);
    CHECK(receive._ResultValue() == 13);
}

TEST_CASE("poll_context poll", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    unsigned short port(12345);
    poll_context::server server(port);
    poll_context::client client(true, port);
    CHECK(0 <= ::connect(client.fd, reinterpret_cast<::sockaddr*>(&client.address), sizeof(client.address)));

    ::poll_context::operation poll{ex, ::stdnet::_Opcode::_Poll, nullptr, 0, server};
    poll._Set_flags(POLLIN);

    ctxt._Submit(poll);

    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);
    CHECK(poll._Flags() == POLLIN);
}

TEST_CASE("poll_context wait_for", "[poll_context]")
{
    using namespace ::std::chrono_literals;
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    
    auto duration1{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(10ms)};
    poll_context::operation wait_for1(ex, ::stdnet::_Opcode::_Wait_for, &duration1);
    auto duration2{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(20ms)};
    poll_context::operation wait_for2(ex, ::stdnet::_Opcode::_Wait_for, &duration2);
    auto duration3{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(30ms)};
    poll_context::operation wait_for3(ex, ::stdnet::_Opcode::_Wait_for, &duration3);

    auto start{::std::chrono::system_clock::now()};
    ctxt._Submit(wait_for1);
    ctxt._Submit(wait_for2);
    ctxt._Submit(wait_for3);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);
    auto end{::std::chrono::system_clock::now()};
    CHECK(duration1 <= ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start));
    CHECK(::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start) <= duration2);
    CHECK(::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start) <= duration3);
    rc = ctxt._Run_one() + ctxt._Run_one();
    CHECK(rc == 2u);
}

TEST_CASE("poll_context wait_for with wake-up", "[poll_context]")
{
    using namespace ::std::chrono_literals;
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    
    auto duration1{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(10ms)};
    poll_context::operation wait_for1(ex, ::stdnet::_Opcode::_Wait_for, &duration1);
    auto duration2{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(20ms)};
    poll_context::operation wait_for2(ex, ::stdnet::_Opcode::_Wait_for, &duration2);

    auto start{::std::chrono::system_clock::now()};
    ctxt._Submit(wait_for2);
    std::thread thread{[&ctxt]{
        auto rc{ctxt._Run_one() + ctxt._Run_one()};
        CHECK(rc == 2u);
    }};

    ctxt._Submit(wait_for1);
    thread.join();

    auto end{::std::chrono::system_clock::now()};
    CHECK(duration1 <= ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start));
    CHECK(::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start) <= duration2);
}

TEST_CASE("poll_context cancel wait_for", "[poll_context]")
{
    using namespace ::std::chrono_literals;
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;

    
    auto duration1{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(10ms)};
    poll_context::operation wait_for1(ex, ::stdnet::_Opcode::_Wait_for, &duration1);
    auto duration2{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(20ms)};
    poll_context::operation wait_for2(ex, ::stdnet::_Opcode::_Wait_for, &duration2);
    auto duration3{::std::chrono::duration_cast<::std::chrono::system_clock::duration>(30ms)};
    poll_context::operation wait_for3(ex, ::stdnet::_Opcode::_Wait_for, &duration3);
    poll_context::operation cancel(ex, ::stdnet::_Opcode::_Cancel, static_cast<::stdnet::_Io_operation*>(&wait_for2));

    auto start{::std::chrono::system_clock::now()};
    ctxt._Submit(wait_for1);
    ctxt._Submit(wait_for2);
    ctxt._Submit(wait_for3);
    ctxt._Submit(cancel);
    auto rc{ctxt._Run_one() + ctxt._Run_one()};
    CHECK(rc == 2u);
    auto end{::std::chrono::system_clock::now()};
    CHECK(::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start) <= duration1 - 5ms);

    rc = ctxt._Run_one();
    CHECK(rc == 1u);
    end = ::std::chrono::system_clock::now();
    CHECK(duration1 <= ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start));
    CHECK(::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start) <= duration1 + 5ms);

    rc = ctxt._Run_one();
    CHECK(rc == 1u);
    end = ::std::chrono::system_clock::now();
    CHECK(duration3 <= ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start));
}
