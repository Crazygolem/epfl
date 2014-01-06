#include <stdlib.h>
#include <dps/dps.h>
#include <dps/altserial.h>
#include <windows.h>
#include <vector>
#include <climits>
#include <string>

class MasterThread {
	IDENTIFY(MasterThread);
public:
	double timer;
};

class SlaveThread {
	IDENTIFY(SlaveThread);
public:
	double timer;
};

class ControlMessage : public dps::SimpleSerial {
	IDENTIFY(ControlMessage);
public:
	int number;
	double timer;

	ControlMessage(int n = 0, double t = 0) {
		number = n;
		timer = t;
	}
};

class Message : public dps::AutoSerial {
	CLASSDEF(Message)
	MEMBERS
		ITEM(int, mid)
		ITEM(double, timer)
		ITEM(std::vector<unsigned int>, data)
	CLASSEND;
};

class Split : public dps::SplitOperation<ControlMessage, ControlMessage, MasterThread> {
	IDENTIFY(Split);
public:
	void execute(ControlMessage* in) {
		int limit = in->number;

		for (int i = 0; i < limit; ++i) {
			ControlMessage* message = new ControlMessage(i);
			message->timer = dps::Timer::getMillis();
			postDataObject(message);
		}
	}
};

class Process : public dps::LeafOperation<ControlMessage, Message, SlaveThread> {
	IDENTIFY(Process);
public:
	void execute(ControlMessage* in) {
		double start = dps::Timer::getMillis();

		Message* out = new Message();
		out->mid = in->number;

		int payload = getController()->getConfig().getValue("payload", 0);
		for (int i = 0; i < payload; ++i)
			out->data.push_back(i);

		// Simulate expensive operation (optional, substracted from RTT)
		std::string delay = std::string(getController()->getConfig().getValue("delay", "none"));
		if (delay != "none") {
			if (delay == "random")
				Sleep(rand() % 8000 + 2000); // Between 2s and 10s
			else if (delay == "distributed")
				Sleep(out->mid * 2000 + 2000);
			else if (delay == "constant")
				Sleep(6000);
			else
				std::cerr << "! Unknown delay: " << delay << std::endl;
		}

		// Removes sleeping time from RTT (addition because it's a starting time)
		out->timer = in->timer + (dps::Timer::getMillis() - start);

		postDataObject(out);
	}
};

class Merge : public dps::MergeOperation<Message, ControlMessage, MasterThread> {
	IDENTIFY(Merge);
public:
	void execute(Message* in) {
		double confLat = dps::Timer::getMillis();
		int payloadSize = getController()->getConfig().getValue("payload", 0);
		int numSlaves = getController()->getConfig().getValue("slaves", 1);
		const char* delay = getController()->getConfig().getValue("delay", "none");
		int arrivalRank = -1;
		confLat = dps::Timer::getMillis() - confLat;

		do {
			double now = dps::Timer::getMillis();
			// Output: payload_size delay num_messages message_id arrival_rank half_rtt_ms
			std::cout << payloadSize << " " << delay << " " << numSlaves << " " << in->mid << " " << ++arrivalRank << " " << now - in->timer - confLat << std::endl;
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

	std::string pattern = std::string(getController()->getConfig().getValue("slaves","1")) + "+1";
	if (DPS_FAILED(masterThread.addThread("0")) || DPS_FAILED(slaveThread.addThread(dps::MPIMapper::get(pattern.c_str())))) {
		std::cout << "Could not map thread collection: " << pattern << std::endl;
		return;
	}

	dps::FlowgraphBuilder graphB =
		dps::FlowgraphNode<Split, dps::ZeroRoute>(masterThread) >>
		dps::FlowgraphNode<Process, dps::RoundRobinRoute>(slaveThread) >>
		dps::FlowgraphNode<Merge, dps::ZeroRoute>(masterThread);

	dps::Flowgraph graph = getController()->createFlowgraph("graph", graphB);

	int nrMessages = getController()->getConfig().getValue("slaves", 1);
	if (!getController()->getConfig().isSet("noheader"))
		std::cout << "payload_size delay num_messages message_id arrival_rank half_rtt_ms" << std::endl;
	getController()->callSchedule(graph, new ControlMessage(nrMessages));
};

int main(int argc, char *argv[]) {
	return dps::dpsMain(argc, argv, new TestApp());
}
