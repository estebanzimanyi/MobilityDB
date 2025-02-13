SET(LOCAL_FILES
  252_tgeo
  # 253_tgeo_inout
  254_tgeo_compops
  255_tgeo_spatialfuncs
  258_tgeo_topops
  259_tgeo_posops
  260_tgeo_distance
  261_tgeo_aggfuncs
  262_tgeo_spatialrels
  264_tgeo_indexes
  )

foreach (f ${LOCAL_FILES})
  process_file(${f} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/${f}.sql.in ${CMAKE_BINARY_DIR}/MobilityDB/sql/${f}.sql @ONLY)
  list(APPEND PACKAGE_SQL_FILES  ${CMAKE_BINARY_DIR}/MobilityDB/sql/${f}.sql)
endforeach()

set(PROJECT_SQL_FILES ${PROJECT_SQL_FILES} ${PACKAGE_SQL_FILES} PARENT_SCOPE)
