# Guest libc archives built with davecc, matching the Bazel //:libc_* genrules.

set(DAVECC_GUEST_LIBC_COMMON_C
  libc/abs.c
  libc/assert_fail.c
  libc/atoi.c
  libc/atol.c
  libc/bsearch.c
  libc/calloc.c
  libc/cdiv.c
  libc/ckdint.c
  libc/complex_core.c
  libc/complex_exp.c
  libc/complex_hyperbolic.c
  libc/complex_inverse.c
  libc/complex_sqrt.c
  libc/complex_trig.c
  libc/ctype.c
  libc/cxx_guard.c
  libc/cxx_new.c
  libc/cxx_rtti.c
  libc/cxx_tls.c
  libc/cxx_tls_stubs.c
  libc/davecc_lifecycle.c
  libc/eh_frame.c
  libc/eh_cxa.c
  libc/eh_terminate.c
  libc/eh_personality.c
  libc/eh_throw.c
  libc/gxx_personality.c
  libc/lsda.c
  libc/unwind_api.c
  libc/exit.c
  libc/fclose.c
  libc/fflush.c
  libc/fgetc.c
  libc/fgets.c
  libc/fopen.c
  libc/fpfuncs.c
  libc/fputc.c
  libc/fputs.c
  libc/fread.c
  libc/free.c
  libc/free_sized.c
  libc/fseek.c
  libc/ftoa.c
  libc/fwrite.c
  libc/getenv.c
  libc/itoa_int.c
  libc/itoa_long.c
  libc/itoa_longlong.c
  libc/itoa_ptr.c
  libc/gets.c
  libc/ldexp.c
  libc/malloc.c
  libc/memccpy.c
  libc/memchr.c
  libc/memcmp.c
  libc/memcpy.c
  libc/memmove.c
  libc/memset.c
  libc/memset_explicit.c
  libc/modf.c
  libc/perror.c
  libc/errno.c
  libc/posix.c
  libc/posix_close.c
  libc/posix_fs.c
  libc/posix_linux.c
  libc/posix_lseek.c
  libc/posix_open.c
  libc/posix_read.c
  libc/printf.c
  libc/printf_common.c
  libc/printf_literal.c
  libc/printf_long.c
  libc/qsort.c
  libc/rand.c
  libc/realloc.c
  libc/scanf.c
  libc/signal.c
  libc/sincos.c
  libc/stdio.c
  libc/stdio_flags.c
  libc/stdio_setvbuf.c
  libc/strcat.c
  libc/strchr.c
  libc/strcmp.c
  libc/strcpy.c
  libc/strcspn.c
  libc/strerror.c
  libc/strings.c
  libc/strlen.c
  libc/strdup.c
  libc/strncat.c
  libc/strncmp.c
  libc/strncpy.c
  libc/strpbrk.c
  libc/strrchr.c
  libc/strspn.c
  libc/strstr.c
  libc/strtod.c
  libc/strtok.c
  libc/strtol.c
  libc/strtoll.c
  libc/strtoul.c
  libc/strtoull.c
  libc/stdlib_extra.c
  libc/stacktrace.c
  libc/syscall.c
  libc/tmpfile.c
  libc/time.c
  libc/threads.c
  libc/ungetc.c
  libc/acos.c
  libc/asin.c
  libc/atan.c
  libc/atan2.c
  libc/ceil.c
  libc/exp.c
  libc/fabs.c
  libc/fenv.c
  libc/floor.c
  libc/frexp.c
  libc/log.c
  libc/math_extra.c
  libc/locale_c.c
  libc/inttypes.c
  libc/uchar.c
  libc/wchar.c
  libc/wctype.c
  libc/pow.c
  libc/sqrt.c
  libc/tan.c
)

# 65C02 keeps a slimmer C set (no TLS / DWARF unwind / guest heap).
set(DAVECC_GUEST_LIBC_65C02_C ${DAVECC_GUEST_LIBC_COMMON_C})
list(REMOVE_ITEM DAVECC_GUEST_LIBC_65C02_C
  libc/cxx_tls.c
  libc/cxx_tls_stubs.c
  libc/eh_frame.c
  libc/gxx_personality.c
  libc/lsda.c
  libc/unwind_api.c
)

file(GLOB DAVECC_GUEST_LIBC_CXX CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libc/*.cc")

function(davecc_guest_libc archive target_triple)
  cmake_parse_arguments(ARG "NO_CXX" "" "CFLAGS;RUNTIME;SOURCES;EXTRA_C;EXCLUDE" ${ARGN})

  if(NOT ARG_SOURCES)
    set(ARG_SOURCES ${DAVECC_GUEST_LIBC_COMMON_C})
  endif()
  list(APPEND ARG_SOURCES ${ARG_EXTRA_C})

  set(output "${CMAKE_BINARY_DIR}/libc/${archive}")
  set(cmd
    "${CMAKE_SOURCE_DIR}/tools/build_guest_libc.sh"
    --davecc "$<TARGET_FILE:davecc_bin>"
    --archivist "$<TARGET_FILE:archivist_bin>"
    --output "${output}"
    --target "${target_triple}"
  )
  foreach(flag IN LISTS ARG_CFLAGS)
    list(APPEND cmd --cflags "${flag}")
  endforeach()
  foreach(src IN LISTS ARG_RUNTIME)
    list(APPEND cmd --runtime "${src}")
  endforeach()
  foreach(src IN LISTS ARG_SOURCES)
    list(APPEND cmd --source "${src}")
  endforeach()
  foreach(src IN LISTS ARG_EXCLUDE)
    list(APPEND cmd --exclude "${src}")
  endforeach()
  if(ARG_NO_CXX)
    list(APPEND cmd --no-cxx)
  endif()

  set(depends
    davecc_bin
    archivist_bin
    "${CMAKE_SOURCE_DIR}/tools/build_guest_libc.sh"
    ${ARG_RUNTIME}
    ${ARG_SOURCES}
    ${DAVECC_GUEST_LIBC_CXX}
  )

  add_custom_command(
    OUTPUT "${output}"
    COMMAND ${cmd}
    DEPENDS ${depends}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Building guest libc ${archive} (${target_triple})"
    VERBATIM
  )
  add_custom_target(davecc_${archive} DEPENDS "${output}")
endfunction()
