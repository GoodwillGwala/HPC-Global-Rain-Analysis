#!/bin/bash
#SBATCH --job-name=WeatherData
#SBATCH --output=WeatherData.out
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=<goodwill.gwala@students.wits.ac.za>
#SBATCH --nodes=12              # Number of nodes
#SBATCH --ntasks=12             # Number of MPI ranks
#SBATCH --ntasks-per-node=1    	# Number of MPI ranks per node
#SBATCH --ntasks-per-socket=2  	# Number of tasks per processor socket on the node
#SBATCH --cpus-per-task=4      	# Number of OpenMP threads for each MPI process/rank
#SBATCH --mem-per-cpu=2000mb   	# Per processor memory request
#SBATCH --time=00:10:00      	# Walltime in hh:mm:ss or d-hh:mm:ss
#SBATCH --partition=jaguar  	# partition, abbreviated by -p


# Number of MPI tasks
#SBATCH -n 4


# Number of cores per task
#SBATCH -c 28





date;hostname;pwd

#module load intel/2018 openmpi/3.1.0

HOST=$(hostname)
local="Goodwill-ThinkCentre"


if [ "$HOST" = "$local" ]; then

	echo Running on Local
    mpirun -n 2 ./main

else

    echo Running on Cluster
    if [ -n "$SLURM_CPUS_PER_TASK" ]; then
  		omp_threads=$SLURM_CPUS_PER_TASK
	else
  		omp_threads=384
	fi
	export OMP_NUM_THREADS=$omp_threads
	echo Using $OMP_NUM_THREADS OpenMP Threads

    srun --mpi=pmi2 -n 12 main


fi
