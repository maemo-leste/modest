#
# Makefile.am for modest
# Time-stamp: <2006-12-10 12:55:27 (djcb)>
#
SUBDIRS= src po

if BUILD_TESTS
SUBDIRS += tests
endif

if BUILD_DOCS
SUBDIRS += docs
endif

EXTRA_DIST=                 \
	mkinstalldirs       \
	autogen.sh          \
	COPYING             \
	intltool-extract.in \
	intltool-merge.in   \
	intltool-update.in

DISTCLEANFILES =            \
	intltool-extract.in \
	intltool-merge.in   \
	intltool-update.in
