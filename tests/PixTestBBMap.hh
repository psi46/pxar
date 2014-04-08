// -- author: Wolfram Erdmann
#ifndef PIXTESTBBMAP_H
#define PIXTESTBBMAP_H

#include "PixTest.hh"

class DLLEXPORT PixTestBBMap: public PixTest {
public:
  PixTestBBMap(PixSetup *, std::string);
  PixTestBBMap();
  virtual ~PixTestBBMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  
  std::vector<TH1*> thrMaps(std::string dac, std::string name, uint8_t dacmin, uint8_t dachi, unsigned int ntrig, uint16_t flag=0);

  void saveDAC( std::string dac);
  void restoreDACs();


  void doTest(); 

private:
  std::vector<uint8_t> rocIds;  // convenience
  int          fParNtrig; 
  int          fParVcalS;   // vcal dac value of the cals probe signal
  int          fPartest;    // direct vcal value for test runs, active if >0
  
  std::map< std::string, std::vector< uint8_t> > fSavedDACs;

  ClassDef(PixTestBBMap, 1); 

};
#endif
