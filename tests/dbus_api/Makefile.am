INCLUDES=\
	@CHECK_CFLAGS@ \
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-I${top_srcdir}/src \
	-I$(top_srcdir) \
	-DPREFIX=\"@prefix@\" \
	-DMODESTLOCALEDIR=\""$(modestlocaledir)"\"
	
objects=\
	@CHECK_LIBS@ \
	$(MODEST_GSTUFF_LIBS) \
        $(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
        $(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
        ${top_srcdir}/libmodest-dbus-client/libmodest-dbus-client-1.0.la
        
noinst_PROGRAMS = test_send_mail      \
		  test_mail_to        \
		  test_open_message   \
		  test_search         \
		  test_delete_message \
		  test_compose_mail \
		  test_open_default_inbox

test_send_mail_SOURCES = test_send_mail.c
test_send_mail_LDADD = $(objects)

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
