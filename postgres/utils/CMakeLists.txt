if(MEOS)
  add_library(utils OBJECT
    bool.c
    date.c
    datetime.c
    # float.c
    formatting.c
    # numeric.c
    numutils.c
    timestamp.c
    )
else()
  add_library(utils OBJECT
    bool.c
    )
endif()

# set_property(TARGET utils PROPERTY C_VISIBILITY_PRESET hidden)
# set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
