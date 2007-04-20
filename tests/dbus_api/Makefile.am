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
        $(MODEST_LIBTINYMAIL_MAEMO_LIBS)
        
noinst_PROGRAMS = test_hello

test_hello_SOURCES = test_hello.c
test_hello_LDADD = $(objects)

