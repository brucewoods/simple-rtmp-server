#!/bin/bash
DATE=`date +"%Y%m%d" --date="-15 day"`
#echo $DATE;
find /home/kotori/hzp_srs/simple-rtmp-server/trunk/tools/ -name "*${DATE}*" | xargs rm -f
