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
HFILE_GLOB=$(top_srcdir)/src/*.h $(top_srcdir)/src/widgets/*.h  
CFILE_GLOB=$(top_srcdir)/src/*.c $(top_srcdir)/src/widgets/*.c  

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
	-I$(top_srcdir)/src                     	\
	-I$(top_srcdir)/src/widgets
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_HILDON_WIDGETS_CFLAGS) \
	$(MODEST_OGS_CFLAGS) \
	$(MODEST_HILDON_HELP_CFLAGS) \
	$(MODEST_HILDON_NOTIFY_CFLAGS) \
	$(MODEST_MOZEMBED_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-DPREFIX=\"@prefix@\" \
	-DMAEMO_CHANGES \
	-DMODEST_LOCALEDIR=\"$(MODEST_LOCALEDIR)\" \
	-DMODEST_PLATFORM=\"$(MODEST_PLATFORM)\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\"$(datadir)/modest/ui/\"

GTKDOC_LIBS= \
	$(MODEST_GSTUFF_LIBS) 			        \
	$(MODEST_MOZEMBED_LIBS) 			\
	$(MODEST_PLATFORM_LIBS)	                        \
	$(MODEST_HILDON_WIDGETS_LIBS)			\
	$(MODEST_HILDON_HELP_LIBS)			\
	$(MODEST_HILDON_NOTIFY_LIBS)			\
	$(MODEST_OGS_LIBS)				\
	${easysetupmaybe}				\
	$(MODEST_PLATFORM)/libmodest-ui.la		\
	widgets/libmodest-widgets.la                    \
	dbus_api/libmodest-dbus-api.la			\
	${top_builddir}/src/modest-modest-account-mgr-helpers.o \
	${top_builddir}/src/modest-modest-account-mgr.o \
	${top_builddir}/src/modest-modest-address-book-dummy.o \
	${top_builddir}/src/modest-modest-cache-mgr.o \
	${top_builddir}/src/modest-modest-conf.o \
	${top_builddir}/src/modest-modest-dimming-rule.o \
	${top_builddir}/src/modest-modest-dimming-rules-group.o \
	${top_builddir}/src/modest-modest-email-clipboard.o \
	${top_builddir}/src/modest-modest-formatter.o \
	${top_builddir}/src/modest-modest-init.o \
	${top_builddir}/src/modest-modest-local-folder-info.o \
	${top_builddir}/src/modest-modest-mail-operation.o \
	${top_builddir}/src/modest-modest-mail-operation-queue.o \
	${top_builddir}/src/modest-modest-main.o \
	${top_builddir}/src/modest-modest-marshal.o \
	${top_builddir}/src/modest-modest-pair.o \
	${top_builddir}/src/modest-modest-progress-object.o \
	${top_builddir}/src/modest-modest-protocol-info.o \
	${top_builddir}/src/modest-modest-runtime.o \
	${top_builddir}/src/modest-modest-search.o \
	${top_builddir}/src/modest-modest-signal-mgr.o \
	${top_builddir}/src/modest-modest-singletons.o \
	${top_builddir}/src/modest-modest-stream-text-to-html.o \
	${top_builddir}/src/modest-modest-text-utils.o \
	${top_builddir}/src/modest-modest-tny-account.o \
	${top_builddir}/src/modest-modest-tny-account-store.o \
	${top_builddir}/src/modest-modest-tny-folder.o \
	${top_builddir}/src/modest-modest-tny-local-folders-account.o \
	${top_builddir}/src/modest-modest-tny-msg.o \
	${top_builddir}/src/modest-modest-tny-outbox-account.o \
	${top_builddir}/src/modest-modest-tny-platform-factory.o \
	${top_builddir}/src/modest-modest-tny-send-queue.o \
	${top_builddir}/src/modest-modest-transport-account-decorator.o \
	${top_builddir}/src/modest-modest-ui-actions.o \
	${top_builddir}/src/modest-modest-ui-dimming-manager.o \
	${top_builddir}/src/modest-modest-ui-dimming-rules.o \
	${top_builddir}/src/modest-modest-widget-memory.o \
	${top_builddir}/src/widgets/modest-account-view.o \
	${top_builddir}/src/widgets/modest-attachments-view.o \
	${top_builddir}/src/widgets/modest-attachment-view.o \
	${top_builddir}/src/widgets/modest-combo-box.o \
	${top_builddir}/src/widgets/modest-details-dialog.o \
	${top_builddir}/src/widgets/modest-dnd.o \
	${top_builddir}/src/widgets/modest-folder-view.o \
	${top_builddir}/src/widgets/modest-global-settings-dialog.o \
	${top_builddir}/src/widgets/modest-gtkhtml-mime-part-view.o \
	${top_builddir}/src/widgets/modest-gtkhtml-msg-view.o \
	${top_builddir}/src/widgets/modest-hbox-cell-renderer.o \
	${top_builddir}/src/widgets/modest-header-view.o \
	${top_builddir}/src/widgets/modest-header-view-observer.o \
	${top_builddir}/src/widgets/modest-header-view-render.o \
	${top_builddir}/src/widgets/modest-isearch-view.o \
	${top_builddir}/src/widgets/modest-limit-retrieve-combo-box.o \
	${top_builddir}/src/widgets/modest-mail-header-view.o \
	${top_builddir}/src/widgets/modest-mime-part-view.o \
	${top_builddir}/src/widgets/modest-msg-view.o \
	${top_builddir}/src/widgets/modest-recpt-editor.o \
	${top_builddir}/src/widgets/modest-recpt-view.o \
	${top_builddir}/src/widgets/modest-retrieve-combo-box.o \
	${top_builddir}/src/widgets/modest-scroll-text.o \
	${top_builddir}/src/widgets/modest-secureauth-combo-box.o \
	${top_builddir}/src/widgets/modest-serversecurity-combo-box.o \
	${top_builddir}/src/widgets/modest-tny-stream-gtkhtml.o \
	${top_builddir}/src/widgets/modest-validating-entry.o \
	${top_builddir}/src/widgets/modest-vbox-cell-renderer.o \
	${top_builddir}/src/widgets/modest-window-mgr.o \
	${top_builddir}/src/widgets/modest-window.o \
	${top_builddir}/src/widgets/modest-zoomable.o 

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += 
