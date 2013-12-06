#include <climits>
#include <iostream>
#include <sstream>
#include <bitset>

using namespace std;

#define MAX_CHUNK_SIZE SIZE_MAX // Max value that size_t can hold

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