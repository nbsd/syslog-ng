# File: modules/secure-logging/crashrecovery/cr_common/CMakeLists.txt

# Define the source files for the library
set(CR_COMMON_SOURCES
	cr_crypto.c
	cr_pi_shared.c
	cr_randomstuff.c
	cr_crypto.h
	cr_pi_shared.h
	cr_randomstuff.h
)

add_library(cr_common_lib STATIC ${CR_COMMON_SOURCES})

set_target_properties(cr_common_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(cr_common_lib PRIVATE ${CMAKE_SOURCE_DIR})

target_compile_options(cr_common_lib PUBLIC
    ${GLIB_CFLAGS}
)

# Specify include directories (corresponding to AM_CPPFLAGS)
target_include_directories(cr_common_lib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
	${GLIB_INCLUDE_DIRS}
)

# Link cr_common against OpenSSL and Glib libraries
target_link_libraries(cr_common_lib PUBLIC
	utils_slog
	${OPENSSL_LIBRARIES}
	${GLIB_LIBRARIES}
)

# Install the SHARED library (.so file)
install(TARGETS cr_common_lib
    LIBRARY DESTINATION lib/syslog-ng/crashrecovery 
    COMPONENT cr_common_lib
)

