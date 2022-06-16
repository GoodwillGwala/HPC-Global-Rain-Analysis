#!/bin/bash

echo Cleaning working directory

HOST=$(hostname)
local="Goodwill-ThinkCentre"

if [ "$HOST" != "$local" ]; then


	echo Changing to working directory on cluster
	cd /home/group11/Goodwill/project
fi



echo
make clean

echo Please enter data directory path, press enter if working on Jaguar cluster
echo
read path
echo
echo Please enter range of years separated by a space e.g. 2016 2017 2019 etc
echo
read -a YEARS
echo
echo 'Please enter resolution of years i.e. 10 (1.0) or 05 (0.5)'
echo
read  res
echo
first_var="10"

files=""



  	echo

	if [ "$res" == "$first_var" ];then

		if [[ -z $path ]];then
			echo Using cluster default path
        	echo Files are being prepared please wait...
        	path=/data/full_data_daily_v2020
        fi


		for entry in "${YEARS[@]}"
		do

		temp=" $path/full_data_daily_v2020_"$res"_"$entry".nc"
		files+="$temp"
#		echo $files
		done

	else

        if [[ -z $path ]];then
        	echo Using cluster default path
        	echo Files are being prepared please wait...
        	path=/data/full_data_daily_V2018_05
        fi

		for entry in "${YEARS[@]}"
		do

		temp=" $path/full_data_daily_v2018_"$res"_"$entry".nc"
		files+="$temp"
		done

	fi




ncrcat -v precip $files  data.nc

echo Files added

echo Files have been successfully prepared






echo Building project...
echo
make
echo Building Done...
echo



echo Running project...
echo

echo Submitting job...
sh ./slurm.sh

echo Running Done...

echo

echo Cleaning working directory....
echo
make clean



echo End of Simulation Program
