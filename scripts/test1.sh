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
 				-w data/test1 \
 				-r data/test3/Calcolo-numerico/lezioni/CN-2021-02-17.pdf \
 				-D removed \
 				-d read \
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
  				-w data/test2 \
  				-W data/test3/Calcolo-numerico/lezioni/CN-2021-02-19.pdf \
  				-r data/test3/Calcolo-numerico/lezioni/CN-2021-02-19.pdf \
  				-d read \
  				-c data/test1/Elakala-waterfalls-vertical.jpg \
  				-l data/test1/progettosol-20_21.pdf,data/test1/Elakala-waterfalls-vertical.jpg \
  				-u data/test1/progettosol-20_21.pdf,data/test1/Elakala-waterfalls-vertical.jpg \
  				-t 200 \
  				-R 4 &
pid2=$!

wait $pid1 $pid2

kill -s SIGHUP $serverpid
wait $serverpid
