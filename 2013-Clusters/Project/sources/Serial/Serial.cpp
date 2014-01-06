#if _MSC_VER >= 1400 // If we are using VS 2005 or greater
# define _CRT_SECURE_NO_WARNINGS // Disable warnings about deprecated functions
#endif

#include <math.h>
#include <windows.h>
#include <climits>
#include <iostream>
#include <sstream>
#include <bitset>
#include "../common/StopWatch.cpp"
#include "../common/Range.cpp"

using namespace std;

#define MAX_CHUNK_SIZE SIZE_MAX // Max value that size_t can hold
#define VERBOSITY(v) if(verbosity >= v)

enum Verbosity {
	NONE = 0,
	QUIET = 1,
	DEFAULT = 2,
	VERBOSE = 3
};

Range* findPrimes(unsigned int min, unsigned int max) {
	// `max` is inclusive, `limit` is exclusive
	const unsigned int limit = max + 1;
	Range* ns = new Range(min, limit - min);
	
	// We sieve only up to the square root of the max
	// Since we compare ints, the casting is fine
	const unsigned int srMax = (unsigned int) sqrt((double) max);

	// Crossing-out up to sqrt(limit)
	for (unsigned int n = min; n <= srMax; n++) {
		// We don't process if not a prime
		// After a pass, the first next uncrossed number is a prime
		count(n, max);
		if (ns->isUnmarked(n)) {
			// Crossing out all multiples of current prime
			// Numbers smaller than n^2 have already been crossed in previous iterations
			for (unsigned int k = n * n; k <= max; k += n) {
				count(k, max);
				ns->cross(k);
			}
		}
	}

	return ns;
}

void start(unsigned int max, Verbosity verbosity) {
	const unsigned int min = 2;

	unsigned long long elapsed;
	Range* sieveResult;

	VERBOSITY(DEFAULT)
		cout << "Searching for primes in the range [2," << max << "]." << endl;

	StopWatch timer;
	sieveResult = findPrimes(min, max);
	elapsed = timer.peek(StopWatch::MILLI);

	VERBOSITY(DEFAULT) {
		cout << "--------------- Stats ---------------" << endl;
		cout << "range_start" << "\t" << "range_end" << "\t" << "primes_count" << "\t" << "wall_time_ms" << endl;
	}
	VERBOSITY(QUIET) {
		cout << min << "\t" << max << "\t" << sieveResult->countUnmarked() << "\t" << elapsed << endl;
	}

	VERBOSITY(VERBOSE) {
		unsigned int limit = sieveResult->start + sieveResult->length;

		cout << "--------------- Primes --------------" << endl;
		for (unsigned int n = sieveResult->start; n < limit; ++n) {
			if (sieveResult->isUnmarked(n))
				cout << n << endl;
		}
		cout << endl << "That's all Folks!" << endl;
	}
}

int main(int argc, char *argv[]) {
	unsigned int max = INT_MAX;
	Verbosity verbose = DEFAULT;
	
	if (argc > 1)
		max = strtoul(argv[1], NULL, 10) & UINT_MAX;
	if (argc > 2) {
		if (strcmp("verbose", argv[2]) == 0)
			verbose = VERBOSE;
		else if (strcmp("default", argv[2]) == 0)
			verbose = DEFAULT;
		else if (strcmp("quiet", argv[2]) == 0)
			verbose = QUIET;
		else if (strcmp("none", argv[2]) == 0)
			verbose = NONE;
	}

	if (max < 2) {
		ostringstream msg;
		msg << "Max must be within [2," << UINT_MAX - 1 << "]: " << max;
		cout << msg.str() << endl;
		throw length_error(msg.str());
	}

	start(max, verbose);
}