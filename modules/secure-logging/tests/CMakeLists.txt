# File: modules/secure-logging/tests/CMakeLists.txt

add_unit_test(
    LIBTEST CRITERION 
    TARGET test_secure_logging 
    DEPENDS 
        secure-logging 
        syslog-ng-secure-logging
        syslogformat
        cr_logger_lib
        cr_common_lib
        cr_verifier_lib
        cr_destination_lib
)

target_include_directories(test_secure_logging PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..                       # For utils_slog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../crashrecovery/cr_destination
    ${CMAKE_CURRENT_SOURCE_DIR}/../crashrecovery/cr_logger
    ${CMAKE_CURRENT_SOURCE_DIR}/../crashrecovery/cr_common
    ${CMAKE_CURRENT_SOURCE_DIR}/../crashrecovery/cr_verifier
)

