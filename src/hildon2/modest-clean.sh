#!/bin/sh

PIDS=`pidof modest` && kill -15 $PIDS


rm -rf /home/user/.modest

exit 0
