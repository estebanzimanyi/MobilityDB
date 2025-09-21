add_library(common OBJECT
  hashfn.c
  pgfnames.c
  stringinfo.c
  )

# set_property(TARGET common PROPERTY C_VISIBILITY_PRESET hidden)
# set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
