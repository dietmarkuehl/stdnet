// examples/client.cpp                                                -*-C++-*-
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
#include <stdnet/basic_stream_socket.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/buffer.hpp>

#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <functional>
#include <iostream>

// ----------------------------------------------------------------------------

int main()
{
    using stream_socket = stdnet::basic_stream_socket<stdnet::ip::tcp>;
    stdnet::io_context context;
    exec::async_scope scope;
    scope.spawn(std::invoke([](auto& context)->exec::task<void> {
        try
        {
            context.get_scheduler();
            stream_socket client(context, stream_socket::endpoint_type(stdnet::ip::address_v4::loopback(), 12345));
            client.get_scheduler();
            char buffer[] = {'h', 'e', 'l', 'l', 'o'};
            co_await stdnet::async_connect(client);
            auto n = co_await stdnet::async_send(client, stdnet::buffer(buffer));
        }
        catch(std::exception const& ex)
        {
            std::cerr << "ERROR: " << ex.what() << '\n';
        }
    }, context));

    std::cout << "running context\n";
    context.run();
    std::cout << "running done\n";
}
