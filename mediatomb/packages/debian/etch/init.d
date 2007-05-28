#! /bin/sh 
#
# MediaTomb initscript
#
# Original Author: Tor Krill <tor@excito.com>.
# Modified by:     Leonhard Wimmer <leo@mediatomb.cc>
#
#

### BEGIN INIT INFO
# Provides:          mediatomb
# Required-Start:    $all
# Required-Stop:     $all
# Should-Start:      $all
# Should-Stop:       $all
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: upnp media server
### END INIT INFO


set -e

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="upnp media server"
NAME=mediatomb
DAEMON=/usr/bin/$NAME
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME
USER=mediatomb
GROUP=mediatomb
LOGFILE=/var/log/mediatomb

# Read config file if it is present.
if [ -r /etc/default/$NAME ]
then
	. /etc/default/$NAME
fi

# if NO_START is set, exit gracefully
[ "$NO_START" = "yes" ] && exit 0

D_ARGS="-c /etc/mediatomb/config.xml -d -u $USER -g $GROUP -P $PIDFILE -l $LOGFILE"

if [ "$INTERFACE" != "" ] ; then
    IFACE_IP=`/sbin/ifconfig | grep -i "$INTERFACE" -A 1 | grep "inet addr" | cut -d " " -f 12 | cut -d ":" -f 2`
    [ "$IFACE_IP" != "" ] && DARGS="$DARGS -i $IFACE_IP"
fi



# Gracefully exit if the package has been removed.
test -x $DAEMON || exit 0

#
#       Function that starts the daemon/service.
#
d_start() {
	touch $PIDFILE
	chown $USER:$GROUP $PIDFILE
	touch $LOGFILE
	chown $USER:$GROUP $LOGFILE
        start-stop-daemon --start --quiet --pidfile $PIDFILE \
                --exec $DAEMON -- $D_ARGS $DAEMON_OPTS
}

#
#       Function that stops the daemon/service.
#
d_stop() {
        start-stop-daemon --stop --signal 2 --retry 5 --quiet --pidfile $PIDFILE \
                --name $NAME
        rm $PIDFILE
}

#
#       Function that sends a SIGHUP to the daemon/service.
#
d_reload() {
        start-stop-daemon --stop --quiet --pidfile $PIDFILE \
                --name $NAME --signal 1
}

case "$1" in
  start)
        echo -n "Starting $DESC: $NAME"
        if [ "$INTERFACE" != "" ] ; then
            # try to add the multicast route
            /sbin/route add -net 239.0.0.0 netmask 255.0.0.0 $INTERFACE >/dev/null 2>&1 || true
        fi
        d_start
        echo "."
        ;;
  stop)
        echo -n "Stopping $DESC: $NAME"
        d_stop
        echo "."
        ;;
  reload|force-reload)
        echo -n "Reloading $DESC configuration..."
        d_reload
        echo "done."
  	;;
  restart)
        #
        #       If the "reload" option is implemented, move the "force-reload"
        #       option to the "reload" entry above. If not, "force-reload" is
        #       just the same as "restart".
        #
        echo -n "Restarting $DESC: $NAME"
        d_stop
        sleep 1
        d_start
        echo "."
        ;;
  *)
        # echo "Usage: $SCRIPTNAME {start|stop|restart|reload|force-reload}" >&2
        echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload}" >&2
        exit 1
        ;;
esac

exit 0

