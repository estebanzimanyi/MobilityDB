if(MEOS)
  set(UTILS_SRCS
    date.c
    datetime.c
    float.c
    formatting.c
    numutils.c
    pg_error.c
    timestamp.c
    )
endif()

list(APPEND UTILS_SRCS
  postgres_types.c
)

add_library(utils OBJECT ${UTILS_SRCS})

# set_property(TARGET utils PROPERTY C_VISIBILITY_PRESET hidden)
# set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
