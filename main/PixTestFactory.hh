#ifndef PIXTESTFACTORY_H
#define PIXTESTFACTORY_H

#include <iostream>

#include "PixTest.hh"
#include "PixSetup.hh"

class PixTestFactory {
public:

  static PixTestFactory*  instance(); 
  PixTest*                createTest(string, PixSetup *); 

protected: 
  PixTestFactory(); 
  ~PixTestFactory(); 

private:
  static PixTestFactory* fInstance; 

  ClassDef(PixTestFactory, 1)

};




#endif
