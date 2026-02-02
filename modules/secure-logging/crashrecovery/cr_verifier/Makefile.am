# File: modules/secure-logging/crashrecovery/cr_verifier/Makefile.am

if ENABLE_SLOG

bin_PROGRAMS += modules/secure-logging/crashrecovery/cr_verifier/cr_verifier

modules_secure_logging_crashrecovery_cr_verifier_cr_verifier_SOURCES = \
	modules/secure-logging/crashrecovery/cr_verifier/cr_verifier.c

modules_secure_logging_crashrecovery_cr_verifier_cr_verifier_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/modules/secure-logging \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_common \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_verifier

modules_secure_logging_crashrecovery_cr_verifier_cr_verifier_CFLAGS = \
	$(AM_CFLAGS) -O3 \
	$(GLIB_CFLAGS) \
	$(OPENSSL_CFLAGS)

modules_secure_logging_crashrecovery_cr_verifier_cr_verifier_LDADD = \
	$(top_builddir)/modules/secure-logging/crashrecovery/cr_verifier/libcr_verifier.la \
	$(top_builddir)/modules/secure-logging/crashrecovery/cr_common/libcr_common.la \
	$(top_builddir)/modules/secure-logging/libutils_slog.la \
	$(top_builddir)/lib/libsyslog-ng.la \
	-lm \
	$(GLIB_LIBS) \
	$(MODULE_DEPS_LIBS) \
	$(OPENSSL_LIBS) \
	$(LIBSYSLOG_SECURE_LOGGING)


# Static Library Target: libcr_verifier.la

noinst_LTLIBRARIES += modules/secure-logging/crashrecovery/cr_verifier/libcr_verifier.la

modules_secure_logging_crashrecovery_cr_verifier_libcr_verifier_la_SOURCES = \
	modules/secure-logging/crashrecovery/cr_verifier/cr_matrix.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_matrix.c \
	modules/secure-logging/crashrecovery/cr_verifier/cr_pi_types.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_pi_verifier.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_pi_verifier.c \
	modules/secure-logging/crashrecovery/cr_verifier/cr_result.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_matrix.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_plain_gauss_helper.h \
	modules/secure-logging/crashrecovery/cr_verifier/cr_plain_gauss_helper.c

modules_secure_logging_crashrecovery_cr_verifier_libcr_verifier_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/modules/secure-logging \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_common \
	-I$(top_srcdir)/modules/secure-logging/crashrecovery/cr_verifier

modules_secure_logging_crashrecovery_cr_verifier_libcr_verifier_la_CFLAGS = \
	$(AM_CFLAGS) -O3 \
	$(GLIB_CFLAGS) \
	$(OPENSSL_CFLAGS)

endif

EXTRA_DIST += modules/secure-logging/crashrecovery/cr_verifier/CMakeLists.txt

