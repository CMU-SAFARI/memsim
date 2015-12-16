// -----------------------------------------------------------------------------
// File: CmpLLCwAWB.h
// Description:
//    Implements a last-level cache
// This cache stores the dirty bit information in a separate DBI
// The DBI is derived from the GenericTagStore
// It also implements aggresive writeback policy, enabled by _doBypass
// -----------------------------------------------------------------------------

/*
It is the job of LLC to generate writebacks with good row buffer locality that are to be sent to the
memory controller so that it can make use of this locality to avoid turnaround time ( and write recovery latency) in the channel 
(aggressive write back)
Right now, we assume that  the memory controller does this correctly, if it does not, we will have to look into 
behaviour of memory controller
*/

#ifndef __CMP_LLC_AWB_H__
#define __CMP_LLC_AWB_H__

// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "MemoryComponent.h"
#include "Types.h"
#include "GenericTagStore.h"
#include "SetDuelingTagStore.h"
#include "BypassTagStore.h" 
// -----------------------------------------------------------------------------
// Standard includes
// -----------------------------------------------------------------------------

#include <iostream>
#include <bitset>
#include <fstream>
#include <string>

// -----------------------------------------------------------------------------
// Few defines
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Class: CmpLLCwAWB
// Description:
//    Baseline lastlevel cache with DBI and implementing aggressive writeback.
//    This has also been augmented with the LLC bypassing mechanism.
// -----------------------------------------------------------------------------

class CmpLLCwAWB : public MemoryComponent {

protected:

  // -------------------------------------------------------------------------
  // Parameters
  // -------------------------------------------------------------------------

  bool _doAWB;					// switch for enabling AWB
  bool _doBypass;				// switch for enabling Bypass

  uint32 _size;					
  uint32 _blockSize;
  uint32 _associativity;
  string _policy;
  string _dbipolicy;
  uint32 _policyVal;
  uint32 _dbiPolicyVal;

  uint32 _tagStoreLatency;
  uint32 _dataStoreLatency;

  uint32 _dbiLatency;						// Only one latency, as we assume dbi is small enough to read the data in parallely
 
  uint32 _numDuelingSets;					// used as _numSamplingSets for lru bypass
  uint32 _maxPSELValue;
 
  uint32 _dbiSize;
  uint32 _granularity;
  uint32 _dbiAssociativity;
  
  // -------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------

  struct TagEntry {
    addr_t vcla;
    addr_t pcla;
    uint32 appID;
    TagEntry() {
    }
  };

  struct DBIEntry {
    bitset <128> dirtyBits;					// 128 because _granularity is dynamic
    DBIEntry() { 
    dirtyBits.reset(); 
    }
  };


  // tag store
  uint32 _numSets;
  //generic_tagstore_t <addr_t, TagEntry> _tags;			// for AWB without bypass
  //bypass_tagstore_t <addr_t, TagEntry> _tags;			// enable for lru (static) bypass
  set_dueling_tagstore_t <addr_t, TagEntry> _tags;		// enable for dynamic bypass
  generic_tagstore_t <addr_t, DBIEntry> _dbi;
  policy_value_t _pval;
  policy_value_t _dbipval;

  // dbi
  uint32 _numdbiSets;

  // per processor hit/miss counters for both POLICY_HIGH and POLICY_BIMODAL
  vector <uint32> _hitsHIGH;
  vector <uint32> _missesHIGH;
  vector <uint32> _hitsBIMODAL;
  vector <uint32> _missesBIMODAL;

  //per processor bypass indicator
  vector <bool> _bypass;  

  uint32 milestone;
  uint32 _epoch;
  double missRateHIGH;						// Used for lru bypass as well
  double missRateBIMODAL;
  double _bypassThreshold;
  // -------------------------------------------------------------------------
  // Structures for cleaning row (aggressive writeback operations)
  // -------------------------------------------------------------------------

  addr_t cleanRow;			// this stores which row is currently being cleaned, should store a physical row
  bool _cleanFlag;			// true indicates that the row is clean

  // -------------------------------------------------------------------------
  // Declare Counters
  // -------------------------------------------------------------------------

  NEW_COUNTER(accesses);
  NEW_COUNTER(reads);
  NEW_COUNTER(writebacks);
  NEW_COUNTER(misses);
  NEW_COUNTER(evictions);
  NEW_COUNTER(dirty_evictions);
  NEW_COUNTER(dbievictions);

