add_library(common OBJECT
  d2s.c
  encnames.c
  f2s.c
  hashfn.c
  pgfnames.c
  psprintf.c
  string.c
  stringinfo.c
  wchar.c
  )

set_property(TARGET common PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
