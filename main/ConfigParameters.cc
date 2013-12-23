#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <cstdlib>
#include <string.h>
#include <stdio.h>

#include <TString.h>

#include "ConfigParameters.hh"

using namespace std;

ConfigParameters * ConfigParameters::fInstance = 0;

bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

// ----------------------------------------------------------------------
ConfigParameters::ConfigParameters() {
  initialize();
}


// ----------------------------------------------------------------------
ConfigParameters::ConfigParameters(string filename) {
  initialize();
  readConfigParameterFile(filename);
}


// ----------------------------------------------------------------------
void ConfigParameters::initialize() {
  cout << fTBName << endl;

  fnRocs = 16;
  fnTbms = 1; 
  fnModules = 1;
  fHubId = 31;
  
  fCustomModule = 0;

  fHvOn = true;
  fTbmEnable = true;
  fTbmEmulator = false;
  fKeithleyRemote = false;
  fGuiMode = false;
  fTbmChannel = 0;
  fHalfModule = 0;

  fEmptyReadoutLength = 54;
  fEmptyReadoutLengthADC = 64;
  fEmptyReadoutLengthADCDual = 40;

  fDACParametersFileName  = "defaultDACParameters.dat";
  fTbmParametersFileName  = "defaultTBMParameters.dat";
  fTBParametersFileName   = "defaultTBParameters.dat";
  fTrimParametersFileName = "defaultTrimParameters.dat";
  fTestParametersFileName = "defaultTestParameters.dat";
  fMaskFileName           = "defaultMaskFile.dat";
  fLogFileName            = "log.txt";
  fDebugFileName          = "debug.log";
  fRootFileName           = "expert.root";

  ia = 1.2;
  id = 1.;
  va = 1.7;
  vd = 2.5;

  rocZeroAnalogCurrent = 0.0;
  fRocType = "psi46v2";
}


// ----------------------------------------------------------------------
ConfigParameters*  ConfigParameters::Singleton() {
  if (fInstance == 0) {fInstance = new ConfigParameters();}
  return fInstance;
}


// ----------------------------------------------------------------------
bool ConfigParameters::readConfigParameterFile(string file) {
  std::ifstream _input(file.c_str());
  if (!_input.is_open())
    {
      cout << "[ConfigParameters] Can not open file '"
		     << file << "' to read Config Parameters." << endl;

      return false;
    }

  cout << "[ConfigParameters] Reading Config-Parameters from '"
       << file << "'." << endl;

  // Read file by lines
  for (std::string _line; _input.good();)
    {
      getline(_input, _line);

      // Skip Empty Lines and Comments (starting from # or - )
      if (!_line.length()
	  || '#' == _line[0]
	  || '-' == _line[0]) continue;

      std::istringstream _istring(_line);
      std::string _name;
      std::string _value;

      _istring >> _name >> _value;

      // Skip line in case any errors occured while reading parameters
      if (_istring.fail() || !_name.length()) continue;

      int _ivalue = atoi(_value.c_str());

      if (0 == _name.compare("testboardName")) { fTBName = _value; }
      else if (0 == _name.compare("directory")) { fDirectory =  _value; }

      else if (0 == _name.compare("tbParameters")) { setTBParameterFileName(_value); }
      else if (0 == _name.compare("tbmParameters")) { setTbmParameterFileName(_value); }
      else if (0 == _name.compare("testParameters")) { setTestParameterFileName(_value); }
      else if (0 == _name.compare("dacParameters")) { setDACParameterFileName(_value); }
      else if (0 == _name.compare("rootFileName")) { setRootFileName(_value); }
      else if (0 == _name.compare("trimParameters")) { setTrimParameterFileName(_value); }

      else if (0 == _name.compare("nModules")) { fnModules                  = _ivalue; }
      else if (0 == _name.compare("nRocs")) { fnRocs                     = _ivalue; }
      else if (0 == _name.compare("hubId")) { fHubId                     = _ivalue; }
      else if (0 == _name.compare("customModule")) { fCustomModule              = _ivalue; }
      else if (0 == _name.compare("halfModule")) { fHalfModule                = _ivalue; }
      else if (0 == _name.compare("dataTriggerLevel")) { fDataTriggerLevel          = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLength")) { fEmptyReadoutLength        = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLengthADC")) { fEmptyReadoutLengthADC     = _ivalue; }
      else if (0 == _name.compare("emptyReadoutLengthADCDual")) { fEmptyReadoutLengthADCDual = _ivalue; }
      else if (0 == _name.compare("hvOn")) { fHvOn                      = _ivalue; }
      else if (0 == _name.compare("keithleyRemote")) { fKeithleyRemote            = _ivalue; }
      else if (0 == _name.compare("tbmEnable")) { fTbmEnable                 = _ivalue; }
      else if (0 == _name.compare("tbmEmulator")) { fTbmEmulator             = _ivalue; }
      else if (0 == _name.compare("tbmChannel")) { fTbmChannel                = _ivalue; }

      else if (0 == _name.compare("ia")) { ia = .001 * _ivalue; }
      else if (0 == _name.compare("id")) { id = .001 * _ivalue; }
      else if (0 == _name.compare("va")) { va = .001 * _ivalue; }
      else if (0 == _name.compare("vd")) { vd = .001 * _ivalue; }

      else if (0 == _name.compare("rocZeroAnalogCurrent")) { rocZeroAnalogCurrent = .001 * _ivalue; }

      else if (0 == _name.compare("rocType")) { fRocType = _value; }

      else { cout << "[ConfigParameters] Did not understand '"
			    << _name << "'." << endl; }
    }

  _input.close();

  return true;
}


