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
add_library(sj_json src/json/scalar.cpp src/json/string.cpp src/json/value.cpp
                    src/json/escape.cpp src/json/detail.cpp)

target_include_directories(sj_args PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(sj_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(sj src/main.cpp)
target_link_libraries(sj PRIVATE sj_args sj_json)

add_executable(test_args tests/test_args.cpp)
target_link_libraries(test_args PRIVATE sj_args Catch2::Catch2WithMain)

foreach(t test_scalar test_string test_value test_escape)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE sj_json Catch2::Catch2WithMain)
endforeach()

option(SIEVE_NATIVE "Build with -march=native (fastest locally, not portable)" OFF)

# Единый профиль оптимизации: -O3 + LTO на всех наших целях,
# чтобы инлайн шел через границы модулей (sj ↔ sj_json ↔ sj_args).
foreach(t sj sj_args sj_json test_args test_scalar test_string test_value test_escape)
  target_compile_options(${t} PRIVATE -O3 -Wall -Wextra)
  set_target_properties(${t} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  if(SIEVE_NATIVE)
    target_compile_options(${t} PRIVATE -march=native -mtune=native)
  endif()
endforeach()

enable_testing()
add_test(NAME cli COMMAND bash ${CMAKE_SOURCE_DIR}/tests/cli.sh $<TARGET_FILE:sj>)
add_test(NAME args COMMAND test_args)
add_test(NAME scalar COMMAND test_scalar)
add_test(NAME string COMMAND test_string)
add_test(NAME value COMMAND test_value)
add_test(NAME escape COMMAND test_escape)
