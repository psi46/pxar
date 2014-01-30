#ifndef PIXTESTPARAMETERS
#define PIXTESTPARAMETERS

#include <map>
#include <vector>

class PixTestParameters {
public:

  PixTestParameters(std::string file); 
  bool readTestParameterFile(std::string file);
  std::map<std::string, std::string> getTestParameters(std::string testName);
  std::vector<std::string> getTests(); 
  //  bool setTestParameter(std::string testname, std::string parname, std::string value); 
  void dump(); 

private: 
  std::map<std::string, std::map<std::string, std::string> > fTests;
  
};

#endif
