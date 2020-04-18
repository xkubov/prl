/**
 * Project 3 for PRL in academic year 2019/2020
 *
 * Date: 16. 3. 2020
 */

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#include <mpi.h>

#define TAG 0

long nextpow2(int num)
{
	long i = 1;
	while (num >>= 1)
		i <<= 1;

	return i == num ? i : i << 1;
}

void readInput(std::vector<double> &vals, int nproc)
{
	uint64_t input;
	while (std::cin >> input)
		vals.push_back(input);

	int angsize = nextpow2(vals.size());

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

void computeAngles(int pid, int nproc, MPI_Win &win, double* angles, int n)
{
	int gap = 2;
	double first;

	MPI_Win_fence(0, win);

	MPI_Get(&first, 1, MPI_DOUBLE, 0, 0, 1, MPI_DOUBLE, win);

	MPI_Win_fence(0, win);
	
	int max = ceil(((double)n)/(nproc*gap));
	for (int i = 0; i < max; i++) {
		int idx = 2*pid + i*nproc*2;
		for (int j = idx; j < idx + 2; j++) {
			if (j < n) // TODO: if power of 2 this can be deleted
				MPI_Get(&angles[j], 1, MPI_DOUBLE, 0, j, 1, MPI_DOUBLE, win);

			MPI_Win_fence(0, win);

			if (j < n) { // TODO: if power of 2 this can be deleted
				angles[j] = j == 0 ? 0 : std::atan((angles[j]-first)/j);
				MPI_Put(&angles[j], 1, MPI_DOUBLE, 0, j, 1, MPI_DOUBLE, win);
			}
		}
	}

	MPI_Win_fence(0, win);
}

void maxscan(int pid, int nproc, MPI_Win &win, double *angles, int n)
{
	// TODO: remove... just for nice output
	MPI_Win_fence(0, win);
	// gap -> each processor takes evey nth element of array.
	// ngap -> gap in next level -> used to compute pid.
	int gap = 1, ngap = 2;

	for (int step = 0; step < std::log2(n); step++) {
		int pgap = gap; gap = ngap; ngap <<= 1;
		int max = ceil(((double)n)/(nproc*gap));
		for (int i = 0; i < max; i++) {
			int idx = gap*pid + i*nproc*gap + gap-1;
			if (idx < n) {
				double max = std::max(angles[idx], angles[idx-pgap]);
				int tgPid = (idx/ngap)%nproc; // TODO: is modulo needed?
				MPI_Put(&max, 1, MPI_DOUBLE, tgPid, idx, 1, MPI_DOUBLE, win);
				if (tgPid != 0)
					MPI_Put(&max, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, win);
			}
		}

		MPI_Win_fence(0, win);
	}
}

void downsweep(int pid, int nproc, MPI_Win &win, double *angles, int n)
{
	// TODO: remove... just for nice output
	MPI_Win_fence(0, win);

	int gap, ngap;
	for (int step = std::log2(n); step > 0; step--) {
		gap = 1 << step; ngap = gap >> 1;
		int max = ceil(((double)n)/(nproc*gap));
		for (int i = 0; i < max; i++) {
			int idx = gap*pid + i*nproc*gap + gap-1;
			if (idx < n) {
				double right = angles[idx];
				double max = std::max(angles[idx], angles[idx-ngap]);
				int tgPid = (idx/ngap)%nproc; // TODO: is modulo needed?
				int tgPid2 = ((idx-ngap)/ngap)%nproc; // TODO: is modulo needed?
				MPI_Put(&max, 1, MPI_DOUBLE, tgPid, idx, 1, MPI_DOUBLE, win);
				MPI_Put(&right, 1, MPI_DOUBLE, tgPid2, idx-ngap, 1, MPI_DOUBLE, win);
				if (tgPid != 0)
					MPI_Put(&max, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, win);

				if (tgPid2 != 0)
					MPI_Put(&right, 1, MPI_DOUBLE, 0, idx-ngap, 1, MPI_DOUBLE, win);
			}
		}

		MPI_Win_fence(0, win);
	}
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

	// For convenience is nsize rounded to nearest power of 2
	int origsize = angs.size();
	int angsize = recieveInputSize();

	MPI_Win_allocate(
		angsize*sizeof(double),
		sizeof(double),
		MPI_INFO_NULL,
		MPI_COMM_WORLD,
		&angles,
		&win
	);
	
	if (pid == 0) {
		angs.resize(angsize, -std::numeric_limits<double>::infinity()); // fill rest with smallest number
		std::memcpy(angles, angs.data(), sizeof(double)*angs.size());
	}

	computeAngles(pid, nproc, win, angles, angsize);

//	if (pid == 0) {
//		std::cout << "After angle compute:" << std::endl;
//		for (int i = 0; i < angsize; i++) {
//			std::cout << "\ti: " << i << " = " << angles[i] << std::endl;
//		}
//		std::cout << "====" << std::endl;
//	}

	maxscan(pid, nproc, win, angles, angsize);
	// clear
	angles[angsize-1] = -std::numeric_limits<double>::infinity();

//	if (pid == 0) {
//		std::cout << "After maxscan:" << std::endl;
//		for (int i = 0; i < angsize; i++) {
//			std::cout << "\tm: " << i << " = " << angles[i] << std::endl;
//		}
//		std::cout << "maxscan = " << angles[angsize-1] << std::endl;
//	}
	downsweep(pid, nproc, win, angles, angsize);

	if (pid == 0) {
		std::cout << "_";
		for (int i = 1; i < origsize; i++) {
			if (angles[i] <= angs[i])
				std::cout << ",v";
			else
				std::cout << ",u";
		}
	}

	MPI_Win_free(&win);
	MPI_Finalize(); 
	
	return 0;
}
