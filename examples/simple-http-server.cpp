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
#include <stdnet/timer.hpp>
#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
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
    void skip_whitespace()
    {
        while (it != end && *it == ' ' || *it == '\t')
            ++it;
    }
    std::string_view search(std::string_view v)
    {
        auto s{std::search(it, end, v.begin(), v.end())};
        auto begin{it};
        it = s != end? s + v.size(): s;
        return {begin, s};
    }
};

template <typename Stream>
struct buffered_stream
{
    static constexpr char sep[]{ '\r', '\n', '\r', '\n', '\0' };
    Stream            stream;
    std::vector<char> buffer = std::vector<char>(1024u);
    std::size_t       pos{};
    std::size_t       end{};

    void consume()
    {
        buffer.erase(buffer.begin(), buffer.begin() + pos);
        end -= pos;
        pos = 0;
    }
    auto read_head() -> exec::task<std::string_view>
    {
        while (true)
        {
            if (buffer.size() == end)
                buffer.resize(buffer.size() * 2);
            auto n = co_await stdnet::async_receive(stream, stdnet::buffer(buffer.data() + end, buffer.size() - end));
            if (n == 0u)
                co_return {};
            end += n;
            pos = std::string_view(buffer.data(), end).find(sep);
            if (pos != std::string_view::npos)
                co_return {buffer.data(), pos += std::size(sep) - 1};
        }
    }
    auto write_response(std::string_view message, std::string_view response) -> exec::task<void>
    {
        std::ostringstream out;
        out << "HTTP/1.1 " << message << "\r\n"
             << "Content-Length: " << response.size() << "\r\n"
             << "\r\n"
             ;
        std::string head(out.str());
        std::size_t p{}, n{};
        do 
            n = co_await stdnet::async_send(stream, stdnet::buffer(head.data() + p, head.size() - p));
        while (0 < n && (p += n) != head.size());

        p = 0;
        do
            n = co_await stdnet::async_send(stream, stdnet::buffer(response.data() + p, response.size() - p));
        while (0 < n && (p += n) != response.size());
    }
};

template <typename Stream>
auto make_client(Stream s) -> exec::task<void>
{
    std::unique_ptr<char const, decltype([](char const* str){ std::cout << str; })> dtor("stopping client\n");
    std::cout << "starting client\n";

    buffered_stream<Stream> stream{std::move(s)};
    bool keep_alive{true};

    while (keep_alive)
    {
        auto head = co_await stream.read_head();
        if (head.empty())
            //-dk:TODO [try to] send an error
            co_return;
        parser p{head.begin(), head.end() - 2};
        std::cout << "found end of header\n";
        auto method{p.find(' ')};
        auto uri{p.find(' ')};
        auto version{p.search("\r\n")};

        std::cout << "method='" << method << "'\n";
        std::cout << "uri='" << uri << "'\n";
        std::cout << "version='" << version << "'\n";

        keep_alive = false;
        while (not p.empty())
        {
            auto key{p.find(':')};
            p.skip_whitespace();
            auto value{p.search("\r\n")};
            std::cout << "key='" << key << "' value='" << value << "\n";
            if (key == "Connection" && value == "keep-alive")
                keep_alive = true;
        }
        co_await stream.write_response("200 OK", hello);
        stream.consume();
        std::cout << "keep-alive=" << std::boolalpha << keep_alive << "\n";
    }
}

auto make_server(auto& context, auto& scope, auto endpoint) -> exec::task<void>
{
    using namespace std::chrono_literals;
    stdnet::ip::tcp::acceptor acceptor(context, endpoint);
    while (true)
    {
        auto[stream, client] = co_await stdnet::async_accept(acceptor);
        std::cout << "received a connection from '" << client << "'\n";
        scope.spawn(exec::when_any(
            stdnet::async_resume_after(context.get_scheduler(), 10s),
            make_client(::std::move(stream))
            )
            | stdexec::upon_error([](auto){ std::cout << "error\n"; })
            );
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