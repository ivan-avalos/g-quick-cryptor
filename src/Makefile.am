## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui
ui_DATA = g_quick_cryptor.ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(G_QUICK_CRYPTOR_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = g_quick_cryptor

g_quick_cryptor_SOURCES = \
	main.c \
	fileman.c \
	fileman.h \
	quickman.c \
	quickman.h


g_quick_cryptor_LDFLAGS =

g_quick_cryptor_LDADD = $(G_QUICK_CRYPTOR_LIBS)


if PLATFORM_WIN32
g_quick_cryptor_LDFLAGS += -Wl,--export-all-symbols
else
g_quick_cryptor_LDFLAGS += -Wl,--export-dynamic
endif

if NATIVE_WIN32
g_quick_cryptor_LDFLAGS += -mwindows
endif


EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

