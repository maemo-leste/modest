#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
REQUIRED_AUTOMAKE_VERSION=1.8
PKG_NAME=modest

(test -f $srcdir/configure.ac \
  && test -f $srcdir/src/modest-main.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from GNOME CVS"
    exit 1
}
export USE_GNOME2_MACROS=1

gnome-autogen.sh || {
    	echo "gnome autoconf does not work, trying the other way..."

	glib-gettextize --copy --force
	libtoolize --automake --copy --force
	intltoolize --automake --copy --force
	aclocal-1.8
	autoconf --force
	autoheader --force
	automake-1.8 --add-missing --copy --force-missing --foreign
	./configure $@
}	
