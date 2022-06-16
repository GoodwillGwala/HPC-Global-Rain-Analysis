CC = g++
MPI_CC = mpicxx
CFLAGS = -O3 -std=c++17 -static-libstdc++ -g
OPENMP_LIB = -fopenmp


NETCDF_LIB = -L/usr/include -I/usr/include -lnetcdf


FORMAT_LIB = $(NETCDF_LIB)

HOME =
COMMON = common
INC_DIR = include
SRC_DIR = src

COM_OBJS = $(INC_DIR)/partitioner.o $(INC_DIR)/netcdf_partitioner.o

PROGS =  main

all: $(PROGS)

$(INC_DIR)/partitioner.o: $(INC_DIR)/partitioner.cpp
	$(MPI_CC) $(CFLAGS) -c $< -o $@ $(FORMAT_LIB) -I$(INC_DIR)

$(INC_DIR)/netcdf_partitioner.o: $(INC_DIR)/netcdf_partitioner.cpp
	$(MPI_CC) $(CFLAGS) -c $< -o $@ $(FORMAT_LIB) -I$(INC_DIR)




main: main.cpp $(COM_OBJS)
	$(MPI_CC) $(CFLAGS) $^ -o $@ $(OPENMP_LIB) $(FORMAT_LIB) -I$(COMMON) -I$(INC_DIR)


clean:
	rm  -f *.o $(COM_OBJS) $(PROGS) data.nc
