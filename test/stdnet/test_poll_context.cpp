// stdnet/test_poll_context.cpp                                       -*-C++-*-
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

#include <stdnet/poll_context.hpp>
#include <unordered_map>
#include <catch2/catch_test_macros.hpp>

// ----------------------------------------------------------------------------

namespace
{
    namespace poll_context
    {
        using expected = std::unordered_map<void*, ::stdnet::_Result>;
        struct operation
            : ::stdnet::_Io_base
        {
            expected& ex;
            operation(expected& ex, ::stdnet::_Opcode op, void* ptr = nullptr)
                : ::stdnet::_Io_base(op, ptr)
                , ex(ex)
            {
            }
            auto _Do_complete() -> void override
            {
                auto it{ex.find(this)};
                if (it != ex.end() && it->second == this->_Result())
                    ex.erase(it);
            }
        };
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("poll_context creation", "[poll_context]")
{
    CHECK(not std::movable<::stdnet::_Poll_context>);
    CHECK(not std::copyable<::stdnet::_Poll_context>);

    ::stdnet::_Poll_context ctxt;
    CHECK(ctxt.empty());
}

TEST_CASE("poll_context cancel non-existing", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;
    ::poll_context::operation cancel{ex, ::stdnet::_Opcode::_Cancel};
    ex[&cancel] = ::stdnet::_Result::_Not_found;

    ctxt._Submit(cancel);
    auto rc = ctxt._Run_one();
    CHECK(rc == 1u);
    CHECK(ex.empty());
}

TEST_CASE("poll_context cancel existing", "[poll_context]")
{
    ::stdnet::_Poll_context   ctxt;
    ::poll_context::expected  ex;
    
    ::poll_context::operation receive{ex, ::stdnet::_Opcode::_Receive};
    ::poll_context::operation cancel{ex, ::stdnet::_Opcode::_Cancel, &receive};

    ex[&receive] = ::stdnet::_Result::_Cancelled;
    ex[&cancel] = ::stdnet::_Result::_Success;

    ctxt._Submit(receive);
    ctxt._Submit(cancel);
    auto rc = ctxt._Run_one() + ctxt._Run_one();
    CHECK(rc == 2u);
    CHECK(ex.empty());
}