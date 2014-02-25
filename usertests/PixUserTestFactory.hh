#ifndef PIXUSERTESTFACTORY_H
#define PIXUSERTESTFACTORY_H

#include <iostream>

#include "PixTest.hh"
#include "PixSetup.hh"

class PixUserTestFactory {
public:

  static PixUserTestFactory*  instance(); 
  PixTest*                    createTest(std::string, PixSetup *); 

protected: 
  PixUserTestFactory() {}; 
  ~PixUserTestFactory() {}; 

private:
  static PixUserTestFactory* fInstance; 

  ClassDef(PixUserTestFactory, 1)

};




#endif
