cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0071 NEW)
project(hyperion-obs)
file (STRINGS ".version" PLUGIN_VERSION)
message(STATUS "hyperion-obs version: ${PLUGIN_VERSION}")

# Macro to get path of first sub dir of a dir, used for MAC OSX lib/header searching
MACRO(FIRSTSUBDIR result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist "${curdir}/${child}")
	  	BREAK()
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable C++11; MSVC doesn't have c++11 feature switch
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	if (CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi")
	endif()
	if(COMPILER_SUPPORTS_CXX11)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
			message(STATUS "No support for C++11 detected. Compilation will most likely fail on your compiler")
	endif()
endif()

# MSVC options
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	if (NOT DEFINED ENV{Qt5_BASE_DIR})
		FIRSTSUBDIR(SUBDIRQT "C:/Qt")
		set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${SUBDIRQT}/msvc2019_64")
	else()
		message(STATUS "Qt5_BASE_DIR: $ENV{Qt5_BASE_DIR}")
		message(STATUS "Add Qt5_BASE_DIR: $ENV{Qt5_BASE_DIR} to CMAKE_PREFIX_PATH")
		set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{Qt5_BASE_DIR}")
	endif()

	if (NOT DEFINED ENV{Qt5_DIR})
		if (NOT DEFINED ENV{Qt5_BASE_DIR})
			set(qt_module_path "${SUBDIRQT}/msvc2019_64/lib/cmake/Qt5")
		else ()
			set(qt_module_path "$ENV{Qt5_BASE_DIR}/lib/cmake/Qt5")
		endif()
	else()
		file(TO_CMAKE_PATH $ENV{Qt5_DIR} Qt5_DIR)
		set(qt_module_path ${Qt5_DIR})
	endif()

	message(STATUS "Add ${qt_module_path} to CMAKE_MODULE_PATH")
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${qt_module_path}")
endif()

if(NOT EXISTS ${OBS_SOURCE})
	if (DEFINED ENV{OBS_SOURCE})
		set(OBS_SOURCE $ENV{OBS_SOURCE})
	endif()
endif()	

set(OBS_SOURCE "" CACHE PATH "Where is the source code of OBS-Studio located")
set(OBS_BUILD "" CACHE PATH "Where is the build directory of OBS-Studio located")

if(NOT EXISTS ${OBS_SOURCE})
	message(FATAL_ERROR "OBS_SOURCE was not specified. Please specify the path to the OBS source.")
endif()

if (WIN32 AND NOT EXISTS ${OBS_BUILD})
	message(FATAL_ERROR "OBS_BUILD was not specified. Please specify the path to the OBS build directory.")
endif()

set(LIBOBS_INCLUDE_DIR "${OBS_SOURCE}/libobs")
set(OBS_FRONTEND_INCLUDE_DIR "${OBS_SOURCE}/UI/obs-frontend-api")

if (WIN32)
	set(LibObs_DIR "${OBS_BUILD}/libobs")
	set(LIBOBS_LIB "${OBS_BUILD}/libobs/$<CONFIGURATION>/obs.lib")
	set(OBS_FRONTEND_LIB "${OBS_BUILD}/UI/obs-frontend-api/$<CONFIGURATION>/obs-frontend-api.lib")
	set(PTHREAD_LIB "${OBS_BUILD}/deps/w32-pthreads/$<CONFIGURATION>/w32-pthreads.lib")

	set(PLUGIN_AUTHOR "hyperion-project")
	set(PLUGIN_GIT "hyperion-obs-plugin")
	set(PLUGIN_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
	set(PLUGIN_ICON "${CMAKE_SOURCE_DIR}/resources/installer.ico")
endif()

include(cmake/FindLibObs.cmake)
find_package(LibObs REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

option(GLOBAL_INSTALLATION "Whether to install for all users (default: OFF)" OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (UNIX)
    if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif()
endif()

if (WIN32)
	configure_file(package/Windows.iss.in ${CMAKE_BINARY_DIR}/Windows.iss)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared flatbuffers library")
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Build Flatbuffers with tests")
add_subdirectory(external/flatbuffers)

export(TARGETS flatc FILE "${CMAKE_BINARY_DIR}/flatc_export.cmake")
set(FLATBUFFERS_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/flatbuffers/include")
set(FLATBUFFERS_FLATC_EXECUTABLE "$<TARGET_FILE:flatc>")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${FLATBUFFERS_INCLUDE_DIRS}
	${OBS_FRONTEND_INCLUDE_DIR}
	${LIBOBS_INCLUDE_DIR}
)

set(FLATBUFFERS_SCHEMA "${CMAKE_SOURCE_DIR}/src/FlatBufferSchema.fbs")
set(FLATBUFFERS_HEADER "${CMAKE_BINARY_DIR}/FlatBufferSchema_generated.h")

add_custom_command(
	OUTPUT ${FLATBUFFERS_HEADER}
	COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE}
	ARGS -c --no-includes --gen-mutable -o ${CMAKE_CURRENT_BINARY_DIR} ${FLATBUFFERS_SCHEMA}
	DEPENDS ${FLATBUFFERS_FLATC_EXECUTABLE} ${FLATBUFFERS_SCHEMA}
	COMMENT "Building Flatbuffer header file"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_source_files_properties(${FLATBUFFERS_HEADER} PROPERTIES GENERATED TRUE)

FILE ( GLOB hyperion_obs_SOURCES "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/src/*.cpp" )

add_library(${PROJECT_NAME} MODULE
	${hyperion_obs_SOURCES}
	${FLATBUFFERS_HEADER}
)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} libobs)
endif()

if (WIN32)
	target_link_libraries(${PROJECT_NAME} ${PTHREAD_LIB} ${LIBOBS_LIB})
endif()

target_link_libraries(${PROJECT_NAME}
	${OBS_FRONTEND_LIB}
	flatbuffers
	Qt5::Core
	Qt5::Widgets
	Qt5::Network
)

if (UNIX)
	include(GNUInstallDirs)
	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

	if (GLOBAL_INSTALLATION)
		# For *.deb installer or install for all user
		install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins)
		install(DIRECTORY locale/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/${PROJECT_NAME}/locale)
	else()
		# Installs into home directory
		install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${PROJECT_NAME}/bin/64bit")
		install(DIRECTORY locale/ DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${PROJECT_NAME}/data/locale")
	endif()
endif()
