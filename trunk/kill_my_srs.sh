PID=`ps -ef | grep srs | grep smile | awk '{print $2}'`
echo $PID
echo `kill -9 $PID`
