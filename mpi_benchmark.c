#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 800  // Matrix size (N x N)

double A[N][N];
double B[N][N];
double C[N][N];

void initialize_matrices() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (double)(i + j);
            B[i][j] = (double)(i - j);
            C[i][j] = 0.0;
        }
    }
}

int main(int argc, char** argv) {
    int num_tasks, rank, num_workers, source, dest, rows, offset;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    num_workers = num_tasks - 1;

    // If running with only 1 process, do sequential computation
    if (num_tasks == 1) {
        initialize_matrices();
        start_time = MPI_Wtime();
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
        
        end_time = MPI_Wtime();
        printf("Sequential calculation finished in %.4f seconds.\n", end_time - start_time);
        MPI_Finalize();
        return 0;
    }

    /* Master Task */
    if (rank == 0) {
        initialize_matrices();
        printf("Starting parallel matrix multiplication of size %d x %d with %d processes...\n", N, N, num_tasks);
        
        start_time = MPI_Wtime();

        // Send matrix data to worker tasks
        rows = N / num_workers;
        offset = 0;

        for (dest = 1; dest <= num_workers; dest++) {
            // If N is not perfectly divisible, last worker gets the remainder
            int current_rows = (dest == num_workers) ? (N - offset) : rows;

            MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&current_rows, 1, MPI_INT, dest, 2, MPI_COMM_WORLD);
            MPI_Send(&A[offset][0], current_rows * N, MPI_DOUBLE, dest, 3, MPI_COMM_WORLD);
            MPI_Send(&B, N * N, MPI_DOUBLE, dest, 4, MPI_COMM_WORLD);

            offset += current_rows;
        }

        // Receive results from worker tasks
        for (source = 1; source <= num_workers; source++) {
            MPI_Recv(&offset, 1, MPI_INT, source, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&rows, 1, MPI_INT, source, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&C[offset][0], rows * N, MPI_DOUBLE, source, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        end_time = MPI_Wtime();
        printf("Parallel calculation finished in %.4f seconds.\n", end_time - start_time);
        
        // Print verification checksum
        double checksum = 0.0;
        for (int i = 0; i < N; i++) checksum += C[i][i];
        printf("Matrix diagonal checksum: %.2f\n", checksum);
    }
    
    /* Worker Tasks */
    else {
        // Receive data from master
        MPI_Recv(&offset, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Dynamically allocate to avoid stack overflow in workers
        double* local_A = (double*)malloc(rows * N * sizeof(double));
        double* local_B = (double*)malloc(N * N * sizeof(double));
        double* local_C = (double*)calloc(rows * N, sizeof(double));

        MPI_Recv(local_A, rows * N, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_B, N * N, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Perform multiplication
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    local_C[i * N + j] += local_A[i * N + k] * local_B[k * N + j];
                }
            }
        }

        // Send results back to master
        MPI_Send(&offset, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
        MPI_Send(&rows, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);
        MPI_Send(local_C, rows * N, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);

        free(local_A);
        free(local_B);
        free(local_C);
    }

    MPI_Finalize();
    return 0;
}
