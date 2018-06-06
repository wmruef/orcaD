#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/wd:3:respawn:\/usr\/local\/orcaD\/weatherd/#wd:3:respawn:\/usr\/local\/orcaD\/weatherd/" /etc/inittab
  sed -i -e "s/wd:23:respawn:\/usr\/local\/orcaD\/weatherd/#wd:23:respawn:\/usr\/local\/orcaD\/weatherd/" /etc/inittab
  /sbin/telinit -t 20 q
else
  systemctl disable weatherd.service
  systemctl stop weatherd.service
fi
sleep 6
pid=`/bin/pidof weatherd`
if [ "$pid" != "" ] ; then
echo "Ooops...could not stop weatherd.  It's still running (pid $pid)..."
else
echo "weatherd is stopped"
fi
