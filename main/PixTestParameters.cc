#include <fstream>
#include <iostream>
#include <string>
#include <algorithm> /* for 'remove()' */

#include "PixTestParameters.hh"
#include "log.h"

using namespace std;
using namespace pxar;


// ----------------------------------------------------------------------
bool bothAreSpaces(char lhs, char rhs) { 
  return (lhs == rhs) && (lhs == ' '); 
}


// ----------------------------------------------------------------------
PixTestParameters::PixTestParameters(string file) {
  readTestParameterFile(file);
}

// ----------------------------------------------------------------------
vector<string> PixTestParameters::getTests() {
  vector<string> a; 
  for (map<string, map<string, string> >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    a.push_back(imap->first);
  }
  return a;
}

// ----------------------------------------------------------------------
bool PixTestParameters::readTestParameterFile(string file) {
  ifstream is(file.c_str()); 
  if (!is.is_open()) {
    LOG(logINFO) << "cannot read " << file;
    return false;
  }

  map<string, string> testparameters;
  string testName, parName, parValString; 
  bool oneTooMuch(false); 
  string line; 
  while (is.good())  {
    if (!oneTooMuch) {
      getline(is, line);
    } else {
      oneTooMuch = false; 
    }

    // -- found a new test, read all its parameters until you hit the next '--'
    if (string::npos != line.find("--")) {
      string::size_type m1 = line.find(" "); 
      if (m1 < line.size()) {
	testName = line.substr(m1+1); 
	testName.erase (std::remove(testName.begin(), testName.end(), ' '), testName.end());
      }
      while (is.good())  {
	getline(is, line);
	if (string::npos != line.find("--")) {
	  oneTooMuch = true; 
	  break;
	} 
	
	// -- parse parameter names and values
	std::string::iterator new_end = std::unique(line.begin(), line.end(), bothAreSpaces);
	line.erase(new_end, line.end());   
	
	
	string::size_type m1 = line.find(" "); 
	if (m1 < line.size()) {
	  parName = line.substr(0, m1); 
	  parValString = line.substr(m1); 
	  testparameters.insert(make_pair(parName, parValString)); 
	} else {
	  break;
	}
      }
    }
    // -- add the map to the complete map
    fTests.insert(make_pair(testName, testparameters));
    testparameters.clear();
  }

  dump();

  return true;
}


// ----------------------------------------------------------------------
map<string, string> PixTestParameters::getTestParameters(string testName) {
  return  fTests[testName]; 
}


// ----------------------------------------------------------------------
void PixTestParameters::dump() {
  for (map<string, map<string, string> >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    LOG(logINFO) << "PixTestParameters: ->" << imap->first << "<-";
    map<string, string> pars = imap->second; 
    for (map<string, string>::iterator imap2 = pars.begin(); imap2 != pars.end(); ++imap2) {  
      LOG(logINFO) << "  " << imap2->first << ": " << imap2->second;
    }
  }
  
}


// ----------------------------------------------------------------------
bool PixTestParameters::setTestParameter(std::string testname, std::string parname, std::string value) {

  for (map<string, map<string, string> >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    LOG(logINFO) << "PixTestParameters: ->" << imap->first << "<-";
    map<string, string> pars = imap->second; 
    for (map<string, string>::iterator imap2 = pars.begin(); imap2 != pars.end(); ++imap2) {  
      LOG(logINFO) << "  " << imap2->first << ": " << imap2->second;
    }
  }
  
  return true; 
}


