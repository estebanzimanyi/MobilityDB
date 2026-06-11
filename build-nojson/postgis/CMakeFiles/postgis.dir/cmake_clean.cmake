file(REMOVE_RECURSE
  "libpostgis.a"
  "libpostgis.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/postgis.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
