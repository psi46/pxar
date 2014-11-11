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
    bool match(const char * s1 , const char * s2, int & );
    bool match(const char * s, string & s1, vector<string> & options, ostream & err);
    bool match(const char *, int &);
    bool match(const char *, int &, int &);
    bool match(const char *, int &, int &, int &);
    bool match(const char *, string &);
    bool match(const char * s, vector<int> & , vector<int> &);
    bool match(const char * s, vector<int> &, const int, const int , vector<int> &, const int, const int);
    bool greedy_match(const char *, string &);
    bool greedy_match(const char *, int &, int&, int&, string &);
    bool concat(unsigned int i, string &s){  s="";  for (;i<argv.size(); i++) s+=argv[i].str(); return true;}

    unsigned int narg(){return argv.size();};
    string str();

    string keyword;
    vector<Arg> argv;
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
    uint8_t type;
    uint32_t w1,w2;
    uint32_t data;
    DRecord(uint8_t T=0xff, uint32_t D=0x00000000, uint16_t W1=0x000, uint16_t W2=0x0000){
        type = T;
        data = D;
        w1 = W1;
        w2 = W2;
    }
};

class CmdProc {

 
 public:
  CmdProc(){init();};
  CmdProc( CmdProc* p);
  ~CmdProc();
  void init();
  void setApi(pxar::pxarCore * api, PixSetup * setup );
  int exec(string s);
  int exec(const char* p){ return exec(string(p));}

  bool process(Keyword, Target, bool );
  bool setDefaultTarget( Target t){ defaultTarget=t; return true; }

  pxar::pxarCore * fApi;
  PixSetup * fPixSetup;
  
  stringstream out; 
  pxar::RegisterDictionary * _dict;
  pxar::ProbeDictionary * _probeDict;
  vector<string>  fD_names;
  vector<string> fA_names;
  static const unsigned int fnDAC_names;
  static const char * const fDAC_names[];
  bool fPixelConfigNeeded;
  unsigned int fTCT, fTRC, fTTK;
  unsigned int fBufsize;
  vector<uint16_t>  fBuf;
  unsigned int fSeq;
  unsigned int fPeriod;
  vector<pair<string,uint8_t> > fSigdelays;
  vector<pair<string,uint8_t> > fSigdelaysSetup;
  bool fPgRunning;
  
  int fDeser400XOR1;
  int fDeser400XOR2;
  int fDeser400err;
  
  bool verbose;
  Target defaultTarget;
  map<string, deque <string> > macros;
  
  
  int tbmset(int address, int value);
  int tbmset   (string name, uint8_t coreMask, int value, uint8_t valueMask=0xff);
  int tbmsetbit(string name, uint8_t coreMask, int bit, int value);
  int tbmget(string name, const uint8_t core, uint8_t & value);
  int tbmscan();
  int rocscan();
  int tctscan(unsigned int tctmin=0, unsigned int tctmax=0);
  
  int countHits();
  int countErrors(int ntrig=1, int nroc_expected=-1);
  int printData(vector<uint16_t> buf, int level);
  int readRocs(uint8_t signal=0xff, double scale=0, std::string units=""  );
  int getBuffer(vector<uint16_t> & buf, int ntrig=1, int verbosity=1);
  int runDaq(vector<uint16_t> & buf, int ntrig, int ftrigkhz, int verbosity=0);
  int burst(vector<uint16_t> & buf, int ntrig, int trigsep=6, int nburst=1, int verbosity=0);
  int getData(vector<uint16_t> & buf, vector<DRecord > & data, int verbosity=1, int nroc_expected=-1);
  int pixDecodeRaw(int, int level=1);
  int setTestboardDelay(string name="all", uint8_t value=0);
  int setTestboardPower(string name, uint16_t value);
  
  int bursttest(int ntrig, int trigsep=6, int nburst=1);
  //int adctest0(const string s);
  int adctest(const string s);
  int sequence(int seq);
  int pg_sequence(int seq, int length=0);
  int pg_restore();
  int pg_loop(int value=0);
  int pg_stop();

  int tb(Keyword);
  int tbm(Keyword, int cores=2);
  int roc(Keyword, int rocid);

  
};


#endif
