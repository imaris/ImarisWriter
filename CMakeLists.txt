cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

#windows only
set (CMAKE_SYSTEM_VERSION 8.1 CACHE TYPE INTERNAL FORCE)

project(ImarisWriter)

cmake_policy(SET CMP0074 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 11)
enable_language(CXX)

file(GLOB INTERFACE interface/*.h interfaceC/*.h)
file(GLOB HDRS ${INTERFACE} writer/*.h)
file(GLOB SRCS c/*.cxx writer/*.cxx)

add_compile_definitions(_HDF5USEDLL_ H5_BUILT_AS_DYNAMIC_LIB)
add_compile_definitions(_SBCS _SCL_SECURE_NO_WARNINGS)
set(CMAKE_CXX_FLAGS "-Zm200 /EHsc /bigobj -w44101 -w44244 -w44267 -w34062 -w34263 -w34265 -w34287 -w34289 -w34296 -w34431 -w34057 -w34092 -w34131 -w34132 -w34189 -w34202 -w34208 -w34245 -w34268 -w34295 -w34389 -w34456 -w34457 -w34505 -w34515 -w34516" CACHE INTERNAL "")

if(NOT DEFINED CMAKE_MODULE_PATH)
    message(STATUS "define an empty cmake module path")
    set(CMAKE_MODULE_PATH)
endif()
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(HDF5 REQUIRED)
if(HDF5_FOUND)
	include_directories(${HDF5_INCLUDE_DIR})
	set(_hdf5_libs ${HDF5_LIBRARIES})
	message("Found HDF." + ${HDF5_INCLUDE_DIR} + "  " + ${HDF5_LIBRARIES})
else()
	message("Not found and not installing HDF.")
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIR})
	message("Found ZLIB." + ${ZLIB_INCLUDE_DIR} + "  " + ${ZLIB_LIBRARY})
else()
	message("Not found and not installing ZLIB.")
endif()

find_package(LZ4 REQUIRED)
if(LZ4_FOUND)
	include_directories(${LZ4_INCLUDE_DIRS})
	message("Found LZ4." + ${LZ4_INCLUDE_DIRS} + "  " + ${LZ4_LIBRARIES})
else()
	message("Not found and not installing LZ4.")
endif()

#find_package(Threads REQUIRED)

set(tgt ImarisWriter_static)
add_library(${tgt} STATIC ${SRCS} ${HDRS})
target_include_directories(${tgt} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${tgt} ${_hdf5_libs} ${ZLIB_LIBRARY} ${LZ4_LIBRARIES})

set(tgt bpImarisWriter96)
add_library(${tgt} SHARED ${SRCS} ${HDRS})
target_include_directories(${tgt} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(${tgt} PRIVATE COMPILE_SHARED_LIBRARY)
target_link_libraries(${tgt} ${_hdf5_libs} ${ZLIB_LIBRARY} ${LZ4_LIBRARIES})

message("Found build." + ${CMAKE_BINARY_DIR})
if(${CMAKE_PROJECT_NAME} STREQUAL ImarisWriter)
	install(FILES ${INTERFACE} DESTINATION ${CMAKE_BINARY_DIR}/include)
	install (TARGETS bpImarisWriter96
         ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
         LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
         RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin)
    
	install (TARGETS ImarisWriter_static
         ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
         LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib)    
else()
    message("Not installing ImarisWriter libraries. CMAKE_PROJECT_NAME is ${CMAKE_PROJECT_NAME}")
endif()


#INSTALL(TARGETS  klblib klblibs test_klbIO test_klbIO_C DESTINATION "${KLB_BINARY_FOLDER}" CONFIGURATIONS Release)