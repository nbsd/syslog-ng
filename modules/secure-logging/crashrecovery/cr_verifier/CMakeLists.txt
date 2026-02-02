# File: modules/secure-logging/crashrecovery/cr_verifier/CMakeLists.txt

# -- Library Target (libcr_verifier_lib.so) ---

set(CR_VERIFIER_LIB_SOURCES
	cr_matrix.c
	cr_pi_verifier.c
	cr_plain_gauss_helper.c
)

add_library(cr_verifier_lib SHARED ${CR_VERIFIER_LIB_SOURCES})
# add_library(cr_verifier_lib SHARED $<TARGET_OBJECTS:cr_verifier_objs>)

target_compile_options(cr_verifier_lib PUBLIC -O3 ${GLIB_CFLAGS})

target_include_directories(cr_verifier_lib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cr_common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_link_libraries(cr_verifier_lib PUBLIC
    cr_common_lib
    ${GLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)


# -- Executable Target (cr_verifier) ---

set(CR_VERIFIER_EXE_SOURCES cr_verifier.c)

add_executable(cr_verifier ${CR_VERIFIER_EXE_SOURCES})

target_compile_options(cr_verifier PRIVATE -O3 ${GLIB_CFLAGS})

target_link_libraries(cr_verifier PRIVATE
	cr_common_lib
	cr_logger_lib
	cr_verifier_lib
	syslog-ng
	${GLIB_LIBRARIES}
	${OPENSSL_LIBRARIES}
	# syslog-ng-secure-logging
)

set_target_properties(cr_verifier PROPERTIES
	INSTALL_RPATH "$ORIGIN/../lib/syslog-ng/crashrecovery;$ORIGIN/../lib"   
	INSTALL_RPATH_USE_ORIGIN TRUE
)

# -- Install Rules ---

# Install the SHARED library (.so file)
# Destination: lib/syslog-ng/crashrecovery (matching the module structure)
install(TARGETS cr_verifier_lib
	LIBRARY DESTINATION lib/syslog-ng/crashrecovery 
	COMPONENT cr_verifier_lib
)

# Install the executable (CLI tool)
# Destination: bin (standard for executables)
install(TARGETS cr_verifier
	RUNTIME DESTINATION bin
	COMPONENT cr_verifier
)

