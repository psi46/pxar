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
PixTestParameters::PixTestParameters(string file, bool verbose) {
  readTestParameterFile(file, verbose);
}

// ----------------------------------------------------------------------
vector<string> PixTestParameters::getTests() {
  vector<string> a; 
  for (map<string, vector<pair<string, string> > >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    a.push_back(imap->first);
  }
  return a;
}

// ----------------------------------------------------------------------
bool PixTestParameters::readTestParameterFile(string file, bool verbose) {
  ifstream is(file.c_str()); 
  if (!is.is_open()) {
    LOG(logDEBUG) << "cannot read " << file;
    return false;
  }

  vector<pair<string, string> > testparameters;
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
      int testCnt(0); 
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
	  parValString = line.substr(m1+1); 
	  testparameters.push_back(make_pair(parName, parValString)); 
	  ++testCnt;
	} else {
	  break;
	}
      }
    }
    // -- add the map to the complete map
    fTests.insert(make_pair(testName, testparameters));
    testparameters.clear();
  }

  if (verbose) dump();

  return true;
}


// ----------------------------------------------------------------------
vector<pair<string, string> > PixTestParameters::getTestParameters(string testName) {
  return  fTests[testName]; 
}


// ----------------------------------------------------------------------
void PixTestParameters::dump() {
  for (map<string, vector<pair<string, string> > >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    LOG(logDEBUG) << "PixTestParameters: ->" << imap->first << "<-";
    vector<pair<string, string> > pars = imap->second; 
    for (vector<pair<string, string> >::iterator imap2 = pars.begin(); imap2 != pars.end(); ++imap2) {  
      LOG(logDEBUG) << "  " << imap2->first << ": " << imap2->second;
    }
  }
  
}


// ----------------------------------------------------------------------
bool PixTestParameters::setTestParameter(string testname, string parname, string value) {

  for (map<string, vector<pair<string, string> > >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    if (imap->first.compare(testname)) continue;
    LOG(logDEBUG) << "PixTestParameters: ->" << imap->first << "<-";
    vector<pair<string, string> > pars = imap->second; 
    for (unsigned int i = 0; i < pars.size(); ++i) {
      if (!pars[i].first.compare(parname)) {
	pars[i].second = value; 
	LOG(logDEBUG) << " setting  " << pars[i].first << " to new value " << pars[i].second;
      }
    }
    return true;
  }

  return false; 
}


// ----------------------------------------------------------------------
bool PixTestParameters::addTestParameter(string testname, string parname, string value) {

  for (map<string, vector<pair<string, string> > >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    if (imap->first.compare(testname)) continue;
    LOG(logDEBUG) << "PixTestParameters: ->" << imap->first << "<- adding parameter " << parname;
    vector<pair<string, string> > pars = imap->second; 
    // -- avoid adding a second identical parameter value (e.g. for PIX)
    bool alreadyIn(false); 
    for (unsigned int i = 0; i < pars.size(); ++i) {
      if (!pars[i].first.compare(parname)) {
	if (!pars[i].second.compare(value)) alreadyIn = true; 
	break;
      }
    }
    if (!alreadyIn) {
      pars.push_back(make_pair(parname, value)); 
      LOG(logDEBUG) << " adding  " << parname << " with value " << value;
    }

    return true;
  }

  return false; 
}


// ----------------------------------------------------------------------
bool PixTestParameters::setTestParameters(string testname, vector<std::pair<std::string, std::string> > v) {

  for (map<string, vector<pair<string, string> > >::iterator imap = fTests.begin(); imap != fTests.end(); ++imap) {  
    if (imap->first.compare(testname)) continue;
    LOG(logDEBUG) << "PixTestParameters: ->" << imap->first << "<-";
    imap->second = v;
    return true;
  }

  return false;
}
