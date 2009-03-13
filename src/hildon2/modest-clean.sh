#!/bin/sh

# First we kill Modest to make sure that, when we remove
# all the data, modest won't write again
PIDS=`pidof modest` && kill -15 $PIDS

# Removing modest user data folder
rm -rf /home/user/.modest

exit 0
