include $(top_srcdir)/Makefile.tool.am

#----------------------------------------------------------------------------
# Headers, etc
#----------------------------------------------------------------------------

pkginclude_HEADERS =

bin_SCRIPTS = \
	libsig_symbols \
	libsig_translator

noinst_HEADERS = \
	global.h

#----------------------------------------------------------------------------
# libsig-<platform>
#----------------------------------------------------------------------------

noinst_PROGRAMS  = libsig-@VGCONF_ARCH_PRI@-@VGCONF_OS@
if VGCONF_HAVE_PLATFORM_SEC
noinst_PROGRAMS += libsig-@VGCONF_ARCH_SEC@-@VGCONF_OS@
endif

LIBSIG_SOURCES_COMMON = \
	clo.c \
	debug.c \
	main.c \
	threads.c \
	tracking.c

libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_SOURCES      = \
	$(LIBSIG_SOURCES_COMMON)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS       = $(LTO_CFLAGS) \
	$(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_PRI_CAPS@)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_PRI_CAPS@)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_PRI@ \
	$(LINK) \
	$(libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS) \
	$(libsig_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS)

if VGCONF_HAVE_PLATFORM_SEC
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_SOURCES      = \
	$(LIBSIG_SOURCES_COMMON)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS       = $(LTO_CFLAGS) \
	$(AM_CFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_SEC_CAPS@)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_SEC_CAPS@)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_SEC@ \
	$(LINK) \
	$(libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS) \
	$(libsig_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS)
endif
