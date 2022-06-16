Introduction
================
A data analysis framework implemetation using **c++** and python is presented. The implementations make use of **MPI** and **OpenMP** for the statistical analysis of global precipitation data in a distributed environment.




<!--
Thread management is an essential part of **multithreading** in **High-Performance Computing**.
c++ contains a standard library that implements the low-level Posix Threads (PThreads) using ```std::thread```.
This library essentially wraps the standard library c++ ```std::thread``` giving the user more control over thread creation and management, task execution using a well defined priority queue and **parallelism**, and thread safe **synchronization**. This library utilizes **OOP** and **PImpl** idioms design patterns. -->




<!--Thread manager that implements a thread pool
===========================================
TODO : Add description-->

Features
===========================================
Implementation A uses **C++** and the **NetCDF** library to implement a parallel partitioning of the data. The partitions are distributed across available nodes where another level of threading using **OpenMP** processes the data using a **MapReduce** idiom. Implementation B achieves the same using **python**

<!---Usage
===========================================--->

Requirements
============

The programs uses **NetCDF** and **C++20**.
This program is best suited for a distributed environment using a cluster when analysing large data sets.



Compiling & Execution
============

To compile and run both implementations, using bash:

./Data.sh

A sample is provided below

Sample Run
===
group11@jaguar1:~$ ./Data.sh
Cleaning working directory
Goodwill-ThinkCentre
Changing to working directory on cluster

rm  -f *.o include/partitioner.o include/netcdf_partitioner.o main data.nc
Please enter data directory path, press enter if working on Jaguar cluster



Please enter range of years separated by a space e.g. 2016 2017 2019 etc

2019 2018 2017 2016 2015 2014 2013 2012 2011 2010 2009 2008 2007 2006 2005 2004 2003 2004 2003 2002 2001 2000 1999 1998 1997 1996 1995 1994 1993 1992 1991 1990 1989 1988 1987 1986 1985 1984 1983 1982

Please enter resolution of years i.e. 10 (1.0) or 05 (0.5)

10


Using cluster default path
Files are being prepared please wait...
Files added
Files have been successfully prepared
Building project...



Running project...

Submitting job...
Mon 17 May 2021 18:07:30 SAST
jaguar1
/home/group11/Goodwill/project

Running on Cluster

Using 24 OpenMP Threads

Total processing time on node 5 = 24.16 secs.

Total processing time on node 4 = 24.63 secs.

Total processing time on node 2 = 24.64 secs.

Total processing time on node 3 = 25.03 secs.

Simulation time = 25.24 secs.

Simulation data is ready...

Scheduler: Initializing with 24 threads and 12 nodes...

Scheduler: Constructing the reduction map for all the threads...

Scheduler: Reduction map for 24 threads is ready.

Total processing time on node 11 = 27.35 secs.

Total processing time on node 9 = 27.60 secs.

Total processing time on node 7 = 27.98 secs.

Total processing time on node 6 = 27.99 secs.

Total processing time on node 8 = 28.04 secs.

Total processing time on node 10 = 28.08 secs.

Total processing time on node 1 = 36.30 secs.

Analytics time = 11.52 secs.

Total processing time on node 0 = 36.76 secs.

Final output on the master node:


Global Statistics

Global Min: 0.01

Global Q1: 0.17

Global Q2: 0.8

Global Q3: 3.64

Global Max: 8.845

Total processing time on node 0 = 37.45 secs.
Running Done...

Cleaning working directory....

rm  -f *.o include/partitioner.o include/netcdf_partitioner.o main data.nc
