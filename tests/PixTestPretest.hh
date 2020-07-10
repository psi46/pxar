#ifndef PIXTESTPRETEST_H
#define PIXTESTPRETEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestPretest: public PixTest {
public:
  class DRecord; // forward declaration

  PixTestPretest(PixSetup *, std::string);
  PixTestPretest();
  virtual ~PixTestPretest();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  std::string toolTip(std::string what);
  void runCommand(std::string);
  void bookHist(std::string);

  void doTest();
  void setVana();
  void programROC();
  void findWorkingPixel();
  void setVthrCompCalDel();
  void setVthrCompId();
  void setCalDel();
  /// Doug's timing setting
  void setTimings();
  /// Wolfram's timing setting (optimized)
  void findTimingCmd();
  /// local replica with histogramming
  void findTiming();

  // and now many helper functions (all from CmdProc)
  int runDaqRaw(std::vector<uint16_t> & buf, int ntrig, int ftrigkhz, int verbosity=0, bool setup=true);
  int runDaqRaw(int ntrig, int ftrigkhz, int verbosity=0);
  int test_timing(int nloop, int d160, int d400, int rocdelay = -1, int htdelay = 0, int tokdelay = 0);
  int countGood(unsigned int nloop, unsigned int ntrig, int ftrigkhz, int nroc);
  int countErrors(unsigned int ntrig=1, int ftrigkhz=0, int nroc_expected=-1, bool setup=true);
  int setupDaq(int ntrig, int ftrigkhz, int verbosity);
  int pg_sequence(int seq, int length);
  int restoreDaq(int verbosity);
  int pg_restore();
  int tbmset(int address, int value);
  int tbmset(std::string name, uint8_t coreMask, int value, uint8_t valueMask=0xff);
  int tbmget(std::string name, const uint8_t coreMask, uint8_t & value);
  int tbmsetbit(std::string name, uint8_t coreMask, int bit, int value);
  void sort_time(int values[], double step, double range);
  bool find_midpoint(int threshold, int data[], uint8_t & position, int & width);
  bool find_midpoint(int threshold, double step, double range,  int data[], uint8_t & position, int & width);
  void clear_DaqChannelCounter(unsigned int f[]) {for(size_t i=0; i<nDaqChannelMax; i++){ f[i]=0;}  }
  int rocIdFromReadoutPosition(unsigned int daqChannel, unsigned int roc){return fDaqChannelRocIdOffset[daqChannel]+roc;}
  int rocIdFromReadoutPositionRaw(unsigned int position);
  int daqChannelFromTbmPort(unsigned int port);
  bool tbmWithDummyHits();
  int getBuffer(std::vector<uint16_t> & buf);
  int resetDaqStatus();
  bool layer1();
  bool tbm08();
  int getData(std::vector<uint16_t> & buf, std::vector<DRecord > & data, int verbosity=1, int nroc_expected=-1, bool resetStats=true);

  static int fPrerun;
  static int fGetBufMethod;

  class DRecord{
  public:
    uint8_t channel;
    uint8_t type;
    enum Type{HIT=0, ROC_HEADER=4, TBM_HEADER=10, TRAILER=14, DUMMYHIT=15};
    uint8_t id;
    uint32_t w1,w2;
    uint32_t data;
    DRecord(uint8_t ch=0, uint8_t T=0xff, uint32_t D=0x00000000, uint16_t W1=0x000, uint16_t W2=0x0000, uint8_t Id=0){
      channel = ch;
      type = T;
      data = D;
      w1 = W1;
      w2 = W2;
      id = Id;
    }
  };

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fParNtrig;
  int     fParVcal, fParDeltaVthrComp;
  double  fParFracCalDel;
  int     fIgnoreProblems;

  // for timing test
  /* TBM core selection masks for settbm */
  #define ALLTBMS 0xf
  #define TBMA    0x5
  #define TBMB    0xa
  #define TBM0    0x3
  #define TBM1    0xc
  #define TBM0A   0x1
  #define TBM0B   0x2
  #define TBM1A   0x4
  #define TBM1B   0x8

  #define STEP160   1.0
  #define RANGE160  6.25
  #define STEP400   0.57
  #define RANGE400  2.5


  bool fPixelConfigNeeded;
  int fDeser400XOR1sum[8];  // count transitions at the 8 phases
  int fDeser400XOR2sum[8];

  unsigned int fTCT, fTRC, fTTK;
  unsigned int fBufsize;
  unsigned int fMaxPeriod;
  std::vector<uint16_t>  fBuf;
  unsigned int fNumberOfEvents;
  unsigned int fHeaderCount;
  std::vector<unsigned int> fHeadersWithErrors;
  unsigned int fSeq;
  unsigned int fPeriod;
  std::vector<std::pair<std::string, uint8_t> > fSigdelays;
  std::vector<std::pair<std::string, uint8_t> > fSigdelaysSetup;
  bool fPgRunning;

  // xor and error counting per daq channel, supposed to replace
  // the global counting variables above at some point
  static const size_t nDaqChannelMax=8;
  unsigned int fDeser400XOR[nDaqChannelMax];
  unsigned int fDeser400SymbolErrors[nDaqChannelMax];
  unsigned int fDeser400PhaseErrors[nDaqChannelMax];
  unsigned int fDeser400XORChanges[nDaqChannelMax];
  unsigned int fRocReadBackErrors[nDaqChannelMax];
  unsigned int fNTBMHeader[nDaqChannelMax];
  unsigned int fNEvent[nDaqChannelMax];
  unsigned int fDaqErrorCount[nDaqChannelMax]; //  any kind of error
  // new with fw4.6
  unsigned int fDeser400_frame_error[nDaqChannelMax];
  unsigned int fDeser400_code_error[nDaqChannelMax];
  unsigned int fDeser400_idle_error[nDaqChannelMax];
  unsigned int fDeser400_trailer_error[nDaqChannelMax];

  static int fNtrigTimingTest;
  static int fIgnoreReadbackErrors;

  unsigned int fGoodLoops[nDaqChannelMax];
  int fDeser400err;
  uint16_t fRocHeaderData[17];

  std::vector<unsigned int> fDaqChannelRocIdOffset;  // filled in setApi

  unsigned int fnDaqChannel;// filled in setApi
  unsigned int fnRocPerChannel;// filled in setApi
  unsigned int fnTbmCore; // =   fApi->_dut->getNTbms();
  unsigned int fnCoresPerTBM; // number of cores per physical TBM
  unsigned int fnTbm;         // number of physical TBMs
  std::vector<int> fTbmChannels;   // daq channels connected to a tbm (bit-pattern) [size=fnTbm]

  ClassDef(PixTestPretest, 1)

};
#endif
