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
        class io_base
            : public ::stdnet::_Io_base
        {
        private:
            bool& flag;

        protected:
            auto _Do_complete() -> void override { this->flag = true; }

        public:
            io_base(bool& flag)
                : ::stdnet::_Io_base(::stdnet::_Opcode::_Nop, nullptr, {}, 0u, 0u)
                , flag(flag)
            {
            }
        };

        class context
            : public ::stdnet::_Context_base
        {
        private:
            ::stdnet::_Io_base*& op;
        
        protected:
            auto _Do_submit(::stdnet::_Io_base& op) -> void override
            {
                this->op = &op;
                this->op->_Complete();
            }
            
            auto _Do_run_one() -> ::std::size_t override
            {
                return 0u;
            }

        public:
            context(::stdnet::_Io_base*& op)
                : op(op)
            {
            }
        };
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("io_base abstract members", "[io_base]")
{
    using io_base = context_base::io_base;
    CHECK(not std::movable<io_base>);
    CHECK(not std::copyable<io_base>);

    bool    flag{};
    io_base obj(flag);
    CHECK(not flag);
    obj._Complete();
    CHECK(flag);
}

TEST_CASE("context_base abstract members", "[context_base]")
{
    using context = context_base::context;
    using io_base = context_base::io_base;
    CHECK(not std::movable<context>);
    CHECK(not std::copyable<context>);

    ::stdnet::_Io_base* op{nullptr};
    context ctxt(op);

    bool flag{};
    io_base obj(flag);

    CHECK(not flag);
    CHECK(op == nullptr);
    ctxt._Submit(obj);
    CHECK(flag);
    CHECK(op == &obj);
}