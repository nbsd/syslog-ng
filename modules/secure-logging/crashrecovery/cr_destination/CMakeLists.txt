# File: modules/secure-logging/crashrecovery/cr_destination/CMakeLists.txt

# Define the sources that are NOT the grammar-generated files
set(CR_DESTINATION_LIB_SOURCES
    "cr_destination.c"
    "cr_destination.h"
    "cr_destination_worker.c"
    "cr_destination_worker.h"
)
#   "cr_destination-parser.c"
#   "cr_destination-parser.h"

add_library(cr_destination_lib STATIC ${CR_DESTINATION_LIB_SOURCES})

set_target_properties(cr_destination_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(cr_destination_lib PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cr_destination_lib PUBLIC
	cr_common_lib
	cr_logger_lib
	syslog-ng
)

add_module(
	TARGET cr_destination
	GRAMMAR cr_destination-grammar
	SOURCES "cr_destination-plugin.c"
            "cr_destination-parser.c"
	DEPENDS cr_destination_lib ${MODULE_DEPS_LIBS}
)
