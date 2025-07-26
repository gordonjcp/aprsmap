AC_INIT([aprsmap], [0.5], [gordonjcp@gjcp.net])
AC_CONFIG_SRCDIR([src/])
AM_INIT_AUTOMAKE([-Wall foreign])
#AM_SILENT_RULES([yes])

AC_PROG_CC
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([APRSMAP], [
    glib-2.0
    gtk+-3.0
    gdk-3.0
    osmgpsmap-1.0
    sqlite3
    libfap

])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT

