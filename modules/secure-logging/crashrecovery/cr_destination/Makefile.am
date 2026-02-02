# File: modules/secure-logging/crashrecovery/cr_destination/Makefile.am

if ENABLE_SLOG

module_LTLIBRARIES += modules/secure-logging/crashrecovery/cr_destination/libcr_destination.la

modules_secure_logging_crashrecovery_cr_destination_libcr_destination_la_SOURCES = \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-grammar.y \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-parser.c \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-parser.h \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-plugin.c \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination.h \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination.c \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination_worker.h \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination_worker.c

BUILT_SOURCES += \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-grammar.y \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-grammar.c \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-grammar.h

EXTRA_DIST += \
	modules/secure-logging/crashrecovery/cr_destination/cr_destination-grammar.ym

modules_secure_logging_crashrecovery_cr_destination_libcr_destination_la_CPPFLAGS  = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_destination \
	-I$(top_builddir)/modules/secure-logging/crashrecovery/cr_destination \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_logger \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_common

modules_secure_logging_crashrecovery_cr_destination_libcr_destination_la_LIBADD = \
	$(MODULE_DEPS_LIBS) \
	modules/secure-logging/crashrecovery/cr_logger/libcr_logger.la \
	modules/secure-logging/crashrecovery/cr_common/libcr_common.la \
	$(top_builddir)/modules/secure-logging/libutils_slog.la

modules_secure_logging_crashrecovery_cr_destination_libcr_destination_la_DEPENDENCIES = \
	$(MODULE_DEPS_LIBS) \
	modules/secure-logging/crashrecovery/cr_logger/libcr_logger.la \
	modules/secure-logging/crashrecovery/cr_common/libcr_common.la \
	$(top_builddir)/modules/secure-logging/libutils_slog.la

modules_secure_logging_crashrecovery_cr_destination_libcr_destination_la_LDFLAGS = $(MODULE_LDFLAGS)

modules/secure-logging/crashrecovery/cr_destination modules/secure-logging/crashrecovery/cr_destination/ mod-cr_destination: modules/secure-logging/crashrecovery/cr_destination/libcr_destination.la

endif

EXTRA_DIST += modules/secure-logging/crashrecovery/cr_destination/CMakeLists.txt

.PHONY: modules/secure-logging/crashrecovery/cr_destination/ mod-cr_destination

