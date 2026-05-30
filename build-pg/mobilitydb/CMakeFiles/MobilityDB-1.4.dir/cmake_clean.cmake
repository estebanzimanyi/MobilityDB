file(REMOVE_RECURSE
  "../libMobilityDB-1.4.pdb"
  "../libMobilityDB-1.4.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/MobilityDB-1.4.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
