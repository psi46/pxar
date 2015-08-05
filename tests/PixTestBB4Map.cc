// -- author: Thomas Weiler (based on BBtest from Rudolf Schimassek)


#include <stdlib.h>   	// atof, atoi
#include <algorithm>  	// std::find
#include <fstream>	// fstream
#include <iomanip>	//setw
#include <vector>

#include "PixTestBB4Map.hh"
#include "log.h"
#include "constants.h"
#include "PixUtil.hh"

using namespace std;
using namespace pxar;

ClassImp(PixTestBB4Map)

//------------------------------------------------------------------------------
PixTestBB4Map::PixTestBB4Map( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParCals(0), fParCalDelLo(-1),fParCalDelHi(-1),fParCalDelStep(0),fParVthrCompLo(-1), fParVthrCompHi(-1), fParVthrCompStep(-1), fParVcal(-1), fParNoisyPixels(-1), fParCut(-1)
{
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestMapeff ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestBB4Map::PixTestBB4Map() : PixTest()
{
  LOG(logDEBUG) << "PixTestBB4Map ctor()";
}

//------------------------------------------------------------------------------
bool PixTestBB4Map::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;
      if (!parName.compare("savecaldelscan")) {
        PixUtil::replaceAll(sval, "checkbox(", "");
        PixUtil::replaceAll(sval, ")", "");
        fParSaveCalDelMaps = !(atoi(sval.c_str())==0);
        setToolTips();
      }
      if( !parName.compare( "ntrig" ) )
	fParNtrig = atoi( sval.c_str() );

      if( !parName.compare( "cals" ) )
	fParCals = atoi( sval.c_str() );

      if( !parName.compare( "caldello" ) )
	fParCalDelLo = atoi( sval.c_str() );

      if( !parName.compare( "caldelhi" ) )
	fParCalDelHi = atoi( sval.c_str() );

      if( !parName.compare( "caldelstep" ) )
	fParCalDelStep = atoi( sval.c_str() );

      if( !parName.compare( "vthrcomplo" ) )
	fParVthrCompLo = atoi( sval.c_str() );

      if( !parName.compare( "vthrcomphi" ) )
	fParVthrCompHi = atoi( sval.c_str() );

      if( !parName.compare( "vthrcompstep" ) )
	fParVthrCompStep = atoi( sval.c_str() );

      if( !parName.compare( "noisypixels" ) )
	fParNoisyPixels = atoi( sval.c_str() );

      if( !parName.compare( "vcal" ) )
	fParVcal = atoi( sval.c_str() );

      if( !parName.compare( "cut" ) )
	fParCut = atof( sval.c_str() );

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestBB4Map::init()
{
  LOG(logDEBUG) << "PixTestBB4Map::init()";

  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBB4Map::setToolTips()
{
  fTestTip = string( "test bump bonds");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestBB4Map::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestBB4Map::~PixTestBB4Map()
{
  LOG(logDEBUG) << "PixTestBB4Map dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestBB4Map::clear(vector<TH2D*>* maps)
{
	//delete histograms stored in maps and clear the vector itself
	//	to avoid warnings from ROOT
	for(uint8_t i=0;i<(*maps).size();++i)
	{
		(*maps)[i]->~TH2D();
	}
	(*maps).clear();
}

//------------------------------------------------------------------------------
void PixTestBB4Map::doTest()
{
	static unsigned int durchlauf = 0;

	LOG(logINFO) << "PixTestBB4Map::doTest() ntrig = " << fParNtrig;

	LOG(logINFO) << "VthrComp (min,max,step) = " 
		     << fParVthrCompLo << " " << fParVthrCompHi 
		     << " " << fParVthrCompStep; 
	LOG(logINFO) << "CalDel (min,max,step)   = " 
		<< fParCalDelLo << " " << fParCalDelHi << " " << fParCalDelStep;
	LOG(logINFO) << "Vcal = " << fParVcal; 
	LOG(logINFO) << "Cut = " << fParCut; 
	if (fParCut < 0.0 || fParCut > 1.0){
	  LOG(logINFO) << "PixTestBB4Map::doTest(): fParCut out of range (0.0 - 1.0), set 1.0";
	  fParCut = 1.0;
	}
	
	//protection from bad parameter settings
	if(fParNtrig == -1 || !fParCals 
	   || fParCalDelLo == -1 || fParCalDelHi == -1 || !fParCalDelStep
	   || fParVthrCompLo == -1 || fParVthrCompHi == -1 
	   || fParVthrCompStep == -1 || fParVcal == -1 || fParNoisyPixels == -1
	   || fParCut == -1)
	{
		LOG(logINFO) << "PixTestBB4Map::doTest(): not all Parameters set";
		return;
	}


	uint16_t flags = 0;
	if( fParCals ) flags = FLAG_CALS;
	LOG(logINFO) << "flag " << flags;

	//save CtrlReg to restore it at the end of the test	
	uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );
	if( fParCals )
	{
		fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal
		LOG(logINFO) << "CtrlReg 4 (large Vcal)";
	}

	//save VthrComp and CalDel to restore them at the end of the test	
	uint8_t std_caldel[16];
	uint8_t std_vthr[16];
	int rocthr[16], goodint[16]; //threshold for each roc 
	bool minthrfound[16]; // minimum threshold found


	LOG(logINFO) << "saving VthrComp and CalDel before running tests)";
	for(uint8_t i=0;i<fApi->_dut->getNRocs();++i)
	{
		std_vthr[i]   = fApi->_dut->getDAC( i, "VthrComp" );
		std_caldel[i] = fApi->_dut->getDAC( i, "CalDel" );
		rocthr[i] = fParVthrCompHi;
		minthrfound[i] = false;
		goodint[i] = 0;
		LOG(logINFO) << (int) i << " " << (int) rocthr[i] << " " 
			     << (bool) minthrfound[i];
	}

	

	//--------------------------------------------
	// find noise threshold via Mapeff with Vcal 0
	//--------------------------------------------
	vector<uint8_t> ROCs = fApi->_dut->getEnabledRocIDs();	//enabled ROCs
	//vector<TH2D*>* noisemaps;	//resulting PixelAlive maps
	vector<TH2D*>* noisemaps = new vector<TH2D*>;
	int integral,thrfound;
	TH2D* histnoisemap = 0;
	TH1D* hNoiseScan[16] = {0};
	TH1D* hvthrcomp = new TH1D("MinimumThreshold","Minimum threshold",
				   ROCs.size(),0,ROCs.size());
	hvthrcomp->GetXaxis()->SetTitle("ROC");
	hvthrcomp->GetYaxis()->SetTitle("minimum VthrComp");
	
	
	for(int i = 0; i<16; ++i){	
	  hNoiseScan[i] = new TH1D(Form("NoiseScan_C%i_V%i",i,durchlauf),
				   Form("NoiseScan_C%i (V%i)",i,durchlauf), 
				   (fParVthrCompHi-fParVthrCompLo+21),
				   (double)(fParVthrCompLo-10.5), 
				   (double)(fParVthrCompHi+10.5));
	  hNoiseScan[i]->GetXaxis()->SetTitle("vthrcomp");
	  hNoiseScan[i]->GetYaxis()->SetTitle("noise hits");
	}

	fApi->setDAC("Vcal",1); // set Vcal to zero
	fApi->setDAC("VthrComp" , fParVthrCompLo);

	mapeff(noisemaps);	//get efficiency maps

	LOG(logINFO) << "starting loop over VthrComp";
	for(int thr = fParVthrCompLo; thr <= fParVthrCompHi; thr += fParVthrCompStep)
	{
		//set VthrComp and get PixelAlive map
		fApi->setDAC("VthrComp" , thr);
		clear(noisemaps);	
		mapeff(noisemaps); 

		LOG(logINFO) << "got noisemap";
		//Draw first noise map for entertainment of user
		//noisemaps->Draw("colz");
		PixTest::update();
		
		//compare contents to starting value (=> decreased efficiency)
		for(unsigned int idx = 0;idx < ROCs.size();++idx)
		{
		  if (minthrfound[ROCs[idx]]) continue;
		  histnoisemap = (*noisemaps)[idx];
		  
		  integral = 0;
		  int nBinX, nBinY;
		  nBinX = histnoisemap->GetNbinsX();
		  nBinY = histnoisemap->GetNbinsY();
		  for (int iBinX=1; iBinX <= nBinX; iBinX++)
		    for (int iBinY=1; iBinY <= nBinY; iBinY++)
		      if (histnoisemap->GetBinContent(iBinX, iBinY) > 0) integral++;
		  hNoiseScan[idx]->SetBinContent (hNoiseScan[ROCs[idx]]
						  ->GetXaxis()->FindBin(thr), 
						  integral);
		  
		  LOG(logINFO) << "ROC " << idx << ", integral " << integral;
		  if(integral >= fParNoisyPixels)
		    {
		      minthrfound[ROCs[idx]] = true;
		      //Set noise threshold
		      if (thr > fParVthrCompLo) rocthr[ROCs[idx]] = 
						  thr - fParVthrCompStep;    
		      else rocthr[ROCs[idx]] = thr;
		      
		      LOG(logINFO) << "ROC #" << (int)ROCs[idx] 
				   << ": threshold found";
		      integral = 0;
		    }
		}
		LOG(logINFO) << "VthrComp = " << thr << " done";
		
		thrfound = 0;
		for (size_t i = 0; i < fApi->_dut->getNRocs(); i++){
		  if (minthrfound[i]) thrfound++; 
		}
		if(static_cast<unsigned int>(thrfound) == fApi->_dut->getNRocs()){
		  LOG(logINFO) << "Minimal threshold for all ROCs found";
		  break;
		}

	}
	//print resulting values to command line	
	stringstream noisestr;
	noisestr << "resulting noise thresholds per ROC =";
	for(unsigned int roc=0;roc < fApi->_dut->getNRocs();++roc){
		noisestr << " " << rocthr[roc];
		//set threshold to rocthr[roc] 
		fApi->setDAC("VthrComp", rocthr[roc], roc);
		// fill minimum threshold values into histogram
		hvthrcomp->Fill(roc,rocthr[roc]);
	}

	
	hvthrcomp->Draw();

	LOG(logINFO) << noisestr.str();

	LOG(logINFO) << "threshold scan done";

	//end noise threshold scan
	


	//------------------------	
	//CalDel scan for all ROCs
	//------------------------

	int minerror[16]= {4160}; // minimum number of errors in one run
	int optcaldel[16] = {0};  // corresponding caldel

	vector<TH2D*>* maps = new vector<TH2D*>; //pointer to the resulting maps of mapeff()


	TH2D* hresult[16] = {0};
	TH2D* hBestEfficiency[16] = {0};
	TH1D* hcaldel[16] = {0};
	TH1D* heffdistr[16] = {0};

	for(int i = 0; i<16; ++i)
	{
		minerror[i] = 4160;
		hcaldel[i] = new TH1D(Form("CalDel Scan_C%i_V%i",i,durchlauf),
				Form("CalDel Scan_C%i (V%i)",i,durchlauf),
				      256,0,255);

		hcaldel[i]->GetXaxis()->SetTitle("CalDel");
		hcaldel[i]->GetYaxis()->SetTitle("Errors");

		heffdistr[i] = new TH1D(Form("Hit Distr_C%i_V%i",i,durchlauf),
				 Form("Hit Distr_C%i (V%i)",i,durchlauf),
				       256,0,255);

		heffdistr[i]->GetXaxis()->SetTitle("Efficiency");
		heffdistr[i]->GetYaxis()->SetTitle("Entries");
		
		hresult[i] = new TH2D(Form("PixelHit_C%i_V%i",i,durchlauf),
				      Form("PixelHit_C%i (V%i)",i,durchlauf),
				      52,-0.5,51.5,80,-0.5,79.5);
		hresult[i]->GetXaxis()->SetTitle("column");
		hresult[i]->GetYaxis()->SetTitle("row");

		hBestEfficiency[i] = new TH2D(Form("BB4MapBest_C%i_V%i",i,durchlauf),
					      Form("BB4Best_C%i (V%i)",i,durchlauf),
					      52,-0.5,51.5,80,-0.5,79.5);
		hBestEfficiency[i]->GetXaxis()->SetTitle("column");
		hBestEfficiency[i]->GetYaxis()->SetTitle("row");
	}

	//set starting values
	fApi->setDAC("CalDel", fParCalDelLo);

	fApi->setDAC("Vcal",fParVcal);

	TH2D* hist = 0;
	int currenterrors = 0,goodpixels = 0;
	int binContentBest = 0, binContent = 0;
	for(uint8_t caldel = fParCalDelLo; caldel <= fParCalDelHi; caldel += fParCalDelStep){
	  //set CalDel
	  fApi->setDAC("CalDel",caldel);
	  clear(maps);	
	  mapeff(maps);	//get efficiency maps
	  
	  for(uint8_t roc = 0; roc < maps->size(); ++roc){
	    if (minerror[roc] == 0) continue;
	    hist = (*maps)[roc];
	    
	    //count empty bins in histogram hist
	    currenterrors = 0;
	    goodpixels = 0;
	    for(int x=1;x<=52;++x){
	      for(int y=1;y<=80;++y){
		binContentBest = hBestEfficiency[roc]->GetBinContent (x,y);
		binContent = hist->GetBinContent(x,y);
		if (binContent > binContentBest) hBestEfficiency[roc]->SetBinContent (x,y,binContent); 
		if(hist->GetBinContent(x,y) < fParCut * fParNtrig)++currenterrors;
		else{ 
		  hresult[roc]->SetBinContent(x,y,1);
		} 
	      }
	    }
	    // save hist to file for later analysis
	    if (fParSaveCalDelMaps) hist->Write(Form("caldelscan_C%i_%i",roc,caldel));
	    
	    //insert value into histogram
	    hcaldel[roc]->Fill(caldel,currenterrors);
	    
	    
	    for(int x=1;x<=52;++x){
	      for(int y=1;y<=80;++y){
		if (hresult[roc]->GetBinContent(x,y)) goodpixels++;
	      }
	    }
	    
	    //draw progress of caldel scan to entertain the user
	    hcaldel[roc]->Draw();
	    hresult[roc]->Draw("colz");
	    PixTest::update();
	    
	    //new minimum error count?
	    if(currenterrors < minerror[roc]){
	      minerror[roc] = currenterrors;
	      optcaldel[roc] = caldel;
	    }
	    LOG(logINFO) << "ROC " << (int)roc << ", missing bumps = " 
			 << currenterrors 
			 << ", total number of good bumps = " << goodpixels;
	  }
	  LOG(logINFO) << "CalDel = " << (int)caldel << " done";
	  if (maps->size() == 0 ) break; 
	}

	LOG(logINFO) << "CalDel scan done";

	//apply best values for CalDel and print them to output
	stringstream str;
	for(uint8_t roc = 0; roc < (*maps).size(); ++roc)
	{
		fApi->setDAC("CalDel",optcaldel[roc],roc);
		str << " " << (int)optcaldel[roc];
	}
	LOG(logINFO) << "resulting values: CalDel = " << str.str();

	
	//--------------------------------------------
	// Efficiency Map for optimal CalDel
	//--------------------------------------------
	
	vector<uint8_t> enabledROCs = fApi->_dut->getEnabledRocIDs();

	clear(maps);	//remove histograms from CalDel scan

	
	// now do efficiency map for best caldel, 
	// VthrComp used from scan over VthrComp
	mapeff(maps);	//get efficiency maps

	//includes file path to save a list of broken bump bonds
	ConfigParameters *cp = fPixSetup->getConfigParameters();

	ofstream f;
	uint16_t counter = 0;

	//number of ROCs in current configuration
	LOG(logINFO) << "maps.size() = " << (*maps).size();

	//open output file for bump bond list, overwrite existing file
	f.open(Form("%s/%s.dat",cp->getDirectory().c_str(),"brokenbumpbonds"));

	fHistList.push_back(hvthrcomp);
	hvthrcomp->Draw();

	for(uint8_t idx = 0; idx < (*maps).size(); ++idx)
	{
		//add CalDel scan to output
		fHistList.push_back(hcaldel[idx]);
		hcaldel[idx]->Draw();
		fHistList.push_back(hresult[idx]);
		hresult[idx]->Draw("colz");

		fHistList.push_back(hBestEfficiency[idx]);
		fHistList.push_back(hNoiseScan[idx]);

		fHistList.push_back((*maps)[idx]);
		hist = (TH2D*) fHistList.back();
		hist->SetTitle(Form("BB4Map_C%i",idx));

		//headline for ROC
		f << "----------\nROC " << setw(2) << (int)idx << ":\n----------\n";



		//print coordinated of bad pixel to file 
		//and fill distribution histogram
		for(int x=1;x<=52;++x)
		{
			for(int y=1;y<=80;++y)
			{
			  heffdistr[idx]->Fill(hist->GetBinContent(x,y));
			  //if(hist->GetBinContent(x,y)==0) 
			  if(hresult[idx]->GetBinContent(x,y)==0) 
				{
					f << "Pix " << setw(2) << x-1 << " " << setw(2) << y-1 << endl;
					++counter;	//count total bad bumps
				}
			}
		}

		//hiostogramms in b/w for better visibility of empty bins

		//replace draw command with the lines below to coloured histograms
		//fHistOptions.insert(make_pair((*maps)[idx],"colz"));	
		//hist->Draw(getHistOption(hist).c_str());
		hist->Draw("colz");	
		// add efficiency distribution histogram 
		fHistList.push_back(heffdistr[idx]);
		heffdistr[idx]->Draw();
		
		fHistList.push_back(heffdistr[idx]);
		heffdistr[idx]->Draw();
		
		hBestEfficiency[idx]->Draw("colz");
		hNoiseScan[idx]->Draw("colz");

		PixTest::update();
	}
	
	f.close();

	//choose focused histogram (BBmap of last ROC)
	fDisplayedHist = find( fHistList.begin(), fHistList.end(), hist );

	//print total number of bad bump bonds
	LOG(logINFO) << counter << " broken Bump Bonds found";


	//reactivate all ROCs that have been activated before the test
	for(uint8_t i=0;i<enabledROCs.size();++i)
		fApi->_dut->setROCEnable(enabledROCs[i],true);

	//restore CtrlReg
	if( fParCals )
	{
		fApi->setDAC( "CtrlReg", ctl );
		LOG(logINFO) << "back to CtrlReg " << (int)ctl;
	}

	//restore VthrComp and CalDel
	for(uint8_t i=0;i<fApi->_dut->getNRocs();++i)
	{
		fApi->setDAC("VthrComp" ,std_vthr[i]   ,i);	
		fApi->setDAC("CalDel"   ,std_caldel[i] ,i);
	}

	LOG(logINFO) << "PixTestBB4Map::doTest() done";

	++durchlauf;	//to avoid overwriting histograms
}


//------------------------------------------------------------------------------
void PixTestBB4Map::mergevectors(vector<TH2D*>* one, vector<TH2D*>* two)
{
	if((*two).size() == 0)	//second vector is empty
	{
		LOG(logINFO) << "tried merging an empty vector into the vector one";
		return;
	}
	else if((*one).size() == 0)	//first vector is empty
	{
		for(vector<TH2D*>::iterator it = (*two).begin();it != (*two).end();++it)
		{
			(*one).push_back(*it);
		}
		return;
	}

	TH2D* hist1(0);
	TH2D* hist2(0);

	for(uint8_t id = 0;id < (*one).size();++id)
	{
		hist1 = (*one)[id];
		hist2 = (*two)[id];

		//add up corresponding histograms
		for(uint8_t x=0; x<52;++x)
		{
			for(uint8_t y=0; y<80;y++)
			{
				hist1->Fill(x,y,hist2->GetBinContent(x+1,y+1));
			}
		}
	}

	return;
}




//------------------------------------------------------------------------------
void PixTestBB4Map::mapeff(vector<TH2D*>* maps)
{

  static int durchlauf = -(fParCalDelHi-fParCalDelLo)/fParCalDelStep-2; 
	//to get V0 in the name of histograms of the first run
  ++durchlauf;	//counter to avoid ROOT warnings 

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  if( fApi ) fApi->_dut->testAllPixels(true);


  uint16_t flags = 0;
  if( fParCals ) flags = FLAG_CALS;

  vector<pixel> vpix; // all pixels, all ROCs

  // get efficiency map
  if( fApi ) vpix = fApi->getEfficiencyMap( flags, fParNtrig );


  // book maps per ROC:

  TH2D* h2(0);

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    //    h2 = new TH2D( Form( "MapBB_V%i_C%d", int(durchlauf), int(roc) ),
    h2 = new TH2D( Form( "BB4Map_C%d_V%d", int(roc) , int(durchlauf)),
		   Form( "partial BB4Map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "PH [ADC]" );
    h2->SetStats(0);
    maps->push_back(h2);
  }

  //fill histogram with data:

  for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
    h2 = maps->at(vpix[ipx].roc());
    if( h2 ) h2->Fill( vpix[ipx].column(), vpix[ipx].row(), vpix[ipx].value() );
  }


  return;
}
