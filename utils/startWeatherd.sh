#!/bin/sh
if [ -s /etc/inittab ]
then
  sed -i -e "s/#wd:3:respawn:\/usr\/local\/orcaD\/weatherd/wd:3:respawn:\/usr\/local\/orcaD\/weatherd/" /etc/inittab
  sed -i -e "s/#wd:23:respawn:\/usr\/local\/orcaD\/weatherd/wd:23:respawn:\/usr\/local\/orcaD\/weatherd/" /etc/inittab
  /sbin/telinit q
else 
  systemctl enable weatherd.service
  systemctl start weatherd.service
fi
sleep 3
pid=`/bin/pidof weatherd`
if [ "$pid" != "" ] ; then
echo "WeatherD started (pid $pid)..."
else
echo "Ooops...could not start weatherd..."
fi
