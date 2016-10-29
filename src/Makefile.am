AM_CPPFLAGS =                           \
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	@DISABLE_DEPRECATED@

bin_PROGRAMS = yad3

yad3_SOURCES = 			\
	about.c			\
	calendar.c		\
	color.c			\
	dnd.c           	\
	file.c			\
	font.c			\
	form.c			\
	icons.c			\
	list.c			\
	multi-progress.c	\
	notebook.c		\
	notification.c		\
	option.c		\
	paned.c			\
	picture.c       \
	print.c			\
	progress.c		\
	scale.c			\
	text.c			\
	util.c			\
	main.c			\
	yad.h			\
	calendar.xpm

if HTML
yad3_SOURCES += html.c
endif

yad3_CFLAGS = $(GTK_CFLAGS) $(HTML_CFLAGS) $(SPELL_CFLAGS) $(SOURCEVIEW_CFLAGS)
yad3_LDADD = $(GTK_LIBS) $(HTML_LIBS) $(SPELL_LIBS) $(SOURCEVIEW_LIBS)

if BUILD_IB
bin_PROGRAMS += yad-icon-browser

yad_icon_browser_SOURCES = browser.c
yad_icon_browser_CFLAGS = $(GTK_CFLAGS)
yad_icon_browser_LDADD = $(GTK_LIBS)
endif

if SYMLINK
install-exec-hook:
	$(LN_S) yad3$(EXEEXT) $(DESTDIR)$(bindir)/yad$(EXEEXT)
endif
