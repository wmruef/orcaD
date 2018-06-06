#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/#ad:3:respawn:\/usr\/local\/orcaD\/auxiliaryd/ad:3:respawn:\/usr\/local\/orcaD\/auxiliaryd/" /etc/inittab
  sed -i -e "s/#ad:23:respawn:\/usr\/local\/orcaD\/auxiliaryd/ad:23:respawn:\/usr\/local\/orcaD\/auxiliaryd/" /etc/inittab
  /sbin/telinit q
else
  systemctl enable auxiliaryd.service
  systemctl start auxiliaryd.service
fi
sleep 6
pid=`/bin/pidof auxiliaryd`
if [ "$pid" != "" ] ; then
echo "Auxiliaryd started (pid $pid)..."
else
echo "Ooops...could not start Auxiliaryd"
fi
