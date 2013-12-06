#include <windows.h>

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