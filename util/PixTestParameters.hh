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
  bool setTestParameter(std::string testname, std::string parname, std::string value);
  bool addTestParameter(std::string testname, std::string parname, std::string value);
  bool setTestParameters(std::string testname, std::vector<std::pair<std::string, std::string> > );
  std::vector<std::string> getTests(); 
  void dump(); 

private: 
  std::map<std::string, std::vector<std::pair<std::string, std::string> > > fTests;
  
};

#endif
