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

struct MPIShared {
	double *data;
	double *original;
	int size;
	MPI_Win win;
};

struct MPIProcInfo {
	int pid;
	int nproc;
};

long nextpow2(int num)
{
	long i = 1;
	while (num >>= 1)
		i <<= 1;

	return i == num ? i : i << 1;
}

void readInput(MPIProcInfo &pi, std::vector<double> &vals)
{
	if (pi.pid != 0)
		return;

	uint64_t input;
	while (std::cin >> input)
		vals.push_back(input);

	int angsize = nextpow2(vals.size());

	for (int i = 0; i < pi.nproc; i++)
		MPI_Send(&angsize, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
}

int recieveInputSize()
{
	int angsize = 0;
	MPI_Status stat;
	MPI_Recv(&angsize, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

	return angsize;
}

void computeAngles(MPIProcInfo &pi, MPIShared &angles)
{
	int gap = 2;
	double first;

	MPI_Win_fence(0, angles.win);

	MPI_Get(&first, 1, MPI_DOUBLE, 0, 0, 1, MPI_DOUBLE, angles.win);

	MPI_Win_fence(0, angles.win);
	
	int max = ceil(((double)angles.size)/(pi.nproc*gap));
	for (int i = 0; i < max; i++) {
		int idx = 2*pi.pid + i*pi.nproc*2;
		for (int j = idx; j < idx + 2; j++) {
			if (j < angles.size) // TODO: if power of 2 this can be deleted
				MPI_Get(&angles.data[j], 1, MPI_DOUBLE, 0, j, 1, MPI_DOUBLE, angles.win);

			MPI_Win_fence(0, angles.win);

			if (j < angles.size) { // TODO: if power of 2 this can be deleted
				angles.data[j] = j == 0 ? 0 : std::atan((angles.data[j]-first)/j);
				MPI_Put(&angles.data[j], 1, MPI_DOUBLE, 0, j, 1, MPI_DOUBLE, angles.win);
			}
		}
	}

	MPI_Win_fence(0, angles.win);
}

void upsweep(MPIProcInfo &pi, MPIShared &angles)
{
	// gap -> each processor takes evey nth element of array.
	// ngap -> gap in next level -> used to compute pid.
	int gap = 1, ngap = 2;

	for (int step = 0; step < std::log2(angles.size); step++) {
		int pgap = gap; gap = ngap; ngap <<= 1;
		int max = ceil(((double)angles.size)/(pi.nproc*gap));
		for (int i = 0; i < max; i++) {
			int idx = gap*pi.pid + i*pi.nproc*gap + gap-1;
			if (idx < angles.size) {
				double max = std::max(angles.data[idx], angles.data[idx-pgap]);
				int tgPid = (idx/ngap)%pi.nproc; // TODO: is modulo needed?
				MPI_Put(&max, 1, MPI_DOUBLE, tgPid, idx, 1, MPI_DOUBLE, angles.win);
				if (tgPid != 0)
					MPI_Put(&max, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, angles.win);
			}
		}

		MPI_Win_fence(0, angles.win);
	}
}

void downsweep(MPIProcInfo &pi, MPIShared &angles)
{
	int gap, ngap;
	for (int step = std::log2(angles.size); step > 0; step--) {
		gap = 1 << step; ngap = gap >> 1;
		int max = ceil(((double)angles.size)/(pi.nproc*gap));
		for (int i = 0; i < max; i++) {
			int idx = gap*pi.pid + i*pi.nproc*gap + gap-1;
			if (idx < angles.size) {
				double right = angles.data[idx];
				double max = std::max(angles.data[idx], angles.data[idx-ngap]);
				int tgPid = (idx/ngap)%pi.nproc; // TODO: is modulo needed?
				int tgPid2 = ((idx-ngap)/ngap)%pi.nproc; // TODO: is modulo needed?
				MPI_Put(&max, 1, MPI_DOUBLE, tgPid, idx, 1, MPI_DOUBLE, angles.win);
				MPI_Put(&right, 1, MPI_DOUBLE, tgPid2, idx-ngap, 1, MPI_DOUBLE, angles.win);
				if (tgPid != 0)
					MPI_Put(&max, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, angles.win);

				if (tgPid2 != 0)
					MPI_Put(&right, 1, MPI_DOUBLE, 0, idx-ngap, 1, MPI_DOUBLE, angles.win);
			}
		}

		MPI_Win_fence(0, angles.win);
	}
}

int main(int argc, char** argv)
{
	MPIProcInfo pi;

	// Initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &pi.nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &pi.pid);

	MPIShared angles;

	std::vector<double> angs;
	readInput(pi, angs);

	// For convenience is nsize rounded to nearest power of 2
	int origsize = angs.size();
	angles.size = recieveInputSize();

	MPI_Win_allocate(
		angles.size*sizeof(double),
		sizeof(double),
		MPI_INFO_NULL,
		MPI_COMM_WORLD,
		&angles.data,
		&angles.win
	);
	
	if (pi.pid == 0) {
		angs.resize(angles.size, -std::numeric_limits<double>::infinity()); // fill rest with smallest number
		std::memcpy(angles.data, angs.data(), sizeof(double)*angs.size());
	}

	computeAngles(pi, angles);
	std::memcpy(angs.data(), angles.data, sizeof(double)*angs.size());

	upsweep(pi, angles);
	// clear
	angles.data[angles.size-1] = -std::numeric_limits<double>::infinity();

	downsweep(pi, angles);

	if (pi.pid == 0) {
		std::cout << "_";
		for (int i = 1; i < origsize; i++) {
			if (angles.data[i] <= angs[i])
				std::cout << ",v";
			else
				std::cout << ",u";
		}
		std::cout << std::endl;
	}

	MPI_Win_free(&angles.win);
	MPI_Finalize(); 
	
	return 0;
}