// ----------------------------------------------------------------------
vector<pair<string,uint8_t> > ConfigParameters::readDacFile(string fname) {
  vector<pair<string,uint8_t> > rocDacs; 

  // -- read in file
  vector<string> lines; 
  char  buffer[5000];
  cout << "reading " << fname << endl;
  ifstream is(fname.c_str());
  while (is.getline(buffer, 200, '\n')) {
    lines.push_back(string(buffer));
  }
  is.close();

  // -- parse lines
  unsigned int ival(0); 
  uint8_t uval(0); 
  
  string::size_type s1, s2; 
  string str1, str2, str3;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    //    cout << lines[i] << endl;   
    // -- remove tabs, adjacent spaces, leading and trailing spaces
    replaceAll(lines[i], "\t", " "); 
    std::string::iterator new_end = std::unique(lines[i].begin(), lines[i].end(), BothAreSpaces);
    lines[i].erase(new_end, lines[i].end()); 
    if (lines[i].substr(0, 1) == string(" ")) lines[i].erase(0, 1); 
    if (lines[i].substr(lines[i].length()-1, 1) == string(" ")) lines[i].erase(lines[i].length()-1, 1); 
    s1 = lines[i].find(" "); 
    s2 = lines[i].rfind(" "); 
    if (s1 != s2) {
      str1 = lines[i].substr(0, s1); 
      str2 = lines[i].substr(s1+1, s2-s1); 
      str3 = lines[i].substr(s2+1); 
    } else {
      cout << "could not read line -->" << lines[i] << "<--" << endl;
    }
    
    ival = atoi(str3.c_str()); 
    uval = ival; //FIXME DOES NOT WORK
    //    cout << "  -->  " << str2 << " ival = " << ival << " uval = " << uval << endl;
    rocDacs.push_back(make_pair(str2, uval)); 

  }

  return rocDacs; 
}

// ----------------------------------------------------------------------
std::vector<std::vector<std::pair<std::string,uint8_t> > > ConfigParameters::getRocDacs() {

  vector<vector<pair<string,uint8_t> > > allDacs; 
  string filename; 

  for (int i = 0; i < fnRocs; ++i) {
    filename = Form("%s_C%d.dat", fDACParametersFileName.c_str(), i); 
    cout << "reading " << filename << endl;
    std::vector<std::pair<std::string,uint8_t> > rocDacs = readDacFile(filename); 
    allDacs.push_back(rocDacs); 
  }

  return allDacs; 
}


