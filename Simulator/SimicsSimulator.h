// -----------------------------------------------------------------------------
// File: SimicsSimulator
// Description:
//    Defines a simics interface to the memory simulator
// -----------------------------------------------------------------------------

#ifndef __SIMICS_SIMULATOR_H__
#define __SIMICS_SIMULATOR_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "Types.h"
#include "MemorySimulator.h"

// -----------------------------------------------------------------------------
// Simics includes
// -----------------------------------------------------------------------------

#include <simics/api.h>
#include <simics/utils.h>
#include <simics/alloc.h>

// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <map>
#include <string>

// -----------------------------------------------------------------------------
// Class: SimicsSimulator
// Description:
//    Defines a simics memory simulator
// -----------------------------------------------------------------------------

class SimicsSimulator {

  public:

    // -------------------------------------------------------------------------
    // simics log. needed
    // -------------------------------------------------------------------------
    log_object_t log;

    // -------------------------------------------------------------------------
    // simulation details
    // -------------------------------------------------------------------------
    string simulatorDefinition;
    string simulatorConfiguration;
    string simulationFolder;

    // -------------------------------------------------------------------------
    // out of order window (not supported yet)
    // -------------------------------------------------------------------------
    uint32 oooWindow;

  protected:


    // -------------------------------------------------------------------------
    // instance of memory simulator
    // -------------------------------------------------------------------------
    MemorySimulator _simulator;


    // -------------------------------------------------------------------------
    // current icount and cycle for each processor
    // -------------------------------------------------------------------------
 
    struct ProcInfo {
      uint64 currentIcount;
      cycles_t currentCycle;
    };


    // -------------------------------------------------------------------------
    // Function to convert generic transaction to memory request
    // -------------------------------------------------------------------------

    MemoryRequest *GenericTransactionToMemoryRequest(
        generic_transaction_t *mop) {

      // assert that its a cpu transaction
      assert(mop -> ini_type & Sim_Initiator_CPU);

      MemoryRequest *request = new MemoryRequest;

      // fill details
      request -> iniType = MemoryRequest::CPU;
      request -> iniPtr = NULL;
      request -> cpuID = SIM_get_processor_number(mop -> ini_ptr);
      request -> ip = SIM_get_program_counter(mop -> ini_ptr);
      request -> icount = SIM_step_count(mop -> ini_ptr);

      // type
      request -> type = (SIM_get_mem_op_type(mop) == Sim_Trans_Store) ?
        MemoryRequest::WRITE : MemoryRequest::READ;

      // addresses and sizes
      request -> virtualAddress = mop -> logical_address;
      request -> physicalAddress = mop -> physical_address;
      request -> size = mop -> size;

      // issue cycle
      request -> issueCycle = SIM_cycle_count(mop -> ini_ptr);
      request -> currentCycle = request -> issueCycle;

      // status of the request
      request -> issued = false;
      request -> serviced = false;
      request -> stalling = false;
      request -> finished = false;

      return request;
    }


  public:


    // -------------------------------------------------------------------------
    // Function to start the simulation
    // -------------------------------------------------------------------------

    void StartSimulation() {
      _simulator.InitializeSimulator(SIM_number_processors(),
          simulationFolder, simulatorDefinition, simulatorConfiguration);
      _simulator.SetStartCycle(SIM_cycle_count(SIM_get_processor(0)));
      _simulator.StartSimulation();
    }


    // -------------------------------------------------------------------------
    // Function to end the simulation
    // -------------------------------------------------------------------------

    void EndSimulation() {
      _simulator.EndSimulation();
    }


    // -------------------------------------------------------------------------
    // Function to end warm up
    // -------------------------------------------------------------------------

    void EndWarmUp() {
      _simulator.EndWarmUp();
    }


    // -------------------------------------------------------------------------
    // Function to issue heartbeat
    // -------------------------------------------------------------------------

    void HeartBeat(uint32 hbCount) {
      _simulator.HeartBeat(hbCount);
    }


    // -------------------------------------------------------------------------
    // Function to process a simics memory request
    // -------------------------------------------------------------------------

    cycles_t ProcessSimicsTransaction(generic_transaction_t *mop) {

      // if its not a cpu transaction return
      if (!(mop -> ini_type & Sim_Initiator_CPU))
        return 0;

      // if the logical address is 0, return (WHY?)
      if (mop -> logical_address == 0)
        return 0;

      // block STC and allow stalls
      mop -> block_STC = 1;
      mop -> may_stall = 1;

      MemoryRequest *request;

      // assume the request is new
      request = GenericTransactionToMemoryRequest(mop);
      _simulator.ProcessMemoryRequest(request);
      while (!(request -> finished))
        _simulator.AdvanceSimulation(request -> currentCycle);

      //cycles_t latency = request -> currentCycle - request -> issueCycle;
      delete request;
      return 0;
    }
};


#endif // __SIMICS_SIMULATOR_H__
