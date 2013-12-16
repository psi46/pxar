#ifndef PIXTBM_H
#define PIXTBM_H

#include <map>
#include <vector>

#include "PixTbm.hh"
#include "PixRoc.hh"

class PixTbm: public TObject {
public:
  PixTbm(); 
  ~PixTbm(); 
  void init();

private: 
  
  static std::map<std::string, int> fDacDef;
  std::map<std::string, int> fDacs;
  

  ClassDef(PixTbm, 1); 
};

#endif

