set(BRAVELIKE_BROWSER_SRCS
  browser_app.cpp
  browser_app.h
  browser_window.cpp
  browser_window.h
  main_win.cpp
)

set(BRAVELIKE_TARGET bravelike_browser)
if(USE_SANDBOX)
  add_library(${BRAVELIKE_TARGET} SHARED ${BRAVELIKE_BROWSER_SRCS})
  SET_LIBRARY_TARGET_PROPERTIES(${BRAVELIKE_TARGET})
else()
  add_executable(${BRAVELIKE_TARGET} WIN32 ${BRAVELIKE_BROWSER_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${BRAVELIKE_TARGET})
endif()

target_include_directories(${BRAVELIKE_TARGET} PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(${BRAVELIKE_TARGET} PRIVATE bravelike_core libcef_lib libcef_dll_wrapper)
add_dependencies(${BRAVELIKE_TARGET} libcef_dll_wrapper)

if(CMAKE_GENERATOR MATCHES "Ninja|Make")
  set(BRAVELIKE_OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
else()
  set(BRAVELIKE_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
endif()
set_target_properties(${BRAVELIKE_TARGET} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${BRAVELIKE_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${BRAVELIKE_OUT_DIR}")

COPY_FILES(${BRAVELIKE_TARGET} "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${BRAVELIKE_OUT_DIR}")
COPY_FILES(${BRAVELIKE_TARGET} "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${BRAVELIKE_OUT_DIR}")
add_custom_command(TARGET ${BRAVELIKE_TARGET} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${BRAVELIKE_OUT_DIR}/config"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/config/sample-blocklist.txt"
          "${BRAVELIKE_OUT_DIR}/config/blocklist.txt"
  VERBATIM)
