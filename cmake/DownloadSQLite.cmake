# DownloadSQLite.cmake
# Downloads the SQLite amalgamation and exposes it as a CMake library target.
# Usage: include(DownloadSQLite) then link against sqlite3_amalgamation.

function(DownloadSQLite)
  include(FetchContent)
  FetchContent_Declare(
    sqlite3_src
    URL      https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
    URL_HASH SHA256=2aae070960a021f5a37e7cc36a0a9d4f753de5c21c18e81e24d7cb26f8fdbd63
    DOWNLOAD_NO_EXTRACT FALSE
  )
  FetchContent_MakeAvailable(sqlite3_src)

  add_library(sqlite3_amalgamation STATIC
    ${sqlite3_src_SOURCE_DIR}/sqlite3.c
  )
  target_include_directories(sqlite3_amalgamation PUBLIC
    ${sqlite3_src_SOURCE_DIR}
  )
  # Use the recommended compile-time options for performance and safety.
  target_compile_definitions(sqlite3_amalgamation PUBLIC
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_JSON1
    SQLITE_DQS=0
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_OMIT_DEPRECATED
  )
  if(MSVC)
    target_compile_options(sqlite3_amalgamation PRIVATE /W0)
  else()
    target_compile_options(sqlite3_amalgamation PRIVATE -w)
  endif()
  # Match the MSVC runtime to kingfn_core when building with CEF.
  set_property(TARGET sqlite3_amalgamation PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endfunction()
