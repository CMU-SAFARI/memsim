// -----------------------------------------------------------------------------
// File: MemorySchedulers.h
// Description:
//    This file contains a list of memory schedulers for the memory controller
//    component to use.
// -----------------------------------------------------------------------------


// -------------------------------------------------------------------------
// FCFS scheduler
// -------------------------------------------------------------------------

MemoryRequest * FCFS() {
  MemoryRequest *request;

  if (_readQ.empty() && _writeQ.empty())
    return NULL; 

  if (_readQ.empty()) {
    request = _writeQ.front();
    _writeQ.pop_front();
    return request;
  }

  if (_writeQ.empty()) {
    request = _readQ.front();
    _readQ.pop_front();
    return request;
  }

  if (_readQ.front() -> currentCycle <= _writeQ.front() -> currentCycle) {
    request = _readQ.front();
    _readQ.pop_front();
    return request;
  }

  else {
    request = _writeQ.front();
    _writeQ.pop_front();
    return request;
  }
}



// -------------------------------------------------------------------------
// FCFS with drain-when-full
// -------------------------------------------------------------------------
/*
MemoryRequest * FCFSDrainWhenFull() {
  MemoryRequest *request;

  if (_readQ.empty() && _writeQ.empty())
    return NULL;

  if (_writeQ.size() == _numWriteBufferEntries)
    _drain = true;

  if (_drain) {
    if (!_writeQ.empty()) {
      request = _writeQ.front();
      _writeQ.pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }

  if (_readQ.empty())
    return NULL;

  request = _readQ.front();
  _readQ.pop_front();
  return request;
}
*/

// -------------------------------------------------------------------------
// FR-FCFS with drain-when-full
// -------------------------------------------------------------------------

#if 0
MemoryRequest * FRFCFSDrainWhenFull() {

  MemoryRequest *request;

  if (_readQ.empty() && _writeQ.empty() && _readRowHits.empty() && 
      _writeRowHits.empty())
    return NULL;

  if (_writeQ.size() >= _numWriteBufferEntries)
    _drain = true;

  list <MemoryRequest *>::iterator it;
  list <list <MemoryRequest *>::iterator> erlist;

  if (_drain) {

    if (!_writeRowHits.empty()) {
      request = _writeRowHits.front();
      _writeRowHits.pop_front();
      return request;
    }

    if (!_writeQ.empty()) {
      for (it = _writeQ.begin(); it != _writeQ.end(); it ++) {
        request = *it;
        if (IsRowBufferHit(request)) {
          _writeRowHits.push_back(request);
          erlist.push_back(it);
        }
      }
      while (!erlist.empty()) {
        _writeQ.erase(erlist.front());
        erlist.pop_front();
      }
      if (!_writeRowHits.empty()) {
        request = _writeRowHits.front();
        _writeRowHits.pop_front();
        return request;
      }
      request = _writeQ.front();
      _writeQ.pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }

  // choose the next read request that hits the row buffer
  if (!_readRowHits.empty()) {
    request = _readRowHits.front();
    _readRowHits.pop_front();
    return request;
  }

  if (_readQ.empty())
    return NULL;

  for (it = _readQ.begin(); it != _readQ.end(); it ++) {
    request = *it;
    if (IsRowBufferHit(request)) {
      _readRowHits.push_back(request);
      erlist.push_back(it);
    }
  }
  while (!erlist.empty()) {
    _writeQ.erase(erlist.front());
    erlist.pop_front();
  }

  if (!_readRowHits.empty()) {
    request = _readRowHits.front();
    _readRowHits.pop_front();
    return request;
  }

  request = _readQ.front();
  _readQ.pop_front();
  return request;

}
#endif

// -------------------------------------------------------------------------
// FR-FCFS with drain-when-full
// -------------------------------------------------------------------------

MemoryRequest * FRFCFSDrainWhenFull() {

  MemoryRequest *request;

  if (_readQ.empty() && _writeQ.empty())
    return NULL;

  
  if (_writeQ.size() >= _numWriteBufferEntries)
    _drain = true;
  

  list <MemoryRequest *>::iterator it;

  
  if (_drain) {
    if (!_writeQ.empty()) {
      for (it = _writeQ.begin(); it != _writeQ.end(); it ++) {
        request = *it;
        if (IsRowBufferHit(request)) {
          _writeQ.erase(it);
          return request;
        }
      }
      request = _writeQ.front();
      _writeQ.pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }
  

    if (_drain) {
    if (!_writeQ.empty()) {
      for (it = _writeQ.begin(); it != _writeQ.end(); it ++) {
        request = *it;
        if (IsRowBufferHit(request)) {                                  // This is the FR part
          _writeQ.erase(it);
          return request;
        }
      }
      request = _writeQ.front();                                       // The queue is the FCFS part
      _writeQ.pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }  


  if (_readQ.empty())
    return NULL;

  for (it = _readQ.begin(); it != _readQ.end(); it ++) {
    request = *it;
    if (IsRowBufferHit(request)) {
      _readQ.erase(it);
      return request;
    }
  }

  request = _readQ.front();
  _readQ.pop_front();
  return request;

}



// -------------------------------------------------------------------------
// FR-FCFS with drain-when-full for dist-wb
// -------------------------------------------------------------------------

MemoryRequest * FRFCFSDrainWhenFulldwb() {

  MemoryRequest *request;

  //if (_readQ.empty() && _writeQ.empty())
  if (_readQ.empty() && CheckifWQempty()) 
    return NULL;

  /*
  if (_writeQ.size() >= _numWriteBufferEntries)
    _drain = true;
  */

  if(!_drain){
  for (int i=0; i < _writeQdwb.size(); i++){                       // this leads to highe priority for earlier write buffers
    if (_writeQdwb[i].size() >= _numWriteBufferEntries){_drain = true;openWriteBuffer = i;break;}
    }
  }

  list <MemoryRequest *>::iterator it;

  /*
  if (_drain) {
    if (!_writeQ.empty()) {
      for (it = _writeQ.begin(); it != _writeQ.end(); it ++) {
        request = *it;
        if (IsRowBufferHit(request)) {
          _writeQ.erase(it);
          return request;
        }
      }
      request = _writeQ.front();
      _writeQ.pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }
  */

    if (_drain) {
    if (!_writeQdwb[openWriteBuffer].empty()) {
      for (it = _writeQdwb[openWriteBuffer].begin(); it != _writeQdwb[openWriteBuffer].end(); it ++) {
        request = *it;
        if (IsRowBufferHit(request)) {                                  // This is the FR part
          _writeQdwb[openWriteBuffer].erase(it);
          return request;
        }
      }
      request = _writeQdwb[openWriteBuffer].front();                                       // The queue is the FCFS part
      _writeQdwb[openWriteBuffer].pop_front();
      return request;
    }
    else {
      _drain = false;
    }
  }  


  if (_readQ.empty())
    return NULL;

  for (it = _readQ.begin(); it != _readQ.end(); it ++) {
    request = *it;
    if (IsRowBufferHit(request)) {
      _readQ.erase(it);
      return request;
    }
  }

  request = _readQ.front();
  _readQ.pop_front();
  return request;

}
// function to check if writeQ is empty
   bool CheckifWQempty(){
     bool flag = true;
     for(int i=0;i<_writeQdwb.size();i++)
            if(!(_writeQdwb[i].empty())){flag = false;break;}
     return flag;
     }
                
