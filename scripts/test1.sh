#!/bin/bash

echo "TEST 1";

socketname="LSOfilestorage.sk"
configpath=$(realpath config/config.txt)
serverpath=$(realpath server.out)
clientpath=$(realpath client.out)

printf "workerThreads = 1\n" > $configpath;
printf "numberOfFiles = 10000\n" >> $configpath;
printf "storageCapacity = 128000000\n" >> $configpath;
printf "socketName = LSOfilestorage.sk\n" >> $configpath;
printf "logFile = logs/log.txt" >> $configpath;

valgrind --leak-check=full --show-leak-kinds=all -s $serverpath &
serverpid=$!
sleep 2s

#"client 1 --------------------------------------------------------------------------------------------"
$clientpath   	-f $socketname \
 				-p \
 				-D removed \
 				-d read \
 				-W data/test1/wallpaper_4k_uhd_19_20210223_2064369960.jpg \
 				-w data/test3 \
 				-r data/test3/Calcolo-numerico/lezioni/CN-2021-02-24.pdf \
 				-l data/test1/progettosol-20_21.pdf,data/test1/478479.jpg \
 				-u data/test1/progettosol-20_21.pdf,data/test1/478479.jpg \
 				-c data/test1/Elakala-waterfalls-vertical.jpg \
  				-t 200 \
  				-R 0 &
pid1=$!


#"client 2 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
  				-D removed \
  				-d read \
  				-W data/test1/images.jpeg \
  				-w data/test2 \
  				-r data/test3/Calcolo-numerico/lezioni/CN-2021-03-03.pdf \
  				-c data/test1/Elakala-waterfalls-vertical.jpg \
  				-l data/test1/images.jpeg \
  				-u data/test1/progettosol-20_21.pdf,data/test1/Elakala-waterfalls-vertical.jpg \
  				-t 200 \
  				-R 4 &
pid2=$!

wait $pid1 $pid2

kill -s SIGHUP $serverpid
wait $serverpid
