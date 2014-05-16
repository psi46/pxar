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
#include "api.h"
#include "constants.h"
#include "helper.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPattern)

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParTrigLoop(0), fParPeriod(0), fParSeconds(0), fPatternFromFile(0), fResultsOnFile(1), fBinOut(0), fFileName("null.dat"), fPixelsFromFile(1), fTestAllPixels(0), fMaskAllPixels(0)
{
	init();
	PixTest::init();
	LOG(logDEBUG) << "PixTestPattern ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern() : PixTest()
{
	LOG(logDEBUG) << "PixTestPattern ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPattern::setParameter(string parName, string sval)
{
	bool found(false);	
	
	for (uint32_t i = 0; i < fParameters.size(); ++i) 
	{

		if (fParameters[i].first == parName) 
		{
			found = true;

			if (!parName.compare("ntrig")){
				fParNtrig = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting Ntrig -> " << fParNtrig;
			}

			if (!parName.compare("trigloop")){
				fParTrigLoop = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting TrigLoop -> " << fParTrigLoop;
			}
						
			if (!parName.compare("period")){
				fParPeriod = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting Period -> " << fParPeriod;
			}

			if (!parName.compare("seconds")){
				fParSeconds = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting Seconds -> " << fParSeconds;
			}

			if (!parName.compare("patternfromfile")){
				fPatternFromFile = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fPatternFromFile -> " << fPatternFromFile;
			}
			   
			if (!parName.compare("resultsonfile")){
				fResultsOnFile = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fResultsOnFile -> " << fResultsOnFile;
			}

			if (!parName.compare("binaryoutput")){
				fBinOut = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fBinOut -> " << fBinOut;
			}

			if (!parName.compare("outfilename")){
				fFileName = sval.c_str();
				LOG(logDEBUG) << "  setting fFileName -> " << fFileName;
			}

			if (!parName.compare("testallpixels")){
				fTestAllPixels = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fTestAllPixels -> " << fTestAllPixels;
			}

			if (!parName.compare("maskallpixels")){
				fMaskAllPixels = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fMaskAllPixels -> " << fMaskAllPixels;
			}

			/*			if (!parName.compare("pixelsfromfile")){
			fPixelsFromFile = atoi(sval.c_str());
			LOG(logDEBUG) << "  setting fPixelsFromFile -> " << fPixelsFromFile;
			} */

	/*		//to set PIXs from testParameters.dat:
			int I = i - 9;
			stringstream stre;
			stre << "pix" << I;
			string pixN = stre.str();
			if (!parName.compare(pixN)) choosePIX(sval);
			pixN.clear();
			*/

			break;
		}
	}
	return found;
}

//------------------------------------------------------------------------------
void PixTestPattern::init()
{
	LOG(logINFO) << "PixTestPattern::init()";

	fDirectory = gFile->GetDirectory(fName.c_str());
	if (!fDirectory)
		fDirectory = gFile->mkdir(fName.c_str());
	fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestPattern::setToolTips()
{

}

//------------------------------------------------------------------------------
void PixTestPattern::bookHist(string name)
{
	fDirectory->cd();
	LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPattern::~PixTestPattern()
{
	LOG(logDEBUG) << "PixTestPattern dtor";
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

//------------------------------------------------------------------------------
void PixTestPattern::choosePIX(string sval)
{
	int pixc(-1), pixr(-1);
	s1 = sval.find(",");
	if (string::npos != s1)	{
		str1 = sval.substr(0, s1);
		pixc = atoi(str1.c_str());
		str2 = sval.substr(s1 + 1);
		pixr = atoi(str2.c_str());
		fPIX.push_back(make_pair(pixc, pixr));
		LOG(logINFO) << "  pixel selected -> " << pixc << " " << pixr; //DEBUG
	}
	else {
		fPIX.push_back(make_pair(-1, -1));
		LOG(logINFO) << "  pixel selected -> none"; //DEBUG
	}
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
			int val1(-1);
			int val2(-1);

			// -- remove tabs, adjacent spaces, leading and trailing spaces
			PixUtil::replaceAll(line, "\t", " ");
			string::iterator new_end = unique(line.begin(), line.end(), PixUtil::bothAreSpaces);
			line.erase(new_end, line.end());
			if (line.length() < 2) continue;

			s1 = line.find(",");

			if (string::npos != s1)
			{
				str1 = line.substr(0, s1);
				str2 = line.substr(s1 + 1);
				val1 = atoi(str1.c_str());
				val2 = atoi(str2.c_str());		
				pg_setup.push_back(make_pair(val1, val2));
				LOG(logDEBUG) << "  pg set to -> " << val1 << " " << val2;
			}

			else
			{
				pg_setup.push_back(make_pair(-1, -1));
				LOG(logINFO) << "PixTestPattern::setPattern() wrong ... "; //DEBUG
			}
		}

	}

	if (!patternFound)
	{
		LOG(logINFO) << "PixTestPattern::setPattern()  '-- Pattern' not found"; //DEBUG
		pg_setup.push_back(make_pair(-1, -1));
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------
bool PixTestPattern::setPixels(string fname, string flag) {

	ifstream is(fname.c_str());
	if (!is.is_open()) {
		LOG(logINFO) << "cannot read " << fname;
		return false;
	}

	bool pixelFound(false);
	string line;

	if (flag == "Unmask")	{LOG(logINFO) << "  unmasked pixel -> ";}
		
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
				LOG(logINFO) << "  selected pixel -> none"; //DEBUG
				LOG(logINFO) << "  unmasked pixel -> none"; //DEBUG
			}
		}
	}

	if (!pixelFound)
	{
		if (flag == "Test")	{
			LOG(logINFO) << "PixTestPattern::setPixels()  '-- Test Pixels' not found"; //DEBUG
			fPIX.push_back(make_pair(-1, -1));
		}
		else {
			LOG(logINFO) << "PixTestPattern::setPixels()  '-- Unmask Pixels' not found"; //DEBUG
			fPIXm.push_back(make_pair(-1, -1));
		}
		return false;
	}
	cout << endl;
	return true;

}


// ----------------------------------------------------------------------
void PixTestPattern::PrintEvents() {

	vector<pxar::Event> daqEvBuffer;
	size_t daqEvBuffsiz;

	if(!fResultsOnFile)
	{
		daqEvBuffer = fApi->daqGetEventBuffer();
		daqEvBuffsiz = daqEvBuffer.size();
	
		if (daqEvBuffsiz <= 1001)
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
			cout << endl << "...................skip events...................." << endl <<endl;
			for (unsigned int i = (daqEvBuffsiz-100); i < daqEvBuffsiz; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}	
			cout << endl;
		}
		cout << "Number of events read from buffer: " << daqEvBuffsiz << endl << endl;
	}
	else 
	{
		cout << endl << "Start reading data from DTB RAM." << endl;
		std::ofstream fout(fFileName.c_str(), std::ios::out);

		if (fBinOut)
		{			
			std::vector<uint16_t> daqdat = fApi->daqGetBuffer();
			std::cout << "Read " << daqdat.size() << " words of data: ";
			if (daqdat.size() > 550000) std::cout << (daqdat.size() / 524288) << "MB." << std::endl;
			else std::cout << (daqdat.size() / 512) << "kB." << std::endl;
			fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
			std::cout << "Writing binary" << endl;
		}

		else
		{
			daqEvBuffer = fApi->daqGetEventBuffer();
			daqEvBuffsiz = daqEvBuffer.size();
			cout << "Read " << daqEvBuffsiz << " events." << endl;
			for (unsigned int i = 0; i < daqEvBuffsiz; i++)	fout << i << " : " << daqEvBuffer[i] << endl;
		}

		fout.close();
		std::cout << "Wrote data to file " << fFileName.c_str() << std::endl;
	}

}

//------------------------------------------------------------------------------
void PixTestPattern::doTest()
{
	fDirectory->cd();
	fHistList.clear();
	pg_setup.clear();  
	PixTest::update();

//-- new DEBUG!!
	fApi->SignalProbe("D1", "clk");
	fApi->SignalProbe("D2", "tout");

//old...
//	fApi->SignalProbe("D1","pgsync");   //to send PG_Sync signal on the ROC via lemo
//	fApi->SignalProbe("D2", "pgsync"); //to see PG_Sync with oscilloscope

	LOG(logINFO) << "PixTestPattern::doTest() ntrig = " << fParNtrig;

	bookHist("bla"); //??! FIXME

	//set the filename
	string fname;
	ConfigParameters* config = ConfigParameters::Singleton();
	std::string f_Directory = config->getDirectory();
	fname = f_Directory + "/testPatterns.dat";  //to read it from -f ?
			
			
	if (!fTestAllPixels)
	{
		//select the pixels:
		if (fPixelsFromFile)
		{
			fPIX.clear();  //to clear Pixels set from gui
			LOG(logINFO) << "Set Pixels from file: " << fname;  //DEBUG
			if (!setPixels(fname, "Test")) return;    //READ FROM FILE	
		}

		LOG(logINFO) << "Set Unmasked Pixels from file: " << fname;  //DEBUG
		if (!setPixels(fname, "Unmask")) return;    //READ FROM FILE	

		// to unmask selected pixels:
		fApi->_dut->testAllPixels(false);

		fApi->_dut->maskAllPixels(true);
		for (unsigned int i = 0; i < fPIX.size(); ++i) {
			if (fPIX[i].first > -1)  
			{
           		fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
				if (!fMaskAllPixels) fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
			}
			else {
				fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false); //??
			}
		}

		if (!fMaskAllPixels){
			for (unsigned int i = 0; i < fPIXm.size(); ++i)			{
				if (fPIXm[i].first > -1)  fApi->_dut->maskPixel(fPIXm[i].first, fPIXm[i].second, false);
				//else  //... ??	debug			}
			}
		}
	}
	else
	{
		fApi->_dut->testAllPixels(true);
		LOG(logINFO) << "  testAllPixels -> true"; //DEBUG
		if (fMaskAllPixels)
		{
			fApi->_dut->maskAllPixels(true);
			LOG(logINFO) << "  all Pixels masked"; //DEBUG
		}
		else                fApi->_dut->maskAllPixels(false);
	}
		
	
	// Start the DAQ:

	//first send only a RES:
	pg_setup.push_back(make_pair(0x0800, 25));     // PG_RESR b001000 
	pg_setup.push_back(make_pair(0x0100, 0));     // PG_TOK  

	// Set the pattern generator:
	fApi->setPatternGenerator(pg_setup);

	fApi->daqStart();

	//send only one trigger to reset:
	fApi->daqTrigger(1);
	LOG(logINFO) << "PixTestPattern::RES|TOK sent once ";

	pg_setup.clear();
	LOG(logINFO) << "PixTestPattern::PG_Setup clean";

	//select the pattern:
	if (fPatternFromFile)
	{
		LOG(logINFO) << "Pattern from file: " << fname;
		if (!setPattern(fname)) return;		//READ FROM FILE	
	}
	else			 //standard pattern
	{
		pg_setup.push_back(make_pair(0x0800, 25));               // PG_RESR b001000 
		pg_setup.push_back(make_pair(0x0400, 100 + 6));			// PG_CAL  b000100
		pg_setup.push_back(make_pair(0x0200, 16));			   // PG_TRG  b000010
		pg_setup.push_back(make_pair(0x0100, 0));		      // PG_TOK  
	}

	//set pattern generator (new api function):
	fApi->setPatternGenerator(pg_setup);

	//send Triggers (loop or single) wrt parameters selection:
	if (!fParTrigLoop){
		//Ntrig times the pg_Sinlgle() == Ntrig times pattern sequence):
		fApi->daqTrigger(fParNtrig);
		LOG(logINFO) << "PixTestPattern:: " << fParNtrig << " pg_Single() sent";
	}
	else
	{
		fApi->daqTriggerLoop(fParPeriod);  //if '-1' automatically set to minimum.  MESSAGE NEEDED.
		LOG(logINFO) << "PixTestPattern:: start TriggerLoop with period " << fParPeriod << " and duration " << fParSeconds << " seconds";
		
		mDelay(fParSeconds*1000);  //wait in milliseconds

	}

	fApi->daqStop();

	// Get events and Print results on shell/file:
	PrintEvents();

	// Reset the pg_setup to default value.
	pg_setup.clear();
	LOG(logDEBUG) << "PixTestPattern::PG_Setup clean";
	pg_setup.push_back(make_pair(0x0800, 25));               // PG_RESR b001000 
	pg_setup.push_back(make_pair(0x0400, 100 + 6));			// PG_CAL  b000100
	pg_setup.push_back(make_pair(0x0200, 16));			   // PG_TRG  b000010
	pg_setup.push_back(make_pair(0x0100, 0));		      // PG_TOK  		
	fApi->setPatternGenerator(pg_setup);
	LOG(logINFO) << "PixTestPattern::       pg_setup set to default.";

	fPIX.clear();
	fPIXm.clear();
	pg_setup.clear();

	LOG(logINFO) << "PixTestPattern::doTest() done for ";

}
