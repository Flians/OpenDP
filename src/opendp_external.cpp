#include "opendp_external.h"
#include "dbWrapper.h"
#include "lefin.h"
#include "defin.h"

using std::cout;
using std::endl;
using namespace odb;

opendp_external::opendp_external() 
: def_file(""), constraint_file(""), is_evaluated(false), db_id(INT_MAX) {};

opendp_external::~opendp_external() {};

void opendp_external::help() {
  cout << "help message will be printed" << endl;
}

void opendp_external::import_lef(const char* lef) {
  odb::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = odb::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = odb::dbDatabase::getDatabase(db_id);
  }
  odb::lefin lefReader(db, false);
  lefReader.createTechAndLib("testlib", lef);
}

void opendp_external::import_def(const char* def) {
  odb::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = odb::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = odb::dbDatabase::getDatabase(db_id);
  }
  odb::defin defReader(db);

  std::vector<odb::dbLib *> search_libs;
  odb::dbSet<odb::dbLib> libs = db->getLibs();
  odb::dbSet<odb::dbLib>::iterator itr;
  for( itr = libs.begin(); itr != libs.end(); ++itr ) {
    search_libs.push_back(*itr);
  }
  odb::dbChip* chip = defReader.createChip( search_libs,  def );
}

void opendp_external::import_constraint(const char* constraint) {
  constraint_file = constraint;
}

void opendp_external::export_def(const char* def) {
  ckt.write_def(def);
}

bool opendp_external::init_opendp() {
  if( constraint_file != "" && ckt.read_constraints(constraint_file)) {
    return false;
  }
  odb::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = odb::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = dbDatabase::getDatabase(db_id);
  }

  FillOpendpStructures(ckt, db);

  ckt.InitOpendpAfterParse();

  return true;
}

bool opendp_external::legalize_place() {
  ckt.simple_placement(nullptr);
  ckt.calc_density_factor(4);
}

bool opendp_external::check_legality() {
  return ckt.check_legality();
}

double opendp_external::get_utilization() {
  return ckt.design_util; 
}

double opendp_external::get_sum_displacement() {
  if( !is_evaluated ) {
    ckt.evaluation();
    is_evaluated = true;
  }
  return ckt.sum_displacement;
}

double opendp_external::get_average_displacement() {
  if( !is_evaluated ) {
    ckt.evaluation();
    is_evaluated = true;
  }
  return ckt.avg_displacement;
}

double opendp_external::get_max_displacement() {
  if( !is_evaluated ) {
    ckt.evaluation();
    is_evaluated = true;
  }
  return ckt.max_displacement;
}

double opendp_external::get_original_hpwl() {
  return ckt.HPWL("INIT");
}

double opendp_external::get_legalized_hpwl() {
  return ckt.HPWL("");
}
