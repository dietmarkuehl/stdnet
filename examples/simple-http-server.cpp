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
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exception>
#include <iostream>

// ----------------------------------------------------------------------------

auto make_client(auto stream) -> exec::task<void>
{
    std::cout << "starting client\n";
    co_await stdexec::just();
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