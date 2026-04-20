# PostgreSQL Types

# Detect whether the system declares BSD string helpers natively (macOS does,
# glibc does not). The generated pg_config.h consumes these values.
include(CheckSymbolExists)
check_symbol_exists(strlcat "string.h" HAS_STRLCAT)
check_symbol_exists(strlcpy "string.h" HAS_STRLCPY)
if(HAS_STRLCAT)
  set(HAVE_DECL_STRLCAT 1)
else()
  set(HAVE_DECL_STRLCAT 0)
endif()
if(HAS_STRLCPY)
  set(HAVE_DECL_STRLCPY 1)
else()
  set(HAVE_DECL_STRLCPY 0)
endif()

configure_file(pg_config.h.in pg_config.h @ONLY)
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/pgtypes")
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/pgtypes")

# When building the MobilityDB extension (MEOS=OFF) on Windows or macOS, the
# pgtypes "frontend"-style files duplicate symbols that the postgres backend
# already provides (palloc, pg_strncasecmp, pg_snprintf, enlargeStringInfo,
# pg_strtoint32, session_timezone, etc.).
#
#   * Windows: linking both yields multiple-definition errors. With my fix the
#     duplicates are dropped and libpostgres' import stubs satisfy the calls.
#   * macOS: link succeeds (pgtypes' definitions just live in the dylib) but
#     unlike glibc, dyld does NOT perform symbol interposition by default, so
#     the dylib's local palloc (which uses malloc) is used, and the backend
#     later pfree's blocks it didn't allocate -> SIGABRT.
#   * Linux glibc handles this via symbol interposition (RTLD_GLOBAL extension
#     loading), so we still ship those files there.
#
# For the few conflicting functions inside varlena.c / timestamp.c (which also
# host unique helpers used by MobilityDB), the .c files themselves gate the
# duplicate bodies with PG_EXT_NO_BACKEND_DUPS.
if((WIN32 OR APPLE) AND NOT MEOS)
  set(PGTYPES_EXT_NO_BACKEND_DUPS TRUE)
  add_definitions(-DPG_EXT_NO_BACKEND_DUPS=1)
  # Backwards-compatible aliases (older comments / file names referenced these)
  set(PGTYPES_EXT_WIN32 TRUE)
  add_definitions(-DPG_EXT_WIN32_BUILD=1)
else()
  set(PGTYPES_EXT_NO_BACKEND_DUPS FALSE)
  set(PGTYPES_EXT_WIN32 FALSE)
endif()

# Set the location of the directory of the time zone database. macOS may also
# have /var/db/timezone/zoneinfo but /usr/share/zoneinfo is the standard
# fallback; Windows has no such path so leave SYSTEMTZDIR undefined and let
# pgtypes use its bundled lookup, or, in extension mode, defer to the
# postgres backend's own timezone resolution.
if(NOT WIN32)
  add_definitions(-DSYSTEMTZDIR="/usr/share/zoneinfo")
  message(STATUS "Directory of the time zone database: /usr/share/zoneinfo")
endif()

# Create an empty list that contains the source files of the pgtypes library.
# The subdirectories below will append its source files to the list.
set(PGTYPES_SOURCES "")

add_subdirectory(access)
add_subdirectory(catalog)
add_subdirectory(common)
add_subdirectory(parser)
add_subdirectory(port)
# add_subdirectory(storage)
add_subdirectory(timezone)
add_subdirectory(utils)

# Uncomment the following line to see all source files included
# message(STATUS "PGTYPES_SOURCES ${PGTYPES_SOURCES}")

# Compile the source files
add_library(pgtypes STATIC ${PGTYPES_SOURCES})

# Create the list of object files
# set(PGTYPES_OBJECTS "$<TARGET_OBJECTS:pgtypes>")

# Define a static library for pgtypes
# add_library(pgtypes STATIC ${PGTYPES_OBJECTS})

target_include_directories(pgtypes PUBLIC "${CMAKE_SOURCE_DIR}/pgtypes")
target_include_directories(pgtypes PUBLIC "${CMAKE_BINARY_DIR}/pgtypes")

