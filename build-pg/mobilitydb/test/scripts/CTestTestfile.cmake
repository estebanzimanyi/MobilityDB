# CMake generated Testfile for 
# Source directory: /tmp/mdb-equiv/mobilitydb/test/scripts
# Build directory: /tmp/mdb-equiv/build-pg/mobilitydb/test/scripts
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(build "/usr/bin/cmake" "--build" "/tmp/mdb-equiv/build-pg")
set_tests_properties(build PROPERTIES  _BACKTRACE_TRIPLES "/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;7;add_test;/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;0;")
add_test(test_setup "/usr/bin/cmake" "-D" "TEST_OPER=\"test_setup\"" "-P" "/tmp/mdb-equiv/build-pg/mobilitydb/test/scripts/test.cmake")
set_tests_properties(test_setup PROPERTIES  DEPENDS "build" FIXTURES_SETUP "DBSETUP" RESOURCE_LOCK "DBLOCK" WORKING_DIRECTORY "/tmp/mdb-equiv/build-pg" _BACKTRACE_TRIPLES "/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;12;add_test;/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;0;")
add_test(teardown "/usr/bin/cmake" "-D" "TEST_OPER=\"teardown\"" "-P" "/tmp/mdb-equiv/build-pg/mobilitydb/test/scripts/test.cmake")
set_tests_properties(teardown PROPERTIES  DBEXT "DBSETUP" FIXTURES_CLEANUP "DB" RESOURCE_LOCK "DBLOCK" WORKING_DIRECTORY "/tmp/mdb-equiv/build-pg" _BACKTRACE_TRIPLES "/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;18;add_test;/tmp/mdb-equiv/mobilitydb/test/scripts/CMakeLists.txt;0;")
