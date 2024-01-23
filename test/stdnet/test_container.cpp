// test/net/test_container.cpp                                   -*-C++-*-
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

#include <stdnet/container.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <concepts>
#include <new>

// ----------------------------------------------------------------------------

namespace intrusive_list
{
    namespace
    {
        struct node
            : stdnet::_Intrusive_node<node>
        {
        };
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("intrusive_node isn't movable or copyable", "[intrusive_node]")
{
    using node_t = intrusive_list::node;
    CHECK(not std::movable<node_t>);
    CHECK(not std::copyable<node_t>);
}

TEST_CASE("intrusive_node is initialized", "[intrusive_node]")
{
    using node_t = intrusive_list::node;
    char buffer[2 * sizeof(node_t)];
    std::fill(std::begin(buffer), std::end(buffer), '\xff');
    auto* node = new(buffer) node_t;

    CHECK(node != nullptr);
    CHECK(node->_D_next == nullptr);
    node->~node_t();
}

// ----------------------------------------------------------------------------

TEST_CASE("intrusive_list is movable but not copyable", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;

    CHECK(std::movable<list_t>);
    CHECK(not std::copyable<list_t>);
}

TEST_CASE("intrusive_list is initialized", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;
    char buffer[2 * sizeof(list_t)];
    std::fill(std::begin(buffer), std::end(buffer), '\xff');
    auto* list = new(buffer) list_t;

    CHECK(list != nullptr);
    CHECK(list->empty());
}

TEST_CASE("intrusive_list push_back", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;

    list_t list;
    node_t nodes[3];
    CHECK(list.empty());

    for (node_t& node: nodes)
    {
        list.push_back(node);
        CHECK(not list.empty());
        CHECK(&nodes[0] == &list.front());
    }
}

TEST_CASE("intrusive_list pop_front", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;

    list_t list;
    node_t nodes[3];
    CHECK(list.empty());

    for (node_t& node: nodes)
    {
        list.push_back(node);
        CHECK(not list.empty());
        CHECK(&nodes[0] == &list.front());
    }

    for (node_t& node: nodes)
    {
        CHECK(&node == &list.front());
        list.pop_front();
    }
    CHECK(list.empty());
}

TEST_CASE("intrusive_list member swap", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;

    list_t list1;
    list_t list2;
    node_t nodes[3];
    node_t extra;
    CHECK(list1.empty());
    CHECK(list2.empty());

    for (node_t& node: nodes)
    {
        list1.push_back(node);
        CHECK(not list1.empty());
        CHECK(&nodes[0] == &list1.front());
    }
    list2.push_back(extra);
    CHECK(not list2.empty());
    CHECK(&extra == &list2.front());

    list1.swap(list2);

    CHECK(not list1.empty());
    CHECK(&extra == &list1.front());
    list1.pop_front();
    CHECK(list1.empty());

    list2.push_back(extra);
    for (node_t& node: nodes)
    {
        CHECK(&node == &list2.front());
        list2.pop_front();
    }
    CHECK(&extra == &list2.front());
}

TEST_CASE("intrusive_list non-member swap", "[intrusive_list]")
{
    using node_t = intrusive_list::node;
    using list_t = stdnet::_Intrusive_list<node_t>;

    list_t list1;
    list_t list2;
    node_t nodes[3];
    node_t extra;
    CHECK(list1.empty());
    CHECK(list2.empty());

    for (node_t& node: nodes)
    {
        list1.push_back(node);
        CHECK(not list1.empty());
        CHECK(&nodes[0] == &list1.front());
    }
    list2.push_back(extra);
    CHECK(not list2.empty());
    CHECK(&extra == &list2.front());

    std::swap(list1, list2);

    CHECK(not list1.empty());
    CHECK(&extra == &list1.front());
    list1.pop_front();
    CHECK(list1.empty());

    list2.push_back(extra);
    for (node_t& node: nodes)
    {
        CHECK(&node == &list2.front());
        list2.pop_front();
    }
    CHECK(&extra == &list2.front());
}

// ----------------------------------------------------------------------------

TEST_CASE("intrusive_queue initialization", "[intrusive_queue]")
{
    int counter{};
    auto wakeup = [counter=&counter]{ ++*counter; };

    using node_t   = intrusive_list::node;
    using queue_t  = stdnet::_Intrusive_queue<node_t, decltype(wakeup)>;

    queue_t queue(wakeup);
    auto    list = queue._Extract();
    CHECK(list.empty());
}

TEST_CASE("intrusive_queue::_Push", "[intrusive_queue]")
{
    int counter{};
    auto wakeup = [counter=&counter]{ ++*counter; };

    using node_t   = intrusive_list::node;
    using queue_t  = stdnet::_Intrusive_queue<node_t, decltype(wakeup)>;

    queue_t queue(wakeup);
    node_t  nodes[3];
    for (auto& n: nodes)
    {
        queue._Push(n);
    }
    CHECK(counter == 1);
    auto    list = queue._Extract();
    CHECK(not list.empty());

    for (int i{}; not list.empty(); ++i)
    {
        CHECK(&nodes[i] == &list.front());
        list.pop_front();
    }
}

TEST_CASE("remove_heap", "[heap]")
{
    int array[] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11 };
    ::std::make_heap(::std::begin(array), ::std::end(array));
    int size{::std::size(array)};

    for (int i{14}; 4 < --i; )
    {
        ::stdnet::remove_heap(::std::begin(array) + i, ::std::begin(array), ::std::begin(array) + size);
        --size;
        CHECK(::std::is_heap(::std::begin(array), ::std::begin(array) + size));
    }
    ::stdnet::remove_heap(::std::begin(array) + size - 1, ::std::begin(array), ::std::begin(array) + size);
    --size;
    CHECK(::std::is_heap(::std::begin(array), ::std::begin(array) + size));
}