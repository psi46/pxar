// -- author: Martino Dall'Osso
// to send different patterns and check the readout

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "PixTestPattern.hh"

#include "log.h"
#include "constants.h"
#include "helper.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPattern)

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern(PixSetup *a, std::string name) : PixTest(a, name), fParPgCycles(0), fParTrigLoop(0), fParPeriod(0), fParSeconds(0), fPatternFromFile(0), fResultsOnFile(1), fBinOut(0), fFileName("null"), fUnMaskAll(0){
	PixTest::init();
	init();
	LOG(logDEBUG) << "PixTestPattern ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern() : PixTest(){ //ctor
}

//------------------------------------------------------------------------------
bool PixTestPattern::setParameter(string parName, string sval)
{
	bool found(false);
	ParOutOfRange = false;
	std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
	for (unsigned int i = 0; i < fParameters.size(); ++i)
	{
		if (fParameters[i].first == parName)
		{
			found = true;
			sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

			if (!parName.compare("pgcycles")){
				fParPgCycles = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting pgcycles -> " << fParPgCycles;
				if (fParPgCycles < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() pg_cycles must be positive";
					found = false; ParOutOfRange = true;
				}
			}

			if (!parName.compare("triggerloop")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fParTrigLoop = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParTrigLoop -> " << fParTrigLoop;
			}

			if (!parName.compare("period")){
				fParPeriod = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParPeriod -> " << fParPeriod;
				if (fParPeriod < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() period must be positive";
					found = false; ParOutOfRange = true;
				}
			}

			if (!parName.compare("seconds")){
				fParSeconds = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParSeconds -> " << fParSeconds;
				if (fParSeconds < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() seconds must be positive";
					found = false; ParOutOfRange = true;
				}
			}

			if (!parName.compare("patternfromfile")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fPatternFromFile = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fPatternFromFile -> " << fPatternFromFile;
			}

			if (!parName.compare("resultsonfile")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fResultsOnFile = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fResultsOnFile -> " << fResultsOnFile;
			}

			if (!parName.compare("binaryoutput")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fBinOut = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fBinOut -> " << fBinOut;
			}

			if (!parName.compare("filename")){
				fFileName = sval.c_str();
				LOG(logDEBUG) << "  setting fFileName -> " << fFileName;
			}

			if (!parName.compare("unmaskall")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fUnMaskAll = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fUnMaskAll -> " << fUnMaskAll;
			}

			break;
		}
	}
	return found;
}

//------------------------------------------------------------------------------
void PixTestPattern::init()
{
	LOG(logDEBUG) << "PixTestPattern::init()";
	fDirectory = gFile->GetDirectory(fName.c_str());
	if (!fDirectory)
		fDirectory = gFile->mkdir(fName.c_str());
	fDirectory->cd();
}

//------------------------------------------------------------------------------
PixTestPattern::~PixTestPattern(){ //dctor
}

