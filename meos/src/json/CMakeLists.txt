set(JSON_SOURCES
  jsonbset.c
  tjsonb.c
  tjsonb_compops.c
  tjsonb_jsonfuncs.c
  # tjsonb_jsonpath.c
)

add_library(json OBJECT ${JSON_SOURCES})
