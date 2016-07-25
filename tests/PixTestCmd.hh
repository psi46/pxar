// -- author: Wolfram Erdmann
#ifndef PIXTESTCMD_H
#define PIXTESTCMD_H

#include "PixTest.hh"
#include "dictionaries.h"

#if (defined WIN32)
#include <Windows4Root.h>  //needed before any ROOT header
#endif

#include <TGFrame.h>
#include <TGTextView.h>
#include <TGTextEntry.h>
#include <TGTextBuffer.h>

#include <deque>
#include <string>
#include <vector>
#include <iostream>  // cout, debugging only (need ostream, though)
#include <sstream>   // for producing string representations
#include <fstream>


class CmdProc;

class DLLEXPORT PixTestCmd: public PixTest {
public:
  PixTestCmd(PixSetup *, std::string);
  PixTestCmd();
  virtual ~PixTestCmd();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 
  void createWidgets();
  void DoTextField();
  void DoUpArrow();
  void DoDnArrow();
  void flush(std::string s);
  void runCommand(std::string s);
  void stopTest();

private:

  TGTransientFrame * tf;
  TGTextView     *     transcript;
  TGHorizontalFrame * textOutputFrame;
  TGHorizontalFrame * cmdLineFrame;
  TGTextEntry * commandLine;
  std::vector<std::string> cmdHistory;
  unsigned int historyIndex;
  
  CmdProc * cmd;
  
  ClassDef(PixTestCmd, 1)

};




/*====================================================================*/
/*  command processor code                                            */
/*====================================================================*/

using namespace std;

// forward declarations
class CmdProc;
class Statement;
class Token;



class IntList{
    int singleValue;
    vector< pair<int,int> > ranges; 
    
    public:
    enum special{IMIN=-1, IMAX=-2, UNDEFINED=-3, IVAR=-4};
    IntList():singleValue(UNDEFINED){ranges.clear();}
    bool parse( Token & , const bool append=false );
    
    int value(){return singleValue;}
    bool isSingleValue(){return (!(singleValue==UNDEFINED));}
    bool isVariable(){return ( (singleValue==IVAR));}
    vector<int> getVect(const int imin=0, const int imax=0);

};

class Arg{
    public:
    static int varvalue;
    enum argtype {UNDEF,STRING_T, IVALUE_T, IVAR_T, ILIST_T};
    Arg(string s):type(STRING_T),svalue(s){};
    Arg(int i):type(IVALUE_T),ivalue(i){};
    Arg(IntList v){
        if( v.isSingleValue() ){
            if (v.isVariable()){
                type = IVAR_T;
                ivalue=0; // determined at execution gime
            }else{
                type=IVALUE_T;
                ivalue=v.value();
            }
        }else{
            type=ILIST_T;
            lvalue=v;
        }
    }
    bool getInt(int & value){ 
        if (type==IVALUE_T){value=ivalue; return true;}
        if (type==IVAR_T){value=varvalue; return true;}
        return false;
    }

    bool getList(IntList & value){ if(type==ILIST_T){ value=lvalue; return true;} return false;}
    bool getVect(vector<int> & value, const int imin=0, const int imax=0){
        if(type==ILIST_T){ 
            value=lvalue.getVect(imin, imax);
            return true;
        }else if(type==IVALUE_T){
            value.push_back( ivalue);
            return true;
        }else if(type==IVAR_T){
            value.push_back( varvalue );
            return true;
        }else{
             return false;
        }
    }
    bool getString(string & value){ if(type==STRING_T){ value=svalue; return true;}return false;}
    bool scmp(const char *s){ return (type==STRING_T)&&( strcmp(s, svalue.c_str())==0 );}

    argtype type;
    string svalue;
    IntList lvalue;
    int ivalue;
    
    string str(){
        stringstream s;
        if (type==IVALUE_T){ s << ivalue;}
        else if (type==IVAR_T){ s << varvalue;}
        else if (type==ILIST_T) { s << "vector("<<")";}
        else if (type==STRING_T){ s << "'" << svalue <<"'";}
        else s <<"???";
        return s.str();
    }
    
    string raw(){
        stringstream s;
        if (type==IVALUE_T){ s << ivalue;}
        else if (type==IVAR_T){ s << varvalue;}
        else if (type==STRING_T){ s << svalue;}
        else s <<"0";
        return s.str();
    }

};