// ----------------------------------------------------------------------
std::vector<std::vector<std::pair<std::string,uint8_t> > > ConfigParameters::getTbmDacs() {

  vector<vector<pair<string,uint8_t> > > allDacs; 
  string filename; 

  // FIXME changing naming scheme for TBMs to incorporate the possible case of >1 TBMs
  for (int i = 0; i < fnTbms; ++i) {
    filename = Form("%s", fTbmParametersFileName.c_str()); 
    cout << "reading " << filename << endl;
    std::vector<std::pair<std::string,uint8_t> > rocDacs = readDacFile(filename); 
    allDacs.push_back(rocDacs); 
  }

  return allDacs; 
}


// ----------------------------------------------------------------------
void ConfigParameters::setTBParameterFileName(const std::string &file) {
  fTBParametersFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setTbmParameterFileName(const std::string &file) {
  fTbmParametersFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setDACParameterFileName(const std::string &file) {
  fDACParametersFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setTrimParameterFileName(const std::string &file) {
  fTrimParametersFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setTestParameterFileName(const std::string &file) {
  fTestParametersFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setRootFileName(const std::string &file) {
  fRootFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setLogFileName(const std::string &file) {
  fLogFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setDebugFileName(const std::string &file) {
  fDebugFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setMaskFileName(const std::string &file) {
  fMaskFileName.assign(fDirectory).append("/").append(file);
}


// ----------------------------------------------------------------------
void ConfigParameters::setDirectory(std::string &d) {
  fDirectory = d;
}

// ----------------------------------------------------------------------
bool ConfigParameters::writeConfigParameterFile() {
  char filename[1000];
  sprintf(filename, "%s/configParameters.dat", fDirectory.c_str());
  FILE * file = fopen(filename, "w");
  if (!file)
    {
      cout << "[ConfigParameters] Can not open file '" << filename
		     << "' to write configParameters." << endl;
      return false;
    }

  cout << "[ConfigParameters] Writing Config-Parameters to '"
       << filename << "'." << endl;

  fprintf(file, "testboardName %s\n\n", fTBName.c_str());

  fprintf(file, "-- parameter files\n\n");

  fprintf(file, "tbParameters %s\n",   fTBParametersFileName.c_str());
  fprintf(file, "dacParameters %s\n",  fDACParametersFileName.c_str());
  fprintf(file, "tbmParameters %s\n",  fTbmParametersFileName.c_str());
  fprintf(file, "trimParameters %s\n", fTrimParametersFileName.c_str());
  fprintf(file, "testParameters %s\n", fTestParametersFileName.c_str());
  fprintf(file, "rootFileName %s\n\n", fRootFileName.c_str());

  fprintf(file, "-- configuration\n\n");

  if (fCustomModule) fprintf(file, "customModule %i\n", fCustomModule);

  fprintf(file, "nModules %i\n", fnModules);
  fprintf(file, "nRocs %i\n", fnRocs);
  fprintf(file, "hubId %i\n", fHubId);
  fprintf(file, "tbmEnable %i\n", fTbmEnable);
  fprintf(file, "tbmEmulator %i\n", fTbmEmulator);
  fprintf(file, "hvOn %i\n", fHvOn);
  fprintf(file, "tbmChannel %i\n\n", fTbmChannel);
  fprintf(file, "halfModule %i\n\n", fHalfModule);
  fprintf(file, "rocType %s\n\n", fRocType.c_str());

  fprintf(file, "-- voltages and current limits\n\n");

  fprintf(file, "ia %i\n"  , static_cast<int>(ia * 1000));
  fprintf(file, "id %i\n"  , static_cast<int>(id * 1000));
  fprintf(file, "va %i\n"  , static_cast<int>(va * 1000));
  fprintf(file, "vd %i\n\n", static_cast<int>(vd * 1000));

  fprintf(file, "rocZeroAnalogCurrent %i\n\n", static_cast<int>(rocZeroAnalogCurrent * 1000));

  fclose(file);
  return true;
}
