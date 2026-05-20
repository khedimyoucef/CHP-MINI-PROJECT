#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define N 800  // Matrix size (N x N)

// ANSI Color Codes (Disabled for native style)
#define RESET   ""
#define BOLD    ""
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#define MAGENTA ""
#define CYAN    ""

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
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &name_len);

    num_workers = num_tasks - 1;

    // If running with only 1 process, do sequential computation
    if (num_tasks == 1) {
        printf(CYAN BOLD "\n[Rank 0 on %s]" RESET " Initializing matrices for sequential benchmark...\n", processor_name);
        initialize_matrices();
        
        printf(CYAN BOLD "[Rank 0 on %s]" RESET " Running computation loop (%d x %d)...\n", processor_name, N, N);
        start_time = MPI_Wtime();
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
            // Display sequential progress
            if (i > 0 && i % (N / 5) == 0) {
                printf(CYAN "[Rank 0 on %s]" RESET " Sequential Progress: %d%% completed...\n", processor_name, (i * 100) / N);
            }
        }
        
        end_time = MPI_Wtime();
        printf(GREEN BOLD "\n[Rank 0 on %s] Sequential calculation finished in %.4f seconds.\n" RESET, processor_name, end_time - start_time);
        
        // Print verification checksum
        double checksum = 0.0;
        for (int i = 0; i < N; i++) checksum += C[i][i];
        printf(BLUE "[Rank 0 on %s] Matrix diagonal checksum: %.2f\n\n" RESET, processor_name, checksum);

        MPI_Finalize();
        return 0;
    }

    /* Master Task */
    if (rank == 0) {
        printf(BLUE BOLD "\n======================================================================\n" RESET);
        printf(BLUE BOLD "                MPI PARALLEL BENCHMARK (MASTER LAYER)                 \n" RESET);
        printf(BLUE BOLD "======================================================================\n" RESET);
        printf(CYAN BOLD "[Master on %s]" RESET " Node setup: %d Total Processes (%d Workers)\n", processor_name, num_tasks, num_workers);
        printf(CYAN BOLD "[Master on %s]" RESET " Initializing input matrices A and B of size %d x %d...\n", processor_name, N, N);
        initialize_matrices();
        
        printf(CYAN BOLD "[Master on %s]" RESET " Distributing rows and matrix data to worker nodes...\n", processor_name);
        start_time = MPI_Wtime();

        // Send matrix data to worker tasks
        rows = N / num_workers;
        offset = 0;

        for (dest = 1; dest <= num_workers; dest++) {
            int current_rows = (dest == num_workers) ? (N - offset) : rows;

            printf(YELLOW "[Master -> Rank %d]" RESET " Dispatching rows %d to %d (%d rows)\n", dest, offset, offset + current_rows - 1, current_rows);

            MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&current_rows, 1, MPI_INT, dest, 2, MPI_COMM_WORLD);
            MPI_Send(&A[offset][0], current_rows * N, MPI_DOUBLE, dest, 3, MPI_COMM_WORLD);
            MPI_Send(&B, N * N, MPI_DOUBLE, dest, 4, MPI_COMM_WORLD);

            offset += current_rows;
        }

        printf(CYAN BOLD "\n[Master on %s]" RESET " All tasks dispatched. Waiting for worker outputs...\n\n", processor_name);

        // Receive results from worker tasks
        for (source = 1; source <= num_workers; source++) {
            MPI_Recv(&offset, 1, MPI_INT, source, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&rows, 1, MPI_INT, source, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&C[offset][0], rows * N, MPI_DOUBLE, source, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            printf(GREEN "[Master <- Rank %d]" RESET " Successfully gathered and assembled %d rows from offset %d.\n", source, rows, offset);
        }

        end_time = MPI_Wtime();
        printf(GREEN BOLD "\n[Master on %s] Parallel computation successfully finished in %.4f seconds.\n" RESET, processor_name, end_time - start_time);
        
        // Print verification checksum
        double checksum = 0.0;
        for (int i = 0; i < N; i++) checksum += C[i][i];
        printf(BLUE BOLD "[Master on %s] Matrix diagonal checksum: %.2f\n" RESET, processor_name, checksum);
        printf(BLUE BOLD "======================================================================\n\n" RESET);
    }
    
    /* Worker Tasks */
    else {
        // Receive data from master
        MPI_Recv(&offset, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        printf(MAGENTA "[Rank %d on %s]" RESET " Task active! Allocated to calculate rows %d to %d (%d rows total).\n", rank, processor_name, offset, offset + rows - 1, rows);

        // Dynamically allocate to avoid stack overflow in workers
        double* local_A = (double*)malloc(rows * N * sizeof(double));
        double* local_B = (double*)malloc(N * N * sizeof(double));
        double* local_C = (double*)calloc(rows * N, sizeof(double));

        MPI_Recv(local_A, rows * N, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_B, N * N, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf(MAGENTA "[Rank %d on %s]" RESET " Successfully received matrix partitions. Starting calculations...\n", rank, processor_name);

        // Perform multiplication with progress updates
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    local_C[i * N + j] += local_A[i * N + k] * local_B[k * N + j];
                }
            }
            // Display progress updates every 25% completed
            if (rows >= 4 && (i + 1) % (rows / 4) == 0) {
                int percent = ((i + 1) * 100) / rows;
                printf(YELLOW "[Rank %d on %s]" RESET " Progress: %d%% completed (%d/%d rows computed).\n", rank, processor_name, percent, i + 1, rows);
            }
        }

        printf(GREEN "[Rank %d on %s]" RESET " Calculation finished! Sending results back to Master...\n", rank, processor_name);

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