class Keyword{
    bool kw(const char* s){ return (strcmp(s, keyword.c_str())==0);};

    public:
    Keyword():keyword(""){};
    Keyword(string s):keyword(s){};

    bool match(const char * s){ return kw(s) && (narg()==0); };
    bool match(const char * s, int & value, const char * s1);
    bool match(const char * s1, const char * s2);
    bool match(const char * s1, const char * s2, string &);
    bool match(const char * s1 ,const char * s2, int & );
    bool match(const char * s1 ,const char * s2, int &, int & );
    bool match(const char * s1 ,const char * s2, int &, int &, int& );

    bool match(const char * s, string & s1, vector<string> & options, ostream & err);
    bool match(const char * s, string & s1, vector<string> & options, int & value,  ostream & err);
    
    bool match(const char *, int &);
    bool match(const char *, int &, int &);
    bool match(const char *, int &, int &, int &);
    bool match(const char *, int &, int &, int &, int &);
    bool match(const char *, int &, int &, int &, int &, int &);
    bool match(const char *, string &);
    bool match(const char *, vector<int> &);    
    bool match(const char *, string &, vector<int> &);    
    bool match(const char * s, vector<int> & , vector<int> &);
    bool match(const char * s, vector<int> &, const int, const int , vector<int> &, const int, const int);
    bool match(const char * s, vector<int> &, const int, const int , vector<int> &, const int, const int, int &);
    bool greedy_match(const char *, string &);
    bool greedy_match(const char *, int &, int&, int&, string &);
    bool concat(unsigned int i, string &s){  s="";  for (;i<argv.size(); i++) s+=argv[i].str(); return true;}

    unsigned int narg(){return argv.size();};
    string str();

    string keyword;
    vector<Arg> argv;
    
    Keyword at(int index){
        Keyword k( keyword );
        k.argv.clear();
        for(unsigned int i=0; i<argv.size(); i++){
            if (argv[i].type==Arg::ILIST_T){
                k.argv.push_back( Arg( argv[i].lvalue.getVect(0,0)[index] ));
            }else{
                k.argv.push_back( Arg(argv[i]) );
            }
        }
        return k;
    };
};


class Token{
  // container for a list of tokens, basically a deque<string> 
  // with the capability to replace tokens by macros
  deque<string> token;
  map<string, deque <string> >::iterator mi;
  vector< deque <string> > stack;

 public:
  Token(){ macros=NULL; token.clear(); stack.clear(); }
  Token(const deque<string> tlist){ macros=NULL; token=tlist; stack.clear(); }
  map<string, deque <string> > * macros;
  //deque-like interface
  string front(bool expand=true);
  void pop_front();
  void push_front(string s);
  bool empty(){return (token.size()==0)&&(stack.size()==0);}
  void push_back(string s){token.push_back(s);}
  // parsing and macro handling
  bool assignment(string & name);
  void add_macro(string name,  deque <string> t){ (*macros)[name]=t; }
};

/* containers for syntax elements */


class Target{
    public:
    IntList lvalues;        // parsed
    vector<int> ivalues;  // expanded
    bool expanded;

    string name;
    void expand( const int imin=0, const int imax=0 ){
        if (expanded) return;
        ivalues=lvalues.getVect(imin, imax); expanded=true;
    }
    unsigned int size(){ if (!expanded){ return 0;}else{ return ivalues.size();}}
    vector<int> values(){return ivalues;}// todo fix unexpanded
    Target():name(""){expanded=false;}
    Target(string s):name(s){expanded=false;}
  
    // for single valued targets
    int value(){ if (!expanded) {return 0;}else{return ivalues.size()==1 ? ivalues[0] : -1;}; }
    Target(string name, const int value):name(name){ivalues.clear();ivalues.push_back( value );expanded=true;}
    Target get(unsigned int i){ Target t(name, ivalues[i]); return t;};

    bool parse( Token & );
    string str();

}; 


class Block{
  vector<Statement *> stmts;
 public:
  Block(){ stmts.clear();}
  bool parse(Token &);
  bool exec(CmdProc *, Target &);
};


class Statement{
  bool isAssignment;    // assign block or value or list or ...
  string name;          // variable/macro  name

  bool has_localTarget; // true if the statement has a target identifier
  Target localTarget;   // target(s) for the following statement
  Block * block;

