#ifndef PIXTESTSETTRIM_H
#define PIXTESTSETTRIM_H

#include "api.h"
#include "PixTest.hh"

class PixTestSetTrim: public PixTest {
public:
  PixTestSetTrim(PixSetup *, std::string);
  PixTestSetTrim();
  virtual ~PixTestSetTrim();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  void bookHist(std::string);
  void RocThrMap( uint8_t roc, uint32_t nTrig,
		  bool xtalk, bool cals );
  void printThrMap( uint8_t roc, int &nok );
  void TrimStep( int roc, int target, int correction,
		 int nTrig, int xtalk, int cals );

  void doTest();

private:

  uint8_t fParVcal; // target threshold
  int     fParNtrig;
  uint8_t modthr[16][52][80];
  uint8_t modtrm[16][52][80];

  uint8_t rocthr[52][80];
  uint8_t roctrm[52][80];

  ClassDef(PixTestSetTrim, 1);

};
#endif
