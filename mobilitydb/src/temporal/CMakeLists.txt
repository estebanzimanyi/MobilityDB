add_library(pg_temporal OBJECT
  doublen.c
  geo_constructors.c
  meos_catalog.c
  set.c
  set_gin.c
  set_aggfuncs.c
  set_ops.c
  skiplist.c
  span.c
  span_aggfuncs.c
  span_analyze.c
  span_gist.c
  span_spgist.c
  span_ops.c
  span_selfuncs.c
  spanset.c
  spanset_ops.c
  tbool_ops.c
  tbox.c
  temporal.c
  temporal_aggfuncs.c
  temporal_analytics.c
  temporal_analyze.c
  temporal_boxops.c
  temporal_compops.c
  temporal_index.c
  temporal_posops.c
  temporal_selfuncs.c
  temporal_supportfn.c
  temporal_tile.c
  temporal_waggfuncs.c
  tnumber_distance.c
  tnumber_gist.c
  tnumber_mathfuncs.c
  tnumber_spgist.c
  ttext_funcs.c
  type_in.c
  type_out.c
  type_util.c
)
