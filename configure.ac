AC_INIT([YAD], [1.0.0], [https://sourceforge.net/p/yad-dialog/tickets/], yad, [https://sourceforge.net/projects/yad-dialog/])
AC_CONFIG_AUX_DIR([build])

AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

AC_PREREQ([2.59])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.40.0])

AC_PROG_CC

GTK_DISABLE_DEPRECATED

dnl glib check needed for g_format_string()
PKG_CHECK_MODULES([GTK], [glib-2.0 >= 2.30 gtk+-3.0 >= 3.10.0 gtk+-unix-print-3.0])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

dnl rgb.txt
AC_ARG_WITH([rgb],
	[AS_HELP_STRING([--with-rgb=PATH],
			[set path to rgb.txt file (default - /etc/X11/rgb.txt)])],
	[with_rgb=$withval], [with_rgb=/etc/X11/rgb.txt])
AC_DEFINE_UNQUOTED([RGB_FILE], ["$with_rgb"], [Path to rgb.txt file])

dnl html widget
AC_ARG_ENABLE([html],
	[AS_HELP_STRING([--enable-html],
			[Build YAD with HTML widget])],
	[build_html=$enableval], [build_html=yes])
if test x$build_html = xyes; then
    PKG_CHECK_MODULES([HTML], [webkit2gtk-4.0], [have_html=yes], [have_html=no])
else
    have_html=no
fi
AC_SUBST([HTML_CFLAGS])
AC_SUBST([HTML_LIBS])
AM_CONDITIONAL([HTML], [test x$have_html = xyes])

if test x$have_html = xyes; then
    AC_DEFINE([HAVE_HTML], [1], [Define this if you have webkit installed])
fi

dnl gio support
AC_ARG_ENABLE([gio],
	[AS_HELP_STRING([--enable-gio],
			[Build YAD with GIO support])],
	[build_gio=$enableval], [build_gio=yes])
if test x$build_gio = xyes; then
    PKG_CHECK_MODULES([GIO], [gio-2.0], [have_gio=yes], [have_gio=no])
else
    have_gio=no
fi
AC_SUBST([GIO_CFLAGS])
AC_SUBST([GIO_LIBS])

if test x$have_gio = xyes; then
    AC_DEFINE([HAVE_GIO], [1], [Define this if you have glib gio framework])
fi

dnl spell check
AC_ARG_ENABLE([spell],
	[AS_HELP_STRING([--enable-spell],
			[Build YAD with Spell check support (default - no)])],
	[with_spell=$enableval], [with_spell=no])
if test x$with_spell = xyes; then
    PKG_CHECK_MODULES([SPELL], [gtkspell3-3.0], [have_spell=yes], [have_spell=no])
else
    have_spell=no
fi
AC_SUBST([SPELL_CFLAGS])
AC_SUBST([SPELL_LIBS])

if test x$have_spell = xyes; then
    AC_DEFINE([HAVE_SPELL], [1], [Define this if you need spell check support])
fi

dnl sourceview
AC_ARG_ENABLE([sourceview],
	[AS_HELP_STRING([--enable-sourceview],
			[Build YAD with GtkSourceView support (default - no)])],
	[with_sourceview=$enableval], [with_sourceview=no])
if test x$with_spell = xyes; then
    PKG_CHECK_MODULES([SOURCEVIEW], [gtksourceview-3.0], [have_sourceview=yes], [have_sourceview=no])
else
    have_sourceview=no
fi
AC_SUBST([SOURCEVIEW_CFLAGS])
AC_SUBST([SOURCEVIEW_LIBS])

if test x$have_sourceview = xyes; then
    AC_DEFINE([HAVE_SOURCEVIEW], [1], [Define this if you need GtkSourceView support])
fi

dnl yad symlink
AC_ARG_ENABLE([symlink],
	[AS_HELP_STRING([--enable-symlink],
			[Make yad symlink (default - no)])],
	[enable_symlink=$enableval], [enable_symlink=no])
AM_CONDITIONAL([SYMLINK], [test x$enable_symlink = xyes])

dnl icon browser
AC_ARG_ENABLE([icon-browser],
	[AS_HELP_STRING([--enable-icon-browser],
			[Build YAD icon browser (default - no)])],
	[build_ib=$enableval], [build_ib=no])
AM_CONDITIONAL([BUILD_IB], [test x$build_ib = xyes])

# *******************************
# Internationalization
# ******************************* 

GETTEXT_PACKAGE=yad
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext domain])

AM_GLIB_GNU_GETTEXT

# *******************************

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])

# *******************************

AC_CONFIG_FILES([
Makefile 
po/Makefile.in
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/96x96/Makefile
data/icons/128x128/Makefile
data/yad.m4
data/yad.spec
])

AC_OUTPUT

echo
echo "Build configuratioh:"
echo "  GTK+ version         - $with_gtk"
echo "  Path to rgb.txt      - $with_rgb"
echo "  HTML widget          - $have_html"
echo "  Spell checking       - $have_spell"
echo "  GtkSourceView        - $have_sourceview"
echo "  GIO support          - $have_gio"
echo "  Make yad symlink     - $enable_symlink"
echo "  Icon browser         - $build_ib"
echo