  NEW_COUNTER(agg_writebacks);
  NEW_COUNTER(dbi_eviction_writebacks);
  NEW_COUNTER(tagstore_eviction_writebacks);

  NEW_COUNTER(clean_requests);
  
  NEW_COUNTER(dbi_misses);
  NEW_COUNTER(dbi_hits);
  NEW_COUNTER(writebackhits);
  NEW_COUNTER(writebackmisses);
  NEW_COUNTER(bypasses);
  NEW_COUNTER(insertions);
  NEW_COUNTER(dbi_reads);
  NEW_COUNTER(dbi_insertions);

public:

  // -------------------------------------------------------------------------
  // Constructor. It cannot take any arguments
  // -------------------------------------------------------------------------

  CmpLLCwAWB() {
    _doAWB = true;
    _doBypass = true;
    _size = 1024;
    _blockSize = 64;
    _associativity = 16;
    _tagStoreLatency = 6;
    _dataStoreLatency = 15;
    _policy = "lru";
    _dbipolicy = "drrip";
    _policyVal = 0;
    _dbiPolicyVal = 0;
    _dbiSize = 128;
    _dbiAssociativity = 16;
    _cleanFlag = true;
    _granularity = 128;
    _bypassThreshold = 0.8;
    _numDuelingSets = 32;
    _epoch = 50000000;
    _maxPSELValue = 1024;
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
      CMP_PARAMETER_BOOLEAN("_doAWB", _doAWB)
      CMP_PARAMETER_BOOLEAN("_doBypass", _doBypass)
      CMP_PARAMETER_UINT("size", _size)
      CMP_PARAMETER_UINT("block-size", _blockSize)
      CMP_PARAMETER_UINT("associativity", _associativity)
      CMP_PARAMETER_STRING("policy", _policy)
      CMP_PARAMETER_STRING("dbi-policy", _dbipolicy)
      CMP_PARAMETER_UINT("policy-value", _policyVal)
      CMP_PARAMETER_UINT("dbi-policy-value", _dbiPolicyVal)
      CMP_PARAMETER_UINT("tag-store-latency", _tagStoreLatency)
      CMP_PARAMETER_UINT("data-store-latency", _dataStoreLatency)
      CMP_PARAMETER_UINT("dbi-size", _dbiSize)
      CMP_PARAMETER_UINT("dbi-associativity", _dbiAssociativity)
      CMP_PARAMETER_UINT("_granularity", _granularity)
      CMP_PARAMETER_DOUBLE("bypass-threshold", _bypassThreshold)
      CMP_PARAMETER_UINT("num-dueling-sets", _numDuelingSets)
      CMP_PARAMETER_UINT("epoch", _epoch)
      CMP_PARAMETER_UINT("max-psel-value",_maxPSELValue)
    CMP_PARAMETER_END
  }


  // -------------------------------------------------------------------------
  // Function to initialize statistics
  // -------------------------------------------------------------------------

  void InitializeStatistics() {

    INITIALIZE_COUNTER(accesses, "Total Accesses");
    INITIALIZE_COUNTER(reads, "Read Accesses");
    INITIALIZE_COUNTER(writebacks, "Writeback Accesses");
    INITIALIZE_COUNTER(misses, "Total Misses");
    INITIALIZE_COUNTER(evictions, "Evictions");
    INITIALIZE_COUNTER(dirty_evictions, "Dirty Evictions");
    INITIALIZE_COUNTER(dbievictions, "DBI Evictions");
    INITIALIZE_COUNTER(agg_writebacks, "Aggressive Writebacks");
    INITIALIZE_COUNTER(dbi_eviction_writebacks, "DBI Eviction Writebacks");
    INITIALIZE_COUNTER(tagstore_eviction_writebacks, "Tagstore Eviction Writebacks");
    INITIALIZE_COUNTER(clean_requests, "Clean Requests");
    INITIALIZE_COUNTER(dbi_misses, "DBI Misses");
    INITIALIZE_COUNTER(dbi_hits, "DBI Hits");
    INITIALIZE_COUNTER(bypasses, "LLC bypasses");
    INITIALIZE_COUNTER(writebackhits, "Writeback hits");
    INITIALIZE_COUNTER(writebackmisses, "Writeback misses");
    INITIALIZE_COUNTER(insertions, "Tagstore insertions");
    INITIALIZE_COUNTER(dbi_reads, "Reads from the DBI");
    INITIALIZE_COUNTER(dbi_insertions, "DBI Insertions");
  }


