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
#include <bitset>
#include "../common/Range.cpp"
#include "../common/Utils.cpp"

using namespace std;

class MasterThread {
	IDENTIFY(MasterThread);
public:
	unsigned int slavesCount;
};

class ControlMessage : public dps::SimpleSerial {
	IDENTIFY(ControlMessage);
};

class DataMessage : public dps::SimpleSerial {
	IDENTIFY(DataMessage);
public:
	vector<unsigned int> primes;

	DataMessage(unsigned int size = 0) {
		this->primes.reserve(size);
	}
};

//! Split operation
class StartSlaves : public dps::SplitOperation<ControlMessage, ControlMessage, MasterThread> {
	IDENTIFY(StartSlaves);

public:
	void execute(ControlMessage* in) {
		getThread()->slavesCount = getController()->getThreadCollection<dps::StatelessThread>("slaves").getSize();
		
		for (unsigned int i = 0; i < getThread()->slavesCount; i++) {
			postDataObject(new ControlMessage());
		}
	}
};

//! Process operation
class Sieve : public dps::LeafOperation<ControlMessage, DataMessage> {
	IDENTIFY(Sieve);

private:
	Range* root;
	Range* chunk;

public:
	void execute(ControlMessage* in) {
		// TODO
	}
};

//! Merge operation
class CollectPrimes : public dps::MergeOperation<DataMessage, ControlMessage, MasterThread> {
	IDENTIFY(CollectPrimes);

private:
	unsigned int primesCount;

public:
	void execute(DataMessage* in) {
		primesCount = 0;

		do {
			// In this implementation, we don't actually write found primes to disk
			primesCount += (unsigned int) in->primes.size();

		} while ((in = waitForNextDataObject()) != NULL);

		// Terminate the operation
		postDataObject(new ControlMessage());
	}
};

class Parallel : public dps::Application {
	IDENTIFY(Parallel);
public:
	//! Start function
	virtual void start();
};

void Parallel::start() {
	// Create thread collections
	std::cout << "Creating threads" << std::endl;
	dps::ThreadCollection<MasterThread> master =
		getController()->createThreadCollection<MasterThread>("master");
	dps::StatelessThreadCollection slaves =
		getController()->createStatelessThreadCollection("slaves");

	// Map thread collections
	if (DPS_FAILED(master.addThread("0"))) {
		cout << "Could not map master thread collection." << endl;
		return;
	}
	
	const char* slavesCount = getController()->getConfig().getValue("slaves", "1");
	string pattern = dps::MPIMapper::get(concat(slavesCount, "x1+1"));
	if (DPS_FAILED(slaves.addThread(pattern))) {
		cout << "Count not map slave threads collection." << endl;
	}

	// Create flow graph
	dps::FlowgraphBuilder flowgraphB =
		dps::FlowgraphNode<StartSlaves, dps::ZeroRoute>(master) >>
		dps::FlowgraphNode<Sieve, dps::RoundRobinRoute>(slaves) >>
		dps::FlowgraphNode<CollectPrimes, dps::ZeroRoute>(master);
	dps::Flowgraph flowgraph = getController()->createFlowgraph("graph", flowgraphB);

	// Invoke parallel schedule
	getController()->callSchedule(flowgraph, new ControlMessage());

	printf("Done :)");
	fflush(stdout);
}

//! Starts up application
int main(int argc, char *argv[]) {
	return dps::dpsMain(argc,argv,new Parallel());
}