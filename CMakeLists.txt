cmake_minimum_required(VERSION 3.28)

set(INCLUDE_DIRS)
function(add_include_dir IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(${DEFAULT_DIR} "")
	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE ${DEFAULT_DIR} CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)
endfunction(add_include_dir)

set(LIBRARIES)
function(add_external_library IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(add_external_library)

function(link_external_library IDENTIFIER)
	set(PRETTYNAME ${IDENTIFIER})
	set(ARGV ${ARGN})
	list(LENGTH ARGV ARGC)
	if(${ARGC} GREATER 0)
		list(GET ARGV 0 PRETTYNAME)
	endif()
	string(TOUPPER ${IDENTIFIER} UIDENTIFIER)

	set(${DEFAULT_DIR} "")
	set(DEPENDENCY_${UIDENTIFIER}_INCLUDE ${DEFAULT_DIR} CACHE PATH "Path to ${PRETTYNAME} include directory.")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} DEPENDENCY_${UIDENTIFIER}_INCLUDE PARENT_SCOPE)

	set(DEPENDENCY_${UIDENTIFIER}_LIBRARY "" CACHE FILEPATH "Path to ${PRETTYNAME} library.")
	set(LIBRARIES ${LIBRARIES} DEPENDENCY_${UIDENTIFIER}_LIBRARY PARENT_SCOPE)
endfunction(link_external_library)

##### CONFIGURATION #####

set(PROJ_NAME util_udm)

project(${PROJ_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(PRECOMPILED_HEADER "stdafx")

option(UTIL_UDM_STATIC "Build as static library?" ON)

link_external_library(glm)
link_external_library(vfilesystem)
link_external_library(sharedutils)
link_external_library(mathutil)
link_external_library(lz4)

set(DEFINITIONS)

##### CONFIGURATION #####

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
if(${UTIL_UDM_STATIC})
	set(LIB_TYPE STATIC)
else()
	set(LIB_TYPE SHARED)
	add_definitions(-DUDM_SHARED)
	add_definitions(-DUDM_EXPORT)
	
	add_definitions(-DSHUTIL_STATIC)
	add_definitions(-DMUTIL_STATIC)
	add_definitions(-DVFILESYSTEM_STATIC)
endif()

foreach(def IN LISTS DEFINITIONS)
	add_definitions(-D${def})
endforeach(def)

add_library(${PROJ_NAME} ${LIB_TYPE})

target_sources(${PROJ_NAME}
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    include/udm.ixx
    include/udm_conversion.ixx
    include/udm_enums.ixx
    include/udm_exception.ixx
    include/udm_trivial_types.ixx
    include/udm_type_structs.ixx
    include/udm_types.ixx
    include/udm_type_structs.ixx
    include/udm_basic_types.ixx

    src/array.cpp
    src/ascii.cpp
    src/c_wrapper.cpp
    src/data.cpp
    src/element.cpp
    src/file.cpp
    src/iterators.cpp
    src/json.cpp
    src/linked_property_wrapper.cpp
    src/property.cpp
    src/property_wrapper.cpp
    src/util.cpp
)

if(MSVC)
	target_compile_options(${PROJ_NAME} PUBLIC "/bigobj")
else()
	target_link_options(${PROJ_NAME} PRIVATE "LINKER:--strip-all")
endif()
if(WIN32)
	target_compile_options(${PROJ_NAME} PRIVATE /wd4251)
	target_compile_options(${PROJ_NAME} PRIVATE /wd4996)
endif()

foreach(LIB IN LISTS LIBRARIES)
	target_link_libraries(${PROJ_NAME} ${${LIB}})
endforeach(LIB)

target_include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

foreach(INCLUDE_PATH IN LISTS INCLUDE_DIRS)
	target_include_directories(${PROJ_NAME} PRIVATE ${${INCLUDE_PATH}})
endforeach(INCLUDE_PATH)

set(TARGET_PROPERTIES LINKER_LANGUAGE CXX)
if(DEFINED PRECOMPILED_HEADER)
	include(PrecompiledHeader.cmake)
	add_precompiled_header(${PROJ_NAME} "src/${PRECOMPILED_HEADER}.h" c++17 FORCEINCLUDE)
endif()
set_target_properties(${PROJ_NAME} PROPERTIES ${TARGET_PROPERTIES})

add_dependencies(${PROJ_NAME} lz4 sharedutils mathutil vfilesystem)
