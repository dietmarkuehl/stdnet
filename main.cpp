#include <iostream>
#include <string_view>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <stdnet/buffer.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>

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

auto make_client(auto client) -> exec::task<void>
{
    try
    {
        char buffer[8];
        while (auto size = co_await stdnet::async_receive(client, ::stdnet::mutable_buffer(buffer)))
        {
            std::cout << "received<" << size << ">(" << std::string_view(+buffer, size) << ")\n";
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
    try
    {
        exec::async_scope         scope;
        stdnet::io_context        context;
        stdnet::ip::tcp::endpoint endpoint(stdnet::ip::address_v4::any(), 12345);
        stdnet::ip::tcp::acceptor acceptor(context, endpoint);

        tag_invoke(::stdnet::async_accept, acceptor);
        auto s = stdnet::async_accept(acceptor);
        auto s1 = stdnet::async_accept(::stdexec::just(), acceptor);
        //auto s2 = stdexec::just() | stdnet::async_accept(acceptor);
        static_assert(::stdexec::sender<decltype(s)>);
        static_assert(::stdexec::sender<decltype(s1)>);

        std::cout << "spawning accept\n";
        scope.spawn(stdnet::async_accept(acceptor)
                    | stdexec::then([&scope](auto client){
                        std::cout << "accepted\n";
                        scope.spawn(make_client(::std::move(client)));
                        })
                    | stdexec::upon_error(error_handler<std::error_code, std::exception_ptr>())
                    );
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
