cmake_minimum_required(VERSION 3.5)
project(hyperion-obs)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(LIBOBS_INCLUDE_DIR "external/obs-studio/libobs")
include(cmake/FindLibObs.cmake)

find_package(LibObs REQUIRED)
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

include_directories("external/obs-studio/UI/obs-frontend-api")

FILE ( GLOB hyperion_obs_SOURCES "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/src/*.cpp" )

add_library(${PROJECT_NAME} MODULE
	${hyperion_obs_SOURCES}
	${Flatbuffer_GENERATED_FBS}
)
	
target_link_libraries(${PROJECT_NAME}
	libobs
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

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins)
install(DIRECTORY locale/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/hyperion-obs/locale)
