#ifndef TBVIRTUALINTERFACE
#define TBVIRTUALINTERFACE

#include "TBInterface.hh"
#include "SysCommand.hh"


class ConfigParameters; 

class TBVirtualInterface: public TBInterface {
public:
  TBVirtualInterface();
  TBVirtualInterface(ConfigParameters *);
  virtual ~TBVirtualInterface();
  virtual bool IsPresent();
  virtual void Cleanup();
  virtual bool Execute(SysCommand *);
  virtual std::vector<int> GetEfficiencyMap(int ntrig, int flag); 
};

#endif

