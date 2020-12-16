#!/bin/bash
#Checks if there are 3 arguments
if [ $# -eq 3 ]
then 
	#checks if the first argument is an existing directory
	if [ -d "$1" ] 
	then
		inputdir=${1%/}
	else
		echo Input directory does not exist.
		exit 1
	fi
	#checks if the second argument is an existing directory
	if [ -d "$2" ] 
	then
		outputdir=${2%/}
	else
		echo Output directory does not exist.
		exit 1
	fi
	#checks if third argument is an integer and if it is greater than zero
	if [ $3 -eq $3 2>/dev/null ] && [ $3 -gt 0 ]
	then
		maxthreads=$3

		for inputfile in ${inputdir}/*.txt
		do
			filename=$(basename ${inputfile})
			for numthreads in $(seq 1 $maxthreads)
			do
				outputfile=${outputdir}/${filename%.*}-${numthreads}.txt
				echo InputFile=$inputfile NumThreads=$numthreads
				./tecnicofs $inputfile $outputfile $numthreads | grep "TecnicoFS completed"
			done
		done
	else
		echo "Incorrect number of threads. (3rd argument)"
		exit 1
	fi
else
	echo "Wrong number of arguments. (3)"
	exit 1
fi