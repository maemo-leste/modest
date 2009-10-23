INCLUDES=\
	@CHECK_CFLAGS@ \
	$(LIBMODEST_DBUS_CLIENT_CFLAGS) \
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_TOOLKIT_DIR) \
	-I${top_srcdir}/src \
	-I$(top_srcdir) \
	-DPREFIX=\"@prefix@\" \
	-DMODESTLOCALEDIR=\""$(modestlocaledir)"\"

objects=\
	@CHECK_LIBS@ \
	$(LIBMODEST_DBUS_CLIENT_LIBS) \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_PLATFORM_LIBS)	\
	$(MODEST_TOOLKIT_LIBS) \
	$(MODEST_HILDON_HELP_LIBS) \
	$(MODEST_HILDON_NOTIFY_LIBS) \
	$(MODEST_OGS_LIBS)

noinst_PROGRAMS = \
	test_mail_to \
	test_open_message   \
	test_search         \
	test_delete_message \
	test_compose_mail \
	test_open_default_inbox \
	test_get_folders \
	test_top_application \
	test_update_account \
	test_open_account \
	test_open_edit_accounts_dialog

test_mail_to_SOURCES = test_mail_to.c
test_mail_to_LDADD = $(objects)

test_open_message_SOURCES = test_open_message.c
test_open_message_LDADD = $(objects)

test_compose_mail_SOURCES = test_compose_mail.c
test_compose_mail_LDADD = $(objects)

test_search_SOURCES = test_search.c
test_search_LDADD = $(objects)

test_delete_message_SOURCES = test_delete_message.c
test_delete_message_LDADD = $(objects)

test_open_default_inbox_SOURCES = test_open_default_inbox.c
test_open_default_inbox_LDADD = $(objects)

test_get_folders_SOURCES = test_get_folders.c
test_get_folders_LDADD = $(objects)

test_top_application_SOURCES = test_top_application.c
test_top_application_LDADD = $(objects)

test_update_account_SOURCES = test_update_account.c
test_update_account_LDADD = $(objects)

test_open_account_SOURCES = test_open_account.c
test_open_account_LDADD = $(objects)

test_open_edit_accounts_dialog_SOURCES = test_open_edit_accounts_dialog.c
test_open_edit_accounts_dialog_LDADD = $(objects)
