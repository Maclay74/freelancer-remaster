add_library(zlib SHARED
  "vendor/adler32.c"
  "vendor/compress.c"
  "vendor/crc32.c"
  "vendor/deflate.c"
  "vendor/gzclose.c"
  "vendor/gzlib.c"
  "vendor/gzread.c"
  "vendor/gzwrite.c"
  "vendor/inffast.c"
  "vendor/inflate.c"
  "vendor/inftrees.c"
  "vendor/trees.c"
  "vendor/uncompr.c"
  "vendor/zutil.c"
  "vendor/minizip/ioapi.c"
  "vendor/minizip/unzip.c"
  "vendor/minizip/zip.c"
  "zlib_version_compat.c"
  "zlib.rc"
  "zlib.def"
)
target_link_libraries(zlib PRIVATE fl_build_options)
target_include_directories(zlib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/zlib/vendor" "${PROJECT_SOURCE_DIR}/src/zlib/vendor/minizip")
target_compile_definitions(zlib PRIVATE ZLIB_WINAPI _CRT_NONSTDC_NO_DEPRECATE)
