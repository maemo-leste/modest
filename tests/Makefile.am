#
# Makefile.am
#

MAINTAINERCLEANFILES    = Makefile.in

TESTS_ENVIRONMENT       = top_builddir=$(top_builddir)  \
			  CK_FORK=yes 			\
			  CK_VERBOSITY=verbose

TESTS                   =                           \
                        check_folder-xfer           \
			check_text-utils            \
			check_modest-conf           \
			check_update-account        \
			check_account-mgr           

noinst_PROGRAMS=				    \
			check_folder-xfer           \
			check_modest-conf	    \
			check_text-utils            \
			check_update-account        \
			check_account-mgr

INCLUDES=\
	@CHECK_CFLAGS@ \
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-I${top_srcdir}/src \
	-DPREFIX=\"@prefix@\" \
	-DMODESTLOCALEDIR=\""$(modestlocaledir)"\"
objects=\
	@CHECK_LIBS@ \
	$(MODEST_GSTUFF_LIBS) \
        $(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
        $(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
        ${top_srcdir}/src/$(MODEST_PLATFORM)/libmodest-ui.la \
        ${top_srcdir}/src/widgets/libmodest-widgets.la \
        ${top_srcdir}/src/modest-account-mgr.o \
        ${top_srcdir}/src/modest-account-mgr-helpers.o \
        ${top_srcdir}/src/modest-account-mgr-priv.o \
        ${top_srcdir}/src/modest-conf.o \
        ${top_srcdir}/src/modest-cache-mgr.o \
        ${top_srcdir}/src/modest-protocol-info.o \
        ${top_srcdir}/src/modest-formatter.o \
	${top_srcdir}/src/modest-runtime.o \
	${top_srcdir}/src/modest-singletons.o \
        ${top_srcdir}/src/modest-pair.o \
        ${top_srcdir}/src/modest-marshal.o \
	${top_srcdir}/src/modest-widget-memory.o \
        ${top_srcdir}/src/modest-tny-account-store.o \
        ${top_srcdir}/src/modest-tny-platform-factory.o \
	${top_srcdir}/src/modest-widget-factory.o \
        ${top_srcdir}/src/modest-tny-send-queue.o \
        ${top_srcdir}/src/modest-mail-operation-queue.o \
        ${top_srcdir}/src/modest-mail-operation.o \
        ${top_srcdir}/src/modest-tny-account.o \
	${top_srcdir}/src/modest-tny-msg.o \
        ${top_srcdir}/src/modest-local-folder-info.o \
        ${top_srcdir}/src/modest-tny-folder.o \
        ${top_srcdir}/src/modest-text-utils.o


check_folder_xfer_SOURCES=\
	check_folder-transfer.c
check_folder_xfer_LDADD = $(objects)

check_update_account_SOURCES=\
	check_update-account.c
check_update_account_LDADD = $(objects)

check_modest_conf_SOURCES=\
	check_modest-conf.c
check_modest_conf_LDADD = $(objects)

check_text_utils_SOURCES=\
	check_text-utils.c
check_text_utils_LDADD = $(objects)

check_account_mgr_SOURCES=\
	check_account-mgr.c
check_account_mgr_LDADD = $(objects)
