// -- author: Wolfram Erdmann
#ifndef PIXTESTCMD_H
#define PIXTESTCMD_H

#include "PixTest.hh"

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
#include <iostream>  // cout, debugging only
#include <sstream>   // for producing string representations

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
    enum special{IMIN=-1, IMAX=-2, UNDEFINED=-3};
    IntList():singleValue(UNDEFINED){ranges.clear();}
    bool parse( Token & , const bool append=false );
    
    int value(){return singleValue;}
    bool isSingleValue(){return (!(singleValue==UNDEFINED));}
    vector<int> getVect(const int imin=0, const int imax=0);
    //vector<int> get(vector<int> );
};

class Arg{
    public:
    enum argtype {UNDEF,STRING_T, IVALUE_T, ILIST_T};
    Arg(string s):type(STRING_T),svalue(s){};
    //Arg(int i):type(ILIST_T){ivalue=i;}
    Arg(IntList v){
        if( v.isSingleValue() ){
            type=IVALUE_T;
            ivalue=v.value();
        }else{
            type=ILIST_T;
            lvalue=v;
        }
    }
    bool getInt(int & value){ if (type==IVALUE_T){value=ivalue; return true;}return false;}

    bool getList(IntList & value){ if(type==ILIST_T){ value=lvalue; return true;} return false;}
    bool getVect(vector<int> & value, const int imin=0, const int imax=0){
        if(type==ILIST_T){ 
            value=lvalue.getVect(imin, imax);
            return true;
        }else if(type==IVALUE_T){
            value.push_back( ivalue);
            return true;
        }else{
             return false;
        }
    }
    bool getString(string & value){ if(type==STRING_T){ value=svalue; return true;}return false;}

    argtype type;
    string svalue;
    IntList lvalue;
    int ivalue;
    
    string str(){
        stringstream s;
        if (type==IVALUE_T){ s << ivalue;}
        else if (type==ILIST_T) { s << "vector("<<")";}
        else if (type==STRING_T){ s << "'" << svalue <<"'";}
        else s <<"???";
        return s.str();
    }

};

class Keyword{
  bool kw(const char* s){ return (strcmp(s, keyword.c_str())==0);};

 public:
 Keyword():keyword(""){};
 Keyword(string s):keyword(s){};

  bool match(const char * s){ return kw(s) && (narg()==0); };
  bool match(const char *, int &);
  bool match(const char *, int &, int &);
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
  Keyword keyword;
  Block * block;

 public:
 Statement():
  isAssignment(false), name(""), has_localTarget(false), keyword(""){block=NULL;};
  ~Statement(){ if (!(block==NULL)) delete block;}
  bool parse( Token & );
  bool exec(CmdProc *, Target &);
};


class CmdProc {

 public:
  CmdProc();
  CmdProc( CmdProc* p);
  ~CmdProc();
  int exec(string s);
  int exec(const char* p){ return exec(string(p));}

  bool process(Keyword, Target );
  bool setDefaultTarget( Target t){ defaultTarget=t; return true; }

  pxar::pxarCore * fApi;
  stringstream out;

  int tbmset(int address, int value);


  bool verbose;
  int tb(Keyword);
  int tbm(Keyword);
  int roc(Keyword, int rocid);

  Target defaultTarget;
  map<string, deque <string> > macros;
  
};


#endif
