// test/net/test_intrusive_list.cpp                                   -*-C++-*-
// ----------------------------------------------------------------------------
//  Copyright (C) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify,
//  merge, publish, distribute, sublicense, and/or sell copies of
//  the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include <stdnet/context_base.hpp>
#include <catch2/catch_test_macros.hpp>
#include <concepts>

// ----------------------------------------------------------------------------

namespace context_base
{
    namespace
    {
        // --------------------------------------------------------------------

        class io_base
            : public ::stdnet::_Io_operation
        {
        private:
            bool& flag;

        protected:
            auto _Do_complete() -> void override { this->flag = true; }

        public:
            io_base(bool& flag)
                : ::stdnet::_Io_operation(::stdnet::_Opcode::_Nop, nullptr, {}, 0u, 0u)
                , flag(flag)
            {
            }
        };

        // --------------------------------------------------------------------

        class context
            : public ::stdnet::_Context_base
        {
        private:
            ::stdnet::_Intrusive_list<::stdnet::_Io_operation> outstanding;
        
        protected:
            auto _Do_submit(::stdnet::_Io_operation& op) -> void override
            {
                this->outstanding.push_back(op);
            }
            auto _Do_try_or_submit(::stdnet::_Io_operation& op) -> void override
            {
                op._Complete();
            }
            
            auto _Do_run_one() -> ::std::size_t override
            {
                if (this->outstanding.empty())
                    return 0u;

                auto& op{this->outstanding.front()};
                this->outstanding.pop_front();
                op._Complete();
                return 1u;
            }
            auto _Do_run() -> ::std::size_t override
            {
                std::size_t rc{};
                while (not this->outstanding.empty() && rc < 2u)
                {
                    ++rc;
                    auto& op{this->outstanding.front()};
                    this->outstanding.pop_front();
                    op._Complete();
                }
                return rc;
            }
        };

        // --------------------------------------------------------------------

        class context_default
            : public ::stdnet::_Context_base
        {
        private:
            ::stdnet::_Intrusive_list<::stdnet::_Io_operation> outstanding;
        
        protected:
            auto _Do_submit(::stdnet::_Io_operation& op) -> void override
            {
                this->outstanding.push_back(op);
            }
            
            auto _Do_run_one() -> ::std::size_t override
            {
                if (this->outstanding.empty())
                    return 0u;

                auto& op{this->outstanding.front()};
                this->outstanding.pop_front();
                op._Complete();
                return 1u;
            }
        };
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("io_base structors", "[io_base]")
{
    CHECK(not std::movable<context_base::io_base>);
    CHECK(not std::copyable<context_base::io_base>);
}

TEST_CASE("io_base abstract members", "[io_base]")
{
    bool    flag{};
    context_base::io_base obj(flag);
    CHECK(not flag);

    obj._Complete();
    CHECK(flag);
}

// ----------------------------------------------------------------------------

TEST_CASE("context_base structors", "[context_base]")
{
    CHECK(not std::movable<context_base::context>);
    CHECK(not std::copyable<context_base::context>);
}

TEST_CASE("context_base _Submit", "[context_base]")
{
    context_base::context ctxt;

    bool flag{};
    context_base::io_base obj(flag);

    CHECK(not flag);
    ctxt._Submit(obj);
    CHECK(not flag);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 1u);
    CHECK(flag);
}

TEST_CASE("context_base _Try_or_submit", "[context_base]")
{
    context_base::context ctxt;

    bool flag{};
    context_base::io_base obj(flag);

    CHECK(not flag);
    ctxt._Try_or_submit(obj);
    CHECK(flag);
    auto rc{ctxt._Run_one()};
    CHECK(rc == 0u);
}

TEST_CASE("context_base _Run_one", "[context_base]")
{
    context_base::context ctxt;

    bool flag{};
    context_base::io_base obj(flag);
    {
        auto rc{ctxt._Run_one()};
        CHECK(rc == 0u);
        CHECK(not flag);
    }

    CHECK(not flag);
    ctxt._Submit(obj);
    CHECK(not flag);
    {
        auto rc{ctxt._Run_one()};
        CHECK(rc == 1u);
        CHECK(flag);
    }
}

TEST_CASE("context_base _Run", "[context_base]")
{
    context_base::context ctxt;

    bool flag[3]{};
    context_base::io_base obj[]{ {flag[0]}, {flag[1]}, {flag[2]} };
    {
        auto rc{ctxt._Run()};
        CHECK(rc == 0u);
        CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    }

    CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    for (auto& o: obj) ctxt._Submit(o);
    CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    {
        auto rc{ctxt._Run()};
        CHECK(rc == 2u);
        CHECK(std::all_of(std::begin(flag), std::begin(flag) + 2, [](auto b){ return b; }));
        CHECK(not flag[2]);
    }
}

TEST_CASE("context_base _Try_or_submit (default)", "[context_base]")
{
    context_base::context_default ctxt;

    bool flag{};
    context_base::io_base obj(flag);

    CHECK(not flag);
    ctxt._Try_or_submit(obj);
    CHECK(not flag);
    auto rc{ctxt._Run_one()};
    CHECK(flag);
    CHECK(rc == 1u);
}

TEST_CASE("context_base _Run (default)", "[context_base]")
{
    context_base::context_default ctxt;

    bool flag[3]{};
    context_base::io_base obj[]{ {flag[0]}, {flag[1]}, {flag[2]} };
    {
        auto rc{ctxt._Run()};
        CHECK(rc == 0u);
        CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    }

    CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    for (auto& o: obj) ctxt._Submit(o);
    CHECK(std::none_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    {
        auto rc{ctxt._Run()};
        CHECK(rc == 3u);
        CHECK(std::all_of(std::begin(flag), std::end(flag), [](auto b){ return b; }));
    }
}
