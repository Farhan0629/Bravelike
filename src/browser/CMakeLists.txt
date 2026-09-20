# CEF is loaded by the root CMakeLists after CEF_ROOT is supplied.
add_executable(bravelike_browser WIN32
  main_win.cpp
)
target_link_libraries(bravelike_browser PRIVATE bravelike_core libcef_lib libcef_dll_wrapper)
target_include_directories(bravelike_browser PRIVATE "${CEF_ROOT}")
COPY_FILES(bravelike_browser "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
COPY_FILES(bravelike_browser "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
