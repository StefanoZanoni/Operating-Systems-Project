#!/bin/bash

echo "STATISTICS";

num_reads=0
read_bytes=0
avg_read=0

num_writes=0
written_bytes=0
avg_write=0

num_locks=0
num_openlocks=0
num_unlocks=0
num_close=0

max_capacity=0
max_files=0
num_replacement=0
max_connections=0

if [ -d "logs" ]; then

	cd logs
	log_file=$(ls -Ar | head -n1)

	# salvo il numero dei thread
	num_workers=$(cat "${log_file}" | head ${log_file} | grep -e "num_workers" | cut -c 15-)
	echo "${num_workers}"

	# letture
	num_reads=$(grep -e "the read file" "${log_file}" | wc -l)
	echo "numero di letture effettuate: ${num_reads}"

	while IFS=',' read information size; do

		num=$(echo "${size}" | cut -c 7-)
		read_bytes=$read_bytes+$num

	done < <(grep -e "the read file" "${log_file}")
	read_bytes=$(bc <<< ${read_bytes})
	echo "totale di byte letti: ${read_bytes}"

	if [ ${num_reads} != 0 ]; then

		avg_read=$(echo "scale=2; ${read_bytes} / ${num_reads}" | bc -l)
		echo "media di byte letti: ${avg_read}"

	fi

	# scritture
	num_writes=$(grep -e "the written file" "${log_file}" | wc -l)
	echo "numero di scritture effettuate: ${num_writes}"

	while IFS=',' read information size; do

		num=$(echo "${size}" | cut -c 7-)
		written_bytes=$written_bytes+$num

	done < <(grep -e "the written file" "${log_file}")
	written_bytes=$(bc <<< ${written_bytes})
	echo "totale di byte scritti: ${written_bytes}"

	if [ ${num_writes} != 0 ]; then

		avg_write=$(echo "scale=2; ${written_bytes} / ${num_writes}" | bc -l)
		echo "media di byte scritti: ${avg_write}"

	fi

	# locks
	num_locks=$(grep -e "the locked file" "${log_file}" | wc -l)
	echo "numero di locks effettuate: ${num_locks}"

	# open locks
	num_openlocks=$(grep -e "the open file" "${log_file}" | wc -l)
	echo "numero di open-locks effettuate: ${num_openlocks}"

	# unlocks
	num_unlocks=$(grep -e "the unlocked file" "${log_file}" | wc -l)
	echo "numero di unlocks effettuate: ${num_unlocks}"

	# close
	num_close=$(grep -e "the closed file" "${log_file}" | wc -l)
	echo "numero di close effettuate: ${num_close}"

	# capacita massima
	while IFS='=' read information size; do

		num="${size}"
		num=${num::-2}
		echo "capacita massima raggiunta dal server:${num} MB"

	done < <(grep -e "Maximum size" "${log_file}")

	# file massimi
	while IFS='=' read information size; do

		num=$(echo "${size}" | cut -c 2-)
		echo "numero massimo di file contenuti nel server: ${num}"

	done < <(grep -e "Maximum number of files" "${log_file}")

	# rimpiazzamenti della cache
	num_replacement=$(grep -e "The cache replacement" "${log_file}" | wc -l)
	echo "numero di volte che e' stato eseguito l'algoritmo di rimpiazzamento della cache:${num_replacement}"

	# numero massimo di connessioni contemporanee
	while IFS='=' read information size; do

		num=$(echo "${size}" | cut -c 2-)
		echo "massimo numero di connessioni contemporanee: ${num}"

	done < <(grep -e "Maximum number of simultaneous" "${log_file}")

	# threads
	thread_rq=()
	for i in $( eval echo {1..${num_workers}} ); do

		thread_rq[$i]=$(grep -e "Thread $i: command no." "${log_file}" | wc -l)

	done 
	echo "Numero di richieste soddisfatte per ogni thread:"
	for i in $(eval echo {1..${num_workers}} ); do

		echo "Worker $i: ${thread_rq[$i]}"

	done


else
	echo "la directory logs non esiste"
fi