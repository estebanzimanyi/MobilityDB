add_library(utils OBJECT
  bool.c
  int.c
  int8.c
  date.c
  datetime.c
  # float.c
  format_type.c
  formatting.c
  # numeric.c
  numutils.c
  # pg_locale.c
  timestamp.c
  varlena.c
  )

# set_property(TARGET utils PROPERTY C_VISIBILITY_PRESET hidden)
# set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
