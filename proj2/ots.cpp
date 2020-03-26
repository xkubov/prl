/**
 * @file ots.cpp
 * @brief Implementation of odd-even transposition sort.
 * @author Peter Kubov
 * @copyright This file is distributed under GPLv3 license.
 */

#include <sys/time.h>

#include <mpi.h>
#include <iostream>
#include <fstream>

#define TAG 0
#define INPUT_FILE "numbers"

void parseInput()
{
	int invar= 0;
	std::fstream input(INPUT_FILE, std::ios::in | std::ios::binary);

	bool first = true;
	for (int number = input.get(); input.good(); number = input.get()) {
		std::cout << (first ? "" : " ") << number; first = false;
		MPI_Send(&number, 1, MPI_INT, invar++, TAG, MPI_COMM_WORLD);
	}

	std::cout << std::endl;
	input.close();
}

void printSorted(int nproc)
{
	MPI_Status stat;

	for (int i = 0, val; i < nproc; i++) {
		MPI_Recv(&val, 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat);
		std::cout << val << std::endl;
	}
}

int main(int argc, char *argv[])
{
	int nproc;       // number of processors
	int pid;         // id of computing processor
	int val, nval;   // value of computing (neighboring) processor

	MPI_Status stat;

	// Initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);

	// Processor with ID 0 does parsing of input.
	// -> sends values to all other processors.
	if (pid == 0)
		parseInput();

	// Receive value
	// All processors including the one that sends values.
	MPI_Recv(&val, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

	int oddN = (nproc/2)*2-1;
	int evenN = ((nproc-1)/2)*2;
	int N = nproc/2;

	struct timeval t1, t2;

	gettimeofday(&t1, 0);
	for (int k = 0; k < N; k++) {
		if ((pid%2 == 0) && (pid < oddN)) { // Only evens -> except last one if odd N
			// Send message to neighbor.
			MPI_Send(&val, 1, MPI_INT, pid+1, TAG, MPI_COMM_WORLD);
			// Recieve message from neighbor.
			MPI_Recv(&val, 1, MPI_INT, pid+1, TAG, MPI_COMM_WORLD, &stat);
		}
		else if (pid <= oddN) { // All odds (We must make sure last even does not get here if odd N)
			// Recieve from neighbor
			MPI_Recv(&nval, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD, &stat);
			if (nval > val) {
				MPI_Send(&val, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD);
				val = nval;
			}
			else {
				MPI_Send(&nval, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD);
			}
		}

		if ((pid%2 == 1) && (pid < evenN)) { // Only odds -> except last one if even N
			// Send message to neighbor.
			MPI_Send(&val, 1, MPI_INT, pid+1, TAG, MPI_COMM_WORLD);
			// Recieve message from neighbor.
			MPI_Recv(&val, 1, MPI_INT, pid+1, TAG, MPI_COMM_WORLD, &stat);
		}
		else if (pid <= evenN && pid != 0) { // All evens (We must make sure last odd does not get here if even N)
			// Recieve from neighbor
			MPI_Recv(&nval, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD, &stat);
			if (nval > val) {
				MPI_Send(&val, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD);
				val = nval;
			}
			else {
				MPI_Send(&nval, 1, MPI_INT, pid-1, TAG, MPI_COMM_WORLD);
			}
		}
	}
	gettimeofday(&t2, 0);

	MPI_Send(&val, 1, MPI_INT, 0, TAG,  MPI_COMM_WORLD);

	if(pid == 0) {
		printSorted(nproc);
		double t = (1000000.0 * (t2.tv_sec - t1.tv_sec) + t2.tv_usec - t1.tv_usec);
		std::cerr << t << std::endl;
	}

	MPI_Finalize(); 
	return 0;

}
