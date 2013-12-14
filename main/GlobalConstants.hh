// Provides a minimal set of basic constants of the project

#ifndef GLOBALCONSTANTS
#define GLOBALCONSTANTS

namespace psi {
const int COLS       = 52;
const int ROWS       = 80;
const int DCOLS      = 26;
const int DCOLROWS   = 160;
const int MODULEROCS = 16;
const int DAC8       = 256; // Max 8-bit dacs
const int DAC4       = 128; // Max 4-bit dacs
}

#define DCOLNUMPIX 160 // # pixels in a double column
#define ROCNUMROWS  80  // # rows
#define ROCNUMCOLS  52  // # columns
#define ROCNUMDCOLS 26  // # double columns (= columns/2)
#define MODULENUMROCS 16 // # max. number of rocs on a module
#define CONTROLNETWORKNUMMODULES 16 // # max. number of modules in a detector setup
#define FIFOSIZE 4096 // size of the fifo buffer on the analog testboard

static const char * const red = "\033[0;31m";
static const char * const blue = "\033[0;35m";
static const char * const purple = "\033[0;36m";
static const char * const normal = "\033[0m";

#endif
