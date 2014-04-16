// -- author: Martino Dall'Osso
#ifndef PixTestSetup_H
#define PixTestSetup_H

#include "PixTest.hh"

class DLLEXPORT PixTestSetup : public PixTest {
public:
	PixTestSetup(PixSetup *, std::string);
	PixTestSetup();
	virtual ~PixTestSetup();
	virtual bool setParameter(std::string parName, std::string sval);
	void init();
	void setToolTips();
	void bookHist(std::string);
	void runCommand(std::string);

	void doTest();
	void saveTbParameters();
	void setTbParameters(int , int);

private:

	int     fClkMax;
	int     fDeserMax;
	int     fParCals;
        bool    ParOutOfRange;

	ClassDef(PixTestSetup, 1)

};
#endif
