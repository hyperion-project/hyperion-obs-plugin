cmake_minimum_required(VERSION 3.5)
project(hyperion-obs)

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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(UNIX)
	include(GNUInstallDirs)
endif()

# Add project specific cmake modules (find, etc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# enable C++11; MSVC doesn't have c++11 feature switch
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
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
		SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${SUBDIRQT}/msvc2019_64")
	else()
		message(STATUS "Qt5_BASE_DIR: $ENV{Qt5_BASE_DIR}")
		message(STATUS "Add Qt5_BASE_DIR: $ENV{Qt5_BASE_DIR} to CMAKE_PREFIX_PATH")
		SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{Qt5_BASE_DIR}")
	endif()

	if (NOT DEFINED ENV{Qt5_DIR})
		if (NOT DEFINED ENV{Qt5_BASE_DIR})
			SET (qt_module_path "${SUBDIRQT}/msvc2019_64/lib/cmake/Qt5")
		else ()
			SET (qt_module_path "$ENV{Qt5_BASE_DIR}/lib/cmake/Qt5")
		endif()
	else()
		SET (qt_module_path "$ENV{Qt5_DIR}")
	endif()

	message(STATUS "Add ${qt_module_path} to CMAKE_MODULE_PATH")
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${qt_module_path}")
endif()

set(OBS_SOURCE "" CACHE STRING "Where is the source code of OBS-Studio located")
set(OBS_BUILD "" CACHE STRING "Where is the build directory of OBS-Studio located")

set(LIBOBS_INCLUDE_DIR "${OBS_SOURCE}/libobs")
set(OBS_FRONTEND_INCLUDE_DIR "${OBS_SOURCE}/UI/obs-frontend-api")

if (WIN32)
	set(LibObs_DIR "${OBS_BUILD}/libobs")
	set(LIBOBS_LIB "${OBS_BUILD}/libobs/$<CONFIGURATION>/obs.lib")
	set(OBS_FRONTEND_LIB "${OBS_BUILD}/UI/obs-frontend-api/$<CONFIGURATION>/obs-frontend-api.lib")
	set(PTHREAD_LIB "${OBS_BUILD}/deps/w32-pthreads/$<CONFIGURATION>/w32-pthreads.lib")
endif()

find_package(Libobs REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared flatbuffers library")
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Build Flatbuffers with tests")
add_subdirectory(external/flatbuffers)

export(TARGETS flatc FILE "${CMAKE_BINARY_DIR}/flatc_export.cmake")
set(FLATBUFFERS_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/flatbuffers/include")
set(FLATBUFFERS_FLATC_EXECUTABLE "$<TARGET_FILE:flatc>")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Flatbuffer_GENERATED_FBS
	hyperion_reply_generated.h
	hyperion_request_generated.h
)

set(Flatbuffer_FBS
	${CMAKE_SOURCE_DIR}/src/hyperion_reply.fbs
	${CMAKE_SOURCE_DIR}/src/hyperion_request.fbs
)

foreach(FBS_FILE ${Flatbuffer_FBS})
	get_filename_component(FLATC_OUTPUT ${FBS_FILE} NAME_WE)
	set(FLATC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FLATC_OUTPUT}_generated.h)

	add_custom_command(
		OUTPUT ${FLATC_OUTPUT}
		COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE}
		ARGS -c --no-includes --gen-mutable -o ${CMAKE_CURRENT_BINARY_DIR} ${FBS_FILE}
		DEPENDS ${FLATBUFFERS_FLATC_EXECUTABLE} ${FBS_FILE}
		COMMENT "Building C++ header for ${FBS_FILE}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endforeach()

set_source_files_properties(${Flatbuffer_GENERATED_FBS} PROPERTIES GENERATED TRUE)

FILE ( GLOB hyperion_obs_SOURCES "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/src/*.cpp" )

add_library(${PROJECT_NAME} MODULE
	${hyperion_obs_SOURCES}
	${Flatbuffer_GENERATED_FBS}
)

include_directories(
	${OBS_FRONTEND_INCLUDE_DIR}
	${LIBOBS_INCLUDE_DIR}
)

if(UNIX)
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

if(ARCH EQUAL 64)
	set(ARCH_NAME "x86_64")
else()
	set(ARCH_NAME "i686")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(UNIX)
	install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins)
	install(DIRECTORY locale/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/hyperion-obs/locale)
endif()
