add_library(pg_buffer OBJECT
  cbuffer.c
  tcbuffer.c
  tcbuffer_inout.c
  # tcbuffer_aggfuncs.c
  tcbuffer_boxops.c
  tcbuffer_distance.c
  tcbuffer_spatialfuncs.c
  tcbuffer_spatialrels.c
  # tcbuffer_tempspatialrels.c
  )
