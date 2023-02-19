# --- Detect if the plugin is build out of tree or not ---
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  set(BUILD_OUT_OF_TREE OFF)
else()
  set(BUILD_OUT_OF_TREE ON)
  cmake_minimum_required(VERSION 3.18)
endif()

project(hyperion-obs VERSION 1.0.2)
set(PROJECT_FULL_NAME "hyperion-obs")

message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")
add_library(${PROJECT_NAME} MODULE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

if(BUILD_OUT_OF_TREE)
	find_package(libobs REQUIRED)
	find_package(obs-frontend-api REQUIRED)
	include(cmake/ObsPluginHelpers.cmake)
	set(OBS_FRONTEND_API_NAME "obs-frontend-api")
else()
	set(OBS_FRONTEND_API_NAME "frontend-api")
endif()

# Import QT
find_qt(COMPONENTS Widgets Core Network REQUIRED)
target_link_libraries(${PROJECT_NAME} Qt::Core Qt::Widgets Qt::Network)
set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

# Compile Flatbuffers library or use Flatbuffers library from system
option(USE_SYSTEM_FLATBUFFERS_LIBS "Use Flatbuffers library from system (default: OFF)" OFF)

if(USE_SYSTEM_FLATBUFFERS_LIBS)
	find_program(FLATBUFFERS_FLATC_EXECUTABLE NAMES flatc REQUIRED)
	find_package(Flatbuffers REQUIRED)
else()
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared flatbuffers library")
	set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Build Flatbuffers with tests")
	add_subdirectory(external/flatbuffers)

	# export the flatc compiler (from cross or native compile)
	set(IMPORT_FLATC "" CACHE STRING "flatc export file (flatc_export.cmake) from a native build")
	if (IMPORT_FLATC)
		include(${IMPORT_FLATC})
	else()
		export(TARGETS flatc FILE "${CMAKE_CURRENT_BINARY_DIR}/flatc_export.cmake")
	endif()

	# define the include for the flatbuffers library
	set(FLATBUFFERS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers/include")

	# define the flatc executable (from cross or native compile)
	set(FLATBUFFERS_FLATC_EXECUTABLE "$<TARGET_FILE:flatc>" CACHE STRING "flatc compiler")
endif()

# Compile Flatbuffers schema file
set(FLATBUFFERS_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/src/FlatBufferSchema.fbs")
set(FLATBUFFERS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/FlatBufferSchema_generated.h")

add_custom_command(
	OUTPUT ${FLATBUFFERS_HEADER}
	COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE}
	ARGS -c --no-includes --gen-mutable -o ${CMAKE_CURRENT_BINARY_DIR} ${FLATBUFFERS_SCHEMA}
	DEPENDS ${FLATBUFFERS_FLATC_EXECUTABLE} ${FLATBUFFERS_SCHEMA}
	COMMENT "Building Flatbuffer header file"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_source_files_properties(${FLATBUFFERS_HEADER} PROPERTIES GENERATED TRUE)

# Set project includes
target_include_directories(${PROJECT_NAME} PUBLIC
 	${CMAKE_CURRENT_BINARY_DIR}
 	${FLATBUFFERS_INCLUDE_DIRS}
)

FILE ( GLOB hyperion_obs_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui" )

# Link frontend-api and libobs with hyperion-obs
target_link_libraries(${PROJECT_NAME}
	OBS::${OBS_FRONTEND_API_NAME}
	OBS::libobs
	flatbuffers
)

# Set Flatbuffers header files and project source files
target_sources(${PROJECT_NAME} PRIVATE
	${hyperion_obs_SOURCES}
	${FLATBUFFERS_HEADER}
)

if(OS_WINDOWS)
	set(PLUGIN_AUTHOR "hyperion-project")
	set(PLUGIN_GIT "hyperion-obs-plugin")
	set(PLUGIN_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
	set(PLUGIN_ICON "${CMAKE_SOURCE_DIR}/resources/installer.ico")
	configure_file(package/Windows.iss.in ${PROJECT_BINARY_DIR}/Windows.iss)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
	target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
	target_link_libraries(${PROJECT_NAME} w32-pthreads)
elseif(OS_MACOS)
	set(MACOS_PACKAGE_UUID "207061E8-795D-4FC8-9BB6-30DF9291E34F")
	set(MACOS_INSTALLER_UUID "A4BEF905-CFE5-45F7-B29F-561FD3838103")
	set(MACOS_BUNDLEID "com.hyperion-project.hyperion-obs")
	set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
	set(MACOSX_PLUGIN_BUNDLE_VERSION "${PROJECT_VERSION}")
	set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")
	configure_file(package/macOS.pkgproj.in ${CMAKE_BINARY_DIR}/macOS.pkgproj)
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
else()
	if(BUILD_OUT_OF_TREE)
		if(NOT LIB_OUT_DIR)
			set(LIB_OUT_DIR "/lib/obs-plugins")
		endif()

		if(NOT DATA_OUT_DIR)
			set(DATA_OUT_DIR "/share/obs/obs-plugins/${PROJECT_NAME}")
		endif()

		set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
		install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_OUT_DIR})
		install(DIRECTORY data/locale DESTINATION ${CMAKE_INSTALL_PREFIX}/${DATA_OUT_DIR})
	else()
		target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
		set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "plugins/hyperion-obs")
	endif()
endif()

setup_plugin_target(${PROJECT_NAME})
