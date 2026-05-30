# Install script for directory: /tmp/mdb-equiv/mobilitydb

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/pgsql/17/share/extension/mobilitydb.control;/usr/local/pgsql/17/share/extension/mobilitydb--1.4.0.sql")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/pgsql/17/share/extension" TYPE FILE FILES
    "/tmp/mdb-equiv/build-pg/mobilitydb.control"
    "/tmp/mdb-equiv/build-pg/mobilitydb--1.4.0.sql"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/pgsql/17/lib/libMobilityDB-1.4.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/pgsql/17/lib" TYPE MODULE FILES "/tmp/mdb-equiv/build-pg/libMobilityDB-1.4.so")
  if(EXISTS "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/pgsql/17/lib/libMobilityDB-1.4.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/tmp/mdb-equiv/build-pg/mobilitydb/CMakeFiles/MobilityDB-1.4.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/pgsql/17/share/extension/mobilitydb_datagen.control;/usr/local/pgsql/17/share/extension/mobilitydb_datagen--1.4.0.sql")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/pgsql/17/share/extension" TYPE FILE FILES
    "/tmp/mdb-equiv/build-pg/mobilitydb_datagen.control"
    "/tmp/mdb-equiv/build-pg/mobilitydb_datagen--1.4.0.sql"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/tmp/mdb-equiv/build-pg/mobilitydb/postgis/cmake_install.cmake")
  include("/tmp/mdb-equiv/build-pg/mobilitydb/meos_src/cmake_install.cmake")
  include("/tmp/mdb-equiv/build-pg/mobilitydb/src/cmake_install.cmake")
  include("/tmp/mdb-equiv/build-pg/mobilitydb/sql/cmake_install.cmake")
  include("/tmp/mdb-equiv/build-pg/mobilitydb/datagen/cmake_install.cmake")
  include("/tmp/mdb-equiv/build-pg/mobilitydb/test/cmake_install.cmake")

endif()

