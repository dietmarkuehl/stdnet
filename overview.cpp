#include <iostream>
#include <functional>
#include <string_view>
#include <event.h>
#include <unistd.h>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <stdexec/concepts.hpp>
#include <stdexec/execution.hpp>

// --- deal with an extern C interface (naively) ------------------------------

extern "C" void callback(int, short, void* ptr)
{
    (*static_cast<std::function<void()>*>(ptr))();
}

// --- read sender ------------------------------------------------------------

template <stdexec::receiver R>
struct read_op
{
    std::remove_cvref_t<R> receiver;
    event_base*            eb;
    int                    fd;
    char*                  buffer;
    std::size_t            size;
    std::function<void()>  trampoline;
    event                  ev{};

    friend void tag_invoke(stdexec::start_t, read_op& self) noexcept
    {
        self.trampoline = [&self]{
            int n = read(self.fd, self.buffer, self.size);
            stdexec::set_value(std::move(self.receiver), n);
        };
        event_assign(&self.ev, self.eb, 0, EV_READ, callback, &self.trampoline);
        event_add(&self.ev, nullptr);
    }
};

struct read_sender
{
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(int)
    >;

    event_base* eb;
    int         fd;
    char*       buffer;
    std::size_t size;

    template <stdexec::receiver R>
    friend read_op<R> tag_invoke(stdexec::connect_t, read_sender&& self, R&& r)
    {
        return { std::forward<R>(r), self.eb, self.fd, self.buffer, self.size };
    }
};

// --- timer sender -----------------------------------------------------------

template <stdexec::receiver R>
struct timer_op
{
    std::remove_cvref_t<R> receiver;
    event_base*            eb;
    timeval                time;
    std::function<void()>  trampoline;
    event                  ev;

    friend void tag_invoke(stdexec::start_t, timer_op& self) noexcept
    {
        self.trampoline = [&self]{
            stdexec::set_value(std::move(self.receiver));
        };
        event_assign(&self.ev, self.eb, 0, EV_TIMEOUT, callback, &self.trampoline);
        event_add(&self.ev, &self.time);
    }
};

struct timer_sender
{
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t()
    >;

    event_base* eb;
    timeval     time;

    template <stdexec::receiver R>
    friend timer_op<R> tag_invoke(stdexec::connect_t, timer_sender&& self, R&& r)
    {
        return { std::forward<R>(r), self.eb, self.time };
    }
};

// ----------------------------------------------------------------------------

int main()
{
    std::cout << std::unitbuf;
    bool done(false);

    event_base* eb = event_base_new();
    exec::async_scope scope;

    char buffer[64];
    scope.spawn(
        read_sender(eb, 0, buffer, sizeof(buffer))
        | stdexec::then([buffer = +buffer, &done](int n){
            std::cout << "read='" << std::string_view(buffer, n) << "'\n";
        })
    );
    scope.spawn(
        std::invoke([](event_base* eb)->exec::task<void>{
            while (true)
            {
                co_await timer_sender(eb, {1, 0});
                std::cout << "time passed\n";
            }
        }, eb)
    );

    while (!done)
    {
        event_base_loop(eb, EVLOOP_ONCE);
    }

    event_base_free(eb);
}