#!/bin/sh
### BEGIN INIT INFO
# Provides: resize
# Required-Start:    mountkernfs
# Required-Stop:     mountkernfs
# Default-Start:     S
# Default-Stop:
### END INIT INFO

echo "finish" > /etc/flag

sleep 10

hwclock --hctosys

: exit 0

