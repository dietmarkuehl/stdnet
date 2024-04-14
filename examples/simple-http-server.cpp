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
#include <stdnet/ring_buffer.hpp>
#include <stdnet/timer.hpp>
#include <stdnet/openssl_context.hpp>
#include <exec/async_scope.hpp>
#include <exec/when_any.hpp>
#include <exec/task.hpp>
#include <algorithm>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ranges>

using namespace std::chrono_literals;
using namespace std::string_view_literals;

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

struct request
{
    std::string              method, uri, version;
    std::unordered_map<std::string, std::string> headers;
    std::string              body;
};

auto read_http_request(auto& stream) -> exec::task<request>
{
    request r;
    auto head = co_await stream.read_head();
    if (not head.empty())
    {
        parser p{head.begin(), head.end() - 2};
        r.method = p.find(' ');
        r.uri = p.find(' ');
        r.version = p.search("\r\n");

        while (not p.empty())
        {
            std::string key{p.find(':')};
            p.skip_whitespace();
            auto value{p.search("\r\n")};
            r.headers[key] = value;
        }
    }
    stream.consume();
    co_return r;
}

std::unordered_map<std::string, std::string> res
{
    {"/", "data/hello.html"},
    {"/fav.png", "data/fav.png"}
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
        auto r = co_await read_http_request(stream);
        if (r.method.empty())
        {
            co_await stream.write_response("550 ERROR", "");
            co_return;
        }

        if (r.method == "GET"sv)
        {
            auto it = res.find(r.uri);
            std::cout << "getting '" << r.uri << "'->" << (it == res.end()? "404": "OK") << "\n";
            if (it == res.end())
            {
                co_await stream.write_response("404 NOT FOUND", "not found");
            }
            else
            {
                std::ifstream in(it->second);
                std::string res(std::istreambuf_iterator<char>(in), {});
                co_await stream.write_response("200 OK", res);
            }
        }

        keep_alive = r.headers["Connection"] == "keep-alive"sv;
        std::cout << "keep-alive=" << std::boolalpha << keep_alive << "\n";
    }
}

auto make_server(auto& context, auto& scope, auto endpoint) -> exec::task<void>
{
    try
    {
        stdnet::ip::tcp::acceptor acceptor(context, endpoint);
        std::cout << "created acceptor for " << endpoint << "\n";
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
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

int main()
{
    std::cout << std::unitbuf;
    try
    {
        stdnet::io_context context;
        stdnet::io_context tls_context{::stdnet::tls_context(context, "ssl/server_cert.pem", "ssl/server_keypair.pem")};

        stdnet::ip::tcp::endpoint endpoint(stdnet::ip::address_v4::any(), 12345);
        stdnet::ip::tcp::endpoint tls_endpoint(stdnet::ip::address_v4::any(), 12346);
        exec::async_scope         scope;

        scope.spawn(make_server(context, scope, endpoint));
        scope.spawn(make_server(tls_context, scope, tls_endpoint));

        tls_context.run();
    }
    catch (std::exception const& ex)
    {
        std::cout << "ERROR: " << ex.what() << "\n";
    }
}
