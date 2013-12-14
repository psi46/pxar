#ifndef TBVIRTUALINTERFACE
#define TBVIRTUALINTERFACE

#include "TBInterface.hh"


class ConfigParameters; 

class TBVirtualInterface: public TBInterface {
public:
  TBVirtualInterface();
  TBVirtualInterface(ConfigParameters *);
  virtual ~TBVirtualInterface();
  virtual bool IsPresent();
  virtual void Cleanup();
  virtual std::vector<int> GetEfficiencyMap(int ntrig, int flag); 
};

#endif

