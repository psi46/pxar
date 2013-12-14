// Defines for which entities a test should be performed

#ifndef TESTRANGE
#define TESTRANGE


#include "GlobalConstants.hh"
#include <TObject.h>
#include <TH2.h>

/// Stores information about which pixels, columns, ROCs, etc. are to be tested or masked
/**
    Uses one bit for every pixel to determine whether this pixel should be included or
    excluded in a test. This can either mean that the pixel is being tested or that it
    is physically masked during the test. Some tests completely disregard this information
    and test or mask or unmask any of the pixels.
 */
class TestRange : public TObject {
  
 public:
  TestRange();   ///< Initializes the range to exclude all pixels
  
  void AddPixel(int roc, int col, int row);                                           ///< Includes a pixel in the range
  void RemovePixel(int roc, int col, int row);                                        ///< Excludes a pixel in the range
  bool ExcludesColumn(int roc, int col);                                              ///< Excludes a column in the range
  bool ExcludesRow(int roc, int row);                                                 ///< Excludes a row in the range
  bool ExcludesRoc(int roc);                                                          ///< Excludes a ROC in the range
  
  void CompleteRange();                                                               ///< Includes all pixels of all ROCs in the range
  void CompleteRoc(int iRoc);                                                         ///< Includes all pixels of the ROC in the range
  
  bool IncludesPixel(int roc, int col, int row);                                      ///< Checks whether a pixel is included in the range
  bool IncludesRoc(int roc);                                                          ///< Checks whether a ROC is included in the range
  bool IncludesDoubleColumn(int roc, int doubleColumn);                               ///< Checks whether a double column is included in the range
  bool IncludesColumn(int roc, int column);                                           ///< Checks whether a column is included in the range
  bool IncludesColumn(int column);                                                    ///< Checks whether a column is included in the range
  int GetValidPixel(int roc, int & col, int & row);
 
  
  void ApplyMaskFile(const char * fileName);                                          ///< Reads a file to selectively disable pixels in the test range
  
  void print();                                                                       ///< Prints all disabled pixels

 protected:
  
  bool pixel[MODULENUMROCS][ROCNUMCOLS][ROCNUMROWS];                                  ///< Array that holds the information about the pixel mask states
  
  ClassDef(TestRange, 1)
};


#endif

