#if _MSC_VER >= 1400 // If we are using VS 2005 or greater
# define _CRT_SECURE_NO_WARNINGS // Disable warnings about deprecated functions
#endif

#include <dps/dps.h>
#include <dps/altserial.h>
#include <math.h>
#include <windows.h>
#include <climits>
#include <iostream>
#include <sstream>
#include "../common/Range.cpp"
#include "../common/Utils.cpp"

using namespace std;

enum Verbosity { NONE, QUIET, DEFAULT, VERBOSE, DEBUG };
Verbosity toVerbosity(string v) {
	return (v == "none") ? NONE :
		(v == "quiet") ? QUIET :
		(v == "default") ? DEFAULT :
		(v == "verbose") ? VERBOSE :
		(v == "debug") ? DEBUG :
		DEFAULT;
}
// Need a macro otherwise getController() won't be available
#define GET_VERBOSITY toVerbosity(string(getController()->getConfig().getValue("verbosity", "default")))
#define VERBOSITY(v) if (!(NODEBUG && (v == DEBUG)) && (GET_VERBOSITY >= v))
#define DEBUG(msg) VERBOSITY(DEBUG) { cout << "## T" << getThreadIndex() << ": " << msg << endl; } else (void)0
#define NODEBUG false // Set to `true` to get full speed (compiler optimizations for VERBOSITY with DEBUG)



class MasterThread {
	IDENTIFY(MasterThread);
public:
	unsigned int slaves;
	unsigned int min;
	unsigned int max;
	double startTime;
};



class ControlMessage : public dps::SimpleSerial {
	IDENTIFY(ControlMessage);

public:
	unsigned int slaves;
	unsigned int chunkId;
	unsigned int min;
	unsigned int max;

	ControlMessage(unsigned int slaves = 0, unsigned int chunkId = -1, unsigned int min = 0, unsigned int max = 0) {
		this->slaves = slaves;
		this->chunkId = chunkId;
		this->min = min;
		this->max = max;
	}
};

class DataMessage : public dps::AutoSerial {
	CLASSDEF(DataMessage)
	MEMBERS
		ITEM(vector<unsigned int>, primes)
	CLASSEND;

public:
	DataMessage(unsigned int size = 0) {
		this->primes.reserve(size);
	}
};


//! Split operation
class StartSlaves : public dps::SplitOperation<ControlMessage, ControlMessage, MasterThread> {
	IDENTIFY(StartSlaves);
public:
	void execute(ControlMessage* in) {
		getThread()->min = in->min;
		getThread()->max = in->max;
		getThread()->slaves = in->slaves;
		getThread()->startTime = dps::Timer::getMillis();

		for (unsigned int i = 0; i < in->slaves; i++) {
			postDataObject(new ControlMessage(in->slaves, i, in->min, in->max));
		}
	}
};

//! Process operation
class Sieve : public dps::LeafOperation<ControlMessage, DataMessage> {
	IDENTIFY(Sieve);
public:
	void execute(ControlMessage* in) {
		// `max` is inclusive, `limit` is exclusive
		const unsigned int limit = in->max + 1;
		const unsigned int srMax = (unsigned int) sqrt((double) in->max);
		const unsigned int srLimit = srMax + 1;
		const unsigned int maxChunkSize = (unsigned int) ceil((double)(in->max - srMax) / in->slaves);
		const unsigned int chunkMin = srMax + 1 + in->chunkId * maxChunkSize; // threadIndex ~ rank and is 0-based
		const unsigned int chunkLimit = min(limit, chunkMin + maxChunkSize);

		DEBUG(1);
		Range* root = new Range(in->min, srLimit - in->min);
		Range* chunk = new Range(chunkMin, chunkLimit - chunkMin);

		DEBUG(2);
		findPrimes(in->min, in->max, chunkMin, chunkLimit, root, chunk);

		unsigned int outSize = chunk->countUnmarked() + (in->chunkId == 0) ? root->countUnmarked() : 0;
		//DataMessage* out = new DataMessage(outSize);
		DataMessage* out = new DataMessage(outSize / 2);

		DEBUG(3);
		if (in->chunkId == 0) {
			for (unsigned int i = root->start; i < root->start + root->length; i++) {
				if (root->isUnmarked(i)) {
					out->primes.push_back(i);
				}
			}
		}
		delete root;

		DEBUG(4);
		for (unsigned int i = chunk->start; i < chunk->start + chunk->length; i++) {
			if (chunk->isUnmarked(i)) {
				out->primes.push_back(i);
			}
		}
		delete chunk;

		DEBUG("Sending " << out->primes.size() << " primes.");
		postDataObject(out);
		DEBUG(5);
	}

