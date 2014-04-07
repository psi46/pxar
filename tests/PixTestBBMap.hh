// -- author: Wolfram Erdmann
#ifndef PIXTESTBBMAP_H
#define PIXTESTBBMAP_H

#include "PixTest.hh"
#include <stdexcept>      // std::out_of_range

class DLLEXPORT PixTestBBMap: public PixTest {
public:
  PixTestBBMap(PixSetup *, std::string);
  PixTestBBMap();
  virtual ~PixTestBBMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  
  /** determine an upper or lower bound for subsequent vcal scancs
   * by binary search
   * nmax = 0 : returns the highest vcal value for which no pixel responds
   * nmax > 0 : returns the lowest vcal value for which at least nmax pixels respond
   * the data from all getXXXMap calls are stored in the result map
   * if data is already in the map, the call is not repeated
   *  
   * returns the lower or higher bound dac value as an int
   */
  int search(std::map< int, std::vector<pxar::pixel> > &, size_t, int );
  
  
  std::map< uint8_t, TH1* > doThresholdMap( std::string, int, int, int, int);
  
  std::map< uint8_t, TH1* > doPulseheightMap(std::string, bool, int);
  
  void phScan( 
  std::vector< uint8_t >   & rocIds, 
  std::map< uint8_t, int> & vmin, 
  std::map< uint8_t, int> & phmin, 
  std::map< uint8_t, int> & phmax,
  std::map< uint8_t, float> & phslope);

  /** adjust the PH dacs of the ROCs for Bumpbonding tests based on 
   * pulse-height maps, i.e. try to get good coverage for small
   * pulse-heights 
   */
  void adjustPHDacsDigv21();

  
  /** fill test loop result into Roc Histogram
   *  the histogram title string should contain a "%2d" which 
   * going to be replaced by the roc id
   */
  std::map< uint8_t, TH1*> fillRocHistograms( std::vector<pxar::pixel> &, std::string);
  
  // convenience helper function for min/max searching with maps
  void setmin( std::map< uint8_t, int> & m, uint8_t key, int value){ 
    try{ int v=m.at(key);  m[key] = std::min( v, value );}
    catch (const std::out_of_range & e){ m[key] = value; } 
  };
  
  void setmax( std::map< uint8_t, int> &m, uint8_t key, int value){ 
    try{ int v=m.at(key);  m[key] = std::max( v, value );}
    catch (const std::out_of_range & e){ m[key] = value; }
  };

  void doTest(); 

private:
  std::vector<uint8_t> rocIds;  // convenience
  int          fParNtrig; 
  std::string  fParMethod;  // "ph" or "thr"
  int          fParVcalS;   // vcal dac value of the cals probe signal
  int          fPartest;    // direct vcal value for test runs, active if >0

  ClassDef(PixTestBBMap, 1); 

};
#endif