 public:
 Statement():
  isAssignment(false), name(""), has_localTarget(false), keyword(""), redirected(false), out_filename(""){block=NULL;};
  ~Statement(){ if (!(block==NULL)) delete block; }
  bool parse( Token & );
  bool exec(CmdProc *, Target &);
  
  Keyword keyword;
  bool redirected;
  string out_filename;

};

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


struct TBMDelays{
    uint8_t register_0[ 4 ];
    uint8_t register_e[ 4 ];
    uint8_t register_a[ 4 ];
    int d400, d160;
    int htdelay[4],tokendelay[4];
    int rocdelay[8];
};



class CmdProc {

 
 public:

  static bool fStopWhateverYouAreDoing;

  CmdProc(){init(); fStopWhateverYouAreDoing=false; };
  CmdProc(PixTestCmd *pixtest){init(); master = pixtest; };
  CmdProc( CmdProc* p);
  ~CmdProc();
  void init();
  void setApi(pxar::pxarCore * api, PixSetup * setup );
  void flush(stringstream & o);
  
  int exec(string s);
  int exec(const char* p){ return exec(string(p));}

  bool process(Keyword, Target, bool, int index=0 );
  bool setDefaultTarget( Target t){ defaultTarget=t; return true; }

  pxar::pxarCore * fApi;
  PixSetup * fPixSetup;
 
  PixTestCmd * master;
  stringstream out; 
  pxar::RegisterDictionary * _dict;
  pxar::ProbeDictionary * _probeDict;
  vector<string>  fD_names;
  vector<string> fA_names;
  static const unsigned int fnDAC_names;
  static const char * const fDAC_names[];
  static int fGetBufMethod;
  static int fNtrigTimingTest;
  static int fIgnoreReadbackErrors;
  
  bool fPixelConfigNeeded;
  unsigned int fTCT, fTRC, fTTK;
  unsigned int fBufsize;
  unsigned int fMaxPeriod;
  vector<uint16_t>  fBuf;
  unsigned int fNumberOfEvents;
  unsigned int fHeaderCount;
  vector<unsigned int> fHeadersWithErrors;
  unsigned int fSeq;
  unsigned int fPeriod;
  vector<pair<string,uint8_t> > fSigdelays;
  vector<pair<string,uint8_t> > fSigdelaysSetup;
  bool fPgRunning;
  
  //int fDeser400XOR1;
  //int fDeser400XOR2;
  int fDeser400XOR1sum[8];  // count transitions at the 8 phases
  int fDeser400XOR2sum[8];
  int fDeser400err;
  static int fPrerun;
  static bool fFW35;  // for fw<=3.5, to be removed
  

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
   
   // read out counting for "countGood(...)", not reset by resetDaqStatus
   unsigned int fGoodLoops[nDaqChannelMax];
   
   void clear_DaqChannelCounter( unsigned int f[]){ 
        for(size_t i=0; i<nDaqChannelMax; i++){ f[i]=0;}
    }
  
   uint16_t fRocHeaderData[17];
   
   // readout configuration
   bool layer1(){ if (fApi->_dut->getNEnabledTbms() == 4 ) {return true;} else {return false;}};
   bool tbm08(){ return fApi->_dut->getTbmType()=="tbm08c"; };
   bool tbmWithDummyHits(){ return !tbm08(); }
   unsigned int fnDaqChannel;// filled in setApi
   unsigned int fnRocPerChannel;// filled in setApi
   unsigned int fnTbmCore; // =   fApi->_dut->getNTbms();
   unsigned int fnTbmPort; // = 2*fnTbmCore;
   vector<unsigned int> fDaqChannelRocIdOffset;  // filled in setApi
   int rocIdFromReadoutPosition(unsigned int daqChannel, unsigned int roc){
       return fDaqChannelRocIdOffset[daqChannel]+roc;
   }
   int rocIdFromReadoutPositionRaw( unsigned int position){
	   // needed for daqGetRawEventBuffer()
//	   uint8_t daqChannel = position / fnRocPerChannel;
	   return position; //fDaqChannelRocIdOffset[daqChannel] + (position % fnRocPerChannel);
       // channels are now sorted in pxar core
   }
   int daqChannelFromTbmPort( unsigned int port){
       if (tbm08()){ return port/2 ; }
       else if(fnTbmCore==4){
          return (port+4) % 8; // cross your fingers
       }
       else{ return port; }
   }

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
  
  
  bool verbose;
  bool redirected;
  bool fEchoExecs;  // echo command from executed files
  uint16_t fDumpFlawed;
  Target defaultTarget;
  map<string, deque <string> > macros;
  
  
  int tbmset(int address, int value);
  int tbmset(string name, uint8_t coreMask, int value, uint8_t valueMask=0xff);
  int tbmsetbit(string name, uint8_t coreMask, int bit, int value);
  int tbmget(string name, const uint8_t core, uint8_t & value);
  TBMDelays tbmgetDelays();
  int tbmscan(const int nloop=10, const int ntrig=100, const int ftrigkhz=10);
  int test_timing(int nloop, int d160, int d400, int rocdelay=-1, int htdelay=0, int tokdelay=0);
  bool set_tbmtiming(int d160, int d400, int rocdelay[], int htdelay[], int tokdelay[], bool reset=true);
  
