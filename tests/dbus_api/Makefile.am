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
        
noinst_PROGRAMS = test_hello test_send_mail

test_hello_SOURCES = test_hello.c
test_hello_LDADD = $(objects)

test_send_mail_SOURCES = test_send_mail.c
test_send_mail_LDADD = $(objects)


