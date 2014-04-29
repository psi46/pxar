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
	void setToolTips();
	void bookHist(std::string);

	void runCommand(std::string );	
	void choosePIX(std::string);
	bool setPattern(std::string);
	bool setPixels(std::string);
	void PrintOnShell(std::vector<pxar::Event> &, pxar::rawEvent, pxar::Event);
	void doTest();

private:

	int     fParNtrig;
	bool	fTestAllPixels;
	bool	fMaskAllPixels;	
	bool	fPatternFromFile;
	bool	fPixelsFromFile;
	std::string str1, str2;
	std::string::size_type s0, s1;
	std::vector<std::pair<uint16_t, uint8_t> > pg_setup;

	ClassDef(PixTestPattern, 1)

};
#endif
