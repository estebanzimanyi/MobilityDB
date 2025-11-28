set(RGEO_SRCS
  lwgeom_utils.c
  planar_graph.c
  trgeo.c
  trgeo_aggfuncs.c
  trgeo_boxops.c
  trgeo_compops.c
  trgeo_distance.c
  trgeo_inst.c
  trgeo_parser.c
  trgeo_seq.c
  trgeo_seqset.c
  trgeo_spatialfuncs.c
  trgeo_spatialrels.c
  trgeo_transform.c
  trgeo_utils.c
  trgeo_vclip.c
  )

add_library(rgeo OBJECT ${RGEO_SRCS})