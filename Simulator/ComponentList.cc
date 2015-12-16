// -----------------------------------------------------------------------------
// Include all component header files here
// -----------------------------------------------------------------------------

#include "CmpTrace.h"
#include "CmpMSHR.h"
#include "CmpCache.h"
#include "CmpStall.h"
#include "CmpMemoryController.h"
#include "CmpUCP.h"

// EAF work
#include "CmpLLC.h"
#include "CmpDynamicLLC.h"
#include "CmpARC.h"
#include "CmpSULLC.h"
#include "CmpRTBCache.h"
#include "CmpSHiPIP.h"
#include "CmpMCT.h"
#include "CmpLLCVTS.h"

// LLC with prefetch stats monitors
#include "CmpLLCwPref.h"

// Prefetchers
#include "CmpNextLinePrefetcher.h"
#include "CmpStreamPrefetcher.h"
#include "CmpStridePrefetcher.h"

// DCP
#include "CmpDCP.h"
#include "CmpPACManHM.h"
#include "CmpFDPAP.h"
#include "CmpFDP.h"

// DBI
#include "CmpLLCwAWB.h"

// DRAMSim
#include "CmpDRAMCtlr.h"

// -----------------------------------------------------------------------------
// Function to create a new component
// -----------------------------------------------------------------------------

MemoryComponent *CreateComponent(string type) {

  // ---------------------------------------------------------------------------
  // Add a entry for each component in the following list
  // ---------------------------------------------------------------------------

  COMPONENT_LIST_BEGIN

  COMPONENT("trace", CmpTrace)
    
    COMPONENT("mshr", CmpMSHR)
    COMPONENT("cache", CmpCache)
    COMPONENT("stall", CmpStall)
    COMPONENT("simple-mc", CmpMemoryController)
    COMPONENT("ucp", CmpUCP)
    COMPONENT("dynamic-llc", CmpDynamicLLC)
    COMPONENT("baseline-llc", CmpLLC)
    COMPONENT("arc", CmpARC)
    COMPONENT("su-llc", CmpSULLC)
    COMPONENT("rtb-cache", CmpRTBCache)
    COMPONENT("ship-ip", CmpSHIPIP)
    COMPONENT("mct", CmpMCT)
    COMPONENT("llc-vts", CmpLLCVTS)

    // LLC with prefetch monitors
    COMPONENT("llc-pref", CmpLLCPref)
    
    // prefetcher
    COMPONENT("next-line-prefetcher", CmpNextLinePrefetcher)
    COMPONENT("stream-prefetcher", CmpStreamPrefetcher)
    COMPONENT("stride-prefetcher", CmpStridePrefetcher)

    // DCP
    COMPONENT("dcp", CmpDCP)
    COMPONENT("pacman", CmpPACMan)
    COMPONENT("fdp-ap", CmpFDPAP)
    COMPONENT("fdp", CmpFDP)

    // DBI
    COMPONENT("dbi", CmpLLCwAWB)

    // DRAMSim
    COMPONENT("dram-ctlr", CmpDRAMCtlr)
    
  COMPONENT_LIST_END
}
