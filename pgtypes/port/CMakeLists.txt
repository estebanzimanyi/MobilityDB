set(PORT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/chklocale.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pg_bitutils.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pg_localeconv_r.c
  ${CMAKE_CURRENT_SOURCE_DIR}/qsort.c
  ${CMAKE_CURRENT_SOURCE_DIR}/qsort_arg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/strerror.c
  )

# Only compile the fallback strlcpy when the platform lacks a native one
# (HAVE_DECL_STRLCPY is detected in the parent CMakeLists.txt).
if(NOT HAS_STRLCPY)
  list(APPEND PORT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/strlcpy.c)
endif()

# pgstrcasecmp.c and snprintf.c duplicate symbols from libpostgres on Windows.
# Only compile them outside the Windows-extension build.
if(NOT PGTYPES_EXT_WIN32)
  list(APPEND PORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pgstrcasecmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/snprintf.c
  )
endif()

# MEOS: on a native Windows (mingw/UCRT) standalone build, PostgreSQL's port
# headers macro-redirect setlocale/fopen/open/setenv/strtof to the
# pgwin32_*/pg_strtof wrappers (and error mapping to _dosmaperr). Compile the
# matching win32 port sources so these resolve at link time. The
# Windows-extension build gets them from libpostgres, so skip them there.
if(WIN32 AND NOT PGTYPES_EXT_WIN32)
  list(APPEND PORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/win32setlocale.c
    ${CMAKE_CURRENT_SOURCE_DIR}/win32env.c
    ${CMAKE_CURRENT_SOURCE_DIR}/win32error.c
    ${CMAKE_CURRENT_SOURCE_DIR}/win32ntdll.c
    ${CMAKE_CURRENT_SOURCE_DIR}/open.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pgsleep.c
    ${CMAKE_CURRENT_SOURCE_DIR}/strtof.c
  )
endif()

set(PGTYPES_SOURCES ${PGTYPES_SOURCES} ${PORT_SOURCES}
  # Mandatory so that the variable update is visible in the parent directory's scope
  PARENT_SCOPE
  )