// ----------------------------------------------------------------------
void PixTestPattern::runCommand(std::string command) {
	std::transform(command.begin(), command.end(), command.begin(), ::tolower);
	LOG(logDEBUG) << "running command: " << command;

	if (!command.compare("resettodefault")) {
		LOG(logINFO) << "PixTestPattern:: reset parameters from testParameters.dat";
		for (unsigned int i = 0; i < fParameters.size(); ++i)
			setParameter(fParameters[i].first, fParameters[i].second);
		return;
	}

	LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

// ----------------------------------------------------------------------
bool PixTestPattern::setPattern(string fname) {

	ifstream is(fname.c_str());
	if (!is.is_open()) {
		LOG(logINFO) << "  cannot read " << fname;
		return false;
	}

	bool patternFound(false);
	string line;

	while (is.good())
	{
		getline(is, line);

		// -- find Pattern section
		if (string::npos != line.find("-- Pattern"))
		{
			patternFound = true;
			continue;
		}

		if (string::npos != line.find("-- Test Pixels")) break;

		if (patternFound)
		{
			int val2(0);

			// -- remove tabs, adjacent spaces, leading and trailing spaces
			PixUtil::replaceAll(line, "\t", " ");
			string::iterator new_end = unique(line.begin(), line.end(), PixUtil::bothAreSpaces);
			line.erase(new_end, line.end());
			if (line.length() < 2) continue;

			s1 = line.find(",");

			if (string::npos != s1)
			{
				str1 = line.substr(0, s1-1);
				str2 = line.substr(s1 + 1);
				val2 = atoi(str2.c_str());

				//check if delay stays within 8bit
				if (val2 < 0 || val2 > 255) {
					LOG(logWARNING) << "PixTestPattern::setPattern() delay out of range [0,255]";
					return false;
				}
				uint8_t del = val2;
				pg_setup.push_back(make_pair(str1, del));
				LOG(logDEBUG) << "  pg set to -> \"" << str1 << "\" " << del;
			}

			else
			{
				pg_setup.push_back(make_pair("", 0));
				LOG(logWARNING) << "PixTestPattern::setPattern() wrong ... "; //DEBUG
			}
		}

	}

	if (!patternFound)
	{
		LOG(logINFO) << "PixTestPattern::setPattern()  '-- Pattern' not found in testPattern.dat";
		pg_setup.push_back(make_pair("", 0));
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------
bool PixTestPattern::setPixels(string fname, string flag) {

	int npix = 0;

	ifstream is(fname.c_str());
	if (!is.is_open()) {
		LOG(logINFO) << "cannot read " << fname;
		return false;
	}

	bool pixelFound(false);
	string line;

	if (flag == "Unmask")	{ LOG(logINFO) << "  unmasked pixel -> "; }

	while (is.good())
	{
		getline(is, line);

		if (flag == "Test")
		{
			// -- find Test Pixels section
			if (string::npos != line.find("-- Test Pixels"))
			{
				pixelFound = true;
				continue;
			}

			if (string::npos != line.find("-- Unmask Pixels")) break;
		}
		else
		{
			// -- find Unmask Pixels section
			if (string::npos != line.find("-- Unmask Pixels"))
			{
				pixelFound = true;
				continue;
			}
		}

		if (pixelFound)
		{
			int pixc, pixr;

			// -- remove tabs, adjacent spaces, leading and trailing spaces
			PixUtil::replaceAll(line, "\t", " ");
			string::iterator new_end = unique(line.begin(), line.end(), PixUtil::bothAreSpaces);
			line.erase(new_end, line.end());
			if (line.length() < 2) continue;

			s0 = line.find(" ");
			s1 = line.find(",");
			if (string::npos != s1)
			{
				str1 = line.substr(s0, s1);
				pixc = atoi(str1.c_str());
				str2 = line.substr(s1 + 1);
				pixr = atoi(str2.c_str());
				if (flag == "Test"){
					fPIX.push_back(make_pair(pixc, pixr));
					LOG(logINFO) << "  selected pixel -> " << pixc << " " << pixr;
					npix++;
				}
				else {
					fPIXm.push_back(make_pair(pixc, pixr));
					cout << " (" << pixc << "," << pixr << ")";
				}
			}
			else
			{
				fPIX.push_back(make_pair(-1, -1));
				fPIXm.push_back(make_pair(-1, -1));
				LOG(logINFO) << "  selected pixel -> none";
				LOG(logINFO) << "  unmasked pixel -> none";
			}
		}
	}

	if (!pixelFound)
	{
		if (flag == "Test")	{
			LOG(logINFO) << "PixTestPattern::setPixels()  '-- Test Pixels' not found in testPattern.dat";
			fPIX.push_back(make_pair(-1, -1));
		}
		else {
			LOG(logINFO) << "PixTestPattern::setPixels()  '-- Unmask Pixels' not found in testPattern.dat";
			fPIXm.push_back(make_pair(-1, -1));
		}
		return false;
	}
	
	if (flag == "Test")	LOG(logINFO) << "PixTestPattern::setPixels() - Tot Pixels armed = " << npix;
	cout << endl;
	return true;

}


// ----------------------------------------------------------------------
void PixTestPattern::PrintEvents(int par1, int par2, string flag) {

	vector<pxar::Event> daqEvBuffer;
	size_t daqEvBuffsiz;

	if (!fResultsOnFile)
	{
		daqEvBuffer = fApi->daqGetEventBuffer();
		daqEvBuffsiz = daqEvBuffer.size();

		if (daqEvBuffsiz <= 201)
		{
			cout << endl << "data from buffer" << endl;
			for (unsigned int i = 0; i < daqEvBuffsiz; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			cout << endl;
		}
		else
		{
			cout << endl << "data from buffer" << endl;
			for (unsigned int i = 0; i <= 100; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			//skip events. If you want all the events printed select 'resultsonfile'.
			cout << endl << "................... SKIP EVENTS TO NOT SATURATE THE SHELL ...................." << endl << endl;
			for (unsigned int i = (daqEvBuffsiz - 100); i < daqEvBuffsiz; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			cout << endl;
		}
		cout << "Number of events read from buffer: " << daqEvBuffsiz << endl << endl;
	}

	else
	{
		cout << endl << "Start reading data from DTB RAM." << endl;
		std::stringstream sstr;
		string FileName;
		if (flag == "trg") sstr << "_" << par1 << "pgCycles";
		else sstr << "_" << par1 << "sec" << "_" << par2;
		if (fBinOut) FileName = f_Directory + "/" + fFileName.c_str() + sstr.str() + ".bin";
		else FileName = f_Directory + "/" + fFileName.c_str() + sstr.str() + ".dat";


		if (fBinOut)
		{
			std::vector<uint16_t> daqdat = fApi->daqGetBuffer();
			std::cout << "Read " << daqdat.size() << " words of data: ";
			if (daqdat.size() > 550000) std::cout << (daqdat.size() / 524288) << "MB." << std::endl;
			else std::cout << (daqdat.size() / 512) << "kB." << std::endl;
			std::ofstream fout(FileName.c_str(), std::ios::out | std::ios::binary);
			std::cout << "Writing binary" << endl;
			fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
			fout.close();
		}

		else
		{
			daqEvBuffer = fApi->daqGetEventBuffer();
			cout << "Read " << daqEvBuffer.size() << " events." << endl;
			std::ofstream fout(FileName.c_str(), std::ios::out);
			std::cout << "Writing decoded events" << endl;
			fout.write(reinterpret_cast<const char*>(&daqEvBuffer[0]), sizeof(daqEvBuffer[0])*daqEvBuffer.size()); //debug!!! TO BE IMPROVED
			//for (unsigned int i = 0; i < daqEvBuffsiz; i++)	fout << i << " : " << daqEvBuffer[i] << endl;
			fout.close();
		}

		std::cout << "Wrote data to: " << FileName.c_str() << std::endl;
		FileName.clear();
	}

}

// ----------------------------------------------------------------------
void PixTestPattern::pgToDefault(vector<pair<std::string, uint8_t> > pg_setup) {
	pg_setup.clear();
	LOG(logDEBUG) << "PixTestPattern::PG_Setup clean";

	pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
	fApi->setPatternGenerator(pg_setup);
	LOG(logINFO) << "PixTestPattern::       pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTestPattern::FinalCleaning() {
	// Reset the pg_setup to default value.
	pgToDefault(pg_setup);

	//clean local variables:
	fPIX.clear();
	fPIXm.clear();
	pg_setup.clear();
}

//------------------------------------------------------------------------------
void PixTestPattern::doTest()
{
	fDirectory->cd();
	fHistList.clear();
	pg_setup.clear();
	PixTest::update();

	//setparameters and check if in range	
	if (ParOutOfRange) return;

	LOG(logINFO) << "PixTestPattern::doTest() start";	

	//set the input filename (for Pattern and Pixels)
	string fname;
	ConfigParameters* config = ConfigParameters::Singleton();
	f_Directory = config->getDirectory();
	fname = f_Directory + "/testPatterns.dat";

	//PIXELS SELECTION

	// to unmask all or only selected pixels:
	if (fUnMaskAll) {
		fApi->_dut->maskAllPixels(false);
		LOG(logINFO) << "All Pixels Unmasked";
	}
	else {
		
		fApi->_dut->maskAllPixels(true);
		LOG(logINFO) << "Set Unmasked Pixels from file: " << fname;
		if (!setPixels(fname, "Unmask")){   //READ FROM FILE	
			FinalCleaning();
			return;
		}
		for (unsigned int i = 0; i < fPIXm.size(); ++i)	{
			if (fPIXm[i].first > -1)  fApi->_dut->maskPixel(fPIXm[i].first, fPIXm[i].second, false);
		}
	}
	
	// to 'arm' only selected pixels:
	fPIX.clear();
	LOG(logINFO) << "Set 'Armed' Pixels from file: " << fname;
	if (!setPixels(fname, "Test")){   //READ FROM FILE	
		FinalCleaning();
		return;    
	}
	fApi->_dut->testAllPixels(false);
	for (unsigned int i = 0; i < fPIX.size(); ++i) {
	 	if (fPIX[i].first > -1)
			{
				fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
				fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
			}
			else {
				fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
			}
		}
	
	// Start the DAQ:

	//first send only a RES:
	pg_setup.push_back(make_pair("resetroc", 0));     // PG_RESR b001000 
	uint16_t period = 28;

	// Set the pattern generator:
	fApi->setPatternGenerator(pg_setup);

	fApi->daqStart();

	//send only one trigger to reset:
	fApi->daqTrigger(1, period);
	LOG(logINFO) << "PixTestPattern::RES sent once ";

	pg_setup.clear();
	LOG(logINFO) << "PixTestPattern::PG_Setup clean";

	//select the pattern:
	if (fPatternFromFile)
	{
		LOG(logINFO) << "Pattern from file: " << fname;
		if (!setPattern(fname)){   //READ FROM FILE	
			fApi->daqStop();
			FinalCleaning();
			return;
		}
	}
	else //standard pattern from config parameters.
	{
		pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
	}

	//set pattern generator:
	fApi->setPatternGenerator(pg_setup);
	finalPeriod = 0;

	//send Triggers (loop or single) wrt parameters selection:
	if (!fParTrigLoop)
	{
		//pg_cycles times the pg_Single() == pg_cycles times pattern sequence):
		finalPeriod = fApi->daqTrigger(fParPgCycles, fParPeriod);
		LOG(logINFO) << "PixTestPattern:: " << fParPgCycles << " pg_Single() sent with period " << finalPeriod;

		fApi->daqStop();

		// Get events and Print results on shell/file:
		PrintEvents(fParPgCycles, 0, "trg");
	}
	else
	{
		bool daq_loop = true;
		int sec = 0;
		int nloop = 1;
		int checkfreq = 2; //frequency of buffer checks (seconds)

		while (daq_loop)
		{
			finalPeriod = fApi->daqTriggerLoop(fParPeriod);
			LOG(logINFO) << "PixTestPattern:: start TriggerLoop with period " << finalPeriod << " and duration " << fParSeconds << " seconds";

			//check every 'checkfreq' seconds if buffer is full less then 90%
			while (fApi->daqStatus()) {
				mDelay(checkfreq * 1000);  //wait in milliseconds
				sec = sec + checkfreq;
				if (sec >= fParSeconds)	{
					daq_loop = false;
					break;
				}
			}

			if (daq_loop) {
				LOG(logINFO) << "PixTestPattern:: after " << sec << " seconds - save data to file to avoid buffer overflow";
				fApi->daqTriggerLoopHalt();
			}
			else fApi->daqStop();

			// Get events and Print results on shell/file:
			PrintEvents(fParSeconds, nloop, "loop");
			nloop++;
		}
	}

	//DAQ - THE END.

	//set PG to default and clean everything:
	FinalCleaning();
	LOG(logINFO) << "PixTestPattern::doTest() done for ";

}
