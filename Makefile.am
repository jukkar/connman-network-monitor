
plugindir = $(libdir)/gio/modules
plugin_cflags =	-Isrc -std=gnu99 \
	@DBUS_CFLAGS@ @GLIB_CFLAGS@ @GOBJECT_CFLAGS@ \
	@GIO_CFLAGS@ @CONCFLAGS@
plugin_ldflags = -no-undefined -module -avoid-version

connman_sources = src/connman-api.c src/connman-api.h

if MAINTAINER_MODE
build_plugindir = $(abs_top_srcdir)/plugins/.libs
else
build_plugindir = $(plugindir)
endif # MAINTAINER_MODE

plugin_LTLIBRARIES = libconnman-network-monitor.la
plugin_objects = $(libconnman_network_monitor_la_OBJECTS)
libconnman_network_monitor_la_CFLAGS = $(plugin_cflags)
libconnman_network_monitor_la_LIBADD = $(DBUS_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(GIO_LIBS)
libconnman_network_monitor_la_SOURCES = src/connman-network-monitor.c \
					$(connman_sources)

if TEST
test_cflags = -std=gnu99 -Wall -O2 -D_FORTIFY_SOURCE=2 \
				$(DBUS_CFLAGS) $(DEV_CFLAGS)

noinst_PROGRAMS = test/network-monitor

test_network_monitor_SOURCES = test/network-monitor.c
test_network_monitor_CFLAGS = @GLIB_CFLAGS@ @GIO_CFLAGS@ @GOBJECT_CFLAGS@
test_network_monitor_LDADD = @GLIB_LIBS@ @DBUS_LIBS@ @GIO_LIBS@ @GOBJECT_LIBS@

endif # TEST

MAINTAINERCLEANFILES = \
        Makefile.in config.h.in configure \
        install-sh ltmain.sh missing mkinstalldirs \
        config.log config.status config.guess config.sub \
        build-stamp compile depcomp acinclude.m4 aclocal.m4
