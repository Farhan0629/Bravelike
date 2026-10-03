set(KINGFN_BROWSER_SRCS
  browser_app.cpp
  browser_app.h
  browser_window.cpp
  browser_window.h
  main_win.cpp
  kingfn.rc
)

set(KINGFN_TARGET kingfn_browser)
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

if(USE_SANDBOX)
  add_library(${KINGFN_TARGET} SHARED ${KINGFN_BROWSER_SRCS})
  SET_LIBRARY_TARGET_PROPERTIES(${KINGFN_TARGET})
else()
  add_executable(${KINGFN_TARGET} WIN32 ${KINGFN_BROWSER_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${KINGFN_TARGET})
endif()

target_include_directories(${KINGFN_TARGET} PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(${KINGFN_TARGET} PRIVATE
  kingfn_core
  libcef_lib
  libcef_dll_wrapper
  ${CEF_STANDARD_LIBS})
add_dependencies(${KINGFN_TARGET} libcef_dll_wrapper)

if(CMAKE_GENERATOR MATCHES "Ninja|Make")
  set(KINGFN_OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
else()
  set(KINGFN_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
endif()
set_target_properties(${KINGFN_TARGET} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${KINGFN_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${KINGFN_OUT_DIR}")

COPY_FILES(${KINGFN_TARGET} "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${KINGFN_OUT_DIR}")
COPY_FILES(${KINGFN_TARGET} "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${KINGFN_OUT_DIR}")
add_custom_command(TARGET ${KINGFN_TARGET} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${KINGFN_OUT_DIR}/config"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/config/sample-blocklist.txt"
          "${KINGFN_OUT_DIR}/config/blocklist.txt"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${KINGFN_OUT_DIR}/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/resources"
          "${KINGFN_OUT_DIR}/resources"
  VERBATIM)

