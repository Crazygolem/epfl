#include <dps/dps.h>
#include <dps/altserial.h>
#include <windows.h>
#include <vector>
#include <climits>

class StopWatch {
public:
	static enum Unit {
		TENTH_MICRO = 1,
		MICRO = 10,
		MILLI = 10000,
		SECOND = 10000000
	};

public:
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

class MasterThread {
	IDENTIFY(MasterThread);
public:
	StopWatch timer;
};

class ControlMessage : public dps::SimpleSerial {
	IDENTIFY(ControlMessage);
public:
	int number;
	ControlMessage(int n = 0) {
		number = n;
	}
};

class Message : public dps::AutoSerial {
	CLASSDEF(Message)
	MEMBERS
		ITEM(int, mid)
		ITEM(unsigned long long, timer)
		ITEM(std::vector<unsigned int>, data)
	CLASSEND;
};

class Split : public dps::SplitOperation<ControlMessage, Message, MasterThread> {
	IDENTIFY(Split);
public:
	void execute(ControlMessage* in) {
		int limit = in->number;

		for (int i = 0; i < limit; ++i) {
			Message* message = new Message();
			message->mid = i;
			
			int payload = getController()->getConfig().getValue("payload", 0);
			for (int j = 0; j < payload; ++j)
				message->data.push_back(j);
			
			message->timer = getThread()->timer.peek(StopWatch::MILLI);
			
			postDataObject(message);
		}
	}
};

class Process : public dps::LeafOperation<Message, Message> {
	IDENTIFY(Process);
public:
	// This public member forbids the compiler "optimizing"
	// by removing the useless operation
	unsigned long long dummy;

public:
	void execute(Message* in) {
		// Expensive operation (optional)
		if (getController()->getConfig().getValue("heavyProcessing", 0) == 1)
			for (int i = 0; i < INT_MAX; ++i)
				dummy = i * i * i;

		in->addRef();
		postDataObject(in);
	}
};

class Merge : public dps::MergeOperation<Message, ControlMessage, MasterThread> {
	IDENTIFY(Merge);
public:
	void execute(Message* in) {
		int payloadSize = getController()->getConfig().getValue("payload", 0);
		int heavyComputations = getController()->getConfig().getValue("heavyProcessing", 0);

		do {
			unsigned long long now = getThread()->timer.peek(StopWatch::MILLI);
			// Output: payload_size heavy_processing message_id rtt_ms
			std::cout << payloadSize << " " << heavyComputations << " " << in->mid << " " << now - in->timer << std::endl;
		} while ((in = waitForNextDataObject()) != NULL);

		postDataObject(new ControlMessage(0));
	}
};

class TestApp : public dps::Application {
	IDENTIFY(TestApp);

public:
	virtual void start();
};

void TestApp::start() {
	dps::ThreadCollection<MasterThread> masterThread =
		getController()->createThreadCollection<MasterThread>("master", 1, false);
	dps::StatelessThreadCollection slaveThread =
		getController()->createStatelessThreadCollection("slave");

	const char *pattern=getController()->getConfig().getValue("pat","1");
	if (DPS_FAILED(masterThread.addThread("0")) || DPS_FAILED(slaveThread.addThread(dps::MPIMapper::get(pattern)))) {
		std::cout << "Could not map thread collection" << std::endl;
		return;
	}

	dps::FlowgraphBuilder graphB =
		dps::FlowgraphNode<Split, dps::ZeroRoute>(masterThread) >>
		dps::FlowgraphNode<Process, dps::RoundRobinRoute>(slaveThread) >>
		dps::FlowgraphNode<Merge, dps::ZeroRoute>(masterThread);

	dps::Flowgraph graph = getController()->createFlowgraph("graph", graphB);

	int nrMessages = getController()->getConfig().getValue("messages", 1);
	std::cout << "payload_size heavy_processing message_id rtt_ms" << std::endl;
	getController()->callSchedule(graph, new ControlMessage(nrMessages));
};

int main(int argc, char *argv[]) {
	return dps::dpsMain(argc, argv, new TestApp());
}
