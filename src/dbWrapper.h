#ifndef __OPENDP_DB_LEFDEF__
#define __OPENDP_DB_LEFDEF__

#include "db.h"

namespace opendp { 
  class circuit;

  void FillOpendpStructures(circuit& ckt, odb::dbDatabase* db);
}


#endif
