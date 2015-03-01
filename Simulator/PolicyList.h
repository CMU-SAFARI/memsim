// -----------------------------------------------------------------------------
// Include policy files here
// -----------------------------------------------------------------------------

#include "TableLRU.h"
#include "TableFIFO.h"
#include "TableReuse.h"
#include "TableSRRIP.h"
#include "TableNRU.h"
#include "TableGeneration.h"
#include "TableDIP.h"
#include "TableDRRIP.h"
#include "TableDRRIP-HP.h"
#include "TableMaxW.h"
#include "TableMinW.h"

template <class key_t, class value_t> 
void generic_table_t <key_t, value_t>::SetTableParameters(
    uint32 size, string policy) {
  assert(_table == NULL);

  // ---------------------------------------------------------------------------
  // ADD AN ENTRY FOR EACH POLICY HERE
  // ---------------------------------------------------------------------------
  
  TABLE_POLICY_BEGIN
  TABLE_POLICY("lru", lru_table_t)
  TABLE_POLICY("fifo", fifo_table_t)
  TABLE_POLICY("reuse", reuse_table_t)
  TABLE_POLICY("srrip", srrip_table_t)
  TABLE_POLICY("nru", nru_table_t)
  TABLE_POLICY("generation", generation_table_t)
  TABLE_POLICY("dip", dip_table_t)
  TABLE_POLICY("drrip", drrip_table_t)
  TABLE_POLICY("drrip-hp", drrip_hp_table_t)
  TABLE_POLICY("maxw", maxw_table_t)
  TABLE_POLICY("minw", minw_table_t)
  TABLE_POLICY_END
}
