// -----------------------------------------------------------------------------
// File: CmpDRAMCtlr.h
// Description:
//
// -----------------------------------------------------------------------------

#ifndef __CMP_DRAM_CTLR_H__
#define __CMP_DRAM_CTLR_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"

#include "DRAM.h"

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <bitset>

#define MAX_BANKS 16

// macros
#define FOR_EACH_CHANNEL                                \
  for (DRAMChannel *channel = _channels;                \
       channel < _channels + _numChannels; channel ++)

#define FOR_EACH_RANK(channel)                          \
  for (DRAMRank *rank = channel -> ranks;               \
       rank < (channel -> ranks) + _numRanks; rank ++)

#define FOR_EACH_BANK(rank)                             \
  for (DRAMBank *bank = rank -> banks;                  \
       bank < (rank -> banks) + _numBanks; bank ++)


#define FOR_EACH_REQUEST(reqlist) \
  for (list <MemoryRequest *>::iterator req = (reqlist).begin();        \
       req != (reqlist).end(); req ++)

#define UPDATE_MAX(a,b) if ((a) < (b)) a = (b)

// -----------------------------------------------------------------------------
// Class: CmpDRAMCtlr
// Description:
//
// -----------------------------------------------------------------------------

class CmpDRAMCtlr : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  // counts
  uint32 _numChannels;
  uint32 _numRanks;
  uint32 _numBanks;
  uint32 _rowSize;
  uint32 _columnSize;

  // timing parameters
  uint32 _tRC;
  uint32 _tRCD;
  uint32 _tRAS;
  uint32 _tCL;
  uint32 _tCWL;
  uint32 _tCCD;
  uint32 _tBL;
  uint32 _tRP;
  uint32 _tRTW;
  uint32 _tWTR;
  uint32 _tWR;
  uint32 _tRTRS;
  uint32 _tFAW;

  uint32 _memProcessorRatio;

  // queue sizes and scheduling algo
  uint32 _numWriteBuffers;
  string _addressMapping;
  string _scheduler;

  

  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  DRAMChannel *_channels;

  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  // NEW_COUNTER(activates);
  // NEW_COUNTER(readacts);
  // NEW_COUNTER(writeacts);
  // NEW_COUNTER(reads);
  // NEW_COUNTER(writes);
  // NEW_COUNTER(precharges);
  // NEW_COUNTER(readtowrites);
  // NEW_COUNTER(writetoreads);


