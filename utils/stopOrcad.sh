#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/od:3:respawn:\/usr\/local\/orcaD\/orcad/#od:3:respawn:\/usr\/local\/orcaD\/orcad/" /etc/inittab
  sed -i -e "s/od:23:respawn:\/usr\/local\/orcaD\/orcad/#od:23:respawn:\/usr\/local\/orcaD\/orcad/" /etc/inittab
  /sbin/telinit -t 20 q
else
  systemctl disable orcad.service
  systemctl stop orcad.service
fi
sleep 6
pid=`/bin/pidof orcad`
if [ "$pid" != "" ] ; then
echo "Ooops...could not stop orcad.  It's still running (pid $pid)..."
else
echo "orcad is stopped"
fi
