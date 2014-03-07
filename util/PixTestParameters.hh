#ifndef PIXTESTPARAMETERS
#define PIXTESTPARAMETERS

#include "pxardllexport.h"

#include <map>
#include <vector>

class DLLEXPORT PixTestParameters {

public:

  PixTestParameters(std::string file, bool verbose = false); 
  bool readTestParameterFile(std::string file, bool verbose);
  std::vector<std::pair<std::string, std::string> > getTestParameters(std::string testName);
  std::vector<std::string> getTests(); 
  void dump(); 

private: 
  std::map<std::string, std::vector<std::pair<std::string, std::string> > > fTests;
  
};

#endif
