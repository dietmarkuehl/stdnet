#  CMakeLists.txt                                                   -*-cmake-*-
#  ----------------------------------------------------------------------------
#   Copyright (C) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
#  ----------------------------------------------------------------------------

set(stdnet_tests
    net/test_intrusive_list.cpp
    )

add_executable(test.stdnet ${stdnet_tests})

#target_include_directories(test.stdnet PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(test.stdnet PUBLIC Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(test.stdnet)