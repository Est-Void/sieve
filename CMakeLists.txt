cmake_minimum_required(VERSION 3.20)
project(sieve VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(Catch2)

add_library(sj_args src/args.cpp)
target_include_directories(sj_args PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(sj src/main.cpp)
target_link_libraries(sj PRIVATE sj_args)

target_compile_options(sj PRIVATE -O3 -Wall -Wextra)
set_target_properties(sj PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

option(SIEVE_NATIVE "Build with -march=native (fastest locally, not portable)" OFF)
if(SIEVE_NATIVE)
  target_compile_options(sj PRIVATE -march=native -mtune=native)
endif()

add_executable(test_args tests/test_args.cpp)
target_link_libraries(test_args PRIVATE sj_args Catch2::Catch2WithMain)

enable_testing()
add_test(NAME cli COMMAND bash ${CMAKE_SOURCE_DIR}/tests/cli.sh $<TARGET_FILE:sj>)
add_test(NAME args COMMAND test_args)
