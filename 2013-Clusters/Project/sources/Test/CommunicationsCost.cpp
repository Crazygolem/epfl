#include <stdlib.h>
#include <dps/dps.h>
#include <dps/altserial.h>
#include <windows.h>
#include <vector>
#include <climits>
#include "../common/StopWatch.cpp"

class MasterThread {
	IDENTIFY(MasterThread);
public:
	StopWatch timer;
};

class SlaveThread {
	IDENTIFY(SlaveThread);
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

class Process : public dps::LeafOperation<Message, Message, SlaveThread> {
	IDENTIFY(Process);
public:
	// This public member forbids the compiler "optimizing"
	// by removing the useless operation
	unsigned long long dummy;

public:
	void execute(Message* in) {
		// Simulate expensive operation (optional, substracted from RTT)
		if (getController()->getConfig().isSet("slaves.constant") || getController()->getConfig().isSet("slaves.random") || getController()->getConfig().isSet("slaves.distributed")) {
			unsigned long long start = getThread()->timer.peek(StopWatch::MILLI);
			if (getController()->getConfig().isSet("slaves.random"))
				Sleep(rand() % 8000 + 2000); // Between 2s and 10s
			else if (getController()->getConfig().isSet("slaves.distributed"))
				Sleep(in->mid * 2000 + 2000);
			else
				Sleep(6000);

			// Removes sleeping time from RTT (addition because it's a starting time)
			in->timer = in->timer + (getThread()->timer.peek(StopWatch::MILLI) - start);
		}

		in->addRef();
		postDataObject(in);
	}
};

class Merge : public dps::MergeOperation<Message, ControlMessage, MasterThread> {
	IDENTIFY(Merge);
public:
	void execute(Message* in) {
		int payloadSize = getController()->getConfig().getValue("payload", 0);
		char* slavesComputations = "none";
		if (getController()->getConfig().isSet("slaves.constant"))
			slavesComputations = "heavy";
		else if (getController()->getConfig().isSet("slaves.random"))
			slavesComputations = "random";
		else if (getController()->getConfig().isSet("slaves.distributed"))
			slavesComputations = "distributed";

		do {
			unsigned long long now = getThread()->timer.peek(StopWatch::MILLI);
			// Output: payload_size heavy_processing message_id rtt_ms
			std::cout << payloadSize << " " << slavesComputations << " " << in->mid << " " << now - in->timer << std::endl;
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
	dps::ThreadCollection<SlaveThread> slaveThread =
		getController()->createThreadCollection<SlaveThread>("slave");

	const char *pattern = getController()->getConfig().getValue("pat","1");
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
	std::cout << "payload_size slaves_processing message_id rtt_ms" << std::endl;
	getController()->callSchedule(graph, new ControlMessage(nrMessages));
};

int main(int argc, char *argv[]) {
	return dps::dpsMain(argc, argv, new TestApp());
}
