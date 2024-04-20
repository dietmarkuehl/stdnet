// http-server-template.cpp                                           -*-C++-*-
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

#include <stdnet/buffer.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/socket.hpp>
#include <stdnet/timer.hpp>

#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using namespace std::string_view_literals;

// ----------------------------------------------------------------------------
// stdnet::ip::tcp::acceptor
// stdnet::ip::tcp::endpoint
// 200 OK
// 301 Moved Permanently (Location: URL)
// 400 Bad Request
// 404 Not Found
// 500 Internal Server Error
// 501 Not Implemented

using on_exit = std::unique_ptr<char const, decltype([](auto m){ std::cout << m << "\n"; })>;

auto send_response(auto& stream, std::string head, std::string body)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << head << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;

    return stdexec::just() | stdexec::let_value([data = out.str(), &stream]()mutable noexcept {
        return stdnet::async_send(stream, stdnet::buffer(data))
             | stdexec::then([](auto&&) noexcept {})
             ;
        }
    )
    ;
}

std::unordered_map<std::string, std::string> data{
    {"/", "data/hello.html" },
    {"fav.png", "data/fav.png" }
};

exec::task<void> make_client(auto stream, auto& scope)
{
    on_exit msg("exiting client");
    std::cout << "inside client\n";
    std::vector<char> b(16);
    std::size_t end{};
    for (std::size_t n; (n = co_await stdnet::async_receive(stream, stdnet::buffer(b.data() + end, b.size() - end))); )
    {
        end += n;
        auto pos = std::string_view(b.data(), b.data() + end).find("\r\n\r\n");
        if (pos != std::string_view::npos)
        {
            std::cout << "read header\n";
            std::istringstream in(std::string(b.data(), b.data() + pos));
            std::string method, url, version;
            if (in >> method >> url >> version)
            {
                if (method == "GET" && data.contains(url))
                {
                    std::ifstream in(data[url]);
                    co_await send_response(stream, "200 OK",
                         std::string(std::istreambuf_iterator<char>(in), {}))
                         ;
                }
                else if (method == "GET" && url == "/exit")
                {
                    co_await send_response(stream, "200 OK", "shutting down")
                         ;
                    scope.get_stop_source().request_stop();
                }
                else
                {
                    co_await send_response(stream, "404 not found", "");
                }
            }

            b.erase(b.begin(), b.begin() + pos);
        }
        if (b.size() == end)
        {
            b.resize(b.size() * 2);
        }
    }
}

auto timeout(auto scheduler, auto duration, auto sender)
{
    return exec::when_any(
        stdnet::async_resume_after(scheduler, duration),
        std::move(sender)
    );
}

exec::task<void> make_server(auto& context, auto& scope)
{
    on_exit msg("exiting server");
    std::cout << "inside server\n";
    stdnet::ip::tcp::endpoint endpoint(stdnet::ip::address_v4::any(), 12345);
    stdnet::ip::tcp::acceptor acceptor(context, endpoint);
    while (true)
    {
        auto[stream, client] = co_await stdnet::async_accept(acceptor);
        std::cout << "connection from " << client << "\n";
        scope.spawn(
            timeout(context.get_scheduler(), 5s, make_client(std::move(stream), scope))
            | stdexec::upon_error([](auto&&){ std::cout << "receiver an error\n";})
            );
    }
}

int main()
{
    on_exit msg("goodbye");
    std::cout << "hello, world\n";
    stdnet::io_context context;
    exec::async_scope scope;
    scope.spawn(make_server(context, scope));
    // scope.get_stop_source().request_stop();
    context.run();
    stdexec::sync_wait(scope.on_empty());
}