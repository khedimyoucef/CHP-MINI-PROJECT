#!/bin/bash
# Compile the MPI program
mpicc -o mpi_test mpi_test.c

# Run the MPI program using the generated hostfile
echo "Running MPI cluster test..."
mpirun --hostfile /home/slave/hostfile -np 4 ./mpi_test
