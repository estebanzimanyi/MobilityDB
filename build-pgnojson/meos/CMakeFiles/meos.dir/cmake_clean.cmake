file(REMOVE_RECURSE
  "libmeos.a"
  "libmeos.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/meos.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
