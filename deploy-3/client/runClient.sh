#!/bin/bash
#Checks if there are 3 arguments
if [ $# -eq 2 ]
then 
	#checks if the first argument is an existing directory
	if [ -d "$1" ] 
	then
		inputdir=${1%/}
	else
		echo Input directory does not exist.
		exit 1
	fi
	#checks if third argument is an integer and if it is greater than zero
	server=$2

	for inputfile in ${inputdir}/*.txt
	do
		filename=$(basename ${inputfile})
		echo Exectuting $inputfile
		./tecnicofs-client $inputfile $server
	done
else
	echo "Wrong number of arguments. (2)"
	exit 1
fi