// Configuration parameters

#ifndef CONFIGPARAMETERS
#define CONFIGPARAMETERS

#include <string>

class ConfigParameters {
public:
  ConfigParameters();
  ConfigParameters(std::string filename);

  void initialize();
  bool readConfigParameterFile(std::string filename);
  bool writeConfigParameterFile();

  static ConfigParameters* Singleton();

  std::string getTBParametersFileName()   {return fTBParametersFileName;}
  std::string getDACParametersFileName()  {return fDACParametersFileName;}
  std::string getTbmParametersFileName()  {return fTbmParametersFileName;}
  std::string getTrimParametersFileName() {return fTrimParametersFileName;}
  std::string getTestParametersFileName() {return fTestParametersFileName;}
  std::string getRootFileName()           {return fRootFileName;}
  std::string getLogFileName()            {return fLogFileName;}
  std::string getMaskFileName()           {return fMaskFileName;}
  std::string getDebugFileName()          {return fDebugFileName;}
  std::string getDirectory()              {return fDirectory;}

  void setTBParameterFileName(const std::string &filename);
  void setDACParameterFileName(const std::string &filename);
  void setTbmParameterFileName(const std::string &filename);
  void setTrimParameterFileName(const std::string &filename);
  void setTestParameterFileName(const std::string &filename);
  void setRootFileName(const std::string &filename);
  void setLogFileName(const std::string &filename);
  void setDebugFileName(const std::string &filename);
  void setMaskFileName(const std::string &filename);
  void setDirectory(std::string &s);

  void setGuiMode(bool a) {fGuiMode = a;}

private:

  int fnRocs, fnModules, fHubId, fDataTriggerLevel, fHalfModule;
  int fCustomModule;
  int fEmptyReadoutLength, fEmptyReadoutLengthADC, fEmptyReadoutLengthADCDual, fTbmChannel;
  double ia, id, va, vd;
  float rocZeroAnalogCurrent;
  std::string fRocType;
  std::string fDirectory;
  std::string fTBName;
  bool fHvOn, fTbmEnable, fTbmEmulator, fKeithleyRemote, fGuiMode;

  std::string fTBParametersFileName;
  std::string fDACParametersFileName;
  std::string fTbmParametersFileName;
  std::string fTrimParametersFileName;
  std::string fTestParametersFileName;
  std::string fRootFileName;
  std::string fLogFileName;
  std::string fMaskFileName;
  std::string fDebugFileName;

  static ConfigParameters* fInstance;
};

#endif
