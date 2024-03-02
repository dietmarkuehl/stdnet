// test/stdnet/buffer.cpp                                             -*-C++-*-
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

#include <stdnet/buffer.hpp>
#include <catch2/catch_all.hpp>
#include <concepts>

// ----------------------------------------------------------------------------

TEST_CASE("stream_errc", "[buffer.synop]")
{
    REQUIRE(::std::same_as<::stdnet::stream_errc, decltype(::stdnet::stream_errc::eof)>);
    REQUIRE(::std::same_as<::stdnet::stream_errc, decltype(::stdnet::stream_errc::not_found)>);
    REQUIRE(::stdnet::stream_errc::eof != ::stdnet::stream_errc::not_found);
}

TEST_CASE("stream_category", "[buffer.synop]")
{
    auto&& cat = ::stdnet::stream_category();
    REQUIRE(::std::same_as<::std::error_category const&, decltype((cat))>);
    REQUIRE(noexcept(::stdnet::stream_category()));
}

TEST_CASE("make_error_code", "[buffer.synop]")
{
    REQUIRE(::std::same_as<::std::error_code,
            decltype(::stdnet::make_error_code(::stdnet::stream_errc::eof))>);
    REQUIRE(noexcept(::stdnet::make_error_code(::stdnet::stream_errc::eof)));
}

TEST_CASE("make_error_condition", "[buffer.synop]")
{
    REQUIRE(::std::same_as<::std::error_condition,
            decltype(::stdnet::make_error_condition(::stdnet::stream_errc::eof))>);
    REQUIRE(noexcept(::stdnet::make_error_condition(::stdnet::stream_errc::eof)));
}

TEST_CASE("buffer types", "[buffer.synop]")
{
    using mutable_buffer = ::stdnet::mutable_buffer;
    using const_buffer = ::stdnet::mutable_buffer;
}

TEST_CASE("buffer traits", "[buffer.synop]")
{
    using is_mutable_buffer_sequence = ::stdnet::is_mutable_buffer_sequence<int>;
    using is_const_buffer_sequence = ::stdnet::is_const_buffer_sequence<int>;
    using is_dynamic_buffer = ::stdnet::is_dynamic_buffer<int>;
}