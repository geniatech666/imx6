#!/bin/sh
### BEGIN INIT INFO
# Provides: resize
# Required-Start:    mountkernfs
# Required-Stop:     mountkernfs
# Default-Start:     S
# Default-Stop:
### END INIT INFO

if [ ! -e /etc/flag ]; then
   sh /etc/resize-helper
   echo "finish" > /etc/flag
fi

: exit 0

