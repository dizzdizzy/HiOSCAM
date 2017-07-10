#/bin/bash
while [ 1 ]
do
  pid=`ps -ef | grep "oscam-1.20.8950" | grep -v 'grep' | awk '{print $2}'`
  if [ -z $pid ]; then
     oscam-1.20.8950 &
  fi
  sleep 60
done

