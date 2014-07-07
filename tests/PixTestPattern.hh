#ifndef PixTestPattern_H
#define PixTestPattern_H

#include "PixTest.hh"
#include "PixUtil.hh"
#include "ConfigParameters.hh"

class DLLEXPORT PixTestPattern : public PixTest {
public:
	PixTestPattern(PixSetup *, std::string);
	PixTestPattern();
	virtual ~PixTestPattern();

	virtual bool setParameter(std::string parName, std::string sval);
	void init();
	void setToolTips();
	void doTest();

	void stop();
	void runCommand(std::string);
	bool setPattern(std::string);
	bool setPixels(std::string, std::string);
	void FillHistos(std::vector<pxar::Event> , std::vector<TH2D*> , std::vector<TProfile2D*> , std::vector<TH1D*> );
	void PrintEvents(int, int, std::string, std::vector<TH2D*> , std::vector<TProfile2D*> , std::vector<TH1D*> );
	void TriggerLoop(int , std::vector<TH2D*> , std::vector<TProfile2D*> , std::vector<TH1D*> );
	void pgToDefault();
	void FinalCleaning();

private:

	int     fParPgCycles;
	int	    fParTrigLoop;
	int     fParPeriod;
	int	    fParSeconds;
	bool	fPatternFromFile;
	bool	fResultsOnFile;
	bool    fBinOut;
	std::string fFileName;
	bool	fUnMaskAll;
	bool	fParFillTree;

	bool    fParOutOfRange;
	bool    fDaq_loop;
	std::string f_Directory;
	std::vector<std::pair<std::string, uint8_t> > fPg_setup;
	std::vector<std::pair<int, int> > fPIXm;
	uint16_t fPeriod;
	int		fCheckFreq;

	ClassDef(PixTestPattern, 1)

};
#endif