  // -------------------------------------------------------------------------
  // Function called when simulation starts
  // -------------------------------------------------------------------------

  void StartSimulation() {

    _doBypass =true;
    milestone = 0;
    _numSets = (_size * 1024) / (_blockSize * _associativity);
   
     _numdbiSets = _dbiSize/_dbiAssociativity;	
 
    _tags.SetTagStoreParameters(_numCPUs, _numSets, _associativity, _policy, _numDuelingSets, _maxPSELValue);		// for dynamic bypass
    //_tags.SetTagStoreParameters(_numCPUs, _numSets, _associativity, _policy, _numDuelingSets);    			// for lru bypass 
    //_tags.SetTagStoreParameters(_numSets, _associativity, _policy);							// for AWB without bypass, ie using a generic tagstore
    _dbi.SetTagStoreParameters(_numdbiSets, 16, _dbipolicy);  

    switch (_policyVal) {
    case 0: _pval = POLICY_HIGH; break;
    case 1: _pval = POLICY_BIMODAL; break;
    case 2: _pval = POLICY_LOW; break;
    }

    switch (_dbiPolicyVal) {
    case 0: _dbipval = POLICY_HIGH; break;
    case 1: _dbipval = POLICY_BIMODAL; break;
    case 2: _dbipval = POLICY_LOW; break;
    }
                           
    _hitsHIGH.resize(_numCPUs, 0);
    _missesHIGH.resize(_numCPUs, 0);
    _hitsBIMODAL.resize(_numCPUs, 0);
    _missesBIMODAL.resize(_numCPUs, 0);
    _bypass.resize(_numCPUs, false);


    // Set the dbi latency, the table for different size and granularity of dbi is present in dbi_latencies.txt, data obtained from CACTII
    // Currently, no variation of latency seen with varying granularity, only with number of entries
    switch (_dbiSize) {
 
    case 2048 :
    case 1024 : 
	_dbiLatency = 3;
	break;

    case 512 :
    case 256 :
    case 128 :
    case 64 :
	_dbiLatency = 2;
	break;

    }

  }


  // -------------------------------------------------------------------------
  // Function called at a heart beat. Argument indicates cycles elapsed after
  // previous heartbeat
  // -------------------------------------------------------------------------

  void HeartBeat(cycles_t hbCount) {
  }


protected:

  // -------------------------------------------------------------------------
  // Function to process a request. Return value indicates number of busy
  // cycles for the component.
  // -------------------------------------------------------------------------


