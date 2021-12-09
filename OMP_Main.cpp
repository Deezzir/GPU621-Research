// Workshop 7 - Prefix Scan Comparison
 // w7.omp.cpp
 // 2018.11.04
 // Chris Szalwinski

#include <iostream>
#include <chrono>
#include <omp.h>

const int MAX_TILES = 8;

template <typename T, typename C>
T reduce(
	const T* in, // points to the data set
	int n,       // number of elements in the data set
	C combine,   // combine operation
	T initial    // initial value
) {

	for (int i = 0; i < n; i++)
		initial = combine(initial, in[i]);
	return initial;
}

template <typename T, typename C>
void excl_scan(
	const T* in,                // source data
	T* out,                     // output data
	int size,                   // size of data sets
	C combine,                  // combine operation
	T initial                   // initial value
) {

	if (size > 0) {
		for (int i = 0; i < size - 1; i++) {
			out[i] = initial;
			initial = combine(initial, in[i]);
		}
		out[size - 1] = initial;
	}
}

template <typename T, typename C>
void scan(
	const T* in,   // source data
	T* out,        // output data
	int size,      // size of source, output data sets
	C combine,     // combine expression
	T initial      // initial value
)
{
	if (size > 0) {
		// allocate memory for maximum number of tiles
		T stage1Results[MAX_TILES];
		T stage2Results[MAX_TILES];
#pragma omp parallel num_threads(MAX_TILES)
		{
			int itile = omp_get_thread_num();
			int ntiles = omp_get_num_threads();
			int tile_size = (size - 1) / ntiles + 1;
			int last_tile = ntiles - 1;
			int last_tile_size = size - last_tile * tile_size;

			// step 1 - reduce each tile separately
			stage1Results[itile] = reduce(in + itile * tile_size,
				itile == last_tile ? last_tile_size : tile_size,
				combine, T(0));
#pragma omp barrier

			// step 2 - perform exclusive scan on stage1Results
			// store exclusive scan results in stage2Results[]
#pragma omp single
			excl_scan(stage1Results, stage2Results, ntiles,
				combine, T(0));

			// step 3 - scan each tile separately using stage2Results[] 
			excl_scan(in + itile * tile_size, out + itile * tile_size,
				itile == last_tile ? last_tile_size : tile_size,
				combine, stage2Results[itile]);
		}
	}
}

// report system time
void reportTime(const char* msg, std::chrono::steady_clock::duration span) {
	auto ms = std::chrono::duration_cast<std::chrono::microseconds>(span);
	std::cout << msg << " - took - " <<
		ms.count() << " microseconds" << std::endl;
}

int main(int argc, char** argv) {
	if (argc > 2) {
		std::cerr << argv[0] << ": invalid number of arguments\n";
		std::cerr << "Usage: " << argv[0] << "\n";
		std::cerr << "Usage: " << argv[0] << "  power_of_2\n";
		return 1;
	}
	std::cout << "OMP Prefix Scan" << std::endl;

	// initial values for testing
	const int N = 9;
	const int in_[N]{ 3, 1, 7, 0, 1, 4, 5, 9, 2 };

	// command line arguments - none for testing, 1 for large arrays
	int n, nt{ 1 };
	if (argc == 1) {
		n = N;
	}
	else {
		n = 1 << std::atoi(argv[1]);
		if (n < N) n = N;
	}
	int* in = new int[n];
	int* out = new int[n];

	// initialize
	for (int i = 0; i < N; i++)
		in[i] = in_[i];
	for (int i = N; i < n; i++)
		in[i] = 1;
	auto add = [](int a, int b) { return a + b; };

	std::chrono::steady_clock::time_point ts, te;

	// Exclusive Prefix Scan
	ts = std::chrono::steady_clock::now();
	scan<int, decltype(add)>(in, out, n, add, (int)0);
	te = std::chrono::steady_clock::now();

	std::cout << MAX_TILES << " thread" << (nt > 1 ? "s" : "") << std::endl;
	for (int i = 0; i < N; i++)
		std::cout << out[i] << ' ';
	std::cout << out[n - 1] << std::endl;
	reportTime("Exclusive Scan", te - ts);

    delete[] in;
    delete[] out;
}
