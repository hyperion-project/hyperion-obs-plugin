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

if(APPLE AND EXISTS /usr/local/opt/qt@5)
	list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@5")
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

if (WIN32 OR APPLE AND NOT EXISTS ${OBS_BUILD})
	message(FATAL_ERROR "OBS_BUILD was not specified. Please specify the path to the OBS build directory.")
endif()

set(LIBOBS_INCLUDE_DIR "${OBS_SOURCE}/libobs")
set(OBS_FRONTEND_INCLUDE_DIR "${OBS_SOURCE}/UI/obs-frontend-api")

if (WIN32)
	set(LIBOBS_LIB "${OBS_BUILD}/libobs/$<CONFIGURATION>/obs.lib")
	set(OBS_FRONTEND_LIB "${OBS_BUILD}/UI/obs-frontend-api/$<CONFIGURATION>/obs-frontend-api.lib")
	set(PTHREAD_LIB "${OBS_BUILD}/deps/w32-pthreads/$<CONFIGURATION>/w32-pthreads.lib")

	set(PLUGIN_AUTHOR "hyperion-project")
	set(PLUGIN_GIT "hyperion-obs-plugin")
	set(PLUGIN_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
	set(PLUGIN_ICON "${CMAKE_SOURCE_DIR}/resources/installer.ico")
endif()

if (APPLE)
	set(LIBOBS_LIB "${OBS_BUILD}/libobs/libobs.dylib")
	set(OBS_FRONTEND_LIB "${OBS_BUILD}/UI/obs-frontend-api/libobs-frontend-api.dylib")

	set(BUNDLE_IDENTIFIER "com.hyperion-project.hyperion-obs")

endif()

include(cmake/FindLibObs.cmake)
find_package(LibObs REQUIRED)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)
message(STATUS "Qt Version: ${Qt5Core_VERSION}")

if (WIN32 OR APPLE)
	if ("${Qt5Core_VERSION}" VERSION_LESS "5.15.2")
		message( FATAL_ERROR "Your Qt version is to old! Minimum required 5.15.2" )
	endif()
endif()

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

if (APPLE)
	configure_file(package/macOS.pkgproj.in ${CMAKE_BINARY_DIR}/macOS.pkgproj)
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

if (UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} libobs)
endif()

if (WIN32)
	target_link_libraries(${PROJECT_NAME} ${PTHREAD_LIB} ${LIBOBS_LIB})
endif()

if (APPLE)
	target_link_libraries(${PROJECT_NAME} ${LIBOBS_LIB})
endif()

target_link_libraries(${PROJECT_NAME}
	${OBS_FRONTEND_LIB}
	flatbuffers
	Qt5::Core
	Qt5::Widgets
	Qt5::Network
)

if (UNIX AND NOT APPLE)
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

if (APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND install_name_tool
			-change @rpath/libobs.0.dylib @executable_path/../Frameworks/libobs.0.dylib
			-change @rpath/libobs-frontend-api.dylib @executable_path/../Frameworks/libobs-frontend-api.dylib
			-change ${CMAKE_PREFIX_PATH}/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore
			-change ${CMAKE_PREFIX_PATH}/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui
			-change ${CMAKE_PREFIX_PATH}/lib/QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork
			-change ${CMAKE_PREFIX_PATH}/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets
			$<TARGET_FILE:${PROJECT_NAME}>
	)

	# Installs into home directory
	install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/${PROJECT_NAME}/bin/")
	install(DIRECTORY locale/ DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/${PROJECT_NAME}/data/locale")
endif()
