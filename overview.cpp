#include <iostream>
#include <functional>
#include <optional>
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
    struct cancel_callback
    {
        timer_op* self;
        void operator()() const noexcept
        {
            if (0 == event_del(&self->ev))
                stdexec::set_stopped(std::move(self->receiver));
        }
    };
    using env_t = decltype(stdexec::get_env(std::declval<R>()));
    using stop_token_t = decltype(stdexec::get_stop_token(std::declval<env_t>()));
    using callback_t = typename stop_token_t::template callback_type<cancel_callback>;

    R                         receiver;
    event_base*               eb;
    timeval                   time;
    std::function<void()>     trampoline;
    event                     ev;
    std::optional<callback_t> cancel;

    friend void tag_invoke(stdexec::start_t, timer_op& self) noexcept
    {
        self.trampoline = [&self]{
            self.cancel.reset();
            stdexec::set_value(std::move(self.receiver));
        };
        event_assign(&self.ev, self.eb, 0, EV_TIMEOUT, callback, &self.trampoline);
        self.cancel.emplace(stdexec::get_stop_token(stdexec::get_env(self.receiver)),
                            cancel_callback{&self});
        event_add(&self.ev, &self.time);
    }
};

struct timer_sender
{
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(),
        stdexec::set_stopped_t()
    >;

    event_base* eb;
    timeval     time;

    template <stdexec::receiver R>
    friend timer_op<std::remove_cvref_t<R>>
    tag_invoke(stdexec::connect_t, timer_sender&& self, R&& r)
    {
        return { std::forward<R>(r), self.eb, self.time };
    }
};

// ----------------------------------------------------------------------------

struct libevent_context
{
    struct receiver
    {
        using is_receiver = void;
        bool& done;
        friend void tag_invoke(stdexec::set_value_t, receiver const& self, auto&&...) { self.done = true; }
        friend void tag_invoke(stdexec::set_error_t, receiver const& self, auto&&) noexcept { self.done = true; }
        friend void tag_invoke(stdexec::set_value_t, receiver const& self) noexcept { self.done = true; }
    };

    bool        done{false};
    event_base* eb;
    libevent_context(): eb(event_base_new()) {}
    ~libevent_context() { event_base_free(eb); }

    template <stdexec::sender S>
    void run_loop(S&& s)
    {
        auto state = stdexec::connect(std::forward<S>(s), receiver{done});
        stdexec::start(state);

        while (!done)
        {
            event_base_loop(eb, EVLOOP_ONCE);
        }
    }
};

int main(int ac, char* av[])
{
    libevent_context  context;
    exec::async_scope scope;

    char buffer[64];
    scope.spawn(
        read_sender(context.eb, 0, buffer, sizeof(buffer))
        | stdexec::then([buffer = +buffer, &scope](int n){
            std::cout << "read='" << std::string_view(buffer, n) << "'\n";
            scope.get_stop_source().request_stop();
        })
    );
    scope.spawn(
        std::invoke([](event_base* eb, int timeout)->exec::task<void>{
            while (true)
            {
                co_await timer_sender(eb, {timeout, 0});
                std::cout << "time passed\n";
            }
        }, context.eb, ac == 2? std::stoi(av[1]): 1)
    );

    std::cout << "starting loop\n";
    context.run_loop(scope.when_empty(stdexec::just()));
    std::cout << "loop done\n";
}