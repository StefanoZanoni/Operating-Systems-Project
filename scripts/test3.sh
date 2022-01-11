#!/bin/bash

echo "TEST 3";

socketname="LSOfilestorage.sk"
configpath=$(realpath config/config.txt)
serverpath=$(realpath server.out)
clientpath=$(realpath client.out)

printf "workerThreads = 8\n" > $configpath;
printf "numberOfFiles = 100\n" >> $configpath;
printf "storageCapacity = 32000000\n" >> $configpath;
printf "socketName = LSOfilestorage.sk\n" >> $configpath;
printf "logFile = logs/log.txt\n" >> $configpath;

current_date=$(date +%s)
stop_date=$(echo "$current_date + 30" | bc)
pids=()

$serverpath &
serverpid=$!
sleep 2s

while [ $(date +%s) -lt $stop_date ]; do

	for i in {0..9}; do

		if ! ps -p ${pids[$i]} > "/dev/null" 2>&1; then
		{	
		$clientpath   	-f $socketname \
 						-D removed \
 						-d read \
 						-t 0 \
 						-w data/test2 \
 						-r data/test3/Calcolo-numerico/lezioni/CN-2021-02-24.pdf \
 						-u data/test1/progettosol-20_21.pdf,data/test1/478479.jpg \
 						-c data/test1/Elakala-waterfalls-vertical.jpg \
  						-R 2 ;
 		} &

		pids=( "${pids[@]:0:$i}" $! "${pids[@]:$i}" )

		fi

	done

done

for i in {0..9}; do

	wait ${pids[$i]}

done

kill -s SIGINT $serverpid
wait $serverpid