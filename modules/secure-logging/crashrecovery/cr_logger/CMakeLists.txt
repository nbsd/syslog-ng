# File: modules/secure-logging/crashrecovery/cr_logger/CMakeLists.txt

# -- Library Target (libcr_logger_lib.so) ---

# Define the library target with a unique name: 'cr_logger_lib'
set(CR_LOGGER_LIB_SOURCES cr_pi_logger.c)

add_library(cr_logger_lib STATIC ${CR_LOGGER_LIB_SOURCES})

# Crucial: Enable Position Independent Code for static libs going into modules
set_target_properties(cr_logger_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(cr_logger_lib PUBLIC
    ${GLIB_CFLAGS}
)

# Specify other required include directories
target_include_directories(cr_logger_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Link cr_logger_lib to cr_common (transitive dependency)
target_link_libraries(cr_logger_lib PUBLIC
    cr_common_lib # Inherits cr_common's PUBLIC headers
    ${GLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)


# -- Executable Target (cr_logger) ---

# Define the executable target using the desired final name: 'cr_logger'
set(CR_LOGGER_EXE_SOURCES cr_logger.c)

add_executable(cr_logger ${CR_LOGGER_EXE_SOURCES})

target_compile_options(cr_logger PRIVATE -O3 ${GLIB_CFLAGS})

target_link_libraries(cr_logger PRIVATE
    cr_common_lib
    cr_logger_lib
    syslog-ng
    ${GLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)
# syslog-ng-secure-logging

set_target_properties(cr_logger PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH_USE_ORIGIN TRUE
)


# -- Install Rules ---

# Install the SHARED library (.so file)
# Destination: lib/syslog-ng/crashrecovery (matching the module structure)
install(TARGETS cr_logger_lib
    LIBRARY DESTINATION lib/syslog-ng/crashrecovery 
    COMPONENT cr_logger_lib
)

# Install the executable (CLI tool)
# Destination: bin (standard for executables)
install(TARGETS cr_logger
    RUNTIME DESTINATION bin
    COMPONENT cr_logger
)

