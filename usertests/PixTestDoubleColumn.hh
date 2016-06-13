#ifndef PixTestDoubleColumn_H
#define PixTestDoubleColumn_H

#include "PixTest.hh"

using std::vector;

class DLLEXPORT PixTestDoubleColumn: public PixTest {
public:
  PixTestDoubleColumn(PixSetup *, std::string);
  PixTestDoubleColumn();
  virtual ~PixTestDoubleColumn();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string); 
  void doTest();
  std::vector< std::vector<int> > readData(int) ;
  void testData();
  void resetDaq();
  void testBuffers(std::vector<TH2D*>, int, int);
  void findWorkingRows();
private:

  int     fParNtrig, fParNpix, fParDelay; 
  int     fParTsMin,fParTsMax;
  int     fParRowOffset;
  vector<pxar::Event> fParDaqDat;
  bool fParDaqDatRead;

  ClassDef(PixTestDoubleColumn, 1)

};
#endif
