# src/browser/CMakeLists.txt
set(BROWSER_SRCS
  browser_app.cpp
  browser_window.cpp
  download_handler.cpp
  update_checker.cpp
  main_win.cpp
  kingfn.rc
)

ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

add_executable(kingfn_browser WIN32 ${BROWSER_SRCS})

SET_EXECUTABLE_TARGET_PROPERTIES(kingfn_browser)

target_include_directories(kingfn_browser PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${CEF_ROOT}"
)

target_link_libraries(kingfn_browser PRIVATE
  kingfn_core
  libcef_lib
  libcef_dll_wrapper
  ${CEF_STANDARD_LIBS}
  winhttp
)

# Copy CEF runtime files next to the executable.
COPY_FILES(kingfn_browser "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(kingfn_browser "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# Copy our runtime resources.
add_custom_command(TARGET kingfn_browser POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:kingfn_browser>/resources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/config"
    "$<TARGET_FILE_DIR:kingfn_browser>/config"
  COMMENT "Copying KINGFN resources and config..."
)
