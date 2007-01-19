## Process this file with automake to produce Makefile.in

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=modest

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../gtk
DOC_SOURCE_DIR=$(top_srcdir)/src

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS= \
	--source-dir=$(top_srcdir)/src/widgets \
	--source-dir=$(top_srcdir)/src/gtk \
	--source-dir=$(top_srcdir)/src

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/*.c
HFILE_GLOB=$(top_srcdir)/src/*.h $(top_srcdir)/src/widgets/*.h  $(top_srcdir)/src/gtk/*.h 
CFILE_GLOB=$(top_srcdir)/src/*.c $(top_srcdir)/src/widgets/*.c  $(top_srcdir)/src/gtk/*.c 

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
IGNORE_HFILES=

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files= \
	modest-preface.sgml \
	modest-architecture.sgml \
	modest-design.sgml \
	modest-coding-guidelines.sgml

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
INCLUDES=						\
	-I$(top_srcdir)/src $(MODEST_GSTUFF_CFLAGS)	\
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/gtk				\
	-I$(top_srcdir)/src/widgets


GTKDOC_LIBS= \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)	\
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		\
	$(MODEST_GSTUFF_LIBS)			\
	$(top_builddir)/src/gtk/libmodest-ui.la		\
	$(top_builddir)/src/widgets/libmodest-widgets.la\
	${top_builddir}/src/modest-account-mgr.o \
	${top_builddir}/src/modest-conf.o \
	${top_builddir}/src/modest-formatter.o \
	${top_builddir}/src/modest-icon-factory.o \
	${top_builddir}/src/modest-local-folder-info.o \
	${top_builddir}/src/modest-mail-operation.o \
	${top_builddir}/src/modest-mail-operation-queue.o \
	${top_builddir}/src/modest-marshal.o \
	${top_builddir}/src/modest-pair.o \
	${top_builddir}/src/modest-presets.o \
	${top_builddir}/src/modest-protocol-info.o \
	${top_builddir}/src/modest-text-utils.o \
	${top_builddir}/src/modest-tny-account-store.o \
	${top_builddir}/src/modest-tny-folder.o \
	${top_builddir}/src/modest-tny-msg-actions.o \
	${top_builddir}/src/modest-tny-platform-factory.o \
	${top_builddir}/src/modest-ui.o \
	${top_builddir}/src/modest-widget-factory.o \
	${top_builddir}/src/modest-widget-memory.o

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += 
