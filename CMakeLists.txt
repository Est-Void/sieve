cmake_minimum_required(VERSION 3.20)
project(sieve VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# По умолчанию — Release: identity-pump должен мериться на -O3, а не на -O0.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(sj src/main.cpp)

# Скорость: -O3 + LTO. -march=native только опционально (ломает переносимость).
target_compile_options(sj PRIVATE -O3 -Wall -Wextra)
set_target_properties(sj PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

option(SIEVE_NATIVE "Build with -march=native (fastest locally, not portable)" OFF)
if(SIEVE_NATIVE)
  target_compile_options(sj PRIVATE -march=native -mtune=native)
endif()

enable_testing()
add_test(NAME cli COMMAND bash ${CMAKE_SOURCE_DIR}/tests/cli.sh $<TARGET_FILE:sj>)
