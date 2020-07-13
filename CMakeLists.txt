cmake_minimum_required(VERSION 3.1..3.18)

project(oggz VERSION 1.1.1 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 99)

include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CMakePackageConfigHelpers)
include(TestBigEndian)
include(TestLargeFiles)
include(CTest)
include(GNUInstallDirs)

find_package(Doxygen)
find_package(Ogg REQUIRED)

set(OGGZ_CONFIG_READ 1)
set(OGGZ_CONFIG_WRITE 1)

find_library(LIBM_FOUND m)
if(LIBM_FOUND)
  list(APPEND CMAKE_REQUIRED_LIBRAIRES m)
  check_library_exists(m ceil "" HAVE_LIBM)
endif()

check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(process.h HAVE_PROCESS_H)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(_stricmp HAVE__STRICMP)
check_function_exists(timezone HAVE_TIMEZONE)
check_symbol_exists(_timezone "time.h" HAVE__TIMEZONE)
test_big_endian(WORDS_BIGENDIAN)

# Large file support
# Adapted from: libsndfile by Erik de Castro Lopo
#
# Find an appropriate type for oggz_off_t
# On systems supporting files larger than 2 Gig, oggz_off_t must be a
# 64 bit value. Unfortunately there is more than one way of ensuring
# this so need to do some pretty rigourous testing here.

if(WIN32)
  set(TYPEOF_OGGZ_OFF_T __int64)
  set(OGGZ_OFF_MAX 0x7FFFFFFFFFFFFFFFLL)
  set(SIZEOF_OGGZ_OFF_T 8)
else()
  set(TYPEOF_OGGZ_OFF_T off_t)
  test_large_files(LFS64)
  if(LFS64)
    set(OGGZ_OFF_MAX 0x7FFFFFFFFFFFFFFFLL)
    set(SIZEOF_OGGZ_OFF_T 8)
  else()
    set(OGGZ_OFF_MAX 0x7FFFFFFF)
    set(SIZEOF_OGGZ_OFF_T 4)
  endif()
endif()
if(SIZEOF_OGGZ_OFF_T EQUAL 8)
  set(PRI_OGGZ_OFF_T "ll")
else()
  set(PRI_OGGZ_OFF_T "l")
endif()

set(PACKAGE "liboggz")
set(PACKAGE_BUGREPORT "ogg-dev@xiph.org")
set(PACKAGE_NAME "liboggz")
set(VERSION ${PROJECT_VERSION})
set(PACKAGE_VERSION ${VERSION})
set(PACKAGE_STRING "${PACKAGE} ${VERSION}")
set(PACKAGE_TARNAME "${PACKAGE}")
set(PACKAGE_URL "")
set(abs_top_srcdir ${CMAKE_CURRENT_SOURCE_DIR})
set(abs_top_builddir ${CMAKE_CURRENT_BINARY_DIR})
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
set(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})

configure_file(config.cmake.in config.h)
configure_file(include/oggz/oggz_off_t_generated.h.in include/oggz/oggz_off_t_generated.h)
if(DOXYGEN_FOUND)
  configure_file(doc/Doxyfile.in doc/Doxyfile)

  set(DOXYGEN_INPUT "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen.stamp
	DEPENDS ${DOXYGEN_INPUT}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
	COMMAND cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen.stamp
	COMMENT "Generating API documentation with Doxygen"
	VERBATIM)
  add_custom_target(
    doc ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen.stamp)
endif()

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif (MSVC)

set(oggzinclude_HEADERS
  include/oggz/oggz.h
  include/oggz/oggz_comments.h
  include/oggz/oggz_constants.h
  include/oggz/oggz_io.h
  include/oggz/oggz_read.h
  include/oggz/oggz_seek.h
  include/oggz/oggz_stream.h
  include/oggz/oggz_packet.h
  include/oggz/oggz_table.h
  include/oggz/oggz_write.h
  include/oggz/oggz_deprecated.h
  include/oggz/oggz_off_t.h
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/oggz/oggz_off_t_generated.h>)

