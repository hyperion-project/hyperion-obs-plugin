find_program(FLATBUFFERS_FLATC_EXECUTABLE NAMES flatc REQUIRED)
find_package(Flatbuffers REQUIRED)

set(FLATBUFFERS_FLATC_EXECUTABLE ${FLATBUFFERS_FLATC_EXECUTABLE} PARENT_SCOPE)
set(FLATBUFFERS_INCLUDE_DIRS ${FLATBUFFERS_INCLUDE_DIRS} PARENT_SCOPE)
include_directories(${FLATBUFFERS_INCLUDE_DIRS})

message(STATUS "Using flatbuffers compiler: " ${FLATBUFFERS_FLATC_EXECUTABLE})

function(compile_flatbuffer_schema SRC_FBS OUTPUT_DIR)
	string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${SRC_FBS})
	set_property(SOURCE ${GEN_HEADER} PROPERTY SKIP_AUTOMOC ON)

	add_custom_command(
		OUTPUT ${GEN_HEADER}
		COMMAND "${FLATBUFFERS_FLATC_EXECUTABLE}" -c --no-includes --gen-mutable
				--gen-object-api
				-o "${OUTPUT_DIR}"
				"${SRC_FBS}"
		DEPENDS "${FLATBUFFERS_FLATC_EXECUTABLE}" ${SRC_FBS})
endfunction()