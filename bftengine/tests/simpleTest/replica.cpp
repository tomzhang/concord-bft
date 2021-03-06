#include <stdio.h>
#include <string.h>
#include <cassert>
#include <string>
#include <thread>

#include "CommFactory.hpp" 
#include "ReplicaConfig.hpp"

#include "commonDefs.h"
#include "Replica.hpp"

using namespace std;
using namespace bftEngine;

void getReplicaConfig(uint16_t replicaId, bftEngine::ReplicaConfig& outConfig);
PlainUdpConfig getUDPConfig(uint16_t id);

class SimpleAppState : public RequestsHandler
{
public:
	virtual int execute(uint16_t clientId, bool readOnly, uint32_t requestSize, const char* request, uint32_t maxReplySize, char* outReply, uint32_t& outActualReplySize) override
	{
		if (readOnly) // read-only request
		{
			assert(requestSize == sizeof(uint64_t));
			const uint64_t reqId = *((uint64_t*)request);
			assert(reqId == READ_VAL_REQ);

			assert(maxReplySize >= sizeof(uint64_t));
			uint64_t* pRet = (uint64_t*)outReply;
			*pRet = lastValue;
			outActualReplySize = sizeof(uint64_t);
		}
		else // read-write request
		{
			assert(requestSize == 2 * sizeof(uint64_t));
			const uint64_t* pReqId = (uint64_t*)request;
			assert(*pReqId == SET_VAL_REQ);
			const uint64_t* pReqVal = (pReqId + 1);

			stateNum++;
			lastValue = *pReqVal;

			assert(maxReplySize >= sizeof(uint64_t));
			uint64_t* pRet = (uint64_t*)outReply;
			*pRet = stateNum;
			outActualReplySize = sizeof(uint64_t);
		}

		return 0;
	}

protected:
	// state
	uint64_t stateNum = 0;
	uint64_t lastValue = 0;
};

int main(int argc, char **argv) 
{
	if (argc < 2) throw std::runtime_error("Unable to read replica id");
	uint16_t id = (argv[1][0] - '0');
	if (id >= 4) throw std::runtime_error("Illegal replica id");

	ReplicaConfig replicaConfig;
	getReplicaConfig(id, replicaConfig);

	PlainUdpConfig udpConf = getUDPConfig(id);
	

	ICommunication* comm = PlainUDPCommunication::create(udpConf);

	SimpleAppState simpleAppState;

	Replica* replica = Replica::createNewReplica(&replicaConfig, &simpleAppState, nullptr, comm, nullptr);

	replica->start();

	// wait forever...
	while (true) std::this_thread::sleep_for(std::chrono::seconds(1)); 
	
	return 0;
}