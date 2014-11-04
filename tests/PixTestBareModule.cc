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
PixTestBareModule::PixTestBareModule(PixSetup *a, std::string name) : PixTest(a, name), fParMaxSteps(1), fStop(false) {
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
			if (!parName.compare("stepmax(1-3)")) {
				fParMaxSteps = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParNSteps -> " << fParMaxSteps;
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
	fTestTip = string("run the test sequence for one ROC up to 'stepmax'.\n")
		+ string("To be run after that probes are in contact with ROC.");
	fSummaryTip = string("to be implemented");
	fStopTip = string("stop the BareModuleTest after that \n")
		+ string("the current step is finished.");
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

	if (!command.compare("stop")){     // Interrupt the test 
		fStop = true;
		LOG(logINFO) << "PixTestBareModule:: STOP PRESSED. Ending test.";
	}
	else if (!command.compare("checkcontact")) {
		checkIfInContact(0);
	}
	else if (!command.compare("pretest")) {
		doStdTest("pretest");
		mDelay(1000);
		PixTest::update();
		LOG(logINFO) << "PixTestBareModule:: step finished. Warning: HV and Power still ON.";
	}
	else if (!command.compare("alive")) {
		doStdTest("alive");
		mDelay(1000);
		PixTest::update();
		LOG(logINFO) << "PixTestBareModule:: step finished. Warning: HV and Power still ON.";
	}
	else if (!command.compare("bumpbonding")) {
		doStdTest("bumpbonding");
		mDelay(1000);
		PixTest::update();
		LOG(logINFO) << "PixTestBareModule:: step finished. Warning: HV and Power still ON.";
	}
	else if (!command.compare("dofullsequence")) {
		fParMaxSteps = 3;
		doTest();
	}
	else
		LOG(logINFO) << "Command " << command << " not implemented.";
}

// ----------------------------------------------------------------------
bool PixTestBareModule::checkIfInContact(bool fullSeq) {	

	PixTest::hvOff();
	LOG(logINFO) << "PixTestBareModule:: HV off for safety.";
	mDelay(2000);

	//check if probes are in contact
	double minIa = 10; // [mA]   loose limit
	LOG(logINFO) << "PixTestBareModule:: checking if probes are in contact.";
	PixTest::powerOn();
	LOG(logINFO) << "PixTestBareModule:: Power on.";
	mDelay(1000);
	double ia = fApi->getTBia()*1E3; // [mA]
	bool checkgood = false;

	if (ia > minIa) {
		LOG(logINFO) << "PixTestBareModule:: contact OK, ia = " << ia << " mA";
		checkgood = true;
	}
	else {
		LOG(logWARNING) << "PixTestBareModule:: ia < " << minIa << " mA - PLEASE CHECK THE PROBES CONTACT.";
		if (fullSeq){
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
					LOG(logINFO) << "PixTestBareModule:: test will continue.";
					checkgood = true;
				}
				else if (!input.compare("stop")) {
					LOG(logINFO) << "PixTestBareModule:: bare module test will be stopped.";
					checkgood = false;
				}
				else {
					LOG(logINFO) << "PixTestBareModule:: unvalid input.";
					checkgood = false;
				}
			} while (!goodIn);
		}
	}
	if (checkgood){
		PixTest::hvOn();
		LOG(logINFO) << "PixTestBareModule:: HV on.";
		mDelay(2000);
		LOG(logINFO) << "PixTestBareModule:: checkIfInContact done.";
		return true;
	}
	else return false;
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
bool PixTestBareModule::doRocTests(int MaxStep) {	
	
	vector<string> suite;
	suite.push_back("pretest");
	suite.push_back("alive");
	suite.push_back("bumpbonding");

	//Pretest
	if (MaxStep >= 1 && !fStop) { doStdTest(suite[0]); }	
	mDelay(1000);
	PixTest::update();
	if (fProblem) {
		LOG(logINFO) << "PixTestBareModule:: Pretest failed. Sequence stopped.";
		return false;
	}
	
	//Alive
	if (MaxStep >= 2 && !fStop) {
		/*   
		//force to low signal before pixel alive (as in pretest)   - debug, needed?
		fApi->setDAC("CtrlReg", 0);
		fApi->setDAC("Vcal", 250);
		LOG(logINFO) << "PixTestBareModule:: Forced CtrlReg = 0, Vcal = 250.";
		*/
		doStdTest(suite[1]); 
	}
	mDelay(1000);
	PixTest::update();

	//BumpBonding
	if (MaxStep >= 3 && !fStop) { doStdTest(suite[2]); }
	mDelay(1000);
	PixTest::update();

	return true;
}

// ----------------------------------------------------------------------
void PixTestBareModule::doTest() {
	
	LOG(logINFO) << "PixTestBareModule:: *******************************";
	LOG(logINFO) << "PixTestBareModule::doTest() start with " << fParMaxSteps << " test steps.";

	bool sequenceEnded = false; //to handle pretest exception

	//ROC test (only if iA > 10 mA)
	if (checkIfInContact(1)) {
		//to do test sequence:
		sequenceEnded = doRocTests(fParMaxSteps);
	}
		
	PixTest::hvOff();
	LOG(logINFO) << "PixTestBareModule:: HV off.";
	mDelay(2000);

	PixTest::powerOff();
	LOG(logINFO) << "PixTestBareModule:: Power off.";
	mDelay(1000);

	//separation
	LOG(logINFO) << "PixTestBareModule:: HV and Power are off.";
	if(sequenceEnded) LOG(logINFO) << "PixTestBareModule:: tests finished, you can separate.";

	LOG(logINFO) << "PixTestBareModule::doTest() done for.";
}
