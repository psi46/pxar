#ifndef PIXTESTTIMING_H
#define PIXTESTTIMING_H

#include "PixTest.hh"

class DLLEXPORT PixTestTiming : public PixTest {
public:
	PixTestTiming(PixSetup *, std::string);
	PixTestTiming();
	virtual ~PixTestTiming();
	virtual bool setParameter(std::string parName, std::string sval);
	void init();
	void setToolTips();
	void bookHist(std::string);
	void runCommand(std::string);

    void ClkSdaScan();
    void PhaseScan();
	void doTest();
    void DummyTest();
	void saveTbParameters();
	std::vector<std::pair<std::string,uint8_t> > getDelays(uint8_t , uint8_t);

private:

    int     fTargetClk;
    int     fNROCs;
    bool    fFastScan;

	ClassDef(PixTestTiming, 1)

};
#endif
