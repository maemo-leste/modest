#
# Makefile.am
#

MAINTAINERCLEANFILES    = Makefile.in

TESTS_ENVIRONMENT       = top_builddir=$(top_builddir)

TESTS                   =                           \
                        check_folder-xfer           \
                        check_update-account        \
			check_modest-conf           

noinst_PROGRAMS=				    \
			check_folder-xfer           \
			check_update-account        \
			check_modest-conf


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
        ${top_srcdir}/src/modest-tny-platform-factory.o \
        ${top_srcdir}/src/modest-conf.o \
        ${top_srcdir}/src/modest-protocol-mgr.o \
        ${top_srcdir}/src/modest-formatter.o \
        ${top_srcdir}/src/modest-pair.o \
        ${top_srcdir}/src/modest-marshal.o \
        ${top_srcdir}/src/modest-tny-account-store.o \
        ${top_srcdir}/src/modest-mail-operation-queue.o \
        ${top_srcdir}/src/modest-mail-operation.o \
        ${top_srcdir}/src/modest-tny-msg-actions.o \
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
