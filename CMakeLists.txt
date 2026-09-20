cmake_minimum_required(VERSION 3.24)
project(Bravelike VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BRAVELIKE_BUILD_TESTS "Build unit tests" ON)
option(BRAVELIKE_ENABLE_CEF "Build the CEF desktop shell" OFF)

add_library(bravelike_core
    src/core/filter_engine.cpp
    src/core/url_utils.cpp
    src/core/privacy_stats.cpp
)
target_include_directories(bravelike_core PUBLIC src)
target_compile_features(bravelike_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(bravelike_core PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(bravelike_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bravelike_filter_demo src/app/filter_demo.cpp)
target_link_libraries(bravelike_filter_demo PRIVATE bravelike_core)

if(BRAVELIKE_BUILD_TESTS)
  enable_testing()
  add_executable(bravelike_tests tests/filter_engine_tests.cpp)
  target_link_libraries(bravelike_tests PRIVATE bravelike_core)
  add_test(NAME bravelike_tests COMMAND bravelike_tests)
endif()

if(BRAVELIKE_ENABLE_CEF)
  if(NOT WIN32)
    message(FATAL_ERROR "The first CEF shell milestone currently targets Windows only")
  endif()
  if(NOT DEFINED CEF_ROOT)
    message(FATAL_ERROR "Set CEF_ROOT to an extracted CEF binary distribution")
  endif()
  list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)
  add_subdirectory(src/browser)
endif()
