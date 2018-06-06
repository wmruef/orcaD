#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/#od:3:respawn:\/usr\/local\/orcaD\/orcad/od:3:respawn:\/usr\/local\/orcaD\/orcad/" /etc/inittab
  sed -i -e "s/#od:23:respawn:\/usr\/local\/orcaD\/orcad/od:23:respawn:\/usr\/local\/orcaD\/orcad/" /etc/inittab
  /sbin/telinit q
else
  systemctl enable orcad.service
  systemctl start orcad.service
fi
sleep 3
pid=`/bin/pidof orcad`
if [ "$pid" != "" ] ; then
echo "Orcad started (pid $pid)..."
else
echo "Ooops...could not start orcad..."
fi
