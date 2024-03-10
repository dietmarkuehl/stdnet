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

#include <iostream>
#include <string_view>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/task.hpp>
#include <stdnet/buffer.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>
#include <stdnet/timer.hpp>

template <typename E>
struct error_handler_base
{
    void operator()(E)
    {
        std::cout << "error_handler\n";
    }
};
template <typename... E>
struct error_handler
    : error_handler_base<E>...
{
};

auto make_client(exec::async_scope& scope, auto client) -> exec::task<void>
{
    try
    {
        char buffer[8];
        while (auto size = co_await stdnet::async_receive(client, ::stdnet::buffer(buffer)))
        {
            std::string_view message(+buffer, size);
            std::cout << "received<" << size << ">(" << message << ")\n";
            if (message.starts_with("exit"))
            {
                std::cout << "exiting\n";
                scope.get_stop_source().request_stop();
            }
            auto ssize = co_await stdnet::async_send(client, ::stdnet::mutable_buffer(buffer, size));
            std::cout << "sent<ssize>(" << ::std::string_view(buffer, ssize) << ")\n";
        }
        std::cout << "client done\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

int main()
{
    std::cout << std::unitbuf;
    std::cout << "example server\n";
    try
    {
        exec::async_scope         scope;
        stdnet::io_context        context;
        stdnet::ip::tcp::endpoint endpoint(stdnet::ip::address_v4::any(), 12345);
        stdnet::ip::tcp::acceptor acceptor(context, endpoint);

        //tag_invoke(::stdnet::async_accept, acceptor);
        auto s = stdnet::async_accept(acceptor);
        auto s1 = stdnet::async_accept(::stdexec::just(), acceptor);
        //auto s2 = stdexec::just() | stdnet::async_accept(acceptor);
        static_assert(::stdexec::sender<decltype(s)>);
        static_assert(::stdexec::sender<decltype(s1)>);

        std::cout << "spawning accept\n";
        scope.spawn(std::invoke([](auto& scope, auto& acceptor)->exec::task<void>{
            while (true)
            {
                auto[stream, endpoint] = co_await stdnet::async_accept(acceptor);
                scope.spawn(
                    make_client(scope, std::move(stream))
                    | stdexec::upon_stopped([]{ std::cout << "client cancelled\n"; })
                    );
                std::cout << "accepted a client\n";
            }
        }, scope, acceptor)
        | stdexec::upon_stopped([]{ std::cout << "acceptor cancelled\n"; })
        );

        scope.spawn(std::invoke([](auto scheduler)->exec::task<void>{
            using namespace std::chrono_literals;
            for (int i{}; i < 100; ++i)
            {
                co_await stdnet::async_resume_after(scheduler, 1'000'000us);
                std::cout << "relative timer fired\n";
                co_await stdnet::async_resume_at(scheduler, ::std::chrono::system_clock::now() + 1s);
                std::cout << "absolute timer fired\n";
            }
        }, context.get_scheduler()));

        std::cout << "running context\n";
        context.run();
        std::cout << "running done\n";
    }
    catch (std::exception const& ex)
    {
        std::cout << "EXCEPTION: " << ex.what() << "\n";
        abort();
    }
}
