#ifndef PIXROC_H
#define PIXROC_H

#include <map>
#include <vector>

#include <TObject.h>

#include "PixRoc.hh"

class PixRoc: public TObject {
public:
  PixRoc(); 
  ~PixRoc(); 
  void init();

private: 
  
  std::map<std::string, int> fDacDef;
  std::map<std::string, int> fDacs;

  ClassDef(PixRoc, 1); 
  
};

#endif

