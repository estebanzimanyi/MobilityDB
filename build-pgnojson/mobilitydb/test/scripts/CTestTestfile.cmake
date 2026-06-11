# CMake generated Testfile for 
# Source directory: /home/esteban/src/_md_json_wt/mobilitydb/test/scripts
# Build directory: /home/esteban/src/_md_json_wt/build-pgnojson/mobilitydb/test/scripts
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(build "/usr/bin/cmake" "--build" "/home/esteban/src/_md_json_wt/build-pgnojson")
set_tests_properties(build PROPERTIES  _BACKTRACE_TRIPLES "/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;7;add_test;/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;0;")
add_test(test_setup "/usr/bin/cmake" "-D" "TEST_OPER=\"test_setup\"" "-P" "/home/esteban/src/_md_json_wt/build-pgnojson/mobilitydb/test/scripts/test.cmake")
set_tests_properties(test_setup PROPERTIES  DEPENDS "build" FIXTURES_SETUP "DBSETUP" RESOURCE_LOCK "DBLOCK" WORKING_DIRECTORY "/home/esteban/src/_md_json_wt/build-pgnojson" _BACKTRACE_TRIPLES "/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;12;add_test;/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;0;")
add_test(teardown "/usr/bin/cmake" "-D" "TEST_OPER=\"teardown\"" "-P" "/home/esteban/src/_md_json_wt/build-pgnojson/mobilitydb/test/scripts/test.cmake")
set_tests_properties(teardown PROPERTIES  DBEXT "DBSETUP" FIXTURES_CLEANUP "DB" RESOURCE_LOCK "DBLOCK" WORKING_DIRECTORY "/home/esteban/src/_md_json_wt/build-pgnojson" _BACKTRACE_TRIPLES "/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;18;add_test;/home/esteban/src/_md_json_wt/mobilitydb/test/scripts/CMakeLists.txt;0;")