	void findPrimes(unsigned int min, unsigned int max, unsigned int chunkMin, unsigned int chunkLimit, Range* root, Range* chunk) {
		// We sieve only up to the square root of the max
		// Since we compare ints, the casting is fine
		const unsigned int srMax = (unsigned int) sqrt((double) max);

		// Crossing-out up to sqrt(limit)
		for (unsigned int n = min; n <= srMax; n++) {
			// We don't process if not a prime
			// After a pass, the first next uncrossed number is a prime
			if (root->isUnmarked(n)) {
				// Crossing out all multiples of current prime up to sqrt(L)
				for (unsigned int k = n * n; k <= srMax; k += n) {
					root->cross(k);
				}
				// Crossing out all mutliples of current prime in the chunk
				// starting with the first multiple of the prime greater or equal to the chunk
				// lower limit
				for (unsigned int k = chunkMin - ((chunkMin - 1) % n + 1) + n; k < chunkLimit; k += n) {
					chunk -> cross(k);
				}
			}
		}
	}
};

//! Merge operation
class CollectPrimes : public dps::MergeOperation<DataMessage, ControlMessage, MasterThread> {
	IDENTIFY(CollectPrimes);
public:
	void execute(DataMessage* in) {
		unsigned int primesCount = 0;
		ostringstream coutbuffer;

		do {
			DEBUG("# primes in message: " << in->primes.size());
			primesCount += in->primes.size();

			VERBOSITY(DEBUG)
				copy(in->primes.begin(), in->primes.end(), ostream_iterator<unsigned int>(coutbuffer, "\n"));

		} while ((in = waitForNextDataObject()) != NULL);
		
		// We stop the timer when all messages have been received.
		double elapsed = dps::Timer::getMillis() - getThread()->startTime;

		if (getController()->getConfig().isSet("printprimes"))
			cout << coutbuffer.str() << endl;

		VERBOSITY(VERBOSE)
			cout << "--------------- Stats ---------------" << endl;
		VERBOSITY(DEFAULT)
			cout << "range_start" << "\t" << "range_end" << "\t" << "nr_slaves" << "\t" << "primes_count" << "\t" << "wall_time_ms" << endl;
		VERBOSITY(QUIET)
			cout << getThread()->min << "\t" << getThread()->max << "\t" << getThread()->slaves << "\t" << primesCount << "\t" << elapsed << endl;
		
		// Terminate the operation
		postDataObject(new ControlMessage());
	}
};

class Parallel : public dps::Application {
	IDENTIFY(Parallel);
public:
	//! Start function
	virtual void start();
	ControlMessage* getParameters();
};

void Parallel::start() {
	if (GET_VERBOSITY == DEBUG && NODEBUG)
		cout << "Verbosity 'debug' not available. Set NODEBUG to false to enable it." << endl;

	ControlMessage* params = getParameters();

	VERBOSITY(DEFAULT)
		cout << "Searching for primes in the range [2," << params->max << "]." << endl;

	// Create thread collections
	dps::ThreadCollection<MasterThread> master =
		getController()->createThreadCollection<MasterThread>("master");
	dps::StatelessThreadCollection slaves =
		getController()->createStatelessThreadCollection("slaves");

	// Map thread collections
	if (DPS_FAILED(master.addThread("0"))) {
		cout << "Could not map master thread collection." << endl;
		return;
	}

	string pattern;
	// Arguments -map and -pat overrides the standard pattern
	if (getController()->getConfig().isSet("map"))
		pattern = string(getController()->getConfig().getValue("map", ""));
	else if (getController()->getConfig().isSet("pat"))
		pattern = dps::MPIMapper::get(getController()->getConfig().getValue("pat", ""));
	else
		pattern = dps::MPIMapper::get(concat(toString(params->slaves), string("+1")).c_str());

	if (DPS_FAILED(slaves.addThread(pattern))) {
		cout << "Could not map slave threads collection. Pattern: '" << pattern << "'" << endl;
		return;
	}

	// Create flow graph
	dps::FlowgraphBuilder flowgraphB =
		dps::FlowgraphNode<StartSlaves, dps::ZeroRoute>(master) >>
		dps::FlowgraphNode<Sieve, dps::RoundRobinRoute>(slaves) >>
		dps::FlowgraphNode<CollectPrimes, dps::ZeroRoute>(master);
	
	dps::Flowgraph flowgraph = getController()->createFlowgraph("graph", flowgraphB);

	// Invoke parallel schedule
	getController()->callSchedule(flowgraph, params);

	VERBOSITY(VERBOSE)
		cout << endl << "That's all Folks!" << endl;
}

ControlMessage* Parallel::getParameters() {
	ControlMessage* out = new ControlMessage();

	// We need some nasty tricks in order to retrieve unsigned ints...
	out->min = strtoul(getController()->getConfig().getValue("min", "2"), NULL, 10);
	out->max = strtoul(getController()->getConfig().getValue("max", toString(INT_MAX).c_str()), NULL, 10);
	out->slaves = strtoul(getController()->getConfig().getValue("slaves", "2"), NULL, 10);

	return out;
};

//! Starts up application
int main(int argc, char *argv[]) {
	return dps::dpsMain(argc,argv,new Parallel());
}