set(liboggz_SOURCES
  src/liboggz/oggz.c
  src/liboggz/oggz_private.h
  src/liboggz/oggz_byteorder.h
  src/liboggz/oggz_compat.h
  src/liboggz/oggz_macros.h
  src/liboggz/oggz_comments.c
  src/liboggz/oggz_io.c
  src/liboggz/oggz_read.c
  src/liboggz/oggz_write.c
  src/liboggz/oggz_seek.c
  src/liboggz/oggz_auto.c
  src/liboggz/oggz_auto.h
  src/liboggz/oggz_stream.c
  src/liboggz/oggz_stream_private.h
  src/liboggz/oggz_table.c
  src/liboggz/oggz_vector.c
  src/liboggz/oggz_vector.h
  src/liboggz/oggz_dlist.c
  src/liboggz/oggz_dlist.h
  src/liboggz/metric_internal.c
  src/liboggz/dirac.c
  src/liboggz/dirac.h
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(oggz ${liboggz_SOURCES} ${oggzinclude_HEADERS})
target_include_directories(oggz
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
  )
target_link_libraries(oggz PUBLIC Ogg::ogg)

configure_file(src/liboggz/Version_script.in ${CMAKE_CURRENT_BINARY_DIR}/src/liboggz/Version_script @ONLY)
if(BUILD_SHARED_LIBS)
  if(WIN32)
    target_sources(oggz PRIVATE win32/liboggz.def)
  elseif(UNIX AND (NOT APPLE))
    set(VERSION_SCRIPT_FLAG "-Wl,--version-script,${CMAKE_CURRENT_BINARY_DIR}/src/liboggz/Version_script")
    set(VERSION_SCRIPT_FLAG_SUPPORTED OFF)
    if(NOT CMAKE_VERSION VERSION_LESS 3.18)
      include(CheckLinkerFlag)
      check_linker_flag(VERSION_SCRIPT_FLAG VERSION_SCRIPT_FLAG_SUPPORTED)
    else()
      # not implemented
    endif()
    if(VERSION_SCRIPT_FLAG_SUPPORTED)
      if(NOT CMAKE_VERSION VERSION_LESS 3.13)
        target_link_options(oggz PRIVATE ${VERSION_SCRIPT_FLAG})
      else()  
        set_property(TARGET oggz APPEND_STRING PROPERTY
          LINK_FLAGS ${VERSION_SCRIPT_FLAG})
      endif()
    endif()
  endif()
endif()
target_compile_definitions(oggz PRIVATE ${LARGE_FILES_DEFINITIONS})
set_target_properties (oggz PROPERTIES
  PUBLIC_HEADER "${oggzinclude_HEADERS}"
  SOVERSION 2.6.0
  VERSION 2)

add_library(Oggz::oggz ALIAS oggz)

set(COMMON_SRCS
  src/tools/oggz_tools.h
  src/tools/oggz_tools.c
  src/liboggz/dirac.h
  src/liboggz/dirac.c)

add_executable(oggz_app src/tools/oggz.c)
set_target_properties (oggz_app PROPERTIES
  OUTPUT_NAME "oggz")
target_include_directories(oggz_app
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

add_executable(oggz-known-codecs src/tools/oggz-known-codecs.c)
target_include_directories(oggz-known-codecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(oggz-known-codecs PRIVATE oggz)

set(oggz_info_SOURCES
  src/tools/oggz-info.c
  src/tools/skeleton.c
  ${COMMON_SRCS})
add_executable(oggz-info ${oggz_info_SOURCES})
target_include_directories(oggz-info PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(oggz-info PRIVATE oggz)
if(HAVE_LIBM)
  target_link_libraries(oggz-info PRIVATE m)
endif()
if(NOT HAVE_GET_OPT)
  target_sources(oggz-info PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-info
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_comment_SOURCES
  src/tools/oggz-comment.c
  ${COMMON_SRCS})
add_executable(oggz-comment ${oggz_comment_SOURCES})
target_include_directories(oggz-comment PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(oggz-comment PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-comment PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-comment
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_dump_SOURCES
  src/tools/oggz-dump.c
  ${COMMON_SRCS})
add_executable(oggz-dump ${oggz_dump_SOURCES})
target_link_libraries(oggz-dump PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-dump PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-dump
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_merge_SOURCES
  src/tools/oggz-merge.c
  ${COMMON_SRCS})
add_executable(oggz-merge ${oggz_merge_SOURCES})
target_link_libraries(oggz-merge PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-merge PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-merge
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_rip_SOURCES
  src/tools/oggz-rip.c
  ${COMMON_SRCS})
add_executable(oggz-rip ${oggz_rip_SOURCES})
target_link_libraries(oggz-rip PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-rip PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-rip
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_validate_SOURCES
  src/tools/oggz-validate.c
  ${COMMON_SRCS})
add_executable(oggz-validate ${oggz_validate_SOURCES})
target_link_libraries(oggz-validate PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-validate PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-validate
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
      win32
      src/liboggz)
endif()

set(oggz_basetime_SOURCES
  src/tools/oggz-basetime.c
  ${COMMON_SRCS})
add_executable(oggz-basetime ${oggz_basetime_SOURCES})
target_link_libraries(oggz-basetime PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-basetime PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-basetime
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_scan_SOURCES
  src/tools/oggz-scan.c
  ${COMMON_SRCS})
add_executable(oggz-scan ${oggz_scan_SOURCES})
target_link_libraries(oggz-scan PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-scan PRIVATE
    win32/getopt.h
    win32/attgetopt.c)
target_include_directories(oggz-scan
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/liboggz)
endif()

set(oggz_sort_SOURCES
  src/tools/oggz-sort.c
  ${COMMON_SRCS})
add_executable(oggz-sort ${oggz_sort_SOURCES})
target_link_libraries(oggz-sort PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-sort PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-sort
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
      win32
      src/liboggz)
endif()

set(oggz_codecs_SOURCES
  src/tools/oggz-codecs.c
  src/tools/mimetypes.c
  ${COMMON_SRCS})
add_executable(oggz-codecs ${oggz_codecs_SOURCES})
target_link_libraries(oggz-codecs PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-codecs PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-codecs
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
      win32
      src/liboggz)
endif()

configure_file(src/tools/oggz-diff.in ${CMAKE_CURRENT_BINARY_DIR}/src/tools/oggz-diff @ONLY)

set(oggz_chop_SOURCES
  src/tools/oggz-chop/cgi.h
  src/tools/oggz-chop/cmd.h
  src/tools/oggz-chop/header.h
  src/tools/oggz-chop/httpdate.h
  src/tools/oggz-chop/oggz-chop.h
  src/tools/oggz-chop/timespec.h
  src/tools/oggz-chop/oggz-chop.c
  src/tools/oggz_tools.c
  src/tools/skeleton.c
  src/tools/mimetypes.c
  src/liboggz/dirac.c
  src/tools/oggz-chop/cmd.c
  src/tools/oggz-chop/cgi.c
  src/tools/oggz-chop/header.c
  src/tools/oggz-chop/httpdate.c
  src/tools/oggz-chop/main.c
  src/tools/oggz-chop/timespec.c)
add_executable(oggz-chop ${oggz_chop_SOURCES})
target_link_libraries(oggz-chop PRIVATE oggz)
if(HAVE_LIBM)
  target_link_libraries(oggz-chop PRIVATE m)
endif()
target_include_directories(oggz-chop
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    win32
    src/tools
    src/liboggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-chop PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
endif()

install(TARGETS
  oggz_app
  oggz-chop
  oggz-codecs
  oggz-comment
  oggz-dump
  oggz-info
  oggz-known-codecs
  oggz-merge
  oggz-rip
  oggz-scan
  oggz-sort
  oggz-validate
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT TOOLS)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/src/tools/oggz-diff COMPONENT TOOLS
  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS oggz
  EXPORT OggzTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT LIBRARY
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/oggz COMPONENT DEV)

set(CMAKE_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Oggz)

install(EXPORT OggzTargets
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
  NAMESPACE Oggz::
)

configure_file(oggz.pc.in oggz.pc @ONLY)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/oggz.pc
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/OggzConfig.cmake.in
    ${PROJECT_BINARY_DIR}/OggzConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
)

write_basic_package_version_file(${PROJECT_BINARY_DIR}/OggzConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${PROJECT_BINARY_DIR}/OggzConfig.cmake
    ${PROJECT_BINARY_DIR}/OggzConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_CONFIGDIR}
  COMPONENT DEV
)

if(DOXYGEN_FOUND)
  install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/doc/html
    ${CMAKE_CURRENT_BINARY_DIR}/doc/latex
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/liboggz)
endif()

set(man_MANS
  doc/oggz.1
  doc/oggz-diff.1
  doc/oggz-dump.1
  doc/oggz-info.1
  doc/oggz-known-codecs.1
  doc/oggz-merge.1
  doc/oggz-chop.1
  doc/oggz-sort.1
  doc/oggz-rip.1
  doc/oggz-comment.1
  doc/oggz-scan.1
  doc/oggz-validate.1
  doc/oggz-codecs.1)
  install(FILES ${man_MANS} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

add_executable(identity src/examples/identity.c)
target_include_directories(identity PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(identity PRIVATE oggz)

add_executable(modify-headers src/examples/modify-headers.c)
target_include_directories(modify-headers PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(modify-headers PRIVATE oggz)

add_executable(rewrite-pages src/examples/rewrite-pages.c)
target_include_directories(rewrite-pages PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(rewrite-pages PRIVATE oggz)

add_executable(fix-eos src/examples/fix-eos.c)
target_include_directories(fix-eos PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(fix-eos PRIVATE oggz)

add_executable(read-file src/examples/read-file.c)
target_include_directories(read-file PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(read-file PRIVATE oggz)

add_executable(read-io src/examples/read-io.c)
target_include_directories(read-io PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(read-io PRIVATE oggz)

add_executable(dump-comments src/examples/dump-comments.c)
target_include_directories(dump-comments PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(dump-comments PRIVATE oggz)

add_executable(write-feed src/examples/write-feed.c)
target_include_directories(write-feed PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(write-feed PRIVATE oggz)

add_executable(write-hungry src/examples/write-hungry.c)
target_include_directories(write-hungry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(write-hungry PRIVATE oggz)

if(BUILD_TESTING)

  add_executable(comment-test src/tests/comment-test.c)
  target_include_directories(comment-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(comment-test PRIVATE oggz)
  add_test(NAME comment-test COMMAND $<TARGET_FILE:comment-test>)

  add_executable(write-bad-guard src/tests/write-bad-guard.c)
  target_include_directories(write-bad-guard PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-guard PRIVATE oggz)
  add_test(NAME write-bad-guard COMMAND $<TARGET_FILE:write-bad-guard>)

  add_executable(write-unmarked-guard src/tests/write-unmarked-guard.c)
  target_include_directories(write-unmarked-guard PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-unmarked-guard PRIVATE oggz)
  add_test(NAME write-unmarked-guard COMMAND $<TARGET_FILE:write-unmarked-guard>)

  add_executable(write-recursive src/tests/write-recursive.c)
  target_include_directories(write-recursive PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-recursive PRIVATE oggz)
  add_test(NAME write-recursive COMMAND $<TARGET_FILE:write-recursive>)

  add_executable(write-bad-bytes src/tests/write-bad-bytes.c)
  target_include_directories(write-bad-bytes PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-bytes PRIVATE oggz)
  add_test(NAME write-bad-bytes COMMAND $<TARGET_FILE:write-bad-bytes>)

  add_executable(write-bad-bos src/tests/write-bad-bos.c)
  target_include_directories(write-bad-bos PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-bos PRIVATE oggz)
  add_test(NAME write-bad-bos COMMAND $<TARGET_FILE:write-bad-bos>)

  add_executable(write-dup-bos src/tests/write-dup-bos.c)
  target_include_directories(write-dup-bos PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-dup-bos PRIVATE oggz)
  add_test(NAME write-dup-bos COMMAND $<TARGET_FILE:write-dup-bos>)

  add_executable(write-bad-eos src/tests/write-bad-eos.c)
  target_include_directories(write-bad-eos PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-eos PRIVATE oggz)
  add_test(NAME write-bad-eos COMMAND $<TARGET_FILE:write-bad-eos>)

  add_executable(write-bad-granulepos src/tests/write-bad-granulepos.c)
  target_include_directories(write-bad-granulepos PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-granulepos PRIVATE oggz)
  add_test(NAME write-bad-granulepos COMMAND $<TARGET_FILE:write-bad-granulepos>)

  add_executable(write-bad-packetno src/tests/write-bad-packetno.c)
  target_include_directories(write-bad-packetno PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-packetno PRIVATE oggz)
  add_test(NAME write-bad-packetno COMMAND $<TARGET_FILE:write-bad-packetno>)

  add_executable(write-bad-serialno src/tests/write-bad-serialno.c)
  target_include_directories(write-bad-serialno PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-bad-serialno PRIVATE oggz)
  add_test(NAME write-bad-serialno COMMAND $<TARGET_FILE:write-bad-serialno>)

  add_executable(write-prefix src/tests/write-prefix.c)
  target_include_directories(write-prefix PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-prefix PRIVATE oggz)
  add_test(NAME write-prefix COMMAND $<TARGET_FILE:write-prefix>)

  add_executable(write-suffix src/tests/write-suffix.c)
  target_include_directories(write-suffix PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(write-suffix PRIVATE oggz)
  add_test(NAME write-suffix COMMAND $<TARGET_FILE:write-suffix>)

  add_executable(read-generated src/tests/read-generated.c)
  target_include_directories(read-generated PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(read-generated PRIVATE oggz)
  add_test(NAME read-generated COMMAND $<TARGET_FILE:read-generated>)

  add_executable(read-stop-ok src/tests/read-stop-ok.c)
  target_link_libraries(read-stop-ok PRIVATE oggz)
  target_include_directories(read-stop-ok PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  add_test(NAME read-stop-ok COMMAND $<TARGET_FILE:read-stop-ok>)

  add_executable(read-stop-err src/tests/read-stop-err.c)
  target_include_directories(read-stop-err PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(read-stop-err PRIVATE oggz)
  add_test(NAME read-stop-err COMMAND $<TARGET_FILE:read-stop-err>)

  add_executable(io-count src/tests/io-count.c)
  target_include_directories(io-count PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-count PRIVATE oggz)
  add_test(NAME io-count COMMAND $<TARGET_FILE:io-count>)

  add_executable(io-read src/tests/io-read.c)
  target_include_directories(io-read PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-read PRIVATE oggz)
  add_test(NAME io-read COMMAND $<TARGET_FILE:io-read>)

  add_executable(io-run src/tests/io-run.c)
  target_include_directories(io-run PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-run PRIVATE oggz)
  add_test(NAME io-run COMMAND $<TARGET_FILE:io-run>)

  add_executable(io-seek src/tests/io-seek.c)
  target_include_directories(io-seek PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-seek PRIVATE oggz)
  add_test(NAME io-seek COMMAND $<TARGET_FILE:io-seek>)

  add_executable(io-write src/tests/io-write.c)
  target_include_directories(io-write PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-write PRIVATE oggz)
  add_test(NAME io-write COMMAND $<TARGET_FILE:io-write>)

  add_executable(io-read-single src/tests/io-read-single.c)
  target_include_directories(io-read-single PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-read-single PRIVATE oggz)
  add_test(NAME io-read-single COMMAND $<TARGET_FILE:io-read-single>)

  add_executable(io-write-flush src/tests/io-write-flush.c)
  target_include_directories(io-write-flush PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(io-write-flush PRIVATE oggz)
  add_test(NAME io-write-flush COMMAND $<TARGET_FILE:io-write-flush>)

  add_executable(seek-stress src/tests/seek-stress.c)
  target_include_directories(seek-stress PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(seek-stress PRIVATE oggz)

  set(httpdate_test_SOURCES
    src/tests/oggz_tests.h
    src/tools/oggz-chop/httpdate.c
    src/tools/oggz-chop/httpdate_test.c)
  add_executable(httpdate_test ${httpdate_test_SOURCES})
  target_include_directories(httpdate_test
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
      src/tests)
  target_link_libraries(httpdate_test PRIVATE oggz)
  add_test(NAME httpdate_test COMMAND $<TARGET_FILE:httpdate_test>)

endif()