public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpDRAMCtlr() {
    _numChannels = 1;
    _numRanks = 1;
    _numBanks = 8;
    _rowSize = 128;
    _columnSize = 64;

    _tRC = 34;
    _tRCD = 10;
    _tRAS = 24;
    _tCL = 10;
    _tCWL = 7;
    _tCCD = 4;
    _tBL = 4;
    _tRP = 10;
    _tRTW = 2;
    _tWTR = 6;
    _tWR = 10;
    _tRTRS = 2;
    _tFAW = 34;

    _memProcessorRatio = 4;

    _numWriteBuffers = 8;
    _addressMapping = "rbRcC";
    _scheduler = "frfcfs-dwf";
  }


  // -------------------------------------------------------------------------
  // Virtual functions to be implemented by the components
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // Function to add a parameter to the component
  // -------------------------------------------------------------------------

  void AddParameter(string pname, string pvalue) {
      
    CMP_PARAMETER_BEGIN

      // Add the list of parameters to the component here
      CMP_PARAMETER_UINT("num-channels", _numChannels)
      CMP_PARAMETER_UINT("num-ranks", _numRanks)
      CMP_PARAMETER_UINT("num-banks", _numBanks)
      CMP_PARAMETER_UINT("row-size", _rowSize)
      CMP_PARAMETER_UINT("column-size", _columnSize)

      CMP_PARAMETER_UINT("trc", _tRC)
      CMP_PARAMETER_UINT("trcd", _tRCD)
      CMP_PARAMETER_UINT("tras", _tRAS)
      CMP_PARAMETER_UINT("tcl", _tCL)
      CMP_PARAMETER_UINT("tcwl", _tCWL)
      CMP_PARAMETER_UINT("tccd", _tCCD)
      CMP_PARAMETER_UINT("tbl", _tBL)
      CMP_PARAMETER_UINT("trp", _tRP)
      CMP_PARAMETER_UINT("trtw", _tRTW)
      CMP_PARAMETER_UINT("twtr", _tWTR)
      CMP_PARAMETER_UINT("twr", _tWR)
      CMP_PARAMETER_UINT("trtrs", _tRTRS)
      CMP_PARAMETER_UINT("tfaw", _tFAW)
      
      CMP_PARAMETER_UINT("mem-processor-ratio", _memProcessorRatio)
      
      CMP_PARAMETER_UINT("num-write-buffers", _numWriteBuffers)
      CMP_PARAMETER_STRING("address-mapping", _addressMapping)
      CMP_PARAMETER_STRING("scheduler", _scheduler)

    CMP_PARAMETER_END
  }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {
    // INITIALIZE_COUNTER(activates, "Total Activates");
    // INITIALIZE_COUNTER(readacts, "Total Activates for reads");
    // INITIALIZE_COUNTER(writeacts, "Total Activates for writes");
    // INITIALIZE_COUNTER(reads, "Total reads");
    // INITIALIZE_COUNTER(writes, "Total writes");
    // INITIALIZE_COUNTER(precharges, "Total precharges");
    // INITIALIZE_COUNTER(readtowrites, "Read to write switches");
    // INITIALIZE_COUNTER(writetoreads, "Write to read switches");
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {

    // create channels
    _channels = new DRAMChannel[_numChannels];

    // for each channel create ranks and banks
    FOR_EACH_CHANNEL {
      channel -> ranks = new DRAMRank[_numRanks];
      FOR_EACH_RANK(channel) {
        rank -> channel = channel;
        rank -> banks = new DRAMBank[_numBanks];
        FOR_EACH_BANK(rank) {
          bank -> channel = channel;
          bank -> rank = rank;
        }
      }
    }

    _tRC *= _memProcessorRatio;
    _tRCD *= _memProcessorRatio;
    _tRAS *= _memProcessorRatio;
    _tCL *= _memProcessorRatio;
    _tCWL *= _memProcessorRatio;
    _tCCD *= _memProcessorRatio;
    _tBL *= _memProcessorRatio;
    _tRP *= _memProcessorRatio;
    _tRTW *= _memProcessorRatio;
    _tWTR *= _memProcessorRatio;
    _tWR *= _memProcessorRatio;
    _tRTRS *= _memProcessorRatio;
    _tFAW *= _memProcessorRatio;
  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {
  }

  // Override end warmup. clear local counters
  void EndWarmUp() {
    FOR_EACH_CHANNEL {
      channel -> numReadToWrites = 0;
      channel -> numWriteToReads = 0;
      FOR_EACH_RANK(channel) {
        FOR_EACH_BANK(rank) {
          memset(bank -> numCmds, 0, sizeof(bank -> numCmds));
          memset(bank -> numActs, 0, sizeof(bank -> numActs));
        }
      }
    }
    _warmUp = false;
    RESET_ALL_COUNTERS;
  }

  // Overrride end simulation
  void EndSimulation() {
    uint64 totalActs = 0;
    uint64 totalReadActs = 0;
    uint64 totalWriteActs = 0;
    uint64 totalReads = 0;
    uint64 totalWrites = 0;
    uint64 totalPres = 0;
    uint64 totalReadToWrites = 0;
    uint64 totalWriteToReads = 0;
    
    for (int i = 0; i < _numChannels; i ++) {
      DRAMChannel *channel = (_channels + i);
      for (int j = 0; j < _numRanks; j ++) {
        for (int k = 0; k < _numBanks; k ++) {
          DRAMBank *bank = &(channel -> ranks[j].banks[k]);
          CMP_LOG("C%d-R%d-B%d-acts = %llu", i, j, k, bank -> numCmds[CMD_ACT]);
          CMP_LOG("C%d-R%d-B%d-readacts = %llu", i, j, k, bank -> numActs[CMODE_READ]);
          CMP_LOG("C%d-R%d-B%d-writeacts = %llu", i, j, k, bank -> numActs[CMODE_WRITE]);
          CMP_LOG("C%d-R%d-B%d-reads = %llu", i, j, k, bank -> numCmds[CMD_READ]);
          CMP_LOG("C%d-R%d-B%d-writes = %llu", i, j, k, bank -> numCmds[CMD_WRITE]);
          CMP_LOG("C%d-R%d-B%d-pres = %llu", i, j, k, bank -> numCmds[CMD_PRE]);

          totalActs += bank -> numCmds[CMD_ACT];
          totalReadActs += bank -> numActs[CMODE_READ];
          totalWriteActs += bank -> numActs[CMODE_WRITE];
          totalReads += bank -> numCmds[CMD_READ];
          totalWrites += bank -> numCmds[CMD_WRITE];
          totalPres += bank -> numCmds[CMD_PRE];
        }
      }
      CMP_LOG("C%d-read-to-writes = %llu", i, channel -> numReadToWrites);
      CMP_LOG("C%d-write-to-reads = %llu", channel -> numWriteToReads);

      totalReadToWrites += channel -> numReadToWrites;
      totalWriteToReads += channel -> numWriteToReads;
    }

    CMP_LOG("total-acts = %llu", totalActs);
    CMP_LOG("total-readacts = %llu", totalReadActs);
    CMP_LOG("total-writeacts = %llu", totalWriteActs);
    CMP_LOG("total-reads = %llu", totalReads);
    CMP_LOG("total-writes = %llu", totalWrites);
    CMP_LOG("total-pres = %llu", totalPres);
    DUMP_STATISTICS;
    CLOSE_ALL_LOGS;
  }


  // override earliest request
  MemoryRequest *EarliestRequest() {
    MemoryRequest *earliest, *request;
    earliest = NULL;
    FOR_EACH_CHANNEL {
      FOR_EACH_REQUEST(channel -> queue[CMODE_READ]) {
        request = *req;
        if (earliest == NULL) {
          earliest = request;
        }
        else if (earliest -> currentCycle > request -> currentCycle) {
          earliest = request;
        }
      }
      FOR_EACH_REQUEST(channel -> queue[CMODE_WRITE]) {
        request = *req;
        if (earliest == NULL) {
          earliest = request;
        }
        else if (earliest -> currentCycle > request -> currentCycle) {
          earliest = request;
        }
      }
    }
    if (!_queue.empty()) {
      request = _queue.top();
      if (earliest == NULL) {
        earliest = request;
      }
      else if (earliest -> currentCycle > request -> currentCycle) {
        earliest = request;
      }
    }
    return earliest;
  }

  // debug info
  void PrintDebugInfo() {
    printf("Current cycle is %llu\n", _currentCycle);
    printf("Simulator cycle is %llu\n", (*_simulatorCycle));
    if (_queue.empty()) { printf("Queue is empty\n"); } 
    FOR_EACH_CHANNEL {
      printf("Channel mode is %d\n", channel -> mode);
      printf("Read requests\n");
      FOR_EACH_REQUEST(channel -> queue[CMODE_READ]) {
        MemoryRequest *request = *req;
        DRAMRank *rank = &(channel -> ranks[request -> dramRankID]);
        DRAMBank *bank = &(rank -> banks[request -> dramBankID]);
        printf("%llu %X %X %d %X %llu\n", request -> currentCycle, request, request -> dramRowID,
               bank -> state, bank -> openRow, bank -> nextIssueCycle[CMD_PRE]);
      }
      printf("Write requests\n");
      FOR_EACH_REQUEST(channel -> queue[CMODE_WRITE]) {
        MemoryRequest *request = *req;
        DRAMRank *rank = &(channel -> ranks[request -> dramRankID]);
        DRAMBank *bank = &(rank -> banks[request -> dramBankID]);
        printf("%llu %X %X %d %X\n", request -> currentCycle, request, request -> dramRowID,
               bank -> state, bank -> openRow);
      }
    }
  }

protected:

  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------

  cycles_t ProcessRequest(MemoryRequest *request) {
    return 0; 
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) { 
    return 0; 
  }

  
  // -------------------------------------------------------------------------
  // Address mapping function
  // -------------------------------------------------------------------------
  
  void AddressMapping(MemoryRequest *request) {

    if (_addressMapping.compare("rbRcC") == 0) {
      addr_t address = request -> virtualAddress;
      address /= _columnSize;
      request -> dramChannelID = address % _numChannels;
      address /= _numChannels;
      request -> dramColumnID = address % _rowSize;
      address /= _rowSize;
      request -> dramRankID = address % _numRanks;
      address /= _numRanks;
      request -> dramBankID = address % _numBanks;
      address /= _numBanks;
      request -> dramRowID = address;
    }

    else {
      fprintf(stderr, "Unknown address mapping scheme\n");
      exit(0);
    }
  }

  
  // -------------------------------------------------------------------------
  // Override process pending requests
  // -------------------------------------------------------------------------

  void ProcessPendingRequests() {

    if (_processing) return;
    _processing = true;

    // check if all queues are empty
    bool empty = true;
    FOR_EACH_CHANNEL {
      if (!channel -> queue[CMODE_READ].empty() ||
          !channel -> queue[CMODE_WRITE].empty()) {
        empty = false;
        break;
      }
    }
    if (empty && _queue.empty()) {
      _processing = false;
      return;
    }

    MemoryRequest *request;

    // Take all processable requests and transfer them to the corresponding queues
    if (!_queue.empty()) {
      request = _queue.top();
      while (request -> currentCycle <= (*_simulatorCycle)) {
        _queue.pop();

        // if request is serviced, send it back
        if (request -> serviced) {
          printf("This should not be happening"); // VIVEK
          SendToNextComponent(request);
        }

        // add it to the corresponding queue
        else {
          // get the channel, rank, bank, row and column
          uint32 channelID, rankID, bankID, rowID, columnID;
          AddressMapping(request);
          switch (request -> type) {
          case MemoryRequest::READ:
          case MemoryRequest::READ_FOR_WRITE:
          case MemoryRequest::PREFETCH:
            // printf("Add -> %X %16X read channel-%u\n", request, 
            //        request -> virtualAddress, request -> dramChannelID); // VIVEK
            _channels[request -> dramChannelID].queue[CMODE_READ].push_back(request);
            break;
          case MemoryRequest::WRITEBACK:
            // printf("Add -> %X %16X write channel-%u\n", request, 
            //        request -> virtualAddress, request -> dramChannelID); // VIVEK
            _channels[request -> dramChannelID].queue[CMODE_WRITE].push_back(request);
            break;
          default:
            fprintf(stderr, "Invalid request to DRAM");
            exit(0);
          }
        }

        if (_queue.empty())
          break;

        request = _queue.top();
      }
    }

    // Call the scheduler
    Scheduler();

    // done processing
    _processing = false;
  }

  
  // -------------------------------------------------------------------------
  // Overall scheduler
  // -------------------------------------------------------------------------

  void Scheduler() {
    // round robin across all channels
    if (_scheduler.compare("frfcfs-dwf") == 0) {
      while (_currentCycle <= (*_simulatorCycle)) {
        FOR_EACH_CHANNEL {
          FRFCFSDWFScheduler(channel);
        }
        _currentCycle += _memProcessorRatio;
      }
      FOR_EACH_CHANNEL {
        FOR_EACH_REQUEST(channel -> queue[CMODE_READ]) {
          MemoryRequest *request = *req;
          if (request -> currentCycle < _currentCycle)
            request -> currentCycle = _currentCycle;
        }
        FOR_EACH_REQUEST(channel -> queue[CMODE_WRITE]) {
          MemoryRequest *request = *req;
          if (request -> currentCycle < _currentCycle)
            request -> currentCycle = _currentCycle;
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  // Channel scheduler
  // -------------------------------------------------------------------------

  void FRFCFSDWFScheduler(DRAMChannel *channel) {

    // if read mode and write buffer is full, switch to write mode
    // else if write mode and write buffer is empty, switch to read mode
    // if some column request is ready and < current cycle, schedule it
    // else if some bank has a request and bank precharged, schedule activate
    // else if some bank has a request and bank activated, schedule precharge

    if (channel -> mode == CMODE_READ &&
        channel -> queue[CMODE_WRITE].size() >= _numWriteBuffers) {
      channel -> mode = CMODE_WRITE;
      channel -> numReadToWrites ++;
    }

    else if (channel -> mode == CMODE_WRITE &&
             channel -> queue[CMODE_WRITE].empty()) {
      channel -> mode = CMODE_READ;
      channel -> numWriteToReads ++;
    }

    // if queue is empty, return
    if (channel -> queue[channel -> mode].empty())
      return;

    // is a row hit request present
    bitset <MAX_BANKS> rowHitPresent;

    rowHitPresent.reset();
    
    // check for ready requests
    FOR_EACH_REQUEST(channel -> queue[channel -> mode]) {
      MemoryRequest *request = *req;

      DRAMRank *rank = &(channel -> ranks[request -> dramRankID]);
      DRAMBank *bank = &(rank -> banks[request -> dramBankID]);

      // check for row hit
      if (bank -> state == BANK_ACTIVATED &&
          bank -> openRow == request -> dramRowID) {
        DRAMCommand colCmd = (channel -> mode) == CMODE_READ ? CMD_READ : CMD_WRITE;

        // check when the request can be issued
        UPDATE_MAX(request -> currentCycle, bank -> nextIssueCycle[colCmd]);
        UPDATE_MAX(request -> currentCycle, channel -> nextIssueCycle[colCmd]);

        // if request is scheulable, schedule,
        // mark request as served, send it back, and return
        if (request -> currentCycle <= _currentCycle) {
          ScheduleRequest(bank, colCmd, request);
          if (colCmd == CMD_READ)
            request -> currentCycle = _currentCycle + _tCL + _tBL;
          else if (colCmd == CMD_WRITE)
            request -> currentCycle = _currentCycle + _tCWL + _tBL;
          request -> serviced = true;
          channel -> queue[channel -> mode].erase(req);
          // printf("Serve <- %X\n", request); // VIVEK
          SendToNextComponent(request);
          return;
        }
        
        rowHitPresent.set(request -> dramBankID);
      }

      // check for ready activate
      else if (bank -> state == BANK_PRECHARGED) {
        // check when an activate is schedulable
        UPDATE_MAX(request -> currentCycle, bank -> nextIssueCycle[CMD_ACT]);
        UPDATE_MAX(request -> currentCycle, rank -> nextActivate);

        // if the request is schedulable, schedule and return
        if (request -> currentCycle <= _currentCycle) {
          ScheduleRequest(bank, CMD_ACT, request);
          return;
        }
      }
    }

    // if there are no ready requests, then precharge the bank
    // corresponding to the first request
    FOR_EACH_REQUEST(channel -> queue[channel -> mode]) {
      MemoryRequest *request = *req;
      
      DRAMRank *rank = &(channel -> ranks[request -> dramRankID]);
      DRAMBank *bank = &(rank -> banks[request -> dramBankID]);

      if (!rowHitPresent.test(request -> dramBankID)) {
        // check when a precharge can be scheduled
        UPDATE_MAX(request -> currentCycle, bank -> nextIssueCycle[CMD_PRE]);

        // if the request is schedulable, schedule and return
        if (request -> currentCycle <= _currentCycle) {
          ScheduleRequest(bank, CMD_PRE, request);
          return;
        }
      }
      
      // request cannot be scheduled this cycle anyway
      else if (request -> currentCycle <= _currentCycle) {
        request -> currentCycle = _currentCycle + _memProcessorRatio;
      }
    }
  }

  void ScheduleRequest(DRAMBank *bank, DRAMCommand cmd,
                       MemoryRequest *request) {

    bank -> lastIssueCycle[cmd] = _currentCycle;
    bank -> numCmds[cmd] ++;

    DRAMChannel *channel = bank -> channel;
    DRAMRank *rank = bank -> rank;
    
    switch (cmd) {
      
    case CMD_ACT:
      bank -> state = BANK_ACTIVATED;
      bank -> openRow = request -> dramRowID;

      // compute next issue cycles
      UPDATE_MAX(bank -> nextIssueCycle[CMD_ACT], _currentCycle + _tRC);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_READ], _currentCycle + _tRCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_WRITE], _currentCycle + _tRCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_PRE], _currentCycle + _tRAS);

      // update rank for tfaw
      rank -> lastActivates.pop_front();
      rank -> lastActivates.push_back(_currentCycle);
      rank -> nextActivate = rank -> lastActivates.front() + _tFAW;

      // read act or write act
      bank -> numActs[channel -> mode] ++;
      break;
      
    case CMD_READ:
      // no change to bank state
      // compute next issue cycles for bank
      UPDATE_MAX(bank -> nextIssueCycle[CMD_ACT], _currentCycle + _tCL);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_READ], _currentCycle + _tCCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_WRITE], _currentCycle + _tCCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_PRE], _currentCycle + _tCL);
      // compute next issue cycles for channel
      UPDATE_MAX(channel -> nextIssueCycle[CMD_READ], _currentCycle + _tCCD);
      UPDATE_MAX(channel -> nextIssueCycle[CMD_WRITE],
                 _currentCycle + _tCL + _tBL + _tRTW - _tCWL);

      break;
      
    case CMD_WRITE:
      // no change to bank state
      // compute next issue cycles for bank
      UPDATE_MAX(bank -> nextIssueCycle[CMD_ACT], _currentCycle + _tCL + _tWR);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_READ], _currentCycle + _tCCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_WRITE], _currentCycle + _tCCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_PRE], _currentCycle + _tCWL + _tWR);
      // compute next issue cycles for channel
      UPDATE_MAX(channel -> nextIssueCycle[CMD_WRITE], _currentCycle + _tCCD);
      UPDATE_MAX(channel -> nextIssueCycle[CMD_READ],
                 _currentCycle + _tCWL + _tBL + _tWTR);
      break;
      
    case CMD_PRE:
      bank -> state = BANK_PRECHARGED;
      // compute next issue cycles
      UPDATE_MAX(bank -> nextIssueCycle[CMD_ACT], _currentCycle + _tRP);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_READ], _currentCycle + _tRP + _tRCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_WRITE], _currentCycle + _tRP + _tRCD);
      UPDATE_MAX(bank -> nextIssueCycle[CMD_PRE], _currentCycle + _tRC);
      break;
    }
  }
};

#endif // __CMP_DRAM_CTLR_H__