  cycles_t ProcessRequest(MemoryRequest *request) {

    // compute the cache block tag
    addr_t ctag = VADDR(request) / _blockSize;

    uint32 setIndex = _tags.index(ctag);
    // compute the cache block logical row, this is used to key into dbi, like ctag is used for tagstores
    addr_t logicalRow = ctag / _granularity;	


    if(_doBypass){ 
    if((*_simulatorCycle % _epoch < 1000)&&(*_simulatorCycle - milestone + 1000 > _epoch)){			// how do ensure periodic update		
    for(uint32 i=0; i < _numCPUs; i++){
	missRateHIGH = (double)_missesHIGH[i] / (double)(_missesHIGH[i] + _hitsHIGH[i]);
	missRateBIMODAL = (double)_missesBIMODAL[i] / (double)(_missesBIMODAL[i] + _hitsBIMODAL[i]);		// disable for lru bypass
	double min = (missRateHIGH < missRateBIMODAL)?missRateHIGH:missRateBIMODAL;
	if (min > _bypassThreshold) _bypass[i] = true;							// for dynamic bypass
	//if (missRateHIGH > _bypassThreshold) _bypass[i] = true;							// for lru bypass
	else _bypass[i] = false;
        _missesHIGH[i] = 0;
        _hitsHIGH[i] = 0;
        _missesBIMODAL[i] = 0;
        _hitsBIMODAL[i] = 0;
        milestone = *_simulatorCycle;
	}
     }
    }


    table_t <addr_t, TagEntry>::entry tagentry;
    table_t <addr_t, DBIEntry>::entry dbientry;
    cycles_t latency;

    // NO WRITES (Complete or partial)
    if (request -> type == MemoryRequest::WRITE || 
        request -> type == MemoryRequest::PARTIALWRITE) {
      fprintf(stderr, "LLC cannot handle direct writes (yet)\n");
      exit(0);
    }

    // check if its a read or write back
    switch (request -> type) {

      // READ request
    case MemoryRequest::READ:
    case MemoryRequest::READ_FOR_WRITE:
    case MemoryRequest::PREFETCH:


      if((!_doBypass)||(!_bypass[request->cpuID]) || ((_tags._type[setIndex].leader)&&(_tags._type[setIndex].appID==request->cpuID))){
      //if(!_doBypass){																// for plain AWB without bypass
      tagentry = _tags.read(ctag);
      INCREMENT(accesses); 
      INCREMENT(reads);

      if (tagentry.valid) {
        request -> serviced = true;
        request -> AddLatency(_tagStoreLatency + _dataStoreLatency);

        // update per processor counters, disable for plain AWB
	
        if (_tags._type[setIndex].appID == request -> cpuID && _tags._type[setIndex].leader){
	if(_tags._type[setIndex].policy == POLICY_HIGH) _hitsHIGH[request -> cpuID] ++;
	else _hitsBIMODAL[request -> cpuID] ++;
	
	}
        
      }
      else {
        INCREMENT(misses);
        request -> AddLatency(_tagStoreLatency);
	// disable for plain AWB
	
        if (_tags._type[setIndex].appID == request -> cpuID && _tags._type[setIndex].leader){
        if(_tags._type[setIndex].policy == POLICY_HIGH) _missesHIGH[request -> cpuID] ++;
	else _missesBIMODAL[request -> cpuID] ++;
	}
	
      }
      

      return _tagStoreLatency;
      }

      else {
	 INCREMENT(bypasses);
         INCREMENT(dbi_reads);	
         // first of all, add dbi lookup delay to request
         //On a bypass read,check the dbi, if not present, don't do anything
         //If present, do dbi read, set request as served, 
         if(_dbi.lookup(logicalRow) && (_dbi[logicalRow].dirtyBits[ctag % _granularity])){
	 request -> serviced = true;
         INCREMENT(dbi_hits);
         request -> AddLatency(_dbiLatency + _tagStoreLatency + _dataStoreLatency);		// after checking DBI, it will go to tagstore to read it     
	 }
	 else {
	 INCREMENT(dbi_misses);
         request -> AddLatency(_dbiLatency);     
	 }
	 return _dbiLatency;                       // this happens when llc is bypassed

      }    

      // WRITEBACK request
    case MemoryRequest::WRITEBACK:

      INCREMENT(accesses);
      INCREMENT(writebacks);

      if (_tags.lookup(ctag)){
	INCREMENT(writebackhits);
        //_tags[ctag].dirty = true;        

        if(_dbi.lookup(logicalRow)){	
        INCREMENT(dbi_reads);
	_dbi[logicalRow].dirtyBits.set(ctag % _granularity);	
	// Not all clean blocks are guaranteed to have their dirty bit info in the DBI
        //INCREMENT(dbi_hits);
	// dummy read to update with replacement policy
        _dbi.read(logicalRow);
        }

	else{
	table_t <addr_t, DBIEntry>::entry dbientry;    
	
        //INCREMENT(dbi_misses);
	HANDLE_DBI_INSERTION(ctag, request, dbientry, true);
// this will handle dbi insertion when dbi entry is not present  
	 }   
        }

      else {
	INCREMENT(writebackmisses);

        INSERT_BLOCK(ctag, true, request);
      }


      request -> serviced = true; 
      return _tagStoreLatency;	
      

     case MemoryRequest::CLEAN:

     // check if the row hasn't been cleaned yet and the that the corresponding dbientry still exists
        if((!_cleanFlag)&&_dbi.lookup(cleanRow)){

           // We don't count DBI hits because for these accesses, misses don't hurt us           
           if(!_dbi[cleanRow].dirtyBits.any()){		// row has been cleaned, no cleaning operation left
             // Should we invalidate it ? 
             _dbi.invalidate(cleanRow);
             _cleanFlag = true;
             request -> serviced = true;
             }

           else{
           int i=0;
           while(!_dbi[cleanRow].dirtyBits[i])	i++;
           
           addr_t wbtag = (cleanRow * _granularity) + i;
           TagEntry wbentry = _tags[wbtag];

           MemoryRequest *writeback =
           new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            wbentry.vcla, wbentry.pcla, _blockSize,
                            request -> currentCycle);
	   INCREMENT(agg_writebacks);
           writeback -> icount = request -> icount;
           writeback -> ip = request -> ip;
           SendToNextComponent(writeback);
           _dbi[cleanRow].dirtyBits.reset(i);
           }

        }

     //  if row is not clean and its dbientry doesn't exist, it means that row has been evicted and its dirty blocks cleaned
        else if((!_cleanFlag)&&(!_dbi.lookup(cleanRow))){
	  // We don't count a DBI miss for this part because a dbi miss doesn't harm us
          _cleanFlag = true;
          request -> serviced = true;
        }
      
         return _tagStoreLatency;			// Effect of port contention
         //return 0; 
    }
  }


  // -------------------------------------------------------------------------
  // Function to process the return of a request. Return value indicates
  // number of busy cycles for the component.
  // -------------------------------------------------------------------------
    
  cycles_t ProcessReturn(MemoryRequest *request) { 

    // if its a writeback or clean from this component, delete it
    if (request -> iniType == MemoryRequest::COMPONENT &&
        request -> iniPtr == this) {
      request -> destroy = true;
      return 0;
    }

    
    addr_t ctag = VADDR(request) / _blockSize;

    uint32 setIndex = _tags.index(ctag);
    if(_doBypass && _bypass[request->cpuID] && (!(_tags._type[setIndex].leader&&(_tags._type[setIndex].appID==request->cpuID)))) // disabled  to see effect of no-allocate
    //if(_doBypass)												// for plain AWB
    return 0;    

    else
    {
    // get the cache block tag
    addr_t ctag = VADDR(request) / _blockSize;

    // if the block is already present, return
    if (_tags.lookup(ctag))
      return 0;

    INSERT_BLOCK(ctag, false, request);

    return 0;
    }

    //else return 0;
  }


   // ----------------------------------------------------------------------
   // Overriding ProcessPendingRequests to enforce priority of demand
   // accesses over DRAM aware writebacks, i.e clean requests
   // ---------------------------------------------------------------------- 
  
    void ProcessPendingRequests() {

      // if already processing, return
      if (_processing) return;
      _processing = true;

      // if the queue is empty, return
      if (_queue.empty()) {
        _processing = false;
        return;
      }

      // get the request on the top of the queue
      pick : MemoryRequest *request = _queue.top();

      while (request -> currentCycle <= (*_simulatorCycle)) {
        
        _queue.pop();

        if (_currentCycle > (*_simulatorCycle)) {
          request -> currentCycle = _currentCycle;
          _queue.push(request);
        }

        // else process the request
        else {
          // get actual current time
          cycles_t now = max(request -> currentCycle, _currentCycle);
          _currentCycle = now;
          // if request is serviced, process return of request
          if (request -> serviced) {
            // CHECK
            cycles_t busyCycles = ProcessReturn(request);
            _currentCycle += busyCycles;
            SendToNextComponent(request);
          }
	// request is yet to be serviced
          else {

	   // Check if it's a CLEAN request
           // if true, check for other READ requests, if present, then return 
           // after incrementing currentCycle of CLEAN request by 1
           // if no READs follow, continue
	   if(request -> type == MemoryRequest::CLEAN){
		bool ReadsPresent = false;
		vector <MemoryRequest *> temp;

		// Check for presence of READs in the queue
		while(!_queue.empty()){
		if(((_queue.top()->type) != MemoryRequest::READ)||((_queue.top()->type) != MemoryRequest::READ_FOR_WRITE)||((_queue.top()->type) != MemoryRequest::PREFETCH)){
		temp.push_back(_queue.top());
		_queue.pop();
		}
		else {
		ReadsPresent = true;
		break;
	          }
	        }

		unsigned sizetemp = temp.size();
		for (unsigned i =0; i<sizetemp;i++){_queue.push(temp.back());temp.pop_back();}
		
		// If READs present, increment currentCycle of CLEAN request by 1, push it back, return
		if(ReadsPresent){
		request -> currentCycle += 1;
		_queue.push(request);
		goto pick;
		}
	   }

            request -> currentCycle = now;
            cycles_t busyCycles = ProcessRequest(request);
            _currentCycle += busyCycles;
            SendToNextComponent(request);
          }

          // if queue is empty, return
          if (_queue.empty()) {
            _processing = false;
            return;
          }

          // get the request on top of the queue
          request = _queue.top();
        }
      }

      _processing = false;
    }

  // -------------------------------------------------------------------------
  // Function to insert a block into the cache
  // Should check for DBI evictions also
  // -------------------------------------------------------------------------

  void INSERT_BLOCK(addr_t ctag, bool dirty, MemoryRequest *request) {

    INCREMENT(insertions);

    table_t <addr_t, TagEntry>::entry tagentry;
    table_t <addr_t, DBIEntry>::entry dbientry;
    
    bool DBIevictedEntry;			// indicates if dbientry is evicted or was already present, true if evicted

    addr_t logicalRow = ctag / _granularity;	

    
    DBIevictedEntry = !(_dbi.lookup(logicalRow));

/*
The following FSM has been implemented :
1. First insert into _dbi.
2. If returned dbientry was valid and evicted, generate writebacks for all the entries corresponding to dirty bits in the 
   evicted dbientry. (nothing to be done when dbientry is valid and already present) Also, delete the tagstore entries 
   corresponding to these dirty bits. 
3. Now insert into _tags.
4. If returned tagentry is valid and dirty (evicted or already present), then generate writeback. 
*/

// if returned entry is due to free list in table, then bool valid will be false

// How to differentiate between evicted and already present, use evictedEntry and lookup

if(dirty)	HANDLE_DBI_INSERTION(ctag, request, dbientry, DBIevictedEntry);
    
// 3. and 4.

// When bypassing is enabled, here _tags is a SetDuelingTagStore, we don't explicitly specify a// _pval
// If policy is plain fucking lru, then _pval is always POLICY_HIGH
// With dip, _pval is chosen between POLICY_HIGH and POLICY_BIMODAL

tagentry = _tags.insert(request -> cpuID, ctag, TagEntry(), ~dirty);				// insertion for dynamic bypass

//tagentry = _tags.insert(ctag, TagEntry());						// insertion for lru bypass, always with POLICY_HIGH

// This is when bypassing is not enabled, either for plain dbi (no opts) or for dbi-awb 
//else tagentry = _tags.insert(ctag, TagEntry());
// this has to be an eviction always

    
    _tags[ctag].vcla = BLOCK_ADDRESS(VADDR(request), _blockSize);
    _tags[ctag].pcla = BLOCK_ADDRESS(PADDR(request), _blockSize);
    _tags[ctag].appID = request -> cpuID;
       
    if (tagentry.valid) {

    // this will let in entries that were evicted or already present

      INCREMENT(evictions);		

bool specialCase = false;
     if(dirty)	specialCase = (((dbientry.key == (tagentry.key) / _granularity)&&(dbientry.value.dirtyBits[(tagentry.key) % _granularity])))?true:false;


// specialCase is that when the evicted dbientry contains the dirty bit info for the evicted tagstore entry(which happens to be 
// dirty) as well

    if (((_dbi.lookup((tagentry.key) / _granularity))&&(_dbi[(tagentry.key) / _granularity].dirtyBits[(tagentry.key) % _granularity])) || specialCase){
// this checks if evicted tagentry has entry in dbi			
// dbientry is generated only in case of dirty = true 
// check if evicted tagentry still has its dirty info in dbi OR
// in the evicted dbientry
// and then, if corresponding dirty bits are set
 
        INCREMENT(dirty_evictions);

        // We should check if the dbientry is still present in the DBI, only then clean the corresponding bit
	if(_dbi.lookup((tagentry.key) / _granularity))	
	_dbi[(tagentry.key) / _granularity].dirtyBits.reset((tagentry.key) % _granularity);
        // This is a cleaning operation, clean entry in DBI if present

        // ********   also invalidate row if it was the last set bit
        if(!(_dbi[(tagentry.key) / _granularity].dirtyBits.any()))	_dbi.invalidate((tagentry.key) / _granularity);

        MemoryRequest *writeback =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            tagentry.value.vcla, tagentry.value.pcla, _blockSize,
                            request -> currentCycle);
        // Generating writebacks when entry was already present and dirty, is it necessary (for memory consistency perhaps?)
        INCREMENT(tagstore_eviction_writebacks);
        writeback -> icount = request -> icount;
        writeback -> ip = request -> ip;
	writeback -> teEviction = true;
        SendToNextComponent(writeback);
        // Generate CLEAN request if no clean requests for previous rows pending and most importantly, if aggressive writeback is enabled
        if(_cleanFlag && (_dbi.lookup((tagentry.key) / _granularity)) && _doAWB){
        // we would not want to generate clean request when we have inserted new dbientry, as it will be a clean row
        MemoryRequest *clean =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::CLEAN, request -> cmpID, 
                            tagentry.value.vcla, tagentry.value.pcla, _blockSize,
                            request -> currentCycle);
        clean -> icount = request -> icount;
        clean -> ip = request -> ip;
        _cleanFlag = false;
        cleanRow = (tagentry.key) / _granularity;
        INCREMENT(clean_requests);
        AddRequest(clean);
        }
       
     }

    }
  
  }

