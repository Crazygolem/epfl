#if _MSC_VER >= 1400 // If we are using VS 2005 or greater
# define _CRT_SECURE_NO_WARNINGS // Disable warnings about deprecated functions
#endif

#include <math.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <bitset>

using namespace std;

#define MAX_CHUNK_SIZE SIZE_MAX // Max value that size_t can hold
#define VERBOSITY(v) if(verbosity >= v)

enum Verbosity {
	NONE = 0,
	QUIET = 1,
	DEFAULT = 2,
	VERBOSE = 3
};

/* ctime's time function does not provide time granularity finer than seconds.
 * So here is a Windows-only way to get millis.
 * Meh... */
class StopWatch {
public:
	static enum Unit {
		TENTH_MICRO = 1,
		MICRO = 10,
		MILLI = 10000,
		SECOND = 10000000
	};

private:
	ULARGE_INTEGER start;

public:
	StopWatch() {
		getTime(&start);
	}

	unsigned long long peek(Unit u) {
		ULARGE_INTEGER now;
		getTime(&now);
		return (now.QuadPart - start.QuadPart) / u;
	}

private:
	void getTime(ULARGE_INTEGER* ul) {
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		ul->LowPart = ft.dwLowDateTime;
		ul->HighPart = ft.dwHighDateTime;
	}
};


class Range {
private:
	bitset<MAX_CHUNK_SIZE> marked;	// Initialized to 0s

public:
	unsigned int start;
	size_t length;

public:
	Range(unsigned int start, size_t length) {
		if (length > MAX_CHUNK_SIZE) {
			ostringstream msg;
			msg << "Range length exceeds MAX_CHUNK_SIZE (" << MAX_CHUNK_SIZE << "): " << length;
			cout << msg.str() << endl;
			throw length_error(msg.str());
		}

		this->start = start;
		this->length = length;
	}

	bool isMarked(unsigned int num) {
		return marked.test(idx(num));
	}

	bool isUnmarked(unsigned int num) {
		return !isMarked(num);
	}

	void cross(unsigned int num) {
		marked.set(idx(num));
	}

	void uncross(unsigned int num) {
		marked.reset(idx(num));
	}

	size_t countMarked() {
		return marked.count();
	}

	size_t countUnmarked() {
		return length - countMarked();
	}

private:
	inline size_t idx(unsigned int num) {
		// Carefully crafted code should not require the test, which increases the running time up to 200%
		//if ((start <= num) && (num < (start + length)))
			return num - start;
		
		ostringstream msg;
		msg << "Out of range [" << start << "," << start + length - 1 << "]: " << num;
		cout << msg.str() << endl;
		throw std::out_of_range(msg.str());
	}
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
		if (ns->isUnmarked(n)) {
			// Crossing out all multiples of current prime
			// Numbers smaller than n^2 have already been crossed in previous iterations
			for (unsigned int k = n * n; k <= max; k += n) {
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