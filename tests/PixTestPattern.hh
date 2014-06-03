// -- author: Martino Dall'Osso
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
	void doTest();

	void runCommand(std::string);
	bool setPattern(std::string);
	bool setPixels(std::string, std::string);
	void PrintEvents(int, int, std::string);
	void pgToDefault(std::vector<std::pair<std::string, uint8_t> >);

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

	bool    ParOutOfRange;
	std::string f_Directory;
	std::string str1, str2;
	std::string::size_type s0, s1;
	std::vector<std::pair<std::string, uint8_t> > pg_setup;
	std::vector<std::pair<int, int> > fPIXm;

	ClassDef(PixTestPattern, 1)

};
#endif