void HANDLE_DBI_INSERTION(addr_t ctag, MemoryRequest *request, table_t <addr_t, DBIEntry>::entry &dbientry, bool DBIevictedEntry){

    INCREMENT(dbi_insertions);
 
    addr_t logicalRow = ctag / _granularity;	

// 1.
		if (_dbipolicy == "maxw") {
			uint32 setIndex = _dbi.index(logicalRow);

			if(_dbi.count(setIndex) == _dbiAssociativity){
				uint32 maxindex = 0;
				uint32 maxvalue = 0;
				for (uint32 i=0;i< _dbiAssociativity; i++){
					table_t<addr_t, CmpLLCwAWB::DBIEntry>::entry tempentry = _dbi.entry_at_location(setIndex, i);
					if(tempentry.value.dirtyBits.count() >= maxvalue ){ maxindex = i;
						maxvalue = tempentry.value.dirtyBits.count();}
				}
				dbientry = _dbi.entry_at_location(setIndex, maxindex);
				_dbi.invalidate(dbientry.key);
			        _dbi.insert(logicalRow, DBIEntry());
			}
			else dbientry = _dbi.insert(logicalRow, DBIEntry());
		}


		else if (_dbipolicy == "minw") {
			uint32 setIndex = _dbi.index(logicalRow);

			if(_dbi.count(setIndex) == _dbiAssociativity){
				uint32 minindex = 0;
				uint32 minvalue = _granularity;
				for (uint32 i=0;i< _dbiAssociativity; i++){
					table_t<addr_t, CmpLLCwAWB::DBIEntry>::entry tempentry = _dbi.entry_at_location(setIndex, i);
					if(tempentry.value.dirtyBits.count() <= minvalue ){ minindex = i;
						minvalue = tempentry.value.dirtyBits.count();}
				}
				dbientry = _dbi.entry_at_location(setIndex, minindex);
				_dbi.invalidate(dbientry.key);
			        _dbi.insert(logicalRow, DBIEntry());
			}
			else dbientry = _dbi.insert(logicalRow, DBIEntry());
		}


    else dbientry = _dbi.insert(logicalRow, DBIEntry(), _dbipval);
   
    _dbi[logicalRow].dirtyBits.set(ctag % _granularity);

  
// 2. 
    if(DBIevictedEntry && dbientry.valid){
    INCREMENT(dbievictions);
    // generate writebacks for all dirty blocks in the row
      for(uint64 i=0;i<_granularity;i++){
           
          if(dbientry.value.dirtyBits[i]){ 
          addr_t discardtag = (dbientry.key * _granularity) + i;
	          
	  // now check if the corresponding block is present in the tagstore
	  // if not, no need to generate writebacks
	  if(_tags.lookup(discardtag)){
          
	  // we have to generate writebacks and remove tagstore entries too
	  // if we do not remove the tagstore entries, we may have cases where tagentry is there but dbientry is not
          
	  //table_t <addr_t, TagEntry>::entry discardentry;
	  //discardentry = _tags.invalidate(discardtag); // Shouldn't do this ?
          
	  TagEntry discardentry = _tags[discardtag]; 
          MemoryRequest *writeback =
          new MemoryRequest(MemoryRequest::COMPONENT, request -> cpuID, this,
                            MemoryRequest::WRITEBACK, request -> cmpID, 
                            discardentry.vcla, discardentry.pcla, _blockSize,
                            request -> currentCycle);
          
          INCREMENT(dbi_eviction_writebacks);
          writeback -> icount = request -> icount;
          writeback -> ip = request -> ip;
          SendToNextComponent(writeback);
          }
        }
      }
    }
  }


};

#endif // __CMP_LLC_DBI_H__
