cmake_minimum_required(VERSION 3.1)

project(oggz VERSION 1.1.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(TestBigEndian)
include(TestLargeFiles)
include(CTest)
include(GNUInstallDirs)

find_package(Doxygen)
find_package(Ogg REQUIRED)

set(OGGZ_CONFIG_READ 1)
set(OGGZ_CONFIG_WRITE 1)

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
check_library_exists(m ceil "" HAVE_LIBM)
if(HAVE_LIBM)
  find_library(LIBM m)
endif()
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
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_link_libraries(oggz PUBLIC Xiph::ogg)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_sources(oggz PRIVATE win32/liboggz.def)
endif()
target_compile_definitions(oggz PRIVATE ${LARGE_FILES_DEFINITIONS})
set_target_properties (oggz PROPERTIES
  PUBLIC_HEADER "${oggzinclude_HEADERS}"
  SOVERSION 2.6.0
  VERSION 2)

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

add_executable(oggz_known_codecs src/tools/oggz-known-codecs.c)
target_link_libraries(oggz_known_codecs PRIVATE oggz)

set(oggz_info_SOURCES
  src/tools/oggz-info.c
  src/tools/skeleton.c
  ${COMMON_SRCS})
add_executable(oggz-info ${oggz_info_SOURCES})
target_link_libraries(oggz-info PRIVATE oggz)
if(HAVE_LIBM)
  target_link_libraries(oggz-info PRIVATE ${LIBM})
endif()
if(NOT HAVE_GET_OPT)
  target_sources(oggz-info PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-info
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
endif()

set(oggz_comment_SOURCES
  src/tools/oggz-comment.c
  ${COMMON_SRCS})
add_executable(oggz-comment ${oggz_comment_SOURCES})
target_link_libraries(oggz-comment PRIVATE oggz)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-comment PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-comment
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
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
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
endif()

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
  target_link_libraries(oggz-chop PRIVATE ${LIBM})
endif()
target_include_directories(oggz-chop
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/tools>)
if(NOT HAVE_GET_OPT)
  target_sources(oggz-chop PRIVATE
    win32/getopt.h
	win32/attgetopt.c)
target_include_directories(oggz-chop
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/liboggz>)
endif()

install (TARGETS oggz oggz_app oggz_known_codecs oggz-info oggz-comment
  oggz-dump oggz-merge oggz-rip oggz-validate oggz-basetime oggz-scan oggz-sort
  oggz-codecs oggz-chop
  RUNTIME DESTINATION       ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION       ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION       ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/oggz)
if(DOXYGEN_FOUND)
  install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/doc/html
    ${CMAKE_CURRENT_BINARY_DIR}/doc/latex
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/liboggz)
endif()
if(UNIX)
set(man_MANS
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-diff.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-dump.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-info.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-known-codecs.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-merge.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-chop.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-sort.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-rip.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-comment.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-scan.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-validate.1
  ${CMAKE_CURRENT_SOURCE_DIR}/doc/oggz-codecs.1)
  install(FILES ${man_MANS} DESTINATION ${CMAKE_INSTALL_MANDIR})
endif()

add_executable(identity src/examples/identity.c)
target_link_libraries(identity PRIVATE oggz)

add_executable(modify-headers src/examples/modify-headers.c)
target_link_libraries(modify-headers PRIVATE oggz)

add_executable(rewrite-pages src/examples/rewrite-pages.c)
target_link_libraries(rewrite-pages PRIVATE oggz)

add_executable(fix-eos src/examples/fix-eos.c)
target_link_libraries(fix-eos PRIVATE oggz)

add_executable(read-file src/examples/read-file.c)
target_link_libraries(read-file PRIVATE oggz)

add_executable(read-io src/examples/read-io.c)
target_link_libraries(read-io PRIVATE oggz)

add_executable(dump-comments src/examples/dump-comments.c)
target_link_libraries(dump-comments PRIVATE oggz)

add_executable(write-feed src/examples/write-feed.c)
target_link_libraries(write-feed PRIVATE oggz)

add_executable(write-hungry src/examples/write-hungry.c)
target_link_libraries(write-hungry PRIVATE oggz)

enable_testing()

add_executable(comment-test src/tests/comment-test.c)
target_link_libraries(comment-test PRIVATE oggz)
add_test(comment-test comment-test)

add_executable(write-bad-guard src/tests/write-bad-guard.c)
target_link_libraries(write-bad-guard PRIVATE oggz)
add_test(write-bad-guard write-bad-guard)

add_executable(write-unmarked-guard src/tests/write-unmarked-guard.c)
target_link_libraries(write-unmarked-guard PRIVATE oggz)
add_test(write-unmarked-guard write-unmarked-guard)

add_executable(write-recursive src/tests/write-recursive.c)
target_link_libraries(write-recursive PRIVATE oggz)
add_test(write-recursive write-recursive)

add_executable(write-bad-bytes src/tests/write-bad-bytes.c)
target_link_libraries(write-bad-bytes PRIVATE oggz)
add_test(write-bad-bytes write-bad-bytes)

add_executable(write-bad-bos src/tests/write-bad-bos.c)
target_link_libraries(write-bad-bos PRIVATE oggz)
add_test(write-bad-bos write-bad-bos)

add_executable(write-dup-bos src/tests/write-dup-bos.c)
target_link_libraries(write-dup-bos PRIVATE oggz)
add_test(write-dup-bos write-dup-bos)

add_executable(write-bad-eos src/tests/write-bad-eos.c)
target_link_libraries(write-bad-eos PRIVATE oggz)
add_test(write-bad-eos write-bad-eos)

add_executable(write-bad-granulepos src/tests/write-bad-granulepos.c)
target_link_libraries(write-bad-granulepos PRIVATE oggz)
add_test(write-bad-granulepos write-bad-granulepos)

add_executable(write-bad-packetno src/tests/write-bad-packetno.c)
target_link_libraries(write-bad-packetno PRIVATE oggz)
add_test(write-bad-packetno write-bad-packetno)

add_executable(write-bad-serialno src/tests/write-bad-serialno.c)
target_link_libraries(write-bad-serialno PRIVATE oggz)
add_test(write-bad-serialno write-bad-serialno)

add_executable(write-prefix src/tests/write-prefix.c)
target_link_libraries(write-prefix PRIVATE oggz)
add_test(write-prefix write-prefix)

add_executable(write-suffix src/tests/write-suffix.c)
target_link_libraries(write-suffix PRIVATE oggz)
add_test(write-suffix write-suffix)

add_executable(read-generated src/tests/read-generated.c)
target_link_libraries(read-generated PRIVATE oggz)
add_test(read-generated read-generated)

add_executable(read-stop-ok src/tests/read-stop-ok.c)
target_link_libraries(read-stop-ok PRIVATE oggz)
add_test(read-stop-ok read-stop-ok)

add_executable(read-stop-err src/tests/read-stop-err.c)
target_link_libraries(read-stop-err PRIVATE oggz)
add_test(read-stop-err read-stop-err)

add_executable(io-count src/tests/io-count.c)
target_link_libraries(io-count PRIVATE oggz)
add_test(io-count io-count)

add_executable(io-read src/tests/io-read.c)
target_link_libraries(io-read PRIVATE oggz)
add_test(io-read io-read)

add_executable(io-run src/tests/io-run.c)
target_link_libraries(io-run PRIVATE oggz)
add_test(io-run io-run)

add_executable(io-seek src/tests/io-seek.c)
target_link_libraries(io-seek PRIVATE oggz)
add_test(io-seek io-seek)

add_executable(io-write src/tests/io-write.c)
target_link_libraries(io-write PRIVATE oggz)
add_test(io-write io-write)

add_executable(io-read-single src/tests/io-read-single.c)
target_link_libraries(io-read-single PRIVATE oggz)
add_test(io-read-single io-read-single)

add_executable(io-write-flush src/tests/io-write-flush.c)
target_link_libraries(io-write-flush PRIVATE oggz)
add_test(io-write-flush io-write-flush)

add_executable(seek-stress src/tests/seek-stress.c)
target_link_libraries(seek-stress PRIVATE oggz)

set(httpdate_test_SOURCES
  src/tests/oggz_tests.h
  src/tools/oggz-chop/httpdate.c
  src/tools/oggz-chop/httpdate_test.c)
add_executable(httpdate_test ${httpdate_test_SOURCES})
target_include_directories(httpdate_test
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/tests>)
target_link_libraries(httpdate_test PRIVATE oggz)
add_test(httpdate_test httpdate_test)
