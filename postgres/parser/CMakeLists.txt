add_library(parser OBJECT
  scansup.c
  )

set_property(TARGET parser PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET parser PROPERTY POSITION_INDEPENDENT_CODE ON)
