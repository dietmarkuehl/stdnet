// examples/simple-http-server.cpp                                    -*-C++-*-
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

#include <stdnet/socket.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/buffer.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <ranges>

// ----------------------------------------------------------------------------

std::string const hello("<html><head><title>Hello</title></head><body>Hello, new world!</body><html>");

// ----------------------------------------------------------------------------

struct parser
{
    char const* it{nullptr};
    char const* end{it};

    bool empty() const { return it == end; }
    std::string_view find(char c)
    {
        auto f{std::find(it, end, c)};
        auto begin{it};
        it = f;
        return {begin, it == end? it: it++};
    }
    std::string_view search(std::string_view v)
    {
        auto s{std::search(it, end, v.begin(), v.end())};
        auto begin{it};
        it = s != end? s + v.size(): s;
        return {begin, s};
    }
};

auto make_client(auto stream) -> exec::task<void>
{
    std::cout << "starting client\n";
    char        buffer[1024];
    std::size_t len{};
    while (true)
    {
        auto n = co_await stdnet::async_receive(stream, stdnet::buffer(buffer + len, std::min(10ul, sizeof(buffer) - len)));
        if (n == 0u)
        {
            std::cout << "failed to read complete buffer\n"; //-dk:TODO send error
            break;
        }
        auto start(4 < len? len - 4: 0);
        len += n;
        auto separator{std::ranges::search(std::string_view(buffer + start, buffer + len), std::string_view("\r\n\r\n"))};
        if (not separator.empty())
        {
            std::cout << "found end of header\n";
            char const* end = separator.end() - 2;
            parser p{buffer, end};
            auto method{p.find(' ')};
            auto uri{p.find(' ')};
            auto version{p.search("\r\n")};

            std::cout << "method='" << method << "'\n";
            std::cout << "uri='" << uri << "'\n";
            std::cout << "version='" << version << "'\n";

            while (not p.empty())
            {
                auto key{p.find(':')};
                auto value{p.search("\r\n")};
                std::cout << "key='" << key << "' value='" << value << "\n";
            }
            break;
        }
    }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Length: " << hello.size() << "\r\n"
             << "\r\n"
             << hello
             ;
    auto r = response.str();
    auto rn = co_await stdnet::async_send(stream, stdnet::buffer(r.c_str(), r.size()));
    std::cout << "response size=" << r.size() << " sent=" << rn << "\n";

    std::cout << "stopping client\n";
}

auto make_server(auto& context, auto& scope, auto endpoint) -> exec::task<void>
{
    stdnet::ip::tcp::acceptor acceptor(context, endpoint);
    while (true)
    {
        auto[stream, client] = co_await stdnet::async_accept(acceptor);
        std::cout << "received a connection from '" << client << "'\n";
        scope.spawn(make_client(::std::move(stream)));
    }
}

int main()
{
    std::cout << std::unitbuf;
    try
    {
        stdnet::io_context        context;
        stdnet::ip::tcp::endpoint endpoint(stdnet::ip::address_v4::any(), 12345);
        exec::async_scope         scope;

        scope.spawn(make_server(context, scope, endpoint));

        context.run();
    }
    catch (std::exception const& ex)
    {
        std::cout << "ERROR: " << ex.what() << "\n";
    }
}