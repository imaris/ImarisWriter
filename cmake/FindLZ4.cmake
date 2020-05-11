#[[.rst:
FindLZ4
-------

This module searches for the LZ4 compression library. If successful it sets
the following variables

* LZ4_FOUND: if the libary has been found
* LZ4_LIBRARY_DIRS: directories where to find the library 
* LZ4_LIBRARIES: paths to the libraries to link with
* LZ4_INCLUDE_DIRS: paths to the include directories with the header files
* LZ4_VERSION: the total version of the library
* LZ4_VERSION_MAJOR: major version number
* LZ4_VERSION_MINOR: minor version number
* LZ4_VERSION_PATCH: patch version (release version in LZ4 terms) number

The module defines two additional variables which can be used to control
the behavior of the module

* LZ4_ROOT: if this is set the module will search for libraries and include 
            directories below this path. In this case the module omits 
            searches in the default directories
* LZ4_USE_STATIC: if OFF (default) the module looks for a shared library,
                  if ON for a static library

#]]
# ===========================================================================
# Copyright 2017 Eugen Wintersberger
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================


include(FindPackageHandleStandardArgs)

set(LZ4_USE_STATIC OFF CACHE BOOL "Use LZ4 shared libraries")
set(LZ4_ROOT "" CACHE PATH "Search path for the LZ4 libraries")

#=============================================================================
# check for the library
#=============================================================================
set(_LZ4_LIB_NAMES)
if(LZ4_USE_STATIC)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        list(APPEND _LZ4_LIB_NAMES liblz4.a)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
        list(APPEND _LZ4_LIB_NAMES liblz4.lib)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        message(STATUS "Static library not implemented on OSX")
    endif()
else()
    list(APPEND _LZ4_LIB_NAMES lz4 liblz4)
endif()

#
# if the user has provided LZ4_ROOT we omit searches in the default 
# locations
#
if(LZ4_ROOT)
    message(STATUS "Looking for LZ4 library below: ${LZ4_ROOT}")
    find_library(LZ4_LIBRARIES 
                 NAMES ${_LZ4_LIB_NAMES}
                 PATHS ${LZ4_ROOT}/lib ${LZ4_ROOT}/bin
                 NO_DEFAULT_PATH)
else()
    find_library(LZ4_LIBRARIES 
                 NAMES ${_LZ4_LIB_NAMES})
endif()

if(LZ4_LIBRARIES MATCHES "LZ4_LIBRARIES-NOTFOUND")
    message(FATAL_ERROR "Could not find LZ4 libraries")
endif()

#
# get the directory where the library are installed
#
get_filename_component(LZ4_LIBRARY_DIRS ${LZ4_LIBRARIES} DIRECTORY)

#=============================================================================
# check for the header files
#=============================================================================

#
# if the user has provided LZ4_ROOT we omit searches in the default 
# locations
#
if(LZ4_ROOT)
    message(STATUS "Looking for LZ4 header files below: ${LZ4_ROOT}")
    find_path(LZ4_INCLUDE_DIRS NAMES lz4.h
        PATHS ${LZ4_ROOT}/include)
else()
    find_path(LZ4_INCLUDE_DIRS NAMES lz4.h)
endif()

if(LZ4_INCLUDE_DIRS MATCHES "LZ4_INCLUDE_DIRS-NOTFOUND")
    message(FATAL_ERROR "Could not find LZ4 header files")
endif()

#=============================================================================
# Get the libary version from the header file
#=============================================================================
file(STRINGS ${LZ4_INCLUDE_DIRS}/lz4.h _LZ4_HEADER_FILE
     REGEX "^#define LZ4_VERSION.*$")

string(REGEX REPLACE ".*LZ4_VERSION_MAJOR[ ]+([0-9]+).*" "\\1"
    LZ4_VERSION_MAJOR "${_LZ4_HEADER_FILE}")
string(REGEX REPLACE ".*LZ4_VERSION_MINOR[ ]+([0-9]+).*" "\\1"
    LZ4_VERSION_MINOR "${_LZ4_HEADER_FILE}")
string(REGEX REPLACE ".*LZ4_VERSION_RELEASE[ ]+([0-9]+).*" "\\1"
    LZ4_VERSION_PATCH "${_LZ4_HEADER_FILE}")
set(LZ4_VERSION "${LZ4_VERSION_MAJOR}.${LZ4_VERSION_MINOR}.${LZ4_VERSION_PATCH}")


#=============================================================================
# putting it all together
#=============================================================================
set(_REQUIRED_VARS LZ4_LIBRARIES LZ4_INCLUDE_DIRS LZ4_LIBRARY_DIRS 
    LZ4_VERSION_MAJOR LZ4_VERSION_MINOR )

find_package_handle_standard_args(LZ4
    REQUIRED_VARS ${_REQUIRED_VARS}
    VERSION_VAR LZ4_VERSION)
