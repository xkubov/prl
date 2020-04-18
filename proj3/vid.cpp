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

#include <chrono>
#define LOG_TIME true

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
	long i = 1; int j = num;
	while (j >>= 1)
		i <<= 1;

	return i == num ? i : i << 1;
}

void computeAngles(MPIProcInfo &pi, MPIShared &angles, int argc, char** argv)
{
	int gap = 2;
	double first = std::strtol(argv[0], nullptr, 10);

	int max = ceil(((double)argc)/(pi.nproc*gap));
	for (int i = 0; i < max; i++) {
		int idx = 2*pi.pid + i*pi.nproc*2;
		for (int j = idx; j < idx + 2; j++) {
			if (j < argc) {
				auto elem = std::strtol(argv[j], nullptr, 10);
				angles.data[j] = j == 0 ? -std::numeric_limits<double>::infinity(): std::atan((elem-first)/j);
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

	argc--;
	argv = &argv[1];

	MPIShared angles = {}; // set nullptr to pointers

	// For convenience is nsize rounded to nearest power of 2
	angles.size = nextpow2(argc);

	MPI_Win_allocate(
		angles.size*sizeof(double),
		sizeof(double),
		MPI_INFO_NULL,
		MPI_COMM_WORLD,
		&angles.data,
		&angles.win
	);
	
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	computeAngles(pi, angles, argc, argv);

	upsweep(pi, angles);
	// clear
	angles.data[angles.size-1] = -std::numeric_limits<double>::infinity();
	MPI_Win_fence(0, angles.win);

	downsweep(pi, angles);
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	if (pi.pid == 0) {
		double first = std::strtol(argv[0], nullptr, 10);
		std::cout << "_";
		for (int i = 1; i < argc; i++) {
			double elem = std::strtol(argv[i], nullptr, 10);
			elem = std::atan((elem-first)/i);
			if (angles.data[i] < elem)
				std::cout << ",v";
			else
				std::cout << ",u";
		}
		std::cout << std::endl;

		if (LOG_TIME) {
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			std::cerr << duration << std::endl;
		}
	}

	MPI_Win_free(&angles.win);
	MPI_Finalize(); 
	
	return 0;
}
