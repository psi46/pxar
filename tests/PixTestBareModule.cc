#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <TSystem.h>
#include "PixUtil.hh"
#include "PixTestFactory.hh"
#include "PixTestBareModule.hh"

#include "log.h"
#include "helper.h"
#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestBareModule)

// ----------------------------------------------------------------------
PixTestBareModule::PixTestBareModule(PixSetup *a, std::string name) : PixTest(a, name), fParNSteps(1), fStop(false) {	
	PixTest::init();
	init();
	LOG(logDEBUG) << "PixTestBareModule ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestBareModule::PixTestBareModule() : PixTest() {
	LOG(logDEBUG) << "PixTestBareModule ctor()";
}

// ----------------------------------------------------------------------
bool PixTestBareModule::setParameter(string parName, string sval) {
	bool found(false);
	string stripParName;
	for (unsigned int i = 0; i < fParameters.size(); ++i) {
		if (fParameters[i].first == parName) {
			found = true;
			if (!parName.compare("ntests")) {
				fParNSteps = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParNSteps -> " << fParNSteps;
				setToolTips();
			}
			break;
		}
	}
	return found;
}


// ----------------------------------------------------------------------
void PixTestBareModule::init() {
	LOG(logDEBUG) << "PixTestBareModule::init()";
	setToolTips();
	fDirectory = gFile->GetDirectory(fName.c_str());
	if (!fDirectory) {
		fDirectory = gFile->mkdir(fName.c_str());
	}
	fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBareModule::setToolTips() {
	fTestTip = string("run the BareModuleTest for one ROC. To be run after that probes are in contact with ROC.");
	fSummaryTip = string("to be implemented");
	fStopTip = string("stop the BareModuleTest after that one step is finished.");
}


// ----------------------------------------------------------------------
void PixTestBareModule::bookHist(string name) {
	LOG(logDEBUG) << "nothing done with " << name;
	fDirectory->cd();
}

//----------------------------------------------------------
PixTestBareModule::~PixTestBareModule() {
	LOG(logDEBUG) << "PixTestBareModule dtor";
}

// ----------------------------------------------------------------------
void PixTestBareModule::runCommand(std::string command) {
	std::transform(command.begin(), command.end(), command.begin(), ::tolower);
	LOG(logDEBUG) << "running command: " << command;

	if (!command.compare("stop")){ 
		// Interrupt the test 
		fStop = true;
		LOG(logINFO) << "PixTestBareModule:: STOP PRESSED. Ending test.";
	}
	else	LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

//----------------------------------------------------------
void PixTestBareModule::doStdTest(std::string test) {
	
	PixTestFactory *factory = PixTestFactory::instance();
	PixTest *t(0);
	t = factory->createTest(test, fPixSetup);		
	t->doTest();
	delete t;
}

// ----------------------------------------------------------------------
void PixTestBareModule::doTestRoc(int step) {

	vector<string> suite;
	suite.push_back("pretest");
	suite.push_back("alive");
	suite.push_back("bumpbonding");
	
	fApi->Pon();
	mDelay(500);

	fApi->HVon();
	LOG(logDEBUG) << "PixTestBareModule::doTestRoc() set HV on";
	mDelay(500);

	//Pretest - vana + tornado
	if (step >= 1 && !fStop) { doStdTest(suite[0]); }
	mDelay(1000);
	PixTest::update();
	
	//Alive
	if (step >= 2 && !fStop) { doStdTest(suite[1]); }
	mDelay(1000);
	PixTest::update();

	//BBMap - test
	if (step >= 3 && !fStop) { doStdTest(suite[2]); }
	mDelay(1000);
	PixTest::update();

	//HVOFF
	fApi->HVoff();
	LOG(logDEBUG) << "PixTestBareModule::doTestRoc() set HV off";

	//POFF
	fApi->Poff();
	LOG(logDEBUG) << "PixTestBareModule::doTestRoc() ROC power off";
}

// ----------------------------------------------------------------------
void PixTestBareModule::doTest() {
	
	LOG(logINFO) << "PixTestBareModule:: *******************************";
	LOG(logINFO) << "PixTestBareModule::doTest() start with " << fParNSteps << " test steps.";

	//HVOFF
	fApi->HVoff();
	LOG(logDEBUG) << "PixTestBareModule:: HV off for safety.";

	//check if probes are in contact
	double minIa = 10; // [mA]
	LOG(logINFO) << "PixTestBareModule:: checking if probes are in contact.";
	fApi->Pon();
	mDelay(1000);
	double ia = fApi->getTBia()*1E3; // [mA]
	if (ia > minIa) { LOG(logINFO) << "PixTestBareModule:: contact OK, ia = " << ia << " mA"; }
	else {   // loose limit
		fApi->Poff();
		LOG(logWARNING) << "PixTestBareModule:: ia < " << minIa << " mA - CHECK PROBES CONTACT.";
		LOG(logWARNING) << "PixTestBareModule:: ENTER continue OR stop.";
		bool goodIn = false;
		do{
			string input;
			std::getline(cin, input);
			string::size_type m1 = input.find(" ");
			if (m1 != string::npos) {
				string parameters = input.substr(m1);
				input = input.substr(0, m1);
			}
			std::transform(input.begin(), input.end(), input.begin(), ::tolower);
			if (!input.compare("continue")) {
				goodIn = true;
				LOG(logINFO) << "PixTestBareModule:: test will continue.";
				break;
			}
			else if (!input.compare("stop")) {
				goodIn = true;
				LOG(logINFO) << "PixTestBareModule:: bare module test will be stopped.";
				return;
			}
			else LOG(logINFO) << "PixTestBareModule:: unvalid input.";
		} while (!goodIn);
	}
	
	//ROC test
	doTestRoc(fParNSteps);
		
	//separation
	LOG(logINFO) << "PixTestBareModule:: HV and LV are off.";
	LOG(logINFO) << "PixTestBareModule:: tests finished, you can separate.";

	LOG(logINFO) << "PixTestBareModule::doTest() done for.";
}