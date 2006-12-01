INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-I${top_srcdir}/src \
	-DPREFIX=\"@prefix@\" \
	-DMODESTLOCALEDIR=\""$(modestlocaledir)"\"

bin_PROGRAMS=\
	folder-xfer\
	update-account

folder_xfer_SOURCES=\
	folder-transfer.c

folder_xfer_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
	${top_srcdir}/src/$(MODEST_PLATFORM)/libmodest-ui.la \
	${top_srcdir}/src/widgets/libmodest-widgets.la \
	${top_srcdir}/src/modest-account-mgr.o \
	${top_srcdir}/src/modest-tny-platform-factory.o \
	${top_srcdir}/src/modest-conf.o \
	${top_srcdir}/src/modest-protocol-mgr.o \
	${top_srcdir}/src/modest-pair.o \
	${top_srcdir}/src/modest-marshal.o \
	${top_srcdir}/src/modest-tny-account-store.o \
	${top_srcdir}/src/modest-mail-operation.o \
	${top_srcdir}/src/modest-tny-msg-actions.o \
	${top_srcdir}/src/modest-text-utils.o

update_account_SOURCES=\
	update-account.c

update_account_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
	${top_srcdir}/src/$(MODEST_PLATFORM)/libmodest-ui.la \
	${top_srcdir}/src/widgets/libmodest-widgets.la \
	${top_srcdir}/src/modest-account-mgr.o \
	${top_srcdir}/src/modest-tny-platform-factory.o \
	${top_srcdir}/src/modest-conf.o \
	${top_srcdir}/src/modest-protocol-mgr.o \
	${top_srcdir}/src/modest-pair.o \
	${top_srcdir}/src/modest-marshal.o \
	${top_srcdir}/src/modest-tny-account-store.o \
	${top_srcdir}/src/modest-mail-operation.o \
	${top_srcdir}/src/modest-tny-msg-actions.o \
	${top_srcdir}/src/modest-text-utils.o