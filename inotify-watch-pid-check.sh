#!/bin/sh

# inotify-watch-pid-check.sh -- by Daniel Roberson
# -- simple script to respawn inotify-watch if it dies.
# -- meant to be placed in your crontab!
# --
# -- * * * * * /path/to/inotify-watch-pid-check.sh

# Season to taste:
PIDFILE="/var/run/inotify-watch.pid"
LOGFILE="/var/log/inotify-watch.log"
BINPATH="/root/inotify-watch/inotify-watch -d -f $PIDFILE -l $LOGFILE"

if [ ! -f $PIDFILE ]; then
    # PIDFILE doesnt exist!
    echo "inotify-watch not running. Attempting to start"
    $BINPATH
    exit
else
    # PID file exists. check if its running!
    kill -0 "$(head -n 1 $PIDFILE)" 2>/dev/null
    if [ $? -eq 0 ]; then
        exit 0
    else
        echo "inotify-watch not running. Attempting to start.."
        $BINPATH
    fi
fi


