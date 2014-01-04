#include <stdlib.h>
#include <dps/dps.h>
#include <dps/altserial.h>
#include <windows.h>
#include <vector>
#include <climits>

class ControlMessage : public dps::SimpleSerial {
	IDENTIFY(ControlMessage);
public:
	int number;
	double timer;

	ControlMessage(int n = 0, double timer = 0) {
		number = n;
		this->timer = timer;
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

class Split : public dps::SplitOperation<ControlMessage, ControlMessage> {
	IDENTIFY(Split);
public:
	void execute(ControlMessage* in) {
		int limit = in->number;

		for (int i = 0; i < limit; ++i) {
			ControlMessage* message = new ControlMessage(i, dps::Timer::getMillis());
			postDataObject(message);
		}
	}
};

class Process : public dps::LeafOperation<ControlMessage, Message> {
	IDENTIFY(Process);
public:
	void execute(ControlMessage* in) {
		double start = dps::Timer::getMillis();

		Message* out = new Message();
		out->mid = in->number;
		out->timer = in->timer;

		int payload = getController()->getConfig().getValue("payload", 0);
		for (int i = 0; i < payload; ++i)
			out->data.push_back(i);

		// Simulate expensive operation (optional, substracted from RTT)
		if (getController()->getConfig().isSet("slaves.constant") || getController()->getConfig().isSet("slaves.random") || getController()->getConfig().isSet("slaves.distributed")) {
			if (getController()->getConfig().isSet("slaves.random"))
				Sleep(rand() % 8000 + 2000); // Between 2s and 10s
			else if (getController()->getConfig().isSet("slaves.distributed"))
				Sleep(out->mid * 2000 + 2000);
			else
				Sleep(6000);
		}

		// Removes processing and sleeping time from RTT (addition because we must shift time forward)
		out->timer = out->timer + (dps::Timer::getMillis() - start);

		postDataObject(out);
	}
};

class Merge : public dps::MergeOperation<Message, ControlMessage> {
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
			unsigned long long now = dps::Timer::getMillis();
			// Output: payload_size heavy_processing message_id half_rtt_ms
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
	dps::StatelessThreadCollection masterThread =
		getController()->createStatelessThreadCollection("master", 1, false);
	dps::StatelessThreadCollection slaveThread =
		getController()->createStatelessThreadCollection("slave");

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
	std::cout << "payload_size slaves_processing message_id half_rtt_ms" << std::endl;
	getController()->callSchedule(graph, new ControlMessage(nrMessages));
};

int main(int argc, char *argv[]) {
	return dps::dpsMain(argc, argv, new TestApp());
}
