SUBDIRS=modest-account-mgr
DIST_SUBDIRS=

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	-DPREFIX=\"@prefix@\" \
	-DOSSO_MODEST_EASYSETUP_LOCALEDIR=\"$(OSSO_MODEST_EASYSETUP_LOCALEDIR)\" \
	-I$(srcdir)/modest-account-mgr


bin_PROGRAMS=\
	osso-modest-easysetup

osso_modest_easysetup_SOURCES=\
	main.c \
	modest-wizard-dialog.h modest-wizard-dialog.c \
	modest-presets.h modest-presets.c \
	modest-easysetup-wizard.h modest-easysetup-wizard.c \
	modest-easysetup-country-combo-box.h modest-easysetup-country-combo-box.c \
	modest-easysetup-provider-combo-box.h modest-easysetup-provider-combo-box.c \
	modest-easysetup-servertype-combo-box.h modest-easysetup-servertype-combo-box.c \
	modest-easysetup-serversecurity-combo-box.h modest-easysetup-serversecurity-combo-box.c \
	modest-easysetup-secureauth-combo-box.h modest-easysetup-secureauth-combo-box.c \
	modest-validating-entry.h modest-validating-entry.c \
	modest-text-utils.h modest-text-utils.c

osso_modest_easysetup_LDADD = 						\
	$(MODEST_GSTUFF_LIBS) \
	modest-account-mgr/libmodestaccountmgr.a

