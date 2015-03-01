//
// File: DRAM.h
// Description
// DRAM related stuff
//

#include "Types.h"
#include "MemoryRequest.h"

#include <cstring>

// List of DRAM Commands
enum DRAMCommand {
  CMD_ACT,
  CMD_READ,
  CMD_WRITE,
  CMD_PRE,
  NUM_CMDS
};


// List of Channel modes
enum DRAMChannelMode {
  CMODE_READ,
  CMODE_WRITE,
  NUM_CMODES,
};


// List of Bank states
enum DRAMBankState {
  BANK_ACTIVATED,
  BANK_PRECHARGED,
  NUM_BANK_STATES
};

// forward declaration
struct DRAMRank;
struct DRAMChannel;


// DRAM Bank Structure
struct DRAMBank {

  DRAMBankState state;
  addr_t openRow; // valid if state is activated

  cycles_t lastIssueCycle[NUM_CMDS];
  cycles_t nextIssueCycle[NUM_CMDS];

  DRAMRank *rank;
  DRAMChannel *channel;

  // stats
  uint64 numCmds[NUM_CMDS];
  uint64 numActs[NUM_CMODES];

  DRAMBank() {
    state = BANK_PRECHARGED;
    memset(lastIssueCycle, 0, sizeof(lastIssueCycle));
    memset(nextIssueCycle, 0, sizeof(nextIssueCycle));

    rank = NULL;
    channel = NULL;

    memset(numCmds, 0, sizeof(numCmds));
    memset(numActs, 0, sizeof(numActs));
  }
};


// DRAM Rank Structure
struct DRAMRank {

  DRAMBank *banks;
  DRAMChannel *channel;

  // for tFAW
  list <cycles_t> lastActivates;
  cycles_t nextActivate;

  DRAMRank() {
    banks = NULL;
    channel = NULL;
    lastActivates.clear();
    lastActivates.push_back(0);
    lastActivates.push_back(0);
    lastActivates.push_back(0);
    lastActivates.push_back(0);
    nextActivate = 0;
  }

  ~DRAMRank() {
    if (banks != NULL) delete [] banks;
  }
};


// DRAM Channel Structure
struct DRAMChannel {
  DRAMRank *ranks;

  int32 lastRank;
  
  DRAMCommand lastOp;
  DRAMCommand lastColumnOp; // READ or WRITE
  cycles_t lastIssueCycle[NUM_CMDS];
  cycles_t nextIssueCycle[NUM_CMDS];

  list <MemoryRequest *> queue[NUM_CMODES]; // one queue for each channel mode
  DRAMChannelMode mode;

  // stats
  uint64 numReadToWrites;
  uint64 numWriteToReads;

  DRAMChannel() {
    ranks = NULL;
    lastRank = -1;
    lastOp = NUM_CMDS;
    lastColumnOp = NUM_CMDS;
    memset(lastIssueCycle, 0, sizeof(lastIssueCycle));
    memset(nextIssueCycle, 0, sizeof(nextIssueCycle));
    mode = CMODE_READ;
    for (int i = 0; i < NUM_CMODES; i ++) queue[i].clear();
    numReadToWrites = 0;
    numWriteToReads = 0;
  }

  ~DRAMChannel() {
    if (ranks != NULL) delete [] ranks;
  }

};
