#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/ad:3:respawn:\/usr\/local\/orcaD\/auxiliaryd/#ad:3:respawn:\/usr\/local\/orcaD\/auxiliaryd/" /etc/inittab
  sed -i -e "s/ad:23:respawn:\/usr\/local\/orcaD\/auxiliaryd/#ad:23:respawn:\/usr\/local\/orcaD\/auxiliaryd/" /etc/inittab
  /sbin/telinit -t 20 q
else
  systemctl disable auxiliaryd.service
  systemctl stop auxiliaryd.service
fi
sleep 6
pid=`/bin/pidof auxiliaryd`
if [ "$pid" != "" ] ; then
echo "Ooops...could not stop auxiliaryd.  It's still running (pid $pid)..."
else
echo "auxiliaryd is stopped"
fi
