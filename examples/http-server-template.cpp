// http-server-template.cpp                                           -*-C++-*-
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
#include <stdnet/internet.hpp>
#include <stdnet/socket.hpp>
#include <stdnet/timer.hpp>

#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using namespace std::string_view_literals;

// ----------------------------------------------------------------------------
// stdnet::ip::tcp::acceptor
// stdnet::ip::tcp::endpoint
// 200 OK
// 301 Moved Permanently (Location: URL)
// 400 Bad Request
// 404 Not Found
// 500 Internal Server Error
// 501 Not Implemented

int main()
{
    std::cout << "hello, world\n";
}