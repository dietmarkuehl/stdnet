#include <iostream>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <stdnet/internet.hpp>
#include <stdnet/io_context.hpp>
#include <stdnet/socket.hpp>

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

        std::cout << "accept spawned\n";
        scope.spawn(stdnet::async_accept(acceptor)
                    | stdexec::then([](auto&&...){ std::cout << "accepted\n"; })
                    | stdexec::upon_error([](auto&&...){})
                    );
        std::cout << "running context\n";
        context.run();
        std::cout << "running done\n";

#if 0
        ::sockaddr_storage addr{};
        ::socklen_t        len(sizeof(addr));
        if (::accept(acceptor.native_handle(), reinterpret_cast<::sockaddr*>(&addr), &len) < 0)
        {
            std::cout << "ERROR: " << ::std::strerror(errno) << "\n";
        }
#endif
    }
    catch (std::exception const& ex)
    {
        std::cout << "EXCEPTION: " << ex.what() << "\n";
    }
}
