#ifndef PIXMODULE_H
#define PIXMODULE_H

#include <map>
#include <vector>

#include "PixTbm.hh"
#include "PixRoc.hh"

class PixModule: public TObject {
public:
  PixModule(); 
  ~PixModule(); 
  void init();

private: 
  
  std::map<std::string, int> fDacDef;
  std::map<std::string, int> fDacs;
  

  ClassDef(PixModule, 1); // testing PixRoc
  std::vector<PixTbm> fTbm; 
  std::vector<PixRoc> fRoc; 
};

#endif

