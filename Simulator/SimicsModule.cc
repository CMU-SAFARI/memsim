// -----------------------------------------------------------------------------
// File: SimicsSimulator.cc
// Description:
//    This file implements the interface to the simics simulator. This file
//    should not be modifies unless some generic functionality is added to all
//    the simulators.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Module includes
// -----------------------------------------------------------------------------

#include "SimicsSimulator.h"


// -----------------------------------------------------------------------------
// Simics includes
// -----------------------------------------------------------------------------

#include <simics/api.h>
#include <simics/utils.h>
#include <simics/alloc.h>
#include <cstdio>


// -----------------------------------------------------------------------------
// Export init local function
// -----------------------------------------------------------------------------

extern "C" { void init_local(); }


// -----------------------------------------------------------------------------
// New instance function for the simulator
// -----------------------------------------------------------------------------

static conf_object_t * SimulatorNewInstance(parse_object_t *obj);


// -----------------------------------------------------------------------------
// Function to capture memory requests from simics
// -----------------------------------------------------------------------------

static cycles_t SimulatorOperate(conf_object_t *mhier, conf_object_t *mobj,
    map_list_t *maplist, generic_transaction_t *mop);


// -----------------------------------------------------------------------------
// Set and get functions for start simulation attribute
// -----------------------------------------------------------------------------

static set_error_t SetStartSimulation(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> StartSimulation();
  return Sim_Set_Ok;
}

static attr_value_t GetStartSimulation(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  return SIM_make_attr_integer(0);
}


// -----------------------------------------------------------------------------
// Set and get functions for end simulation attribute
// -----------------------------------------------------------------------------

static set_error_t SetEndSimulation(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> EndSimulation();
  return Sim_Set_Ok;
}

static attr_value_t GetEndSimulation(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  return SIM_make_attr_integer(0);
}


// -----------------------------------------------------------------------------
// Set and get functions for warm up attribute attribute
// -----------------------------------------------------------------------------

static set_error_t SetWarmUp(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> EndWarmUp();
  return Sim_Set_Ok;
}

static attr_value_t GetWarmUp(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  return SIM_make_attr_integer(0);
}


// -----------------------------------------------------------------------------
// Set and get functions for heart beat
// -----------------------------------------------------------------------------

static set_error_t SetHeartBeat(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> HeartBeat(val -> u.integer);
  return Sim_Set_Ok;
}

static attr_value_t GetHeartBeat(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  return SIM_make_attr_integer(0);
}


// -----------------------------------------------------------------------------
// Set and get functions for simulator definition file
// -----------------------------------------------------------------------------

static set_error_t SetSimDef(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> simulatorDefinition = (string)(val -> u.string);
  return Sim_Set_Ok;
}

static attr_value_t GetSimDef(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  return SIM_make_attr_string(sim -> simulatorDefinition.c_str());
}


// -----------------------------------------------------------------------------
// Set and get functions for simulator configuration file
// -----------------------------------------------------------------------------

static set_error_t SetSimConf(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> simulatorConfiguration = (string)(val -> u.string);
  return Sim_Set_Ok;
}

static attr_value_t GetSimConf(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  return SIM_make_attr_string(sim -> simulatorConfiguration.c_str());
}


// -----------------------------------------------------------------------------
// Set and get functions for simulation folder name
// -----------------------------------------------------------------------------

static set_error_t SetSimulationFolder(void *dummy, conf_object_t *obj, 
    attr_value_t *val, attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  sim -> simulationFolder = (string)(val -> u.string);
  return Sim_Set_Ok;
}

static attr_value_t GetSimulationFolder(void *dummy, conf_object_t *obj,
    attr_value_t *idx) {
  SimicsSimulator *sim = (SimicsSimulator *)(obj);
  return SIM_make_attr_string(sim -> simulationFolder.c_str());
}


// -----------------------------------------------------------------------------
// Information for registering the simulator module with simics
// -----------------------------------------------------------------------------

static conf_class_t *simulatorClass;
static class_data_t simulatorData;
static timing_model_interface_t simulatorIFC;


// -----------------------------------------------------------------------------
// Module initialization function
// -----------------------------------------------------------------------------

void init_local() {

  // Initialize the simulator class
  simulatorData.new_instance = SimulatorNewInstance;
  simulatorData.description = "This is a generic simulator";

  // Register the class
  simulatorClass = SIM_register_class("Simulator", &simulatorData);
  if (!simulatorClass) {
    pr_err("Failed to register the simulator class\n");
    return;
  }

  // Register the timing model interface
  simulatorIFC.operate = SimulatorOperate;
  SIM_register_interface(simulatorClass, "timing-model", &simulatorIFC);

  // Register the start simulation attribute
  SIM_register_typed_attribute(simulatorClass, "StartSimulation", 
      GetStartSimulation, NULL, SetStartSimulation, NULL, Sim_Attr_Pseudo,
      "i", NULL, "");

  // Register the end simulation attribute
  SIM_register_typed_attribute(simulatorClass, "EndSimulation", 
      GetEndSimulation, NULL, SetEndSimulation, NULL, Sim_Attr_Pseudo,
      "i", NULL, "");

  // Register the warmup attribute
  SIM_register_typed_attribute(simulatorClass, "EndWarmUp", 
      GetWarmUp, NULL, SetWarmUp, NULL, Sim_Attr_Pseudo,
      "i", NULL, "");

  // Register the heart beat attribute
  SIM_register_typed_attribute(simulatorClass, "HeartBeat", 
      GetHeartBeat, NULL, SetHeartBeat, NULL, Sim_Attr_Pseudo,
      "i", NULL, "");

  // Register the simulator definition
  SIM_register_typed_attribute(simulatorClass, "Definition", 
      GetSimDef, NULL, SetSimDef, NULL, Sim_Attr_Required,
      "s", NULL, "");

  // Register the parameter value
  SIM_register_typed_attribute(simulatorClass, "Configuration", 
      GetSimConf, NULL, SetSimConf, NULL, Sim_Attr_Required,
      "s", NULL, "");

  // Register the parameter value
  SIM_register_typed_attribute(simulatorClass, "Folder", 
      GetSimulationFolder, NULL, SetSimulationFolder, NULL, Sim_Attr_Required,
      "s", NULL, "");
}


// -----------------------------------------------------------------------------
// New instance function
// -----------------------------------------------------------------------------

static conf_object_t * SimulatorNewInstance(parse_object_t *obj) {

  // Create a new instance of the simulator
  SimicsSimulator *ms = new SimicsSimulator;

  // Initialize the log
  SIM_log_constructor(&ms -> log, obj);

  return (conf_object_t *) ms;
}


// -----------------------------------------------------------------------------
// Simulator operate function
// -----------------------------------------------------------------------------

static cycles_t SimulatorOperate(conf_object_t *mhier, conf_object_t *mobj,
    map_list_t *maplist, generic_transaction_t *mop) {

  // Get the simulator object
  SimicsSimulator *ms = (SimicsSimulator *) mhier;
  
  // Process the request and return the latency
  return ms -> ProcessSimicsTransaction(mop);
}
