add_library(temporal OBJECT
  doublen.c
  error.c
  lifting.c
  meos.c
  meos_catalog.c
  postgres_types.c
  skiplist.c
  set.c
  set_ops.c
  span.c
  span_aggfuncs.c
  span_index.c
  span_ops.c
  spanset.c
  spanset_ops.c
  tbool_ops.c
  tbox.c
  tbox_index.c
  temporal.c
  temporal_aggfuncs.c
  temporal_analytics.c
  temporal_boxops.c
  temporal_compops.c
  temporal_modif.c
  temporal_restrict.c
  temporal_tile.c
  temporal_waggfuncs.c
  tinstant.c
  tnumber_distance.c
  tnumber_mathfuncs.c
  tsequence.c
  tsequenceset.c
  ttext_funcs.c
  type_in.c
  type_out.c
  type_parser.c
  type_util.c
)

add_library(temporal_meos OBJECT
  set_aggfuncs_meos.c
  set_meos.c
  set_ops_meos.c
  span_meos.c
  span_tile_meos.c
  span_aggfuncs_meos.c
  span_ops_meos.c
  spanset_meos.c
  spanset_ops_meos.c
  tbool_ops_meos.c
  temporal_aggfuncs_meos.c
  temporal_boxops_meos.c
  temporal_compops_meos.c
  temporal_meos.c
  temporal_posops_meos.c
  temporal_restrict_meos.c
  temporal_tile_meos.c
  tinstant_meos.c
  tnumber_distance_meos.c
  tnumber_mathfuncs_meos.c
  tsequence_meos.c
  tsequenceset_meos.c
  ttext_funcs_meos.c
  type_in_meos.c
)

