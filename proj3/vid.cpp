/**
 * Project 3 for PRL in academic year 2019/2020
 *
 * Date: 16. 3. 2020
 */

#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>

#include <mpi.h>

#define TAG 0

void readInput(std::vector<double> &vals, int nproc)
{
	uint64_t input;
	while (std::cin >> input)
		vals.push_back(input);

	int angsize = vals.size();
	for (int i = 0; i < nproc; i++)
		MPI_Send(&angsize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
}

int recieveInputSize()
{
	int angsize = 0;
	MPI_Status stat;
	MPI_Recv(&angsize, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

	return angsize;
}

void compueAngles(int pid, int nproc, MPI_Win &win, double* angles, int n)
{
	int gap = 2;
	double first;

	MPI_Win_fence(0, win);

	MPI_Get(&first, 1, MPI_DOUBLE, 0, 0, 1, MPI_DOUBLE, win);

	MPI_Win_fence(0, win);
	
	int max = ceil(((double)n)/(nproc*gap));
	for (int i = 0; i < max; i++) {
		int idx = 2*pid + i*nproc*2;
		for (int j = idx; j < idx + 2; j++)
			if (j < n)
				MPI_Get(&angles[j], 1, MPI_DOUBLE, 0, j, 1, MPI_DOUBLE, win);

		MPI_Win_fence(0, win);

		for (int j = idx; j < idx + 2; j++)
			if (j < n)
				angles[j] = j == 0 ? 0 : std::atan((angles[j]-first)/j);
	}

//	MPI_Win_fence(0, win);
}

void maxscan(int pid, int nproc, MPI_Win &win, double *angles, int n)
{
	int gap, ngap = 2;
	for (int step = 0; step < ceil(std::log2(n)-1); step++) {
		gap = ngap; ngap <<= 1;
		int max = ceil(((double)n)/(nproc*gap));
		for (int i = 0; i < max; i++) {
			int idx = gap*pid + i*nproc*gap;
			if (idx < n) {
				double sec = (idx+gap-1 < n) ? angles[idx+gap-1] : 0;

				double max = std::max(angles[idx], sec);
				int tgPid = ((idx+gap-1)/ngap)%nproc; // TODO: is modulo needed?

				MPI_Put(&max, 1, MPI_DOUBLE, tgPid, idx+gap-1, 1, MPI_DOUBLE, win);
			}
		}

		MPI_Win_fence(0, win);
	}
}

void downsweep(int pid, int nproc, MPI_Win &win, double *angles, int n)
{
}

int main(int argc, char** argv)
{
	int nproc;       // number of processors
	int pid;         // id of computing processor

	// Initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);

	double* angles = nullptr;
	MPI_Win win;

	std::vector<double> angs;
	if (pid == 0)
		readInput(angs, nproc);

	int angsize = recieveInputSize();

	MPI_Win_allocate(
		angsize*sizeof(double),
		sizeof(double),
		MPI_INFO_NULL,
		MPI_COMM_WORLD,
		&angles,
		&win
	);
	
	if (pid == 0)
		std::memcpy(angles, angs.data(), sizeof(double)*angsize);

	compueAngles(pid, nproc, win, angles, angsize);
	for (int i = 0; i < angsize; i++) {
		std::cout << pid << ": " << angles[i] << std::endl;
	}

	maxscan(pid, nproc, win, angles, angsize);

	if (pid == 0)
		std::cout << "maxscan = " << angles[angsize-1] << std::endl;

	MPI_Win_free(&win);
	MPI_Finalize(); 
	
	return 0;
}
