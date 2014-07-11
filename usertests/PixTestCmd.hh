// -- author: Wolfram Erdmann
#ifndef PIXTESTCMD_H
#define PIXTESTCMD_H

#include "PixTest.hh"
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

class Arg{
 public:
  enum argtype {UNDEF,STRING, ILIST};
 Arg(string s):type(STRING),svalue(s){};
 Arg(int i):type(ILIST){lvalue.clear(); lvalue.push_back(i);}
 Arg(vector<int> v):type(ILIST),lvalue(v){};

  bool getInt(int & value){ if ((type==ILIST)&&(lvalue.size()==1)){value=lvalue[0]; return true;}return false;}

  bool getList(vector<int> & value){ if(type==ILIST){ value=lvalue; return true;} return false;}
  bool getString(string & value){ if(type==STRING){ value=svalue; return true;}return false;}

  argtype type;
  string svalue;
  vector<int> lvalue;
  string str(){
    stringstream s;
    if ((type==ILIST)&&(lvalue.size()==1)){ s << lvalue[0];}
    else if (type==ILIST) { s << "vector("<<lvalue.size()<<")";}
    else if (type==STRING){ s << "'" << svalue <<"'";}
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
  bool match(const char *, string &);
  bool match(const char * s, vector<int> & , vector<int> &);
 
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
  vector<int> values;

  string name;
  int value(){ return values.size()==1 ? values[0] : -1;};  // for single valued targets

 Target():name(""){values.clear(); values.push_back(-1);}
 Target(string name, int value=-1):name(name){values.clear();values.push_back(value);}
  Target get(unsigned int i){ Target t(name, values[i]); return t;};

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
  bool parse( Token & );
  bool exec(CmdProc *, Target &);
};


class CmdProc {

 public:
  CmdProc();
  int exec(string s);
  int exec(const char* p){ return exec(string(p));}

  bool process(Keyword, Target );
  bool setDefaultTarget( Target t){ defaultTarget=t; return true; }

  pxar::pxarCore * fApi;
  stringstream out;



  bool verbose;
  int tb(Keyword);
  int roc(Keyword, int rocid);

  Target defaultTarget;
  map<string, deque <string> > macros;
  map< string, int > vars;
  
};


#endif
