# Set the location of the directory of the time zone database
add_definitions(-DSYSTEMTZDIR="/usr/share/zoneinfo")
message(STATUS "Directory of the time zone database: /usr/share/zoneinfo")

set(PGTYPES_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pg_config.h.in
    ${PGTYPES_BINARY_DIR}/pg_config.h
)

add_subdirectory(common)
add_subdirectory(port)
add_subdirectory(timezone)
add_subdirectory(utils)

target_include_directories(common PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(port PRIVATE   ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(timezone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(pgtypes STATIC
  $<TARGET_OBJECTS:common>
  $<TARGET_OBJECTS:port>
  $<TARGET_OBJECTS:timezone>
  $<TARGET_OBJECTS:utils>
)

target_include_directories(pgtypes
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}           # normal headers
        ${PGTYPES_BINARY_DIR}                 # configured pg_config.h
)

get_target_property(PGTYPES_INCLUDES pgtypes INCLUDE_DIRECTORIES)
message(STATUS "pgtypes include dirs = ${PGTYPES_INCLUDES}")





