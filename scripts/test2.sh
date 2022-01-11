#!/bin/bash

echo "TEST 2";

socketname="LSOfilestorage.sk"
configpath=$(realpath config/config.txt)
serverpath=$(realpath server.out)
clientpath=$(realpath client.out)

printf "workerThreads = 4\n" > $configpath;
printf "numberOfFiles = 10\n" >> $configpath;
printf "storageCapacity = 10000000\n" >> $configpath;
printf "socketName = LSOfilestorage.sk\n" >> $configpath;
printf "logFile = logs/log.txt" >> $configpath;

$serverpath &
serverpid=$!
sleep 1s

#"client 1 --------------------------------------------------------------------------------------------"
$clientpath   	-f $socketname \
 				-p \
 				-d read \
  				-D removed \
  				-w data/test1 \
  				-W data/test1/progettosol-20_21.pdf \
  				-r data/test1/progettosol-20_21.pdf &
pid1=$!



#"client 2 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
 				-d read \
 				-D removed \
 				-R 4 &
pid2=$!



#"client 3 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
 				-d read \
 				-D removed \
 				-c data/test1/progettosol-20_21.pdf &
pid3=$!
 				


#"client 4 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
 				-d read \
 				-D removed \
 				-l data/test1/lezioni_teoria/01-introduction.pdf,data/test1/lezioni_teoria/02-kernel.pdf &
pid4=$!
 				


#"client 5 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
 				-d read \
 				-D removed \
 				-c data/test1/lezioni_teoria/01-introduction.pdf,data/test1/lezioni_teoria/02-kernel.pdf &
 				#-w data/test2 &
pid5=$!
 				


#"client 6 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
 				-d read \
 				-D removed \
 				-u data/test2/lezioni-laboratorio/lezione3/c004preprocessore.pdf \
 				-c data/test2/lezioni-laboratorio/lezione3/c004preprocessore.pdf &
pid6=$!
				


#"client 7 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
				-d read \
				-D removed \
 				-c data/test2/lezioni-laboratorio/lezione3/c004preprocessore.pdf &
pid7=$!



#"client 8 --------------------------------------------------------------------------------------------"
$clientpath		-f $socketname \
 				-p \
  				-d read \
  				-D removed \
  				-l data/test2/lezioni-laboratorio/lezione3/c003punfunzegenerico.pdf \
 				-c data/test2/lezioni-laboratorio/lezione3/c003punfunzegenerico.pdf,data/test2/lezioni-laboratorio/lezione3/reentrantfunc.pdf &
pid8=$!

wait $pid1 $pid2 $pid3 $pid4 $pid5 $pid6 $pid7 $pid8 				

kill -s SIGHUP $serverpid
wait $serverpid