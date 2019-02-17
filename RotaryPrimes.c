#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* ***Program Control*** */
#define MAX 1000000
/* ********************* */

// Returns n^x
// Assumes x > 0
int intPow(int n, int x) {
    if (x == 1)
        return n;
    if (x % 2 == 0)
        return intPow(n, x/2) * intPow(n, x/2);
    else
        return n * intPow(n, x-1);
}

int isPrime(int n) {
    int i;
    if (n % 2 == 0 && n != 2)
        return 0;
    for (i = 3; i <= n/2; i += 2)
        if (n % i == 0)
            return 0;
    return 1;
}

// Returns number of rotary primes found,
// 0 if not a rotary prime
int checkN(int n, int *found, int count) {
    if (!isPrime(n))
        return 0;

    int i = 0;
    while (i < count) {
        if (found[i] == n)
            return 0;
        if (found[i] == -1)
            i = count;
        i++;
    }
    
    // Get number of digits
    int dig = 1;
    while (n > intPow(10, dig))
        dig++;
    
    // Rotate n and check for prime
    int check[dig];
    check[0] = n;

    int underflow;
    for (i = 1; i < dig; i++) {
        underflow = n%10;
        n = n/10 + underflow * intPow(10, dig - 1);
        // Check for duplicates
        // i.e. 11 registers twice because 11 and 11 are both prime when rotated
        int j;
        for (j = 0; j < i; j++) {
            if (n == check[j]) {
                check[i] = -1;
                j = i;
            }
        }
        if (check[i] != -1) {
            if (!isPrime(n))
                return 0;
            else
                check[i] = n; 
        }
    }
    int dups = 0;
    for (i = count; i < count+dig; i++)
        if (check[i - count] != -1)
            found[i] = check[i - count];
        else
            dups++;

    return dig - dups;
}

int main (int argc, char* argv[]) {
    int thisID, numprocs;
    int n, i, j, k;

    int range[2] = {2, MAX};
    int result;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &thisID);

    int found[100];
    for (i = 0; i < 100; i++)
        found[i] = -1;

    if (thisID == 0) {
        // Root node
        printf("Calculating number of rotary primes between ");
        printf("%d and %d using %d nodes...\n", range[0], range[1], numprocs);
        
        // Start timer
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);

        // Partition calculation range between nodes
        if (numprocs > 1) {
            n = (MAX - 2)/numprocs;
            range[1] = range[0] + n;
            for (i = numprocs - 1; i > 0; i--) {
                MPI_Send(&range, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
                range[0] = range[1] + 1;
                range[1] = (range[0] + n)>(MAX)?(MAX):(range[0] + n);
            }
        }
        printf("<Node-0>---> [%d, %d]\n", range[0], range[1]);

        // Synchronize nodes
        result = 0;
        MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Calculate number of rotary primes for partitioned range
        for (i = range[0]; i <= range[1]; i++) {
            n = result;
            result += checkN(i, found, result);
        }
       
        // Add results from nodes
        int nodeFound[100];
        printf("Progress:\n%.2f%%\n", (float)100./numprocs );
        for (i = 1; i < numprocs; i++) {
            MPI_Recv(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&nodeFound, n, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Merge lists and update number of found primes
            j = 0;
            while(j < n) {
                if (nodeFound[j] == -1)
                    j = n;
                else {
                    for (k = 0; k < result; k++)
                        if (nodeFound[j] == found[k])
                            k = result+1;
                        
                    if (k == result) {
                        found[k] = nodeFound[j];
                        //printf("%d\n", found[k]);
                        result++;
                    }
                    j++;
                }
            }

            printf("%.2f%% \n", (float)((i+1)*100.)/numprocs);
        }

        // Stop timer
        gettimeofday(&t2, NULL);

        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.;
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.;
 
        printf("Final result: %d\nRuntime: %.4lfs\n", result, elapsedTime/1000.);
    } else {
        // Branch node
        
        // Recieve range from root node
        MPI_Recv(&range, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Synchronize nodes
        MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD);
        printf("<Node-%d>---> [%d, %d]\n", thisID, range[0], range[1]);
        // Calculate number of rotary primes for partitioned range
        for (i = range[0]; i <= range[1]; i++)
            result += checkN(i, found, result);

        // Send number of rotary primes in range to root node
        MPI_Send(&result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&found, result, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
  
    MPI_Finalize();
}

