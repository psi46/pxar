#ifndef TBINTERFACE
#define TBINTERFACE

#include <iostream>
#include <vector>

class ConfigParameters; 

class TBInterface {
public:
  TBInterface();
  TBInterface(ConfigParameters *);
  virtual ~TBInterface();

  void HVoff();
  void HVon();

  void Poff();
  void Pon();

  virtual bool IsPresent() = 0;
  virtual void Cleanup() = 0;
  virtual std::vector<int> GetEfficiencyMap(int, int);
};

#endif
