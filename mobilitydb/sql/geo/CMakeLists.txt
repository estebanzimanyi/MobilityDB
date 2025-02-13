SET(LOCAL_FILES
  222_tgeo
  # 223_tgeo_inout
  224_tgeo_compops
  225_tgeo_spatialfuncs
  228_tgeo_topoops
  229_tgeo_posops
  230_tgeo_distance
  231_tgeo_aggfuncs
  232_tgeo_spatialrels
  234_tgeo_indexes
  )

foreach (f ${LOCAL_FILES})
  process_file(${f} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/${f}.sql.in ${CMAKE_BINARY_DIR}/MobilityDB/sql/${f}.sql @ONLY)
  list(APPEND PACKAGE_SQL_FILES  ${CMAKE_BINARY_DIR}/MobilityDB/sql/${f}.sql)
endforeach()

set(PROJECT_SQL_FILES ${PROJECT_SQL_FILES} ${PACKAGE_SQL_FILES} PARENT_SCOPE)
