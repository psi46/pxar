#ifndef CURRENTVSDAC_H
#define CURRENTVSDAC_H

#include "api.h"
#include "PixTest.hh"

class CurrentvsDac: public PixTest {
public:
  CurrentvsDac(PixSetup *, std::string);
  CurrentvsDac();
  virtual ~CurrentvsDac();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  std::string fParDAC; 

  ClassDef(CurrentvsDac, 1);

};
#endif