  int test_timing2(int nloop, int d160, int d400, int rocdelay[], int htdelay[], int tokdelay[], int daqchannel=-1);
  int post_timing();

  #define STEP160   1.0
  #define RANGE160  6.25
  #define STEP400   0.57
  #define RANGE400  2.5
  int find_timing(int npass=0);
  void sort_time(int values[], double step, double range);
  bool find_midpoint(int threshold, int data[], uint8_t & position, int & width);
  bool find_midpoint(int threshold, double step, double range,  int data[], uint8_t & position, int & width);

  int rawscan(int level=0);
  int rocscan();
  int tctscan(unsigned int tctmin=0, unsigned int tctmax=0);
  int levelscan();
  
  int countHits();
  vector<int> countHits( vector<DRecord> data, size_t nroc=16);
  int countErrors(unsigned int ntrig=1, int ftrigkhz=0, int nroc_expected=-1, bool setup=true);
  int countGood(unsigned int nloop, unsigned int ntrig, int ftrigkhz, int nroc);
  int printData(vector<uint16_t> buf, int level, unsigned int nheader=0);
  int dumpBuffer(vector<uint16_t> buf, ofstream & fout, int level=0);
  
  int rawRocReadback(uint8_t  signal, std::vector<uint16_t> &);
  int readRocsAnalog(uint8_t  signal, double scale, std::string units);
  int readRocs(uint8_t signal=0xff, double scale=0, std::string units=""  );
  int getBuffer(vector<uint16_t> & buf);
  int setupDaq(int ntrig, int ftrigkhz, int verbosity=0);
  int restoreDaq(int verbosity=0);
  int runDaq(vector<uint16_t> & buf, int ntrig, int ftrigkhz, int verbosity=0, bool setup=true);
  int runDaq(int ntrig, int ftrigkhz, int verbosity=0);
  int runDaqRandom(int ntrig, int ftrigkhz, int verbosity=0);
  int runDaqRandom(vector<uint16_t> & buf, vector<DRecord> & data, int ntrig, int ftrigkhz, int verbosity=0);
  int maskHotPixels(int ntrig, int ftrigkHz, int multiplier=2, float percentile=0.9);

  int drainBuffer(bool tellme=true);
  int daqStatus();
  int resetDaqStatus();
  int burst(vector<uint16_t> & buf, int ntrig, int trigsep=6, int caltrig=0, int nburst=1, int verbosity=0);
  int getData(vector<uint16_t> & buf, vector<DRecord > & data, int verbosity=1, int nroc_expected=-1, bool resetStats=true);
  int pixDecodeRaw(int, int level=1);
  int pixDecodeRaw(int raw, uint8_t & col, uint8_t & row, uint8_t & ph);

  int setTestboardDelay(string name="all", uint8_t value=0);
  int setTestboardPower(string name, uint16_t value);
  
  int bursttest(int ntrig, int trigsep=6, int nburst=1, int caltrig=0, int nloop=1);
  int adctest(const string s);
  int tbmread(uint8_t regId, int hubid);
  string tbmprint(uint8_t regId, int hubid);
  int tbmreadback();
  
  int sequence(int seq);
  int pg_sequence(int seq, int length=0);
  int pg_restore();
  int pg_loop(int value=0);
  int pg_stop();

  int tb(Keyword);
  int tbm(Keyword, int cores=ALLTBMS);
  int roc(Keyword, int rocid);
  
  void stop(bool force=true);
  bool stopped();

  char wait_for_key_pressed(){char ch; cout << "Press enter: "; cin  >> ch; return ch; }
  
};


#endif
