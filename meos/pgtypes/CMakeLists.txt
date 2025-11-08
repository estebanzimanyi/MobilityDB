
# PostgreSQL Types
configure_file(pg_config.h.in pg_config.h)
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/pgtypes")
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/pgtypes")

# Set the location of the directory of the time zone database
add_definitions(-DSYSTEMTZDIR="/usr/share/zoneinfo")
message(STATUS "Directory of the time zone database: /usr/share/zoneinfo")

add_subdirectory(access)
add_subdirectory(catalog)
add_subdirectory(common)
add_subdirectory(parser)
add_subdirectory(port)
# add_subdirectory(storage)
add_subdirectory(timezone)
add_subdirectory(utils)




