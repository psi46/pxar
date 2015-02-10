#ifndef PIXTESTBAREMODULE_H
#define PIXTESTBAREMODULE_H

#include "PixTest.hh"

class DLLEXPORT PixTestBareModule : public PixTest {

public:
	PixTestBareModule(PixSetup *, std::string);
	PixTestBareModule();
	virtual ~PixTestBareModule();
	virtual bool setParameter(std::string parName, std::string sval);
	void init();
	void setToolTips();
	void bookHist(std::string);
	
	void runCommand(std::string );
	bool checkIfInContact(bool );
	bool doStdTest(std::string );
	bool doRocTests(int );
	void doTest();

private:

	int  fParMaxSteps;
	bool fStop;
	bool fBBMap;
	bool fBB2Map;
	double fminIa;

	ClassDef(PixTestBareModule, 1)

};
#endif
