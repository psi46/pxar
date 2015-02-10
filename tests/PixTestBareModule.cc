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
PixTestBareModule::PixTestBareModule(PixSetup *a, std::string name) : PixTest(a, name), fParMaxSteps(1), fStop(false), fBBMap(false), fBB2Map(false), fminIa(10.){
	PixTest::init();
	init();
	LOG(logDEBUG) << "PixTestBareModule ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestBareModule::PixTestBareModule() : PixTest() {
	LOG(logDEBUG) << "PixTestBareModule ctor()";
}

// ----------------------------------------------------------------------
bool PixTestBareModule::setParameter(string parName, string sval) {  //debug - add roc num
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
			if (!parName.compare("bbmap")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fBBMap = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("bb2map(desy)")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fBB2Map = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("mincurrent(ma)")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fminIa = atof(sval.c_str());
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
		+ string("stepmax: 1=pretest, 2=alive, 3=BB.");
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
		mDelay(1000);
		PixTest::update();

		PixTest::hvOff();
		mDelay(2000);
		PixTest::powerOff();
		mDelay(1000);
		PixTest::update();

		LOG(logINFO) << "PixTestBareModule:: HV and Power are off.";
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
	LOG(logINFO) << "PixTestBareModule:: checking if probes are in contact.";
	PixTest::powerOn();
	LOG(logINFO) << "PixTestBareModule:: Power on.";
	mDelay(1000);
	double ia = fApi->getTBia()*1E3; // [mA]
	bool checkgood = false;

	if (ia > fminIa) {
		LOG(logINFO) << "PixTestBareModule:: contact OK, ia = " << ia << " mA";
		checkgood = true;
	}
	else {
		LOG(logWARNING) << "PixTestBareModule:: ia < " << fminIa << " mA - PLEASE CHECK THE PROBES CONTACT.";
		if (fullSeq){
			LOG(logWARNING) << "PixTestBareModule:: enter 'c' to continue OR 's' to stop.";
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

				if (!input.compare("c") || !input.compare("C")) {
					LOG(logINFO) << "PixTestBareModule:: test will continue.";
					checkgood = true;
					goodIn = true;
				}
				else if (!input.compare("s") || !input.compare("S")) {
					LOG(logINFO) << "PixTestBareModule:: bare module test will be stopped.";
					checkgood = false;
					goodIn = true;
				}
				else {
					LOG(logINFO) << "PixTestBareModule:: unvalid input. Please retry.";
					checkgood = false;
				}
			} while (!goodIn);
		}
		else checkgood = false;
	}
	if (checkgood){
		if (fullSeq){
			PixTest::hvOn();
			LOG(logINFO) << "PixTestBareModule:: HV on.";
			mDelay(2000);
		}
		LOG(logINFO) << "PixTestBareModule:: checkIfInContact done.";
		return true;
	}
	else {
		mDelay(2000);
		return false;
	}
}

//----------------------------------------------------------
bool PixTestBareModule::doStdTest(std::string test) {
	
	PixTestFactory *factory = PixTestFactory::instance();
	PixTest *t(0);
	t = factory->createTest(test, fPixSetup);
	t->doTest();

	//to copy 'locally' the test histos
	bool newHist = true;
	string firstname, name;
	while (newHist){		
		TH1* h = t->nextHist();
		if (firstname == "") firstname = h->GetName();
		else name = h->GetName();
		if (firstname == name) newHist = false;
		else {
			fHistList.push_back(h);
			fHistOptions.insert(make_pair(h, t->getHistOption(h)));
			h->Draw(t->getHistOption(h).c_str());
			fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
			PixTest::update();
		}
	}

	PixTest::update();
	delete t;
	cout << fProblem << endl; //debug
	return fProblem;
}

// ----------------------------------------------------------------------
bool PixTestBareModule::doRocTests(int MaxStep) {	
	
	vector<string> suite;
	suite.push_back("pretest");
	suite.push_back("alive");
	suite.push_back("bb");
	suite.push_back("bb2");

	//Pretest
	if (MaxStep >= 1 && !fStop) { 
		if (!doStdTest(suite[0])) {
			LOG(logWARNING) << "PixTestBareModule:: Pretest failed. Sequence stopped.";
			return false;
		}
	}
	mDelay(1000);
	PixTest::update();

	//Alive
	if (MaxStep >= 2 && !fStop) {
		if (!doStdTest(suite[1])) { 
			LOG(logWARNING) << "PixTestBareModule:: Alive failed. Sequence stopped.";
			return false; 
		}
	}
	mDelay(1000);
	PixTest::update();

	//BumpBonding
	if (MaxStep >= 3 && !fStop) {
		cout << fBBMap << fBB2Map << endl;
		if (fBBMap && !fBB2Map) { 
			if (!doStdTest(suite[2])) { 
				LOG(logWARNING) << "PixTestBareModule:: BBMap failed. Sequence stopped.";
				return false; 
			}
		}
		else if (fBB2Map && !fBBMap) { 
			if (!doStdTest(suite[3])) { 
				LOG(logWARNING) << "PixTestBareModule:: BB2Map failed. Sequence stopped.";
				return false; 
			}
		}
		else {
			LOG(logWARNING) << "PixTestBareModule:: Please select the BB test mode.";
			LOG(logINFO) << "PixTestBareModule:: Test stopped.";
			return false;
		}
	}
	mDelay(1000);
	PixTest::update();
	return true;
}

// ----------------------------------------------------------------------
void PixTestBareModule::doTest() {
	
	LOG(logINFO) << "PixTestBareModule:: *******************************";
	LOG(logINFO) << "PixTestBareModule::doTest() start with " << fParMaxSteps << " test steps.";

	fDirectory->cd();
	bool sequenceEnded = false; //to handle pretest exception

	//ROC test (only if iA > 10 mA)
	if (checkIfInContact(1)) {
		//to do test sequence:
		sequenceEnded = doRocTests(fParMaxSteps);
		PixTest::update();
	}

	PixTest::hvOff();
	mDelay(2000);
	PixTest::powerOff();
	mDelay(1000);

	//separation
	LOG(logINFO) << "PixTestBareModule:: HV and Power are off.";
	if(sequenceEnded) LOG(logINFO) << "PixTestBareModule:: Tests finished, now you can separate.";

	LOG(logINFO) << "PixTestBareModule::doTest() done for.";
}
