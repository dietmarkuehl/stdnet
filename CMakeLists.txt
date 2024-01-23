cmake_minimum_required(VERSION 3.22.1)
project(stdnet)

# --- Make Catch2 available ----------------------------------------------------
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

# --- Make stdexec available ---------------------------------------------------
set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(stdexec)

# --- Make libevent available --------------------------------------------------

add_subdirectory(libevent)

# --- stdnet -------------------------------------------------------------------
include_directories(include)
add_compile_options(-Wno-deprecated-declarations -Wno-dangling-field)
set(CMAKE_CXX_STANDARD 20)

#add_executable(main main.cpp)
#target_link_libraries(main STDEXEC::stdexec)
add_executable(overview overview.cpp)
target_link_libraries(overview STDEXEC::stdexec)
target_link_libraries(overview event_core_shared)
