#include "dbWrapper.h"
#include "circuit.h"

using namespace std;
using namespace odb;

namespace opendp {

static void FillOpendpLayer(circuit& ckt, dbTech* tech);
static void FillOpendpLibrary(circuit& ckt, dbSet<dbLib> &lib );
static void FillOpendpSite(circuit& ckt, dbSet<dbSite> &sites, const int dbu );
static void FillOpendpMacro(circuit& ckt, dbSet<dbMaster> &macros, const int dbu );

static void FillOpendpRow(circuit& ckt, dbSet<dbRow> &rows); 
static void FillOpendpPin(circuit& ckt, dbSet<dbBTerm> &bTerms);
static void FillOpendpCell(circuit& ckt, dbSet<dbInst> &insts);


void FillOpendpStructures(circuit& ckt, dbDatabase* db) {
  dbChip* chip = db->getChip();
  dbBlock* block = chip->getBlock();

  dbTech* tech = db->getTech(); 

  dbSet<dbLib> lib = db->getLibs();

//  dbSet<dbInst> insts = db->get

  // initialize opendp's layer structure
  FillOpendpLayer( ckt, tech );  
  FillOpendpLibrary( ckt, lib );

  ckt.design_name = block->getConstName();
  ckt.DEFdist2Microns = tech->getDbUnitsPerMicron(); 

  dbSet<dbRow> rows = block->getRows();
  dbSet<dbBTerm> bTerms = block->getBTerms();
  
  FillOpendpRow( ckt, rows );
  FillOpendpPin( ckt, bTerms);
}

static void FillOpendpLayer(circuit& ckt, dbTech* tech) {

  // iterate only routing layer.
  int layerCount = tech->getRoutingLayerCount();
  for (int i=1; i<=layerCount; i++) {
    dbTechLayer* layer = tech->findRoutingLayer(i);
    opendp::layer* myLayer = ckt.locateOrCreateLayer( layer->getConstName() );

    myLayer->type = layer->getType().getString();
    myLayer->direction = layer->getDirection().getString(); 
    myLayer->xPitch = myLayer->yPitch = layer->getPitch();
    myLayer->width = layer->getWidth();

    if( layer->hasMaxWidth() ) {
      myLayer->maxWidth = layer->getMaxWidth();
    }

//    cout << layer->getName() << endl; 
//    cout << layer->getType().getString() << endl;
//    cout << layer->getDirection().getString() << endl;
//    cout << layer->getPitch() << endl;
//    cout << layer->getMaxWidth() << endl;

  }
}

static void FillOpendpLibrary(circuit& ckt, dbSet<dbLib> &lib ) {
  dbSet<dbLib>::iterator liter;

  for(liter = lib.begin(); liter != lib.end(); ++liter) {
    dbLib* curLib = *liter;
  
    const int dbu = curLib->getDbUnitsPerMicron();
   
    dbSet<dbSite> sites = curLib->getSites();
    FillOpendpSite( ckt, sites, dbu ); 

    dbSet<dbMaster> masters = curLib->getMasters();
    FillOpendpMacro( ckt, masters, dbu );
  }
}

static void FillOpendpSite( circuit& ckt, dbSet<dbSite> &sites, const int dbu) {
  dbSet<dbSite>::iterator siter;
  for(siter = sites.begin(); siter != sites.end(); ++siter) {
    dbSite* site = *siter;
    opendp::site* mySite = ckt.locateOrCreateSite( site->getConstName() );

    mySite->width = 1.0 * site->getWidth() / dbu;
    mySite->height = 1.0 * site->getHeight() / dbu;

    if( site->getSymmetryX() ) {
      mySite->symmetries.push_back("X");
    }
    if( site->getSymmetryY() ) {
      mySite->symmetries.push_back("Y");
    }
    if( site->getSymmetryR90() ) {
      mySite->symmetries.push_back("R90");
    }
  }
}

static void FillOpendpMacro( circuit& ckt, dbSet<dbMaster> &masters, const int dbu) {
  dbSet<dbMaster>::iterator miter;
  for(miter = masters.begin(); miter != masters.end(); ++miter) {
    dbMaster* master = *miter;
    opendp::macro* macro = ckt.locateOrCreateMacro( master->getConstName() );
   
    const char* siteName = master->getSite()->getConstName(); 
    macro->sites.push_back( ckt.site2id.find(siteName)->second );

    int origX = 0, origY = 0;
    master->getOrigin(origX, origY);
    macro->xOrig = origX;
    macro->yOrig = origY;

    macro->width = 1.0 * master->getWidth() / dbu;
    macro->height = 1.0 * master->getHeight() / dbu;

    macro->type = master->getType().getString(); 

    dbSet<dbMTerm> mTerms = master->getMTerms();
    dbSet<dbMTerm>::iterator mtIter; 
    for(mtIter = mTerms.begin(); mtIter != mTerms.end(); ++mtIter) {
      dbMTerm* mTerm = *mtIter;

      macro_pin myPin;
      myPin.direction = mTerm->getIoType().getString();
      // pins' SHAPE and USE seems not available from OpenDB??

      const string pinName = mTerm->getConstName();
      dbSet<dbMPin> mPins = mTerm->getMPins(); 
      dbSet<dbMPin>::iterator mpIter;
      for(mpIter = mPins.begin(); mpIter != mPins.end(); ++mpIter) {
        dbMPin* mPin = *mpIter;

        dbSet<dbBox> geom = mPin->getGeometry();
        dbSet<dbBox>::iterator bIter;
        for(bIter = geom.begin(); bIter != geom.end(); ++bIter) {
          dbBox* box = *bIter;

          dbTechLayer* layer = box->getTechLayer();
          // skip for non-routing layer box info
          if( layer->getType() != dbTechLayerType::ROUTING ) {
            continue;
          }

          opendp::rect tmpRect;

          tmpRect.xLL = 1.0 * box->xMin() / dbu;
          tmpRect.yLL = 1.0 * box->yMin() / dbu;
          tmpRect.xUR = 1.0 * box->xMax() / dbu;
          tmpRect.yUR = 1.0 * box->yMax() / dbu;

          myPin.port.push_back(tmpRect);
//          cout << master->getConstName() << " " 
//            << pinName << " " 
//            << layer->getConstName() << " " 
//            << box->xMin() << " " << box->yMin() << " " 
//            << box->xMax() << " " << box->yMax() << endl;
          
        }
      }
      macro->pins[pinName] = myPin;
    }
  }

}

// Row Sort Function
static bool SortByRowCoordinate(const opendp::row& lhs,
    const opendp::row& rhs);
// Row Generation Function
static vector<opendp::row> GetNewRow(const circuit& ckt);


static void FillOpendpRow(circuit& ckt, dbSet<dbRow> &rows) {
  dbSet<dbRow>::iterator rIter; 
  for(rIter = rows.begin(); rIter != rows.end(); ++rIter) {
    dbRow* row = *rIter;
    opendp::row* myRow = ckt.locateOrCreateRow(row->getConstName());

    myRow->site = ckt.site2id.at( row->getSite()->getConstName() );
    int origX = 0, origY = 0;
    row->getOrigin(origX, origY); 
    myRow->origX = origX; 
    myRow->origY = origY;

    myRow->siteorient = row->getOrient().getString();
    myRow->numSites = row->getSiteCount();
    myRow->stepX = row->getSpacing();
    // stepY is always zero. if it's nonzero, very WEIRD design :(
    myRow->stepY = 0; 

//    cout << row->getConstName() << " " << myRow->numSites << " " << myRow->stepX << endl;

    if( fabs(ckt.rowHeight - 0.0f) <= DBL_EPSILON ) {
      ckt.rowHeight = ckt.sites[ myRow->site ].height 
        * ckt.DEFdist2Microns; 
    }

    if( ckt.wsite == 0 ) {
      ckt.wsite = int(ckt.sites[myRow->site].width 
          * ckt.DEFdist2Microns + 0.5f);
    }
    // update coreArea
    ckt.core.xLL = min(1.0*myRow->origX, ckt.core.xLL);
    ckt.core.yLL = min(1.0*myRow->origY, ckt.core.yLL);
    ckt.core.xUR = max(1.0*myRow->origX + myRow->numSites * ckt.wsite, 
        ckt.core.xUR);
    ckt.core.yUR = max(1.0*myRow->origY + ckt.rowHeight, ckt.core.yUR);
  }
      
  // Newly update DIEAREA information
  ckt.lx = ckt.die.xLL = 0.0;
  ckt.by = ckt.die.yLL = 0.0;
  ckt.rx = ckt.die.xUR = ckt.core.xUR - ckt.core.xLL;
  ckt.ty = ckt.die.yUR = ckt.core.yUR - ckt.core.yLL;

  cout << "CoreArea: " << endl;
  ckt.core.print();
  cout << "DieArea: " << endl;
  ckt.die.print();

  if( ckt.prevrows.size() <= 0) {
    cerr << "  ERROR: rowSize is 0. Please define at least one ROW in DEF" << endl;
    exit(1);
  }

  // sort ckt.rows
  sort(ckt.prevrows.begin(), ckt.prevrows.end(), SortByRowCoordinate);

  // change ckt.rows as CoreArea;
  ckt.rows = GetNewRow(ckt);
} 

static void FillOpendpPin(circuit& ckt, dbSet<dbBTerm> &bTerms) {
  dbSet<dbBTerm>::iterator btIter; 

  for(btIter = bTerms.begin(); btIter != bTerms.end(); ++btIter) {
    dbBTerm* bTerm = *btIter;
    opendp::pin* myPin = ckt.locateOrCreatePin( bTerm->getConstName() );
    
    myPin->type = (bTerm->getIoType() == dbIoType::INPUT)? PI_PIN : PO_PIN;
    int placeX = 0, placeY = 0;
    bTerm->getFirstPinLocation(placeX, placeY);

    // FIRM : placer cannot move this pin .
    myPin->isFixed = (bTerm->getFirstPinPlacementStatus() == dbPlacementStatus::FIRM)? 
      true : false; 
    myPin->x_coord = placeX - ckt.core.xLL;
    myPin->y_coord = placeY - ckt.core.yLL;
  }
}

static void FillOpendpCell(circuit& ckt, dbSet<dbInst> &insts) {

}

// Y first and X second
// First row should be in the first index to get orient
static bool SortByRowCoordinate(
    const opendp::row& lhs,
    const opendp::row& rhs) {
  if( lhs.origY < rhs.origY ) {
    return true;
  }
  if( lhs.origY > rhs.origY ) {
    return false;
  }

  return ( lhs.origX < rhs.origX );
}

// Generate New Row Based on CoreArea
static vector<opendp::row> GetNewRow(const circuit& ckt) {
  // Return Row Vectors
  vector<opendp::row> retRow;

  // calculation X and Y from CoreArea
  int rowCntX = IntConvert((ckt.core.xUR - ckt.core.xLL)/ckt.wsite);
  int rowCntY = IntConvert((ckt.core.yUR - ckt.core.yLL)/ckt.rowHeight);

  unsigned siteIdx = ckt.prevrows[0].site;
  string curOrient = ckt.prevrows[0].siteorient;

  for(int i=0; i<rowCntY; i++) {
    opendp::row myRow;
    myRow.site = siteIdx;
    myRow.origX = IntConvert(ckt.core.xLL);
    myRow.origY = IntConvert(ckt.core.yLL + i * ckt.rowHeight);

    myRow.stepX = ckt.wsite;
    myRow.stepY = 0;

    myRow.numSites = rowCntX;
    myRow.siteorient = curOrient;
    retRow.push_back(myRow);

    // curOrient is flipping. e.g. N -> FS -> N -> FS -> ...
    curOrient = (curOrient == "N")? "FS" : "N";
  }
  return retRow;
}

}


