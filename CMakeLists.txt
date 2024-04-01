#  Copyright (c) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
# 
#  Licensed under the Apache License Version 2.0 with LLVM Exceptions
#  (the "License"); you may not use this file except in compliance with
#  the License. You may obtain a copy of the License at
# 
#    https://llvm.org/LICENSE.txt
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

cmake_minimum_required(VERSION 3.22.1)
project(stdnet)

# --- Make Catch2 available ----------------------------------------------------
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)
include(Catch)

# --- Make stdexec available ---------------------------------------------------
set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(stdexec)
include_directories(stdexec/include)

# --- Make libevent available --------------------------------------------------

add_subdirectory(libevent)

# --- stdnet -------------------------------------------------------------------
include_directories(include)
add_compile_options(-Wno-deprecated-declarations -Wno-dangling-field)
set(CMAKE_CXX_STANDARD 23)

list(APPEND stdnet_examples
    client
    simple-http-server
    overview
    server
)
foreach(example ${stdnet_examples})
    add_executable(${example} examples/${example}.cpp)
    target_link_libraries(${example} STDEXEC::stdexec event_core_shared)
endforeach()

list (APPEND stdnet_tests
    buffer
)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    include(CTest)
    list(TRANSFORM stdnet_tests PREPEND "test/stdnet/")
    list(TRANSFORM stdnet_tests APPEND ".cpp")

    add_executable(test_stdnet ${stdnet_tests})
    set_target_properties(test_stdnet PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_link_libraries(test_stdnet
        STDEXEC::stdexec
        event_core_shared
        Catch2::Catch2WithMain
    )
    catch_discover_tests(test_stdnet)
endif()
