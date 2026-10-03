cmake_minimum_required(VERSION 3.24)
project(KINGFN VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

option(KINGFN_BUILD_TESTS "Build unit tests" ON)
option(KINGFN_ENABLE_CEF "Build the CEF desktop browser" OFF)

if(DEFINED BRAVELIKE_ENABLE_CEF AND NOT DEFINED KINGFN_ENABLE_CEF)
  set(KINGFN_ENABLE_CEF ${BRAVELIKE_ENABLE_CEF})
endif()
if(DEFINED BRAVELIKE_BUILD_TESTS AND NOT DEFINED KINGFN_BUILD_TESTS)
  set(KINGFN_BUILD_TESTS ${BRAVELIKE_BUILD_TESTS})
endif()

add_library(kingfn_core
    src/core/filter_engine.cpp
    src/core/navigation.cpp
    src/core/site_shields.cpp
    src/core/url_utils.cpp
    src/core/privacy_stats.cpp
)
target_include_directories(kingfn_core PUBLIC src)
target_compile_features(kingfn_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(kingfn_core PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(kingfn_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(kingfn_filter_demo src/app/filter_demo.cpp)
target_link_libraries(kingfn_filter_demo PRIVATE kingfn_core)

if(KINGFN_BUILD_TESTS)
  enable_testing()
  add_executable(kingfn_tests tests/filter_engine_tests.cpp)
  target_link_libraries(kingfn_tests PRIVATE kingfn_core)
  add_test(NAME kingfn_tests COMMAND kingfn_tests)
  add_executable(kingfn_site_shields_tests tests/site_shields_tests.cpp)
  target_link_libraries(kingfn_site_shields_tests PRIVATE kingfn_core)
  add_test(NAME kingfn_site_shields_tests COMMAND kingfn_site_shields_tests)
endif()

if(KINGFN_ENABLE_CEF)
  if(NOT WIN32)
    message(FATAL_ERROR "The desktop browser milestone currently targets Windows only")
  endif()

  set(CEF_VERSION "152.0.7+g83ffcba+chromium-152.0.7977.83" CACHE STRING "CEF binary version")
  set(CEF_CHANNEL "" CACHE STRING "CEF release channel")
  set(CEF_PLATFORM "windows64")
  set(CEF_DISTRIBUTION_TYPE "minimal")

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(DownloadCEF)
  DownloadCEF("${CEF_PLATFORM}" "${CEF_CHANNEL}" "${CEF_VERSION}" "${CEF_DISTRIBUTION_TYPE}" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cef")

  list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)

  # Match the static MSVC runtime used by CEF's Windows wrapper.
  set_property(TARGET kingfn_core PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

  add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
  add_subdirectory(src/browser)
  PRINT_CEF_CONFIG()
endif()